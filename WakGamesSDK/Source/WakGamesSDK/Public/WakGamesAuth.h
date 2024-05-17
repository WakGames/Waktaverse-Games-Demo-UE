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
	UPROPERTY(BlueprintCallable, Category = "Utility|Auth")
	static FString GenerateCodeVerifier();

	/** Generates the Code Challenge from the Code Verifier */
	UPROPERTY(BlueprintCallable, Category = "Utility|Auth")
	static FString GenerateCodeChallenge(const FString& CodeVerifier);

	/** Generates a random string for CSRF protection */
    UPROPERTY(BlueprintCallable, Category = "Utility|Auth")
	static FString GenerateCsrfState();
};
