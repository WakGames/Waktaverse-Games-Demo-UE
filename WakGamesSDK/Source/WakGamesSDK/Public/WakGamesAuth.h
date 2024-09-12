// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include <cstdlib>
#include "WakGamesAuth.generated.h"

/**
 * 
 */
UCLASS()
class WAKGAMESSDK_API UWakGamesAuth : public UObject
{
	GENERATED_BODY()
	
public:
	/** Generates a random string for the OAuth Code Verifier */
	UFUNCTION(BlueprintCallable, Category = "WakGames|Auth")
	static FString GenerateCodeVerifier();

	/** Generates the Code Challenge from the Code Verifier */
	UFUNCTION(BlueprintCallable, Category = "WakGames|Auth")
	static FString GenerateCodeChallenge(const FString& CodeVerifier);

	/** Generates a random string for CSRF protection */
    UFUNCTION(BlueprintCallable, Category = "WakGames|Auth")
	static FString GenerateCsrfState();
};
