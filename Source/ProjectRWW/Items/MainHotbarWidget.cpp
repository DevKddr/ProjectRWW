// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainHotbarWidget.h"
#include "MainInventorySlotWidget.h"
#include "MainInventoryComponent.h"
#include "Components/HorizontalBox.h"

void UMainHotbarWidget::BuildSlotWidgets()
{
	UMainInventoryComponent* InventoryComponent = GetInventoryComponent();
	if (!SlotBar || !SlotWidgetClass || HotbarSlotCount <= 0 || !InventoryComponent)
	{
		return;
	}

	for (int32 i = 0; i < HotbarSlotCount; ++i)
	{
		UMainInventorySlotWidget* SlotWidget = CreateWidget<UMainInventorySlotWidget>(this, SlotWidgetClass);
		if (!SlotWidget)
		{
			continue;
		}

		SlotWidget->OwningComponent = InventoryComponent;

		SlotBar->AddChildToHorizontalBox(SlotWidget);
		SlotWidgets.Add(SlotWidget);
	}
}
