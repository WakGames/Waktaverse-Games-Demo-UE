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

#include "SDK/WakSDK_GameInstanceSubsystem.h"
#include "WakgamesCallbackServer.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogWakgamesCallbackServer, Log, All);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTokenIssuedDelegate, const bool, bSuccess);

/**
 * 
 */
UCLASS()
class WAKGAMESSDK_API UWakgamesCallbackServer : public UObject
{
	GENERATED_BODY()
	
public:
	// Delegate 변수: 토큰 발급이 완료되면 호출
	UPROPERTY(BlueprintAssignable, Category = "Wakgames")
	FOnTokenIssuedDelegate OnTokenIssued;
	
	UWakgamesCallbackServer();
	UWakgamesCallbackServer(UWakSDK_GameInstanceSubsystem* NewWakGames) { this->WakGamesSubsystem = NewWakGames; }
	virtual void BeginDestroy() override;

	UFUNCTION(BlueprintGetter, Category = "WakGames|CallbackServer")
	FString GetClientId() const { return ClientId; }

	UFUNCTION(BlueprintSetter, Category = "WakGames|CallbackServer")
	void SetClientId(FString NewClientId) { ensure(this != nullptr); this->ClientId = NewClientId; }
	
	FString GetCsrfState() const { return CsrfState; }
	void SetCsrfState(FString NewCsrfState) { ensure(this != nullptr); this->CsrfState = NewCsrfState; }

	FString GetCodeVerifier() const { return CodeVerifier; }
	void SetCodeVerifier(FString NewCodeVerifier) { ensure(this != nullptr); this->CodeVerifier = NewCodeVerifier; }

	bool GetbRunning() const { return bRunning; }
	void SetbRunning(bool NewbRunning) { ensure(this != nullptr); this->bRunning = NewbRunning; }
	
	void SetWakGames(UWakSDK_GameInstanceSubsystem* NewWakGames) { ensure(this != nullptr); this->WakGamesSubsystem = NewWakGames; }
	
	UFUNCTION(BlueprintCallable, Category = "WakGames|CallbackServer")
	void StartServer(int32 ListenPort, UWakSDK_GameInstanceSubsystem* Subsystem);

	UFUNCTION(BlueprintCallable, Category = "WakGames|CallbackServer")
	void StopServer();



private:
	FHttpRouteHandle RouteHandle;
	TSharedPtr<IHttpRouter> HttpRouter;
	
	// TODO: GameInstance 클래스로 옮겨서 싱글톤 패턴으로 수정
	UPROPERTY()
	UWakSDK_GameInstanceSubsystem* WakGamesSubsystem = nullptr;

	UPROPERTY(BlueprintGetter = GetClientId, BlueprintSetter = SetClientId)
	FString ClientId = "";
	
	UPROPERTY(NotBlueprintable)
	FString CsrfState = "";

	UPROPERTY(NotBlueprintable)
	FString CodeVerifier = "";

	UPROPERTY(NotBlueprintable)
	bool bRunning = false;

	// TokenResult
	FString AccessToken = "";
	
	FString RefreshToken = "";
	
	FString IdToken = "";

	
	/** 콜백 요청 처리, 쿼리 파라미터 분석하여 토큰 get ? 리다이렉션 : 오류 로그 */
	void HandleRequests(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
	void GetToken(const FString& Code);
	// void ParseQueryString(const FString& QueryString, TMap<FString, FString>& Params);
	int32 FindAvailablePort();

	// void StopServer();
};
