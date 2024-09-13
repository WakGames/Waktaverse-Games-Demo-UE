// // Fill out your copyright notice in the Description page of Project Settings.
//
//
// #include "Auth/WakGamesDefaultTokenStorage.h"
// #include "Misc/ConfigCacheIni.h"
//
// FString UWakGamesDefaultTokenStorage::GetAccessToken() const
// {
//     return AccessToken;
// }
//
// FString UWakGamesDefaultTokenStorage::GetRefreshToken() const
// {
//     return RefreshToken;
// }
//
// int32 UWakGamesDefaultTokenStorage::GetIdToken() const
// {
//     return IdToken;
// }
//
// void UWakGamesDefaultTokenStorage::UpdateToken(const FString& NewAccessToken, const FString& NewRefreshToken, int32 NewIdToken)
// {
//     AccessToken = NewAccessToken;
//     RefreshToken = NewRefreshToken;
//     IdToken = NewIdToken;
//
//     SaveTokens();
// }
//
// void UWakGamesDefaultTokenStorage::ClearToken()
// {
//     UpdateToken(TEXT(""), TEXT(""), -1);
// }
//
// void UWakGamesDefaultTokenStorage::LoadTokens()
// {
//     GConfig->GetString(TEXT("/Script/WakGamesSDK.WakgamesDefaultTokenStorage"), TEXT("AccessToken"), AccessToken, GGameIni);
//     GConfig->GetString(TEXT("/Script/WakGamesSDK.WakgamesDefaultTokenStorage"), TEXT("RefreshToken"), RefreshToken, GGameIni);
//     GConfig->GetInt(TEXT("/Script/WakGamesSDK.WakgamesDefaultTokenStorage"), TEXT("IdToken"), IdToken, GGameIni);
// }
//
// void UWakGamesDefaultTokenStorage::SaveTokens() const
// {
//     GConfig->SetString(TEXT("/Script/WakGamesSDK.WakgamesDefaultTokenStorage"), TEXT("AccessToken"), *AccessToken, GGameIni);
//     GConfig->SetString(TEXT("/Script/WakGamesSDK.WakgamesDefaultTokenStorage"), TEXT("RefreshToken"), *RefreshToken, GGameIni);
//     GConfig->SetInt(TEXT("/Script/WakGamesSDK.WakgamesDefaultTokenStorage"), TEXT("IdToken"), IdToken, GGameIni);
//     GConfig->Flush(false, GGameIni);  // 변경사항을 디스크에 저장
// }
//
// void UWakGamesDefaultTokenStorage::BeginDestroy()
// {
//     SaveTokens();  // Ensure tokens are saved when the object is destroyed
//     Super::BeginDestroy();
// }