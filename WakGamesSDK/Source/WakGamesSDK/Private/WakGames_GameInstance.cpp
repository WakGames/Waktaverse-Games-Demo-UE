// Fill out your copyright notice in the Description page of Project Settings.


#include "WakGames_GameInstance.h"

void UWakGames_GameInstance::Init()
{
	Super::Init();
	WakGamesInstance = NewObject<UWakGames>();
	SetWakGamesInstance(WakGamesInstance);
}