// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainInventoryWidget.h"
#include "MainInventorySlotWidget.h"
#include "MainInventoryComponent.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"

void UMainInventoryWidget::BuildSlotWidgets()
{
	UMainInventoryComponent* InventoryComponent = GetInventoryComponent();
	if (!SlotGrid || !SlotWidgetClass || SlotsPerRow <= 0 || !InventoryComponent)
	{
		return;
	}

	// 정수 올림 나눗셈: (A + B - 1) / B는 A/B를 올림한 값과 같다.
	const int32 TotalRows = (UMainInventoryComponent::InventorySlotCount + SlotsPerRow - 1) / SlotsPerRow;

	for (int32 i = 0; i < UMainInventoryComponent::InventorySlotCount; ++i)
	{
		UMainInventorySlotWidget* SlotWidget = CreateWidget<UMainInventorySlotWidget>(this, SlotWidgetClass);
		if (!SlotWidget)
		{
			continue;
		}

		SlotWidget->OwningComponent = InventoryComponent;

		// 0~8번(핫바)이 맨 아래 줄에 오도록 행 번호를 뒤집는다.
		const int32 Row = (TotalRows - 1) - (i / SlotsPerRow);
		const int32 Column = i % SlotsPerRow;
		SlotGrid->AddChildToUniformGrid(SlotWidget, Row, Column);

		SlotWidgets.Add(SlotWidget);
	}
}
