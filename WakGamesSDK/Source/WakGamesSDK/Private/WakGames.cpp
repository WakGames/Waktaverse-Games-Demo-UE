// Copyrightⓒ2024 Waktaverse Games All rights reserved. 


#include "WakGames.h"
#include "WakGamesAuth.h"

void UWakGames::StartLogin()
{
    CsrfState = UWakGamesAuth::GenerateCsrfState();
    CodeVerifier = UWakGamesAuth::GenerateCodeVerifier();
    CodeChallenge = UWakGamesAuth::GenerateCodeChallenge(CodeVerifier);

    // 콜백 서버 시작
    CallbackServer = NewObject<UWakgamesCallbackServer>();
    CallbackServer->ClientId = ClientId;
    CallbackServer->CsrfState = CsrfState;
    CallbackServer->CodeVerifier = CodeVerifier;
    CallbackServer->StartServer(CallbackServerPort);

    // OAuth 인증 페이지 열기
    FString AuthUri = FString::Printf(TEXT("%s/oauth/authorize?responseType=code&clientId=%s&state=%s&callbackUri=%s&challengeMethod=S256&challenge=%s"), 
        *Host, *ClientId, *CsrfState, 
        *FGenericPlatformHttp::UrlEncode(FString::Printf(TEXT("http://localhost:%d/callback"), CallbackServerPort)),
        *CodeChallenge);
    FPlatformProcess::LaunchURL(*AuthUri, nullptr, nullptr);
}

UWakGames* UWakGames::CreateWakGames(UObject* Outer)
{
    return NewObject<UWakGames>(Outer);
}