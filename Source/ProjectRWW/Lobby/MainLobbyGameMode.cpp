// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainLobbyGameMode.h"
#include "MainLobbyPlayerController.h"
#include "Database/MainPlayerDataRepository.h"
#include "Database/MainSessionServerStatusRepository.h"
#include "GameFramework/PlayerState.h"

void AMainLobbyGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	PlayerDataRepository = NewObject<UMainPlayerDataRepository>(this);
	if (!PlayerDataRepository->Open(UMainPlayerDataRepository::GetDefaultDatabasePath()))
	{
		UE_LOG(LogTemp, Error, TEXT("[ProjectRWW] PlayerData.db 열기 실패"));
	}

	StatusRepository = NewObject<UMainSessionServerStatusRepository>(this);
	if (!StatusRepository->Open(UMainSessionServerStatusRepository::GetDefaultDatabasePath()))
	{
		UE_LOG(LogTemp, Error, TEXT("[ProjectRWW] SessionServerStatus.db 열기 실패"));
	}
}

void AMainLobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	const FString PlayerIP = NewPlayer->GetNetConnection()
		? NewPlayer->GetNetConnection()->LowLevelGetRemoteAddress()
		: TEXT("Unknown");
	UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] 접속 IP: %s"), *PlayerIP);

	AMainLobbyPlayerController* LobbyController = Cast<AMainLobbyPlayerController>(NewPlayer);
	if (!LobbyController || !PlayerDataRepository)
	{
		return;
	}

	// 실제 로그인/인증 시스템은 범위 밖 — 임시로 플레이어 이름을 ID로 사용한다.
	const FString PlayerID = NewPlayer->PlayerState ? NewPlayer->PlayerState->GetPlayerName() : FString();
	const FMainPlayerRecord Record = PlayerDataRepository->LoadPlayerData(PlayerID);

	LobbyController->SetPlayerRecord(Record);
	UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] 로비 접속: %s - KillCount %d, Currency %d"), *PlayerID, Record.KillCount, Record.Currency);
}

FString AMainLobbyGameMode::AssignSessionServer()
{
	if (!StatusRepository)
	{
		return FString();
	}

	for (const FMainSessionServerEntry& Entry : SessionServers)
	{
		const int32 CurrentCount = StatusRepository->GetPlayerCount(Entry.Address);
		if (CurrentCount < Entry.MaxPlayers)
		{
			return Entry.Address;
		}
	}

	// 모든 서버가 꽉 참 — 인원이 가장 적은 서버로 보낸다.
	if (SessionServers.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("[ProjectRWW] SessionServers 배열이 비어있습니다."));
		return FString();
	}

	const FMainSessionServerEntry* LeastPlayerServer = &SessionServers[0];
	int32 LeastPlayerServerCount = StatusRepository->GetPlayerCount(LeastPlayerServer->Address);

	for (const FMainSessionServerEntry& Entry : SessionServers)
	{
		const int32 CurrentCount = StatusRepository->GetPlayerCount(Entry.Address);
		if (CurrentCount < LeastPlayerServerCount)
		{
			LeastPlayerServer = &Entry;
			LeastPlayerServerCount = CurrentCount;
		}
	}

	return LeastPlayerServer->Address;
}

void AMainLobbyGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (PlayerDataRepository)
	{
		PlayerDataRepository->Close();
	}
	if (StatusRepository)
	{
		StatusRepository->Close();
	}

	Super::EndPlay(EndPlayReason);
}
