// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "HttpManager.h"
#include "Json.h"
#include "WakgamesAuth.h"
#include "WakSDK_TokenStorage.h"
#include "Kismet/GameplayStatics.h"
#include "WakSDK_GameInstanceSubsystem.generated.h"

/**
 * 
 */

USTRUCT(BlueprintType)
struct FTokenStorage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString AccessToken = "";

	UPROPERTY(BlueprintReadOnly)
	FString RefreshToken = "";

	UPROPERTY(BlueprintReadOnly)
	FString IdToken = "";

	/** Helper Method to update TokenStorage */
	void UpdateToken(const FString& NewAccessToken, const FString& NewRefreshToken, const FString& NewIdToken)
	{
		AccessToken = NewAccessToken;
		RefreshToken = NewRefreshToken;
		IdToken = NewIdToken;
	}

	/** Helper Method to clear TokenStorage */
	void ClearToken()
	{
		AccessToken.Empty();
		RefreshToken.Empty();
		IdToken.Empty();
	}
};

USTRUCT(BlueprintType)
struct FUserProfile
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString Id = "";

	UPROPERTY(BlueprintReadOnly)
	FString Name = "";

	UPROPERTY(BlueprintReadOnly)
	FString ProfileImg = "";

	/** Helper Method to update TokenStorage */
	void UpdateUserProfile(const FString& NewId, const FString& NewName, const FString& NewProfileImg)
	{
		Id = NewId;
		Name = NewName;
		ProfileImg = NewProfileImg;
	}

	/** Helper Method to clear TokenStorage */
	void ClearUserProfile()
	{
		Id.Empty();
		Name.Empty();
		ProfileImg.Empty();
	}
};




UCLASS(BlueprintType)
class WAKGAMESSDK_API UWakSDK_GameInstanceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UPROPERTY(BlueprintReadOnly, Category = "WakGames|Auth")
	bool bLoggedIn = false;
	
	UFUNCTION(BlueprintCallable, Category = "WakGames|Auth")
	void StartLogin();

	UFUNCTION(BlueprintCallable, Category = "WakGames|Auth")
	void RefreshToken();

	UFUNCTION(BlueprintCallable, Category = "WakGames|Auth")
	void FetchUserProfile();

	UFUNCTION()
	FString GetHost() const { return Host; }

	UFUNCTION()
	int32 GetCallbackServerPort() const { return CallbackServerPort; }

	
	// TokenStorage Related
	UFUNCTION(NotBlueprintable)
	void UpdateTokenStorage(const FString& NewAccessToken, const FString& NewRefreshToken, const FString& NewIdToken);
		
	UFUNCTION(NotBlueprintable)
	bool LoadTokenStorage();
	
	UFUNCTION(BlueprintCallable, Category = "WakGames|Auth")
	void ClearTokenStorage()
	{
		TokenStorage.ClearToken();
	}

	UFUNCTION(BlueprintCallable, Category = "WakGames|Auth")
	void Logout()
	{
		ClearTokenStorage();
		ClearUserProfile();

		UWakSDK_TokenStorage* TokenSaveGame = Cast<UWakSDK_TokenStorage>(
			UGameplayStatics::CreateSaveGameObject(UWakSDK_TokenStorage::StaticClass())
		);

		if (TokenSaveGame)
		{
			TokenSaveGame->AccessToken.Empty();
			TokenSaveGame->RefreshToken.Empty();
			TokenSaveGame->IdToken.Empty();

			UGameplayStatics::SaveGameToSlot(TokenSaveGame, TokenSaveGame->GetSaveSlotName(), 0);
		}

		bLoggedIn = false;
	}

	UFUNCTION(BlueprintCallable, Category = "WakGames|Auth")
	void TryAutoLogin()
	{
		if (LoadTokenStorage())
		{
			FetchUserProfile();
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("No saved token found"));
		}
	}
	
	// User Profile Related
	UFUNCTION(BlueprintCallable, Category = "WakGames|UserProfile")
	void updateUserProfile(const FString& NewId, const FString& NewName, const FString& NewProfileImg)
	{
		UserProfile.UpdateUserProfile(NewId, NewName, NewProfileImg);
	}

	UFUNCTION(BlueprintCallable, Category = "WakGames|UserProfile")
	void ClearUserProfile()
	{
		UserProfile.ClearUserProfile();
	}

	UFUNCTION(BlueprintCallable, Category = "WakGames|UserProfile")
	FUserProfile GetUserProfile() const { return UserProfile; }
	
	// Multi-cast delegates
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnTokenRefreshCompleted, bool);
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnGetUserProfileCompleted, const FUserProfile&, bool);

	FOnTokenRefreshCompleted OnTokenRefreshCompleted;
	FOnGetUserProfileCompleted OnGetUserProfileCompleted;
	
	// Delegate 콜백 메서드
	UFUNCTION()
	void OnTokenIssued(const bool bSuccess) { FetchUserProfile(); }


private:
	FString CsrfState;
	FString CodeVerifier;
	FString CodeChallenge;

	UPROPERTY()
	FTokenStorage TokenStorage;

	UPROPERTY()
	FUserProfile UserProfile;
	
	FString Host = TEXT("https://waktaverse.games");
	// TODO: ClientId와 CallbackServerPort는 블루프린트에서 Set 가능하도록 수정
	FString ClientId = TEXT("wakgames_demo");
	int32 CallbackServerPort = 27610;

	class UWakgamesCallbackServer* CallbackServer;

	// HTTP 요청 관련 콜백 함수
	void OnTokenResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void OnRefreshTokenResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void OnGetUserProfileReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

	void ApiMethod(const FString& Api, const FString& Verb, const FString& Content, TFunction<void(FHttpResponsePtr, bool)> Callback, bool bIsRetry = false);

	void HandleGameExit();
};
