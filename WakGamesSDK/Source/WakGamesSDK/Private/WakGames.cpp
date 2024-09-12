// Copyrightⓒ2024 Waktaverse Games All rights reserved. 


#include "WakGames.h"
#include "WakGamesAuth.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "JsonObjectConverter.h"
#include "WakgamesCallbackServer.h"
#include "Auth/WakgamesTokenStorage.h"

void UWakGames::StartLogin(UWakGames* WakGamesInstance, UWorld* World)
{
    if (WakGamesInstance == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("WakGamesInstance is null"));
        return;
    }
    CsrfState = UWakGamesAuth::GenerateCsrfState();
    CodeVerifier = UWakGamesAuth::GenerateCodeVerifier();
    CodeChallenge = UWakGamesAuth::GenerateCodeChallenge(CodeVerifier);

    UE_LOG(LogTemp, Log, TEXT("CsrfState: %s"), *CsrfState);

    // 콜백 서버 시작
    if (CallbackServer == nullptr)
    {
        UE_LOG(LogTemp, Log, TEXT("CallbackServer is null"));
        CallbackServer = NewObject<UWakgamesCallbackServer>(WakGamesInstance);
    }
    CallbackServer->SetClientId(this->ClientId);
    CallbackServer->SetCsrfState(CsrfState);
    CallbackServer->SetCodeVerifier(CodeVerifier);
    CallbackServer->StartServer(CallbackServerPort, World);

    // OAuth 인증 페이지 열기
    FString AuthUri = FString::Printf(TEXT("%s/oauth/authorize?responseType=code&clientId=%s&state=%s&callbackUri=%s&challengeMethod=S256&challenge=%s"), 
        *Host, *CallbackServer->GetClientId(), *CallbackServer->GetCsrfState(), 
        *FGenericPlatformHttp::UrlEncode(FString::Printf(TEXT("http://localhost:%d/callback"), CallbackServerPort)),
        *CodeChallenge);
    FPlatformProcess::LaunchURL(*AuthUri, nullptr, nullptr);
}

void UWakGames::RefreshToken()
{
    if (TokenStorage->GetRefreshToken().IsEmpty())
    {
        OnTokenRefreshCompleted.Broadcast(false);
        return;
    }

    FString Url = FString::Printf(TEXT("%s/api/oauth/refresh"), *Host);

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->OnProcessRequestComplete().BindUObject(this, &UWakGames::OnRefreshTokenResponseReceived);

    Request->SetURL(Url);
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetHeader(TEXT("User-Agent"), FString::Printf(TEXT("WakGames_Game/%s"), *ClientId));
    Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *TokenStorage->GetRefreshToken()));

    // 헤더 출력
    UE_LOG(LogTemp, Log, TEXT("HTTP Request Headers:"));
    TArray<FString> AllHeaders = Request->GetAllHeaders();
    for (const FString& Header : AllHeaders)
    {
        UE_LOG(LogTemp, Log, TEXT("%s"), *Header);
    }
    // 끝

    Request->ProcessRequest();
}


void UWakGames::OnRefreshTokenResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (bWasSuccessful && Response->GetResponseCode() == 200)
    {
        FString ResponseString = Response->GetContentAsString();
        UE_LOG(LogTemp, Log, TEXT("Refresh Token Response: %s"), *ResponseString);

        FRefreshTokenResult Result;
        if (FJsonObjectConverter::JsonObjectStringToUStruct(ResponseString, &Result, 0, 0))
        {
            TokenStorage->UpdateToken(Result.AccessToken, Result.RefreshToken, FCString::Atoi(*Result.IdToken));
            OnTokenRefreshCompleted.Broadcast(true);
            return;
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to refresh token. Response Code: %d"), Response->GetResponseCode());
    }

    OnTokenRefreshCompleted.Broadcast(true);
}

void UWakGames::GetUserProfile()
{
    FString Url = FString::Printf(TEXT("%s/api/game-link/user/profile"), *Host);

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->OnProcessRequestComplete().BindUObject(this, &UWakGames::OnUserProfileReceived);

    Request->SetURL(Url);
    Request->SetVerb(TEXT("GET"));
    Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *TokenStorage->GetAccessToken()));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetHeader(TEXT("User-Agent"), FString::Printf(TEXT("WakGames_Game/%s"), *ClientId));

    Request->ProcessRequest();
}

void UWakGames::OnUserProfileReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (bWasSuccessful && Response->GetResponseCode() == 200)
    {
        FString ResponseString = Response->GetContentAsString();
        UE_LOG(LogTemp, Log, TEXT("User Profile Response: %s"), *ResponseString);

        FUserProfileResult Result;
        if (FJsonObjectConverter::JsonObjectStringToUStruct(ResponseString, &Result, 0, 0))
        {
            OnGetUserProfileCompleted.Broadcast(Result, true);
            return;
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to retrieve user profile. Response Code: %d"), Response->GetResponseCode());
    }

    // FUserProfileResult EmptyResult;
    OnGetUserProfileCompleted.Broadcast(FUserProfileResult(), false);
}

void UWakGames::ApiMethod(const FString& Api, const FString& Verb, const FString& Content, TFunction<void(FHttpResponsePtr, bool)> Callback)
{
    FString Url = FString::Printf(TEXT("%s/%s"), *Host, *Api);

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->OnProcessRequestComplete().BindLambda([Callback](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
    {
        Callback(Response, bWasSuccessful);
    });

    Request->SetURL(Url);
    Request->SetVerb(Verb);
    Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *TokenStorage->GetAccessToken()));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetHeader(TEXT("User-Agent"), FString::Printf(TEXT("WakGames_Game/%s"), *ClientId));

    if (!Content.IsEmpty())
    {
        Request->SetContentAsString(Content);
    }

    Request->ProcessRequest();
}