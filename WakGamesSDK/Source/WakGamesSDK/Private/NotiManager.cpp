// Fill out your copyright notice in the Description page of Project Settings.


#include "NotiManager.h"
#include "Kismet/GameplayStatics.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "TimerManager.h"
#include "UnrealClient.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

FString UNotiManager::NotificationWidgetPath = TEXT("/Game/Levels/UI/Achievements/WBP_NotiPanel");

UNotiManager::UNotiManager()
{
    // 블루프린트 위젯 클래스 로드
    static ConstructorHelpers::FClassFinder<UUserWidget> NotificationWidgetBPClass(*NotificationWidgetPath);
    if (NotificationWidgetBPClass.Class != nullptr)
    {
        NotificationWidgetClass = NotificationWidgetBPClass.Class;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("NotificationWidgetClass not found! Check the path."));
    }
}

void UNotiManager::AddNotification(const FString& Title, const FString& Description, const FString& IconURL)
{
    if (NotificationWidgetClass == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("NotificationWidgetClass is null."));
        return;
    }

    UWorld* World = GEngine->GetWorldFromContextObjectChecked(this);
    if (World == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("World is null."));
        return;
    }

    // 새로운 알림창 위젯 생성
    UUserWidget* NewNotification = CreateWidget<UUserWidget>(World, NotificationWidgetClass);
    if (NewNotification == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to create notification widget."));
        return;
    }

    // 위젯 초기화 (Title, Description, IconURL 설정)
    // 위젯 블루프린트에 InitializeNotification 함수가 있다고 가정합니다.
    UFunction* InitFunction = NewNotification->FindFunction(FName("InitializeNotification"));
    if (InitFunction)
    {
        struct FInitializeNotificationParams
        {
            FString Title;
            FString Description;
            FString IconURL;
        };

        FInitializeNotificationParams Params;
        Params.Title = Title;
        Params.Description = Description;
        Params.IconURL = IconURL;

        NewNotification->ProcessEvent(InitFunction, &Params);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("InitializeNotification function not found in widget."));
    }

    // 뷰포트에 위젯 추가
    NewNotification->AddToViewport();

    // 기존 알림창들을 위로 이동
    for (int32 i = 0; i < ActiveNotifications.Num(); ++i)
    {
        if (UUserWidget* Widget = ActiveNotifications[i])
        {
            if (UCanvasPanelSlot* CanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(Widget))
            {
                FVector2D Position = CanvasSlot->GetPosition();
                Position.Y -= Widget->GetDesiredSize().Y; // 위젯의 높이만큼 위로 이동
                CanvasSlot->SetPosition(Position);
            }
        }
    }

    // 새로운 알림창 위치 설정
    if (UCanvasPanelSlot* NewCanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(NewNotification))
    {
        FVector2D NewPosition = FVector2D(290, GetNextNotificationPositionY());
        NewCanvasSlot->SetPosition(NewPosition);
    }

    // 알림창 큐에 추가
    ActiveNotifications.Add(NewNotification);

    // 알림창 소멸 예약
    FTimerDelegate TimerCallback;
    TimerCallback.BindUFunction(this, FName("OnNotificationExpired"), NewNotification);

    FTimerHandle TimerHandle;
    World->GetTimerManager().SetTimer(TimerHandle, TimerCallback, NotificationDuration, false);

    // 최대 알림창 수를 초과하면 가장 오래된 알림창 제거
    if (ActiveNotifications.Num() > MaxNotificationsOnScreen)
    {
        UUserWidget* OldNotification = ActiveNotifications[0];
        if (OldNotification)
        {
            OldNotification->RemoveFromParent();
        }
        ActiveNotifications.RemoveAt(0);
    }
}

void UNotiManager::ProcessQueue()
{
    // 타이머를 사용하여 알림창을 관리하므로, 이 함수는 필요 없을 수 있습니다.
}

void UNotiManager::OnNotificationExpired(UUserWidget* ExpiredNotification)
{
    if (ExpiredNotification)
    {
        ExpiredNotification->RemoveFromParent();
        ActiveNotifications.Remove(ExpiredNotification);
    }
}

float UNotiManager::GetNextNotificationPositionY(int32 NotiIndex) const
{
    // 인덱스를 기반으로 Y 위치 계산
    const float NotificationHeight = 290.0f; // 위젯의 높이에 맞게 조정
    const float BasePositionY = 0.0f; // 화면 아래에서 시작하는 Y 위치
    if (NotiIndex == -1)
    {
        NotiIndex = ActiveNotifications.Num();
    }
    return BasePositionY + (NotiIndex * NotificationHeight);
}