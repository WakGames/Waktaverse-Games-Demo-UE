// Copyrightⓒ2024 Waktaverse Games All rights reserved. 

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "HttpManager.h"
#include "Json.h"
#include "WakgamesAuth.h"

#include "WakGames.generated.h"

// using FCallbackDelegate = TFunction<void(TSharedPtr<FJsonObject>, int32)>; // C++에서의 콜백 정의

/**
 * 
 */
USTRUCT(BlueprintType)
struct FRefreshTokenResult
{
    GENERATED_BODY()

    UPROPERTY()
    FString AccessToken;

    UPROPERTY()
    FString RefreshToken;

    UPROPERTY()
    FString IdToken;
};

USTRUCT(BlueprintType)
struct FUserProfileResult
{
    GENERATED_BODY()

    UPROPERTY()
    int32 Id;

    UPROPERTY()
    FString Name;

    UPROPERTY()
    FString ProfileImg;
};

USTRUCT(BlueprintType)
struct FSuccessResult
{
    GENERATED_BODY()

    UPROPERTY()
    bool Success;
};


UCLASS(BlueprintType, Blueprintable)
class WAKGAMESSDK_API UWakGames : public UObject
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category="Wakgames")
	FString GetHost() const { return Host; }

	UFUNCTION(BlueprintCallable, Category="Wakgames")
	int32 GetCallbackServerPort() const { return CallbackServerPort; }

	UFUNCTION(BlueprintGetter, Category="Wakgames")
	FString GetClientId() const { return ClientId; }
	
	UFUNCTION(BlueprintCallable, Category="Wakgames")
    void StartLogin(UWakGames* WakGamesInstance, UWorld* World);

	UFUNCTION(BlueprintCallable, Category = "WakGames")
    void RefreshToken();

    UFUNCTION(BlueprintCallable, Category = "WakGames")
    void GetUserProfile();
	

	// Multi-cast delegates
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnTokenRefreshCompleted, bool);
    DECLARE_MULTICAST_DELEGATE_TwoParams(FOnGetUserProfileCompleted, const FUserProfileResult&, bool);

    FOnTokenRefreshCompleted OnTokenRefreshCompleted;
    FOnGetUserProfileCompleted OnGetUserProfileCompleted;

private:
	FString CsrfState;
    FString CodeVerifier;
    FString CodeChallenge;

	static UWakGames* Instance;
    class UWakgamesCallbackServer* CallbackServer;
	class IWakgamesTokenStorage* TokenStorage;

	UPROPERTY(BlueprintGetter = GetClientId)
	FString ClientId = TEXT("wakgames_demo");

	int32 CallbackServerPort = 27610;

	FString Host = TEXT("https://waktaverse.games");

	void OnTokenResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    void OnRefreshTokenResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    void OnUserProfileReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

    void ApiMethod(const FString& Api, const FString& Verb, const FString& Content, TFunction<void(FHttpResponsePtr, bool)> Callback);
};
