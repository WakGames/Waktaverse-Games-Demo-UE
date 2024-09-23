#include "WakgamesCallbackServer.h"
#include "SDK/WakSDK_GameInstanceSubsystem.h"  // WakGamesSubsystem 헤더 포함
#include "Kismet/GameplayStatics.h"

UWakgamesCallbackServer::UWakgamesCallbackServer()
{
	SetbRunning(false);
	WakGamesSubsystem = nullptr;
}

void UWakgamesCallbackServer::BeginDestroy()
{
	Super::BeginDestroy();
	StopServer();
}

bool UWakgamesCallbackServer::RestartServer(int32 ListenPort,
                                            UWakSDK_GameInstanceSubsystem* Subsystem)
{
	StopServer();
	if (StartServer(ListenPort, Subsystem))
	{
		return true;
	}
	return false;
}

void UWakgamesCallbackServer::StopServer()
{
	if (!GetbRunning())
	{
		UE_LOG(LogTemp, Error, TEXT("WakGamesCallbackServer : Server is not running"));
		return;
	}

	if (this)
	{
		if (HttpRouter.IsValid())
		{
			if (RouteHandle.IsValid())
			{
				HttpRouter->UnbindRoute(RouteHandle);
				RouteHandle.Reset();
				UE_LOG(LogTemp, Warning,
				       TEXT("WakGamesCallbackServer : RouteHandle unbound."));
			}
			
			FHttpServerModule::Get().StopAllListeners();
			HttpRouter.Reset();
			UE_LOG(LogTemp, Warning,
			       TEXT("WakGamesCallbackServer : HTTP Listener unbound."));
		}

		if (this->IsRooted())
		{
			this->RemoveFromRoot();
		}
		this->MarkAsGarbage();
		SetbRunning(false);
	}
}

bool UWakgamesCallbackServer::StartServer(int32 ListenPort,
                                          UWakSDK_GameInstanceSubsystem* Subsystem)
{
	if (GetbRunning())
	{
		UE_LOG(LogTemp, Warning, TEXT("WakGamesCallbackServer : Server is already running"));
		StopServer();
	}

	if (!Subsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("WakGamesCallbackServer: WakGamesSubsystem instance is null, cannot start server"));
		return false;
	}

	WakGamesSubsystem = Subsystem;
	
	FHttpServerModule& HttpServerModule = FHttpServerModule::Get();
	HttpRouter = HttpServerModule.GetHttpRouter(ListenPort);

	if (HttpRouter.IsValid())
	{
		// 안전한 람다 캡처를 위해 Weak Pointer 사용
		TWeakObjectPtr<UWakgamesCallbackServer> WeakThis(this);
		
		// Lambda를 TFunction으로 명시적으로 캐스팅하여 FHttpRequestHandler로 변환
		RouteHandle = HttpRouter->BindRoute(
			FHttpPath(TEXT("/callback")),
			EHttpServerRequestVerbs::VERB_GET,
			FHttpRequestHandler::CreateLambda(
			[WeakThis](const FHttpServerRequest& Request,
				   const FHttpResultCallback& OnComplete) -> bool
			{
				// 요청 데이터를 복사하여 람다 캡처로 전달
				TSharedPtr<FHttpServerRequest> RequestCopy = MakeShareable(new FHttpServerRequest(Request));
				FHttpResultCallback OnCompleteCopy = OnComplete;

				AsyncTask(ENamedThreads::GameThread, [WeakThis, RequestCopy, OnCompleteCopy]()
				{
					if (WeakThis.IsValid())
					{
						WeakThis->HandleRequests(*RequestCopy, OnCompleteCopy);
					}
					else
					{
						UE_LOG(LogTemp, Warning, TEXT("WakgamesCallbackServer: self.instance is no longer valid on game thread"));
					}
				});
				return true;
			})
		);

		SetbRunning(true);
		if (GetbRunning())
		{
			HttpServerModule.StartAllListeners();
			UE_LOG(LogTemp, Warning,
				   TEXT("WakGamesCallbackServer : Server started on port %d, (bRunning: %d)"),
				   ListenPort, GetbRunning());
			return true;	
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("WakGamesCallbackServer : Failed to create HttpRouter"));
			return false;
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("WakGamesCallbackServer : Failed to get HttpRouter"));
		return false;
	}
}

