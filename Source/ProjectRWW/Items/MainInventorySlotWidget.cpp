// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainInventorySlotWidget.h"
#include "ItemDataManager.h"
#include "Engine/Texture2D.h"

void UMainInventorySlotWidget::SetSlotData(int32 InSlotIndex, const FInventorySlot& SlotData)
{
	SlotIndex = InSlotIndex;

	UTexture2D* Icon = nullptr;
	if (!SlotData.IsEmpty())
	{
		UItemDataManager* ItemDataManager = GetGameInstance() ? GetGameInstance()->GetSubsystem<UItemDataManager>() : nullptr;
		FItemData ItemData;
		if (ItemDataManager && ItemDataManager->GetItemData(SlotData.ItemIndex, ItemData))
		{
			Icon = Cast<UTexture2D>(ItemData.IconPath.TryLoad());
		}
	}

	OnIconChanged(Icon);
}

void UMainInventorySlotWidget::HandleClicked()
{
	OnSlotClicked.Broadcast(SlotIndex);
}
