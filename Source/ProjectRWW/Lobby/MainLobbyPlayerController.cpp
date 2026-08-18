// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainLobbyPlayerController.h"
#include "MainLobbyGameMode.h"
#include "GameFramework/PlayerState.h"

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
		// 로비에서 이미 PlayerID로 설정해둔 이름을 세션 서버로 이동할 때도 이어서 넘긴다.
		// 안 그러면 Title→로비 때처럼 세션 서버에서도 OSS 자동 생성 이름으로 되돌아간다.
		const FString PlayerID = PlayerState ? PlayerState->GetPlayerName() : FString();
		const FString TravelURL = FString::Printf(TEXT("%s?PlayerID=%s"), *Address, *PlayerID);
		ClientTravel(TravelURL, ETravelType::TRAVEL_Absolute);
	}
}
