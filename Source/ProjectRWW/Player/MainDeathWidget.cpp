// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainDeathWidget.h"
#include "MainPlayerController.h"

void UMainDeathWidget::RequestRespawn()
{
	if (AMainPlayerController* PC = Cast<AMainPlayerController>(GetOwningPlayer()))
	{
		PC->Server_RequestRespawn();
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
	}

	RemoveFromParent();
}

void UMainDeathWidget::RequestReturnToLobby()
{
	if (AMainPlayerController* PC = Cast<AMainPlayerController>(GetOwningPlayer()))
	{
		PC->Server_RequestReturnToLobby();
	}
}
