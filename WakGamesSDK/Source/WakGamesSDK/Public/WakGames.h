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
#include "WakGamesCallbackServer.h"
#include "WakGames.generated.h"

// using FCallbackDelegate = TFunction<void(TSharedPtr<FJsonObject>, int32)>; // C++에서의 콜백 정의

/**
 * 
 */
UCLASS()
class WAKGAMESSDK_API UWakGames : public UObject
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category="Wakgames")
	FString GetHost() const { return Host; }

	UFUNCTION(BlueprintCallable, Category="Wakgames")
	int32 GetCallbackServerPort() const { return CallbackServerPort; }

	UFUNCTION(BlueprintCallable, Category="Wakgames")
    void StartLogin();

	UFUNCTION(BlueprintCallable, Category = "WakGames")
    static UWakGames* CreateWakGames(UObject* Outer);

private:
	UPROPERTY()
    UWakgamesCallbackServer* CallbackServer;
	int32 CallbackServerPort = 65535;
	FString Host = TEXT("https://waktaverse.games");

	FString ClientId = TEXT("wakgames_demo");
	FString CsrfState;
    FString CodeVerifier;
    FString CodeChallenge;
};
