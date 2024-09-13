#include "WakgamesCallbackServer.h"
#include "SDK/WakSDK_GameInstanceSubsystem.h"  // WakGamesSubsystem 헤더 포함
#include "Kismet/GameplayStatics.h"

UWakgamesCallbackServer::UWakgamesCallbackServer()
{
	SetbRunning(false);
	WakGamesSubsystem = nullptr;
	// this->AddToRoot();
}

void UWakgamesCallbackServer::BeginDestroy()
{
	Super::BeginDestroy();
	// StopServer();

	// if (this->IsRooted())
	// {
	// 	this->RemoveFromRoot();
	// }
	// if (!this->IsRooted())
	// {
	// 	this->MarkAsGarbage();
	// }
}

void UWakgamesCallbackServer::StopServer()
{
	if (!GetbRunning())
	{
		UE_LOG(LogTemp, Error, TEXT("WakGamesCallbackServer : Server is not running"));
		return;
	}

	if (this->IsRooted())
	{
		this->RemoveFromRoot();
	}
	if (!this->IsRooted())
	{
		this->MarkAsGarbage();
	}
	
	if (HttpRouter && RouteHandle)
	{
		HttpRouter->UnbindRoute(RouteHandle);
		FHttpServerModule::Get().StopAllListeners();
		HttpRouter.Reset();
		RouteHandle.Reset();
		// SetbRunning(false);

		// this->MarkAsGarbage();
		UE_LOG(LogTemp, Log,
		       TEXT("WakGamesCallbackServer : Callback server stopped and listener unbound."));
	}
	SetbRunning(false);
}

void UWakgamesCallbackServer::StartServer(int32 ListenPort,
                                          UWakSDK_GameInstanceSubsystem* Subsystem)
{
	if (GetbRunning())
	{
		UE_LOG(LogTemp, Warning, TEXT("WakGamesCallbackServer : Server is already running"));
		StopServer();
		StartServer(ListenPort, Subsystem); // restart server
		return;
	}

	if (!Subsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid World context"));
		return;
	}

	if (Subsystem)
	{
		WakGamesSubsystem = Subsystem;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("WakGamesSubsystem instance is null, cannot start server"));
		return;
	}

	FHttpServerModule& HttpServerModule = FHttpServerModule::Get();
	HttpRouter = HttpServerModule.GetHttpRouter(ListenPort);

	if (HttpRouter.IsValid())
	{
		// Lambda를 TFunction으로 명시적으로 캐스팅하여 FHttpRequestHandler로 변환
		RouteHandle = HttpRouter->BindRoute(
			FHttpPath(TEXT("/callback")),
			EHttpServerRequestVerbs::VERB_GET,
			FHttpRequestHandler::CreateLambda(
				[this](const FHttpServerRequest& Request,
				       const FHttpResultCallback& OnComplete) -> bool
				{
					HandleRequests(Request, OnComplete);
					return true;
				})
		);

		HttpServerModule.StartAllListeners();
		SetbRunning(true);
		UE_LOG(LogTemp, Log,
		       TEXT("WakGamesCallbackServer : Server started on port %d, (bRunning: %d)"),
		       ListenPort, GetbRunning());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("WakGamesCallbackServer : Failed to create HttpRouter"));
	}
}

void UWakgamesCallbackServer::HandleRequests(const FHttpServerRequest& Request,
                                             const FHttpResultCallback& OnComplete)
{
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
