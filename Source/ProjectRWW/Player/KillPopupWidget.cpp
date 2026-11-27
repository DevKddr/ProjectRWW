// Copyright Epic Games, Inc. All Rights Reserved.

#include "KillPopupWidget.h"
#include "Engine/World.h"
#include "PopupRowWidget.h"
#include "TimerManager.h"

void UKillPopupWidget::AddKill(const FKillNotice& Notice)
{
	const bool bFirstKill = !bShowing;
	if (bFirstKill)
	{
		ShownTotalCredit = 0;
	}
	bShowing = true;
	ShownTotalCredit += Notice.Credit;

	ReceiveKill(Notice.KillStreak, ShownTotalCredit, bFirstKill);

	UPopupRowWidget* Row = NameRowClass ? CreateWidget<UPopupRowWidget>(this, NameRowClass) : nullptr;
	if (Row)
	{
		ReceiveNameRowCreated(Row, Notice.VictimName);
		PushRow(Row);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[ProjectRWW] 처치 팝업의 NameRowClass가 비어 있음"));
	}

	if (UWorld* World = GetWorld())
	{
		// 같은 핸들로 다시 걸면 시간을 처음부터 다시 센다.
		World->GetTimerManager().SetTimer(ExpireTimerHandle, this, &UKillPopupWidget::HandleExpired, RowSeconds, false);
	}
}

void UKillPopupWidget::ClearPopup()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ExpireTimerHandle);
	}
	bShowing = false;
	ClearRows();
	ReceiveCleared();
}

void UKillPopupWidget::HandleExpired()
{
	bShowing = false;
	ReceiveExpired();
}