void UWakgamesCallbackServer::HandleRequests(const FHttpServerRequest& Request,
                                             const FHttpResultCallback& OnComplete)
{
	if (!WakGamesSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("WakGamesSubsystem is null"));
		// 500 Internal Server Error 반환
		TUniquePtr<FHttpServerResponse> Response = FHttpServerResponse::Create(
			TEXT("Internal Server Error"), TEXT("text/plain"));
		Response->Code = EHttpServerResponseCodes::ServerError;
		OnComplete(MoveTemp(Response));
		return;
	}
	
	if (Request.RelativePath.GetPath().Left(9).Equals(TEXT("/callback"), ESearchCase::IgnoreCase))
	{
		TUniquePtr<FHttpServerResponse> Response = FHttpServerResponse::Create(
			TEXT("Not Found"), TEXT("text/plain"));
		Response->Code = EHttpServerResponseCodes::NotFound;
		OnComplete(MoveTemp(Response));
		return;
	}

	const TMap<FString, FString>& QueryParams = Request.QueryParams;

	FString Code, State, Error, Message;
	if (QueryParams.Contains(TEXT("code"))) Code = QueryParams[TEXT("code")];
	if (QueryParams.Contains(TEXT("state"))) State = QueryParams[TEXT("state")];
	if (QueryParams.Contains(TEXT("error")))
	{
		UE_LOG(LogTemp, Error, TEXT("WakGamesCallbackServer : Error = %s"),
		       *QueryParams[TEXT("error")]);
		Error = QueryParams[TEXT("error")];
	}
	if (QueryParams.Contains(TEXT("message"))) Message = QueryParams[TEXT("message")];

	bool bSuccess = (Error.IsEmpty() && State == CsrfState);

	if (bSuccess)
	{
		GetToken(Code);
		TUniquePtr<FHttpServerResponse> Response = FHttpServerResponse::Create(
			TEXT("Success"), TEXT("text/plain"));
		Response->Code = EHttpServerResponseCodes::Moved;

		TArray<FString, FDefaultAllocator> LocationHeader;
		LocationHeader.Add(
			FString::Printf(
				TEXT("%s/oauth/authorize?success=1"), *WakGamesSubsystem->GetHost()));
		Response->Headers.Add(TEXT("Location"), MoveTemp(LocationHeader));
		OnComplete(MoveTemp(Response));
	}

	if (!bSuccess)
	{
		TUniquePtr<FHttpServerResponse> Response = FHttpServerResponse::Create(
			TEXT("Bad Request"), TEXT("text/plain"));
		Response->Code = EHttpServerResponseCodes::BadRequest;
		Response->Body = TArray<uint8>((const uint8*)TCHAR_TO_UTF8(*Error), Error.Len());
		OnComplete(MoveTemp(Response));
	}
}

void UWakgamesCallbackServer::GetToken(const FString& Code)
{
	if (!WakGamesSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("WakGames instance is null"));
		return;
	}

	FString CallbackUri = FGenericPlatformHttp::UrlEncode(
		FString::Printf(
			TEXT("http://localhost:%d/callback"), WakGamesSubsystem->GetCallbackServerPort()));
	FString GetTokenUri = FString::Printf(
		TEXT(
			"%s/api/oauth/token?grantType=authorization_code&clientId=%s&code=%s&verifier=%s&callbackUri=%s"),
		*WakGamesSubsystem->GetHost(), *ClientId, *Code, *CodeVerifier, *CallbackUri);

	UE_LOG(LogTemp, Log, TEXT("WakGamesCallbackServer : %s"), *GetTokenUri);
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetURL(GetTokenUri);
	HttpRequest->SetVerb(TEXT("POST"));
	HttpRequest->
		SetHeader(TEXT("User-Agent"), FString::Printf(TEXT("Wakgames_Game/%s"), *ClientId));
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

	HttpRequest->OnProcessRequestComplete().BindLambda(
		[this](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
		{
			if (bWasSuccessful && (Response->GetResponseCode() == 200 || Response->GetResponseCode()
				== 201 || Response->GetResponseCode() == 204 || Response->GetResponseCode() == 304))
			{
				FString ResponseString = Response->GetContentAsString();
				TSharedPtr<FJsonObject> JsonObject;
				TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);

				if (FJsonSerializer::Deserialize(Reader, JsonObject))
				{
					AccessToken = JsonObject->GetStringField(TEXT("accessToken"));
					RefreshToken = JsonObject->GetStringField(TEXT("refreshToken"));
					IdToken = JsonObject->GetStringField(TEXT("idToken"));

					// Store the tokens in the subsystem
					WakGamesSubsystem->UpdateTokenStorage(this->AccessToken, this->RefreshToken,
					                                      this->IdToken);


					UE_LOG(LogTemp, Log, TEXT("WakGamesCallbackServer : Token received"));
					UE_LOG(LogTemp, Log, TEXT("WakGamesCallbackServer : AccessToken = %s"),
					       *AccessToken);
					UE_LOG(LogTemp, Log, TEXT("WakGamesCallbackServer : RefreshToken = %s"),
					       *RefreshToken);
					UE_LOG(LogTemp, Log, TEXT("WakGamesCallbackServer : IdToken = %s"), *IdToken);

					WakGamesSubsystem->UpdateTokenStorage(AccessToken, RefreshToken, IdToken);
					// TODO: BROADCAST BELOW NOT IN USE
					OnTokenIssued.Broadcast(true);
				}
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("HTTP Response: %s"), *Response->GetContentAsString());
				UE_LOG(LogTemp, Log, TEXT("HTTP Response Code: %d"), Response->GetResponseCode());
			}
		});

	HttpRequest->ProcessRequest();
}
