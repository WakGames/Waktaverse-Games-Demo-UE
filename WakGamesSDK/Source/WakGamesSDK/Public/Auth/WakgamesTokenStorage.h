// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "WakgamesTokenStorage.generated.h"

/**
 * 
 */
// UInterface로 인터페이스 정의
UINTERFACE(MinimalAPI)
class UWakgamesTokenStorage : public UInterface
{
    GENERATED_BODY()
};

// IInterface로 메서드 정의
class IWakgamesTokenStorage
{
    GENERATED_BODY()

public:
    virtual FString GetAccessToken() const = 0;
    virtual FString GetRefreshToken() const = 0;
    virtual int32 GetIdToken() const = 0;

    virtual void UpdateToken(const FString& AccessToken, const FString& RefreshToken, int32 IdToken) = 0;
    virtual void ClearToken() = 0;
};