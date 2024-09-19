// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Blueprint/UserWidget.h"
#include "NotiManager.generated.h"

/**
 * UNotiManager is a management class responsible for handling the achievement notifications from Waktaverse Games Service and displaying them on the screen.
 * It is a singleton class that can be accessed from anywhere in the game.
 * Every notification panel is treated as a separate widget and is added to the queue to be managed by the UNotiManager (Handling Add and Removal of notification panel).
 */
UCLASS(Blueprintable)
class WAKGAMESSDK_API UNotiManager : public UObject
{
	GENERATED_BODY()
	UNotiManager();
	
public:
    /** 새로운 알림창을 큐에 추가하고, 큐에 존재하는 모든 기활성 패널을 위로 한 칸 이동 */
    UFUNCTION(BlueprintCallable, Category="WakGames|Notification")
    void AddNotification(FString Title, FString Description, FString IconURL);

    /** 알림창 큐를 관리하는 함수 (만료된 알림창이 있는지 체크, 업데이트)*/
    UFUNCTION()
    void ProcessQueue();

    /** 알림창이 소멸될 때 호출되는 함수 */
    UFUNCTION()
    void OnNotificationExpired(UUserWidget* ExpiredNotification);

protected:
    // 알림창 패널이 구현되어있는 UMG 블루프린트 클래스에 대한 참조 (WBP_NotiPanel.uasset)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="WakGames|Notification")
    TSubclassOf<UUserWidget> NotificationWidgetClass;
    
    // 알림창 패널이 구현되어있는 UMG 블루프린트 클래스의 경로
    static const FString NotificationWidgetPath;

    // 새로운 알림창의 Y 위치
    float GetNextNotificationPositionY(float NotiIndex = -1) const;

private:
	/** 현재 활성 상태의 알림창 리스트 */
	UPROPERTY()
	TArray<UUserWidget*> ActiveNotifications;
        
	// 최대 표시 가능한 알림창 수
	const int32 MaxNotificationsOnScreen = 10;

	// 알림창의 표시 시간 (초 단위)
	const float NotificationDuration = 5.0f;
};