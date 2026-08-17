// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainLobbyPlayerController.h"
#include "MainLobbyGameMode.h"

void AMainLobbyPlayerController::RWW_SearchSession()
{
	Server_SearchSession();
}

void AMainLobbyPlayerController::Server_SearchSession_Implementation()
{
	AMainLobbyGameMode* LobbyGameMode = GetWorld()->GetAuthGameMode<AMainLobbyGameMode>();
	const FString Address = LobbyGameMode ? LobbyGameMode->AssignSessionServer() : FString();
	if (!Address.IsEmpty())
	{
		ClientTravel(Address, ETravelType::TRAVEL_Absolute);
	}
}
