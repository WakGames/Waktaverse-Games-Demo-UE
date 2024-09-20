// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "WakSDK_TokenStorage.generated.h"

/**
 * SaveGame Class to store WaktaverseGames OAuth 2.0 Tokens
 */
UCLASS()
class WAKGAMESSDK_API UWakSDK_TokenStorage : public USaveGame
{
	GENERATED_BODY()

public:
	UWakSDK_TokenStorage();

	UPROPERTY(VisibleAnywhere, Category = "Tokens")
	FString AccessToken;

	UPROPERTY(VisibleAnywhere, Category = "Tokens")
	FString RefreshToken;

	UPROPERTY(VisibleAnywhere, Category = "Tokens")
	FString IdToken;

	FString GetSaveSlotName() const { return SaveSlotName; }
protected:
	// NOTE: static 선언이 아니라 로드시에는 하드코딩되어있으므로 SlotName 수정할 경우 반드시 체크
	FString SaveSlotName = TEXT("WakGamesTokenSave");
};
