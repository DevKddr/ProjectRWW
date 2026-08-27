// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainHotbarWidget.h"
#include "MainInventorySlotWidget.h"
#include "Components/HorizontalBox.h"

void UMainHotbarWidget::BuildSlotWidgets()
{
	if (!SlotBar || !SlotWidgetClass || HotbarSlotCount <= 0)
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

		SlotBar->AddChildToHorizontalBox(SlotWidget);
		SlotWidgets.Add(SlotWidget);
	}
}
