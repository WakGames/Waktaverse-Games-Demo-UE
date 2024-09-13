// Fill out your copyright notice in the Description page of Project Settings.


#include "SDK/WakSDK_GameInstanceSubsystem.h"
#include "WakgamesCallbackServer.h"
#include "WakGamesAuth.h"
#include "HttpModule.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "JsonUtilities.h"
#include "Kismet/GameplayStatics.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

void UWakSDK_GameInstanceSubsystem::Deinitialize()
{
	Super::Deinitialize();

	HandleGameExit();
}


// TODO: 1초에 5번 정도의 속도로 반복 호출할 경우 Race Condition이 발생하여 크래시가 일어날 수 있음. 임시로 초당 버튼 클릭 가능 횟수를 제한하도록 하는걸 추천합니다.
void UWakSDK_GameInstanceSubsystem::StartLogin()
{
	CsrfState = UWakGamesAuth::GenerateCsrfState();
	CodeVerifier = UWakGamesAuth::GenerateCodeVerifier();
	CodeChallenge = UWakGamesAuth::GenerateCodeChallenge(CodeVerifier);

	// 콜백 서버 시작
	if (CallbackServer == nullptr)
	{
		CallbackServer = NewObject<UWakgamesCallbackServer>(this);
		CallbackServer->AddToRoot();
	}
	else
	{
		CallbackServer->StopServer();
		CallbackServer = nullptr;
		StartLogin();
	}
	
	if (!CallbackServer->OnTokenIssued.IsAlreadyBound(this, &UWakSDK_GameInstanceSubsystem::OnTokenIssued))
	{
		CallbackServer->OnTokenIssued.AddDynamic(this, &UWakSDK_GameInstanceSubsystem::OnTokenIssued);
	}
	CallbackServer->SetClientId(ClientId);
	CallbackServer->SetCsrfState(CsrfState);
	CallbackServer->SetCodeVerifier(CodeVerifier);
	CallbackServer->StartServer(CallbackServerPort, this);

	// OAuth 인증 페이지 열기
	FString AuthUri = FString::Printf(
		TEXT(
			"%s/oauth/authorize?responseType=code&clientId=%s&state=%s&callbackUri=%s&challengeMethod=S256&challenge=%s"),
		*Host, *CallbackServer->GetClientId(), *CallbackServer->GetCsrfState(),
		*FGenericPlatformHttp::UrlEncode(
			FString::Printf(TEXT("http://localhost:%d/callback"), CallbackServerPort)),
		*CodeChallenge);
	UE_LOG(LogTemp, Error, TEXT("왁겜즈 로그인 시작 : \"딸깍\""));
	FPlatformProcess::LaunchURL(*AuthUri, nullptr, nullptr);

	// FTimerHandle TimerHandle;
	// GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
	// {
	// 	UE_LOG(LogTemp, Warning, TEXT("Login Timeout"));
	// 	if (CallbackServer)
	// 	CallbackServer->StopServer();
	// }, 60.0f, false);
}

void UWakSDK_GameInstanceSubsystem::RefreshToken()
{
	if (TokenStorage.RefreshToken.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("RefreshToken is empty"));
		OnTokenRefreshCompleted.Broadcast(false);
		return;
	}

	FString Url = FString::Printf(TEXT("%s/api/oauth/refresh"), *Host);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->OnProcessRequestComplete().BindUObject(
		this, &UWakSDK_GameInstanceSubsystem::OnRefreshTokenResponseReceived);

	Request->SetURL(Url);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetHeader(TEXT("User-Agent"), FString::Printf(TEXT("WakGames_Game/%s"), *ClientId));
	Request->SetHeader(
		TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *TokenStorage.RefreshToken));

	Request->ProcessRequest();
}

void UWakSDK_GameInstanceSubsystem::FetchUserProfile()
{
	FString Url = FString::Printf(TEXT("%s/api/game-link/user/profile"), *Host);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->OnProcessRequestComplete().BindUObject(
		this, &UWakSDK_GameInstanceSubsystem::OnGetUserProfileReceived);

	Request->SetURL(Url);
	Request->SetVerb(TEXT("GET"));
	Request->SetHeader(
		TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *TokenStorage.AccessToken));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetHeader(TEXT("User-Agent"), FString::Printf(TEXT("WakGames_Game/%s"), *ClientId));

	Request->ProcessRequest();
}




void UWakSDK_GameInstanceSubsystem::OnRefreshTokenResponseReceived(
	FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (bWasSuccessful && Response->GetResponseCode() == 200 || Response->GetResponseCode() == 201
		|| Response->GetResponseCode() == 204 || Response->GetResponseCode() == 304)
	{
		FString ResponseString = Response->GetContentAsString();
		UE_LOG(LogTemp, Log, TEXT("Refresh Token Response: %s"), *ResponseString);

		FTokenStorage Result;
		if (FJsonObjectConverter::JsonObjectStringToUStruct(ResponseString, &Result, 0, 0))
		{
			UpdateTokenStorage(Result.AccessToken, Result.RefreshToken, Result.IdToken);
			UE_LOG(LogTemp, Log, TEXT("Token refreshed and updated successfully"));
			OnTokenRefreshCompleted.Broadcast(true);
			return;
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to refresh token. Response Code: %d"),
		       Response->GetResponseCode());
	}

	OnTokenRefreshCompleted.Broadcast(false);
}

void UWakSDK_GameInstanceSubsystem::OnGetUserProfileReceived(FHttpRequestPtr Request,
                                                          FHttpResponsePtr Response,
                                                          bool bWasSuccessful)
{
	if (bWasSuccessful && Response->GetResponseCode() == 200 || Response->GetResponseCode() == 201
		|| Response->GetResponseCode() == 204 || Response->GetResponseCode() == 304)
	{
		FString ResponseString = Response->GetContentAsString();
		UE_LOG(LogTemp, Log, TEXT("User Profile Response: %s"), *ResponseString);

		FUserProfile Result;
		if (FJsonObjectConverter::JsonObjectStringToUStruct(ResponseString, &Result, 0, 0))
		{
			// TODO: 현재 브로드캐스팅을 듣는 바인딩 함수가 존재하지 않음을 기억.
			OnGetUserProfileCompleted.Broadcast(Result, true);
			UserProfile.UpdateUserProfile(Result.Id, Result.Name, Result.ProfileImg);
			return;
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to retrieve user profile. Response Code: %d"),
		       Response->GetResponseCode());
	}

	OnGetUserProfileCompleted.Broadcast(FUserProfile(), false);
}

void UWakSDK_GameInstanceSubsystem::ApiMethod(const FString& Api, const FString& Verb,
                                              const FString& Content,
                                              TFunction<void(FHttpResponsePtr, bool)> Callback)
{
	FString Url = FString::Printf(TEXT("%s/%s"), *Host, *Api);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->OnProcessRequestComplete().BindLambda(
		[Callback](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
		{
			Callback(Response, bWasSuccessful);
		});

	Request->SetURL(Url);
	Request->SetVerb(Verb);
	Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *TokenStorage.AccessToken));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetHeader(TEXT("User-Agent"), FString::Printf(TEXT("WakGames_Game/%s"), *ClientId));

	if (!Content.IsEmpty())
	{
		Request->SetContentAsString(Content);
	}

	Request->ProcessRequest();
}

void UWakSDK_GameInstanceSubsystem::HandleGameExit()
{
	if (CallbackServer && CallbackServer->IsRooted())
	{
		CallbackServer->StopServer();
		UE_LOG(LogTemp, Log, TEXT("CallbackServer removed from root and marked for garbage collection on game exit."));
	}
}
