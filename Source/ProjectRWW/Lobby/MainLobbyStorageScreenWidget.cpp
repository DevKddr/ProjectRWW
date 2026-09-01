// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainLobbyStorageScreenWidget.h"
#include "Items/MainInventoryWidget.h"
#include "Items/MainStorageWidget.h"
#include "Items/MainInventoryComponent.h"
#include "Items/MainStorageComponent.h"
#include "Lobby/MainLobbyPlayerController.h"

void UMainLobbyStorageScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (AMainLobbyPlayerController* PC = Cast<AMainLobbyPlayerController>(GetOwningPlayer()))
	{
		if (InventoryWidget)
		{
			InventoryWidget->SetContainerComponent(PC->InventoryComponent);
		}
		if (StorageWidget)
		{
			StorageWidget->SetContainerComponent(PC->StorageComponent);
		}
	}
}
