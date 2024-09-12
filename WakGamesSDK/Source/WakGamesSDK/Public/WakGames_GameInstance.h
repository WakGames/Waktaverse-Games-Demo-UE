// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WakGames.h"
#include "Engine/GameInstance.h"
#include "WakGames_GameInstance.generated.h"

/**
 * 
 */
UCLASS()
class WAKGAMESSDK_API UWakGames_GameInstance : public UGameInstance
{
	GENERATED_BODY()
	virtual void Init() override;
	
public:
	UFUNCTION(BlueprintCallable, BlueprintGetter, Category = "WakGames|GameInstance")
	UWakGames* GetWakGamesInstance() const { return WakGamesInstance; }

	UFUNCTION(BlueprintCallable, BlueprintSetter, Category = "WakGames|GameInstance")
	void SetWakGamesInstance(UWakGames* NewWakGamesInstance) { this->WakGamesInstance = NewWakGamesInstance; }	

private:
	UPROPERTY(BlueprintGetter = GetWakGamesInstance, BlueprintSetter = SetWakGamesInstance)
    UWakGames* WakGamesInstance = nullptr;
};
