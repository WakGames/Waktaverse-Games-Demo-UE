// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "HttpServerModule.h"
#include "HttpServerRequest.h"
#include "HttpServerResponse.h"
#include "HttpRouteHandle.h"
#include "HttpServer.h"
#include "WakGames.h"
#include "WakgamesCallbackServer.generated.h"

/**
 * 
 */
UCLASS()
class WAKGAMESSDK_API UWakgamesCallbackServer : public UObject
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ClientId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString CsrfState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString CodeVerifier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool IsRunning = false;

	// TokenResult
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString AccessToken;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString RefreshToken;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString IdToken;

	UFUNCTION(BlueprintCallable, Category = "Utility|CallbackServer")
	void StartServer(int32 ListenPort);

private:
	FHttpRouteHandle RouteHandle;
	TSharedPtr<IHttpRouter> HttpRouter;

	/** 콜백 요청 처리, 쿼리 파라미터 분석하여 토큰 get ? 리다이렉션 : 오류 로그 */
	void HandleRequests(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
	void GetToken(const FString& Code);
	void ParseQueryString(const FString& QueryString, TMap<FString, FString>& Params);

	// void StopServer();
};
