// Fill out your copyright notice in the Description page of Project Settings.


#include "WakgamesCallbackServer.h"

void UWakgamesCallbackServer::StartServer(int32 ListenPort)
{
    if (IsRunning)
    {
        UE_LOG(LogTemp, Error, TEXT("WakGamesCallbackServer : Server is already running"));
        return;
    }

    FHttpServerModule* HttpServerModule = &FHttpServerModule::Get();
    if (HttpServerModule)
    {
        HttpsRouter = HttpServerModule->GetHttpRouter(ListenPort);
        if (HttpRouter.IsValid())
        {
            RouteHande = HttpRouter->BindRoute(
                FHttpPath("/callback"),
                EHttpServerRequestVerbs::VERB_GET,
                [this](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
                {
                    HandleRequests(Request, OnComplete);
                    return true;
                }
            );

            IsRunning = true;
            UE_LOG(LogTemp, Log, TEXT("WakGamesCallbackServer : Server started on port %d"), ListenPort);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("WakGamesCallbackServer : Failed to create HttpRouter"));
        }
    } 
}

void UWakgamesCallbackServer::HandleRequests(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
    if (Request.RelativePath != TEXT("/callback"))
    {
        // OnComplete.Execute(FHttpServerResponse(404));
        TSharedPtr<FHttpServerResponse> Response = FHttpServerResponse::Create();
        Response->Code = EHttpServerResponseCodes::NotFound;
        Response->Body = FString("Not Found");
        OnComplete(MoveTemp(Response));
        return;
    }

    TMap<FString, FString> QueryParams;
    if (ParseQueryString(Request.QueryString, QueryParams))
    {
        FString Code, State, Error, Message;
        QueryParams.Find(TEXT("code"), Code);
        QueryParams.Find(TEXT("state"), State);
        QueryParams.Find(TEXT("error"), Error);
        QueryParams.Find(TEXT("message"), Message);

        bool bSuccess = Error.IsEmpty() && State == CsrfState;

        if(bSuccess)
        {
            try
            {
                {
                    GetToken(Code);
                    TSharedPtr<FHttpServerResponse> Response = FHttpServerResponse::Create();
                    Response->Code = EHttpServerResponseCodes::Moved;
                    Response->Headers.Add(TEXT("Location"), FString::Printf(TEXT("%s/oauth/authorize?success=1"), Wakgames.Host));
                    OnComplete(MoveTemp(Response));
                }
            }
            catch(const std::exception& e)
            {
                UE_LOG(LogTemp, Error, TEXT("WakGamesCallbackServer : %s"), UTF8_TO_TCHAR(e.what()));
                bSuccess = false;
            }
        }

        if (!bSuccess)
        {
            TSharedPtr<FHttpServerResponse> Response = FHttpServerResponse::Create();
            Response->Code = EHttpServerResponseCodes::BadRequest;
            Response->Body = (Error.IsEmpty() && Message.IsEmpty()) ? TEXT("Error: Invalid State") : FString::Printf(TEXT("%s: %s"), *Error, *Message);
            OnComplete(MoveTemp(Response));
        }
    }
}

void UWakgamesCallbackServer::GetToken(const FString& Code)
{
    FString CallbackUri = FGenericPlatformHttp::UrlEncode(FString::Printf(TEXT("http://localhost:%d/callback"), HttpRouter->GetPort()));
    FString GetTokenUri = FString::Printf(TEXT("%s/api/oauth/token?grantType=authorization_code&clientId=%s&code=%s&verifier=%s&callbackUri=%s"),
        *Wakgames.Host, *ClientId, *Code, *CodeVerifier, *CallbackUri);

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
    HttpRequest->SetURL(GetTokenUri);
    HttpRequest->SetVerb(TEXT("POST"));
    HttpRequest->SetUserAgent(TEXT("Wakgames_Game/%s", *ClientId))
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
                IdToken = JsonObject->GetIntegerField(TEXT("idToken"));
            }
        }
    });

    HttpRequest->ProcessRequest();
}

void UWakgamesCallbackServer::ParseQueryString(const FString& QueryString, TMap<FString, FString>& Params)
{
    FString TempQueryString = QueryString;
    if (TempQueryString.StartsWith(TEXT("?")))
    {
        TempQueryString = TempQueryString.RightChop(1);
    }

    TArray<FString> Pairs;
    TempQueryString.ParseIntoArray(Pairs, TEXT("&"), true);
    for (const FString& Pair : Pairs)
    {
        FString Key, Value;
        if (Pair.Split(TEXT("="), &Key, &Value))
        {
            Params.Add(FURL::Decode(Key), FURL::Decode(Value));
        }
    }

    return Params.Num() > 0;
}