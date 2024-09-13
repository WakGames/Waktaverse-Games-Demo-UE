// // Fill out your copyright notice in the Description page of Project Settings.
//
// #pragma once
//
// #include "CoreMinimal.h"
// #include "WakGamesTokenStorage.h"
// #include "WakGamesDefaultTokenStorage.generated.h"
//
//
// /**
//  * 
//  */
// // WakgamesDefaultTokenStorage 클래스가 IWakgamesTokenStorage 인터페이스를 구현
// UCLASS(Blueprintable)
// class WAKGAMESSDK_API UWakGamesDefaultTokenStorage : public UObject, public IWakGamesTokenStorage
// {
//     GENERATED_BODY()
//
// public:
//     // IWakgamesTokenStorage 인터페이스의 메서드 구현
//     virtual FString GetAccessToken() const override;
//     virtual FString GetRefreshToken() const override;
//     virtual int32 GetIdToken() const override;
//
//     virtual void UpdateToken(const FString& AccessToken, const FString& RefreshToken, int32 IdToken) override;
//     virtual void ClearToken() override;
//
// protected:
// 	virtual void BeginDestroy() override;
//
// private:
// 	void LoadTokens();
// 	void SaveTokens() const;
//
// private:
//     FString AccessToken;
//     FString RefreshToken;
//     int32 IdToken;
// };