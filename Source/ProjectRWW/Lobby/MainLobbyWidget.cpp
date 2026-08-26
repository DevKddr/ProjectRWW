// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainLobbyWidget.h"
#include "MainLobbyPlayerController.h"

void UMainLobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (AMainLobbyPlayerController* PC = Cast<AMainLobbyPlayerController>(GetOwningPlayer()))
	{
		PC->OnPlayerRecordUpdated.AddDynamic(this, &UMainLobbyWidget::HandlePlayerRecordUpdated);
		HandlePlayerRecordUpdated(); // 위젯이 뜨기 전에 이미 도착해있던 값을 즉시 반영

		PC->OnSessionListUpdated.AddDynamic(this, &UMainLobbyWidget::HandleSessionListUpdated);
	}
}

void UMainLobbyWidget::HandlePlayerRecordUpdated()
{
	if (const AMainLobbyPlayerController* PC = Cast<AMainLobbyPlayerController>(GetOwningPlayer()))
	{
		PlayerID = PC->GetPlayerRecord().PlayerID;
		PlayerName = PC->GetPlayerRecord().PlayerName;
	}
}

void UMainLobbyWidget::HandleSessionListUpdated()
{
	if (const AMainLobbyPlayerController* PC = Cast<AMainLobbyPlayerController>(GetOwningPlayer()))
	{
		SessionList = PC->GetCachedSessionList();
	}
	OnSessionListRefreshed.Broadcast();
}

void UMainLobbyWidget::OnSetNameClicked(const FString& NewName)
{
	if (AMainLobbyPlayerController* PC = Cast<AMainLobbyPlayerController>(GetOwningPlayer()))
	{
		PC->Server_SetPlayerName(NewName);
	}
}

void UMainLobbyWidget::OnSearchSessionClicked()
{
	if (AMainLobbyPlayerController* PC = Cast<AMainLobbyPlayerController>(GetOwningPlayer()))
	{
		PC->Server_SearchSession();
	}
}

void UMainLobbyWidget::OnRefreshSessionListClicked()
{
	if (AMainLobbyPlayerController* PC = Cast<AMainLobbyPlayerController>(GetOwningPlayer()))
	{
		PC->Server_RequestSessionList();
	}
}
