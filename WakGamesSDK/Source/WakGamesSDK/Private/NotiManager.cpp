// Fill out your copyright notice in the Description page of Project Settings.


#include "NotiManager.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "TimerManager.h"
#include "UnrealClient.h"
#include "UObject/ConstructorHelpers.h"

const FString UNotiManager::NotificationWidgetPath = TEXT(
	"/Game/Levels/UI/Achievements/WBP_NotiPanel.uasset");

UNotiManager::UNotiManager()
{
	// 알림창 블루프린트를 로드하여 클래스 변수에 저장
	ConstructorHelpers::FClassFinder<UUserWidget> WidgetClassFinder(*NotificationWidgetPath);
	if (WidgetClassFinder.Succeeded())
	{
		NotificationWidgetClass = WidgetClassFinder.Class;
		UE_LOG(LogTemp, Log,
		       TEXT("[-] NotiManager: Notification widget class loaded successfully."));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[-] NotiManager: Failed to load Notification widget class."));
	}
}

void UNotiManager::AddNotification(FString Title, FString Description, FString IconURL)
{
	if (!NotificationWidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[-] NotiManager: NotificationWidgetClass is not set."));
		return;
	}

	// 알림창이 가득 찼을 경우, 첫 번째 알림을 제거
	if (ActiveNotifications.Num() >= MaxNotificationsOnScreen)
	{
		OnNotificationExpired(ActiveNotifications[0]);
	}

	// 알림창 위젯 생성
	if (UUserWidget* NewNotification = CreateWidget<UUserWidget>(
		GetWorld(), NotificationWidgetClass))
	{
		// 알림창의 초기 위치 설정 (우측 최하단 모서리)
		FVector2D ScreenPosition = FVector2D(290.0f, GetNextNotificationPositionY());
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(NewNotification->Slot))
		{
			CanvasSlot->SetPosition(ScreenPosition);
		}

		// 알림창을 뷰포트에 추가
		NewNotification->AddToViewport();

		// 큐에 알림창 추가
		ActiveNotifications.Add(NewNotification);

		// 일정 시간이 지나면 알림창을 소멸시킴
		// TODO: 알림창이 꽉 차 먼저 소멸되는 경우에 대한 핸들링 필요 (free된걸 다시 free하려고 시도할 수 있음)
		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this, NewNotification]()
{
	OnNotificationExpired(NewNotification);
}, NotificationDuration, false);
	}
}

void UNotiManager::ProcessQueue()
{
	// 알림창의 위치 업데이트
	for (int32 i = 0; i < ActiveNotifications.Num(); i++)
	{
		const FVector2D NewPosition = FVector2D(290.0f, GetNextNotificationPositionY());
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(ActiveNotifications[i]->Slot))
		{
			CanvasSlot->SetPosition(NewPosition);
		}
	}
}

void UNotiManager::OnNotificationExpired(UUserWidget* ExpiredNotification)
{
	if (ExpiredNotification)
	{
		// 뷰포트에서 알림창 제거
		ExpiredNotification->RemoveFromParent();
		ActiveNotifications.Remove(ExpiredNotification);
	}

	// 큐 업데이트
	ProcessQueue();
}

/** 최하단으로부터의 desired height */
float UNotiManager::GetNextNotificationPositionY(float NotiIndex) const
{
	constexpr float NotificationHeight = 80.0f;
	constexpr float Padding = 0.0f;
	NotiIndex = (NotiIndex == -1) ? ActiveNotifications.Num() : NotiIndex;
	return (NotiIndex * (NotificationHeight + Padding));
}
