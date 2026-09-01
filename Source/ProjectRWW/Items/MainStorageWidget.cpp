// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainStorageWidget.h"
#include "MainInventorySlotWidget.h"
#include "MainStorageComponent.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"

void UMainStorageWidget::BuildSlotWidgets()
{
	UMainStorageComponent* Storage = Cast<UMainStorageComponent>(ContainerComponent.Get());
	if (!SlotGrid || !SlotWidgetClass || !Storage)
	{
		return;
	}

	const int32 Cols = FMath::Max(1, Storage->GetColumnCount());
	const int32 SlotCount = Storage->Slots.Num();

	for (int32 i = 0; i < SlotCount; ++i)
	{
		UMainInventorySlotWidget* SlotWidget = CreateWidget<UMainInventorySlotWidget>(this, SlotWidgetClass);
		if (!SlotWidget)
		{
			continue;
		}

		SlotWidget->OwningComponent = Storage;

		const int32 Row = i / Cols;
		const int32 Column = i % Cols;
		SlotGrid->AddChildToUniformGrid(SlotWidget, Row, Column);

		SlotWidgets.Add(SlotWidget);
	}
}
