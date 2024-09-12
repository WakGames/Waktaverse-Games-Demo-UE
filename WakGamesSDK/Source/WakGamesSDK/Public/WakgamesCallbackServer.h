// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "HttpServerRequest.h"
#include "HttpServerResponse.h"
#include "HttpRouteHandle.h"

#include "HttpModule.h"
#include "JsonUtilities.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "IHttpRouter.h"
#include "Dom/JsonObject.h"

#include "HttpServerModule.h"
#include "HttpPath.h"
#include "Sockets.h"
#include "SocketSubsystem.h"

#include "WakGames.h"
#include "WakgamesCallbackServer.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogWakgamesCallbackServer, Log, All);

/**
 * 
 */
UCLASS()
class WAKGAMESSDK_API UWakgamesCallbackServer : public UObject
{
	GENERATED_BODY()
	
public:
	UWakgamesCallbackServer();
	UWakgamesCallbackServer(UWakGames* NewWakGames) { this->WakGames = NewWakGames; }
	virtual void BeginDestroy() override;

	UFUNCTION(BlueprintGetter, Category = "WakGames|CallbackServer")
	FString GetClientId() const { return ClientId; }

	UFUNCTION(BlueprintSetter, Category = "WakGames|CallbackServer")
	void SetClientId(FString NewClientId) { this->ClientId = NewClientId; }
	
	FString GetCsrfState() const { return CsrfState; }
	void SetCsrfState(FString NewCsrfState) { this->CsrfState = NewCsrfState; }

	FString GetCodeVerifier() const { return CodeVerifier; }
	void SetCodeVerifier(FString NewCodeVerifier) { this->CodeVerifier = NewCodeVerifier; }

	bool GetbRunning() const { return bRunning; }
	void SetbRunning(bool NewbRunning) { this->bRunning = NewbRunning; }
	
	void SetWakGames(UWakGames* NewWakGames) { this->WakGames = NewWakGames; }
	
	UFUNCTION(BlueprintCallable, Category = "WakGames|CallbackServer")
	void StartServer(int32 ListenPort, UWorld* World);

	UFUNCTION(BlueprintCallable, Category = "WakGames|CallbackServer")
	void StopServer();

private:
	FHttpRouteHandle RouteHandle;
	TSharedPtr<IHttpRouter> HttpRouter;
	// TODO: GameInstance 클래스로 옮겨서 싱글톤 패턴으로 수정
	UWakGames* WakGames = nullptr;

	UPROPERTY(BlueprintGetter = GetClientId, BlueprintSetter = SetClientId)
	FString ClientId = "";
	
	UPROPERTY(NotBlueprintable)
	FString CsrfState;

	UPROPERTY(NotBlueprintable)
	FString CodeVerifier;

	UPROPERTY(NotBlueprintable)
	bool bRunning = false;

	// TokenResult
	FString AccessToken;
	
	FString RefreshToken;
	
	FString IdToken;

	
	/** 콜백 요청 처리, 쿼리 파라미터 분석하여 토큰 get ? 리다이렉션 : 오류 로그 */
	void HandleRequests(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
	void GetToken(const FString& Code);
	// void ParseQueryString(const FString& QueryString, TMap<FString, FString>& Params);
	int32 FindAvailablePort();

	// void StopServer();
};
