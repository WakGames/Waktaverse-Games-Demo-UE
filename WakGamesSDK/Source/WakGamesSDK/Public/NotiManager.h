// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
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

public:
	UNotiManager();
	
	/** 새로운 알림창을 큐에 추가하고, 큐에 존재하는 모든 기활성 패널의 index를 ++하여 위로 한 칸 이동
	 * 만약 새로운 index가 MaxNotificationsOnScreen보다 크다면, 해당 알림창을 강제로 제거한다.
	 */
	UFUNCTION(BlueprintCallable, Category="WakGames|Notification")
	void AddNotification(const FString& Title, const FString& Description, const FString& IconURL);

	/** 알림창 큐를 관리하는 함수 (NotificationDuration가 만료된 알림창이 있는지 체크하여 정리) */
	UFUNCTION()
	void ProcessQueue();

	/** 알림창을 소멸할 때 호출하는 함수. 큐를 pop하고 RemoveFromParent()를 통해 뷰포트에서도 알림창을 제거 */
	UFUNCTION()
	void OnNotificationExpired(UUserWidget* ExpiredNotification);

protected:
	// UMG를 통해 알림창 패널이 구현되어있는 Widget BP 블루프린트 클래스에 대한 참조 (/Content/Levels/UI/Achievements/WBP_NotiPanel.uasset)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="WakGames|Notification")
	TSubclassOf<UUserWidget> NotificationWidgetClass;

	// 알림창 패널이 구현되어있는 UMG 블루프린트 클래스의 경로
	static FString NotificationWidgetPath;

	// 새로운 알림창의 Y 위치. queue에서의 index를 기준으로 연산한다. (제일 먼더 들어온 원소가 맨 위로 올라가고 새로운 원소는 맨 아래의 고정 위치에 생성)
	float GetNextNotificationPositionY(int32 NotiIndex = -1) const;

private:
	/** 현재 활성 상태의 알림창을 원소호 하는 queue */
	UPROPERTY()
	TArray<UUserWidget*> ActiveNotifications;

	// 최대 표시 가능한 알림창 수
	UPROPERTY(EditDefaultsOnly, Category="WakGames|Notification")
	int32 MaxNotificationsOnScreen = 10;

	// 알림창의 표시 시간 (초 단위)
	UPROPERTY(EditDefaultsOnly, Category="WakGames|Notification")
	float NotificationDuration = 5.0f;
};
