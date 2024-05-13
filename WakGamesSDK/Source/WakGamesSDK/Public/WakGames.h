// Copyrightⓒ2024 Waktaverse Games All rights reserved. 

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "HttpManager.h"
// #include "JsonUtilities.h"
#include "Json.h"
#include "WakgamesAuth.h"
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
	UPROPERTY(BlueprintReadOnly, Category="Wakgames")
		FString HOST = TEXT("https://waktaverse.games");

	UFUNCTION(BlueprintCallable, Category="Wakgames")
    void StartLogin();

private:
	FString ClientId;
	int32 CallbackServerPort;

};
