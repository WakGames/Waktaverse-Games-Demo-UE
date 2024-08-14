// Fill out your copyright notice in the Description page of Project Settings.

#include "WakgamesCallbackServer.h"
#include "WakGames.h"

UWakGames* WakGames;

UWakgamesCallbackServer::UWakgamesCallbackServer()
{
    IsRunning = false;
}

void UWakgamesCallbackServer::StartServer(int32 ListenPort)
{
    if (IsRunning)
    {
        UE_LOG(LogTemp, Error, TEXT("WakGamesCallbackServer : Server is already running"));
        return;
    }

    while (ListenPort == 0)
    {
        ListenPort = FindAvailablePort();
    }

    FHttpServerModule& HttpServerModule = FHttpServerModule::Get();
    HttpRouter = HttpServerModule.GetHttpRouter(ListenPort);
  
    if (HttpRouter.IsValid())
    {
        // Lambda를 TFunction으로 명시적으로 캐스팅하여 FHttpRequestHandler로 변환
        RouteHandle = HttpRouter->BindRoute(
            FHttpPath(TEXT("/callback")),
            EHttpServerRequestVerbs::VERB_GET,
            FHttpRequestHandler::CreateLambda([this](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete) -> bool
            {
                HandleRequests(Request, OnComplete);
                return true;
            })
        );

        HttpServerModule.StartAllListeners();
        IsRunning = true;
        UE_LOG(LogTemp, Log, TEXT("WakGamesCallbackServer : Server started on port %d"), ListenPort);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("WakGamesCallbackServer : Failed to create HttpRouter"));
    }
}

void UWakgamesCallbackServer::HandleRequests(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
    if (Request.RelativePath.GetPath() != TEXT("/callback"))
    {
        TUniquePtr<FHttpServerResponse> Response = FHttpServerResponse::Create(TEXT("Not Found"), TEXT("text/plain"));
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
        UE_LOG(LogTemp, Error, TEXT("WakGamesCallbackServer : Error = %s"), *QueryParams[TEXT("error")] );
        Error = QueryParams[TEXT("error")];
    }
    if (QueryParams.Contains(TEXT("message"))) Message = QueryParams[TEXT("message")];

    bool bSuccess = Error.IsEmpty() && State == CsrfState;

    if (bSuccess)
    {
        try
        {
            GetToken(Code);
            TUniquePtr<FHttpServerResponse> Response = FHttpServerResponse::Create(TEXT("Success"), TEXT("text/plain"));
            Response->Code = EHttpServerResponseCodes::Moved;

            TArray<FString, FDefaultAllocator> LocationHeader;
            LocationHeader.Add(FString::Printf(TEXT("%s/oauth/authorize?success=1"), *WakGames->GetHost()));
            Response->Headers.Add(TEXT("Location"), MoveTemp(LocationHeader));
            OnComplete(MoveTemp(Response));
        }
        catch (const std::exception& e)
        {
            UE_LOG(LogTemp, Error, TEXT("Exception from WakGamesCallbackServer : %s"), *FString(e.what()));
            bSuccess = false;
        }
    }

    if (!bSuccess)
    {
        TUniquePtr<FHttpServerResponse> Response = FHttpServerResponse::Create(TEXT("Bad Request"), TEXT("text/plain"));
        Response->Code = EHttpServerResponseCodes::BadRequest;
        Response->Body = TArray<uint8>((const uint8*)TCHAR_TO_UTF8(*Error), Error.Len());
        OnComplete(MoveTemp(Response));
    }
}

void UWakgamesCallbackServer::GetToken(const FString& Code)
{
    FString CallbackUri = FGenericPlatformHttp::UrlEncode(FString::Printf(TEXT("http://localhost:%d/callback"), WakGames->GetCallbackServerPort()));
    FString GetTokenUri = FString::Printf(TEXT("%s/api/oauth/token?grantType=authorization_code&clientId=%s&code=%s&verifier=%s&callbackUri=%s"),
        *WakGames->GetHost(), *ClientId, *Code, *CodeVerifier, *CallbackUri);

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
    HttpRequest->SetURL(GetTokenUri);
    HttpRequest->SetVerb(TEXT("POST"));
    HttpRequest->SetHeader(TEXT("User-Agent"), FString::Printf(TEXT("Wakgames_Game/%s"), *ClientId));
    HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

    HttpRequest->OnProcessRequestComplete().BindLambda([this](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
    {
        if (bWasSuccessful && Response.IsValid())
        {
            FString ResponseString = Response->GetContentAsString();
            TSharedPtr<FJsonObject> JsonObject;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);

            if (FJsonSerializer::Deserialize(Reader, JsonObject))
            {
                AccessToken = JsonObject->GetStringField(TEXT("accessToken"));
                RefreshToken = JsonObject->GetStringField(TEXT("refreshToken"));
                IdToken = JsonObject->GetStringField(TEXT("idToken"));

                UE_LOG(LogTemp, Log, TEXT("WakGamesCallbackServer : Token received"));
                UE_LOG(LogTemp, Log, TEXT("WakGamesCallbackServer : AccessToken = %s"), *AccessToken);
                UE_LOG(LogTemp, Log, TEXT("WakGamesCallbackServer : RefreshToken = %s"), *RefreshToken);
                UE_LOG(LogTemp, Log, TEXT("WakGamesCallbackServer : IdToken = %s"), *IdToken);
            }
        }
    });

    HttpRequest->ProcessRequest();
}

int32 UWakgamesCallbackServer::FindAvailablePort()
{
    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    TSharedRef<FInternetAddr> Addr = SocketSubsystem->CreateInternetAddr();

    Addr->SetAnyAddress();
    Addr->SetPort(0);
  
    FSocket* Socket = SocketSubsystem->CreateSocket(NAME_Stream, TEXT("FindAvailablePort"), false);
  
    bool bBindSuccess = Socket->Bind(*Addr);
  
    if (!bBindSuccess)
    {
        Socket->Close();
        return 0;
    }

    int32 Port = Socket->GetPortNo();
  
    Socket->Close();
    return Port;
}
