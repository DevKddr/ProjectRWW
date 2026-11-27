// Copyright Epic Games, Inc. All Rights Reserved.

#include "PopupRowStackWidget.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "PopupRowWidget.h"

void UPopupRowStackWidget::PushRow(UPopupRowWidget* Row)
{
	if (!Row || !RowPanel)
	{
		return;
	}

	// 같은 프레임에 들어온 줄들(연속 킬 보상 등)은 서로를 이전 줄로 만들지 않는다.
	if (LastPushFrame != GFrameCounter)
	{
		LastPushFrame = GFrameCounter;
		for (FPopupRow& OldRow : Rows)
		{
			if (!OldRow.bAged && OldRow.Widget)
			{
				OldRow.bAged = true;
				OldRow.Widget->ReceiveAged();
			}
		}
	}

	// 모든 줄을 같은 바닥 자리에 겹쳐 놓는다. 위로 올리는 것은 줄 BP가 한다.
	if (UOverlaySlot* PanelSlot = RowPanel->AddChildToOverlay(Row))
	{
		PanelSlot->SetHorizontalAlignment(HAlign_Left);
		PanelSlot->SetVerticalAlignment(VAlign_Bottom);
	}

	FPopupRow NewRow;
	NewRow.Widget = Row;
	const double PreviousExpire = Rows.IsEmpty() ? 0.0 : Rows[0].ExpireTime;
	NewRow.ExpireTime = FMath::Max(Clock + RowSeconds, PreviousExpire + RemoveIntervalSeconds);
	Rows.Insert(NewRow, 0);
	Row->ReceiveEnter();

	while (Rows.Num() > MaxRows)
	{
		StartLeaving(Rows.Num() - 1);
	}
	NotifySlotChanges();
}

void UPopupRowStackWidget::ClearRows()
{
	for (const FPopupRow& Row : Rows)
	{
		if (Row.Widget)
		{
			Row.Widget->RemoveFromParent();
		}
	}
	for (const FPopupRow& Row : LeavingRows)
	{
		if (Row.Widget)
		{
			Row.Widget->RemoveFromParent();
		}
	}
	Rows.Reset();
	LeavingRows.Reset();
}

void UPopupRowStackWidget::StartLeaving(int32 Index)
{
	FPopupRow Row = Rows[Index];
	Rows.RemoveAt(Index);
	if (!Row.Widget)
	{
		return;
	}

	Row.RemoveTime = Clock + Row.Widget->LeaveSeconds;
	LeavingRows.Add(Row);
	Row.Widget->ReceiveLeave();
}

void UPopupRowStackWidget::NotifySlotChanges()
{
	for (int32 i = 0; i < Rows.Num(); ++i)
	{
		FPopupRow& Row = Rows[i];
		if (Row.SlotIndex != i && Row.Widget)
		{
			Row.SlotIndex = i;
			Row.Widget->ReceiveSlotChanged(i);
		}
	}
}

void UPopupRowStackWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	Clock += InDeltaTime;

	// 시간이 다 된 줄을 오래된 것부터 내보낸다.
	while (!Rows.IsEmpty() && Clock >= Rows.Last().ExpireTime)
	{
		StartLeaving(Rows.Num() - 1);
	}

	// 나가는 연출이 끝난 줄을 없앤다.
	for (int32 i = LeavingRows.Num() - 1; i >= 0; --i)
	{
		if (Clock >= LeavingRows[i].RemoveTime)
		{
			if (LeavingRows[i].Widget)
			{
				LeavingRows[i].Widget->RemoveFromParent();
			}
			LeavingRows.RemoveAt(i);
		}
	}
}
