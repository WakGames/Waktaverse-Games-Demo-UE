// Copyrightⓒ2024 Waktaverse Games All rights reserved. 


#include "WakGames.h"
#include "WakGamesAuth.h"

void UWakGames::StartLogin()
{
    FString CsrfState = UWakGamesAuth::GenerateCsrfState();
    FString CodeVerifier = UWakGamesAuth::GenerateCodeVerifier();
    FString CodeChallenge = UWakGamesAuth::GenerateCodeChallenge(CodeVerifier);

    // 콜백 서버 시작 로직 (C++에서는 HTTP 리스너 구현 또는 외부 라이브러리 사용 필요)

    // OAuth 인증 페이지 열기
    FString AuthUri = FString::Printf(TEXT("%s/oauth/authorize?responseType=code&clientId=%s&state=%s&callbackUri=%s&challengeMethod=S256&challenge=%s"), *HOST, *ClientId, *CsrfState, *FString::Printf(TEXT("http://localhost:%d/callback"), CallbackServerPort), *CodeChallenge);
    FPlatformProcess::LaunchURL(*AuthUri, nullptr, nullptr);

    // 로그인 상태 체크 및 콜백 호출 로직 구현 필요
}