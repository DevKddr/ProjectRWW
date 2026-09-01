// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainInventorySlotContainerWidget.h"
#include "MainInventorySlotWidget.h"
#include "MainInventoryComponent.h"
#include "Player/MainPlayerController.h"

UMainInventoryComponent* UMainInventorySlotContainerWidget::GetInventoryComponent() const
{
	if (const AMainPlayerController* PC = Cast<AMainPlayerController>(GetOwningPlayer()))
	{
		return PC->InventoryComponent;
	}
	return nullptr;
}

void UMainInventorySlotContainerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UMainInventoryComponent* InventoryComponent = GetInventoryComponent())
	{
		InventoryComponent->OnSlotsChanged.AddDynamic(this, &UMainInventorySlotContainerWidget::HandleInventoryChanged);
	}

	// 슬롯 위젯 생성은 최초 한 번만 - AddToViewport()를 부를 때마다 NativeConstruct가
	// 재호출되므로, 매번 새로 만들면 슬롯이 중복 생성된다.
	if (SlotWidgets.Num() == 0)
	{
		BuildSlotWidgets();
	}

	RefreshAllSlots();
}

void UMainInventorySlotContainerWidget::NativeDestruct()
{
	if (UMainInventoryComponent* InventoryComponent = GetInventoryComponent())
	{
		InventoryComponent->OnSlotsChanged.RemoveDynamic(this, &UMainInventorySlotContainerWidget::HandleInventoryChanged);
	}

	Super::NativeDestruct();
}

void UMainInventorySlotContainerWidget::RefreshAllSlots()
{
	UMainInventoryComponent* InventoryComponent = GetInventoryComponent();
	if (!InventoryComponent)
	{
		return;
	}

	for (int32 i = 0; i < SlotWidgets.Num(); ++i)
	{
		if (InventoryComponent->Slots.IsValidIndex(i))
		{
			SlotWidgets[i]->SetSlotData(i, InventoryComponent->Slots[i]);
		}
	}
}

void UMainInventorySlotContainerWidget::HandleInventoryChanged()
{
	RefreshAllSlots();
}
