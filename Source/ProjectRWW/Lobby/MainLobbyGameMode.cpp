// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainLobbyGameMode.h"
#include "MainLobbyPlayerController.h"
#include "Database/MainPlayerDataRepository.h"
#include "Database/MainSessionServerStatusRepository.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"

void AMainLobbyGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	PlayerDataRepository = NewObject<UMainPlayerDataRepository>(this);
	StatusRepository = NewObject<UMainSessionServerStatusRepository>(this);
}

FString AMainLobbyGameMode::InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal)
{
	// Super를 먼저 호출한다 — 내부적으로 Options의 "Name="을 파싱해서 ChangeName을 부르는데,
	// 이걸 우리보다 먼저 실행되게 해야 우리가 마지막에 덮어쓴 값이 살아남는다.
	const FString ErrorMessage = Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);

	// URL의 예약 키인 "Name"은 OSS(Online Subsystem)가 우선적으로 덮어써서 무시된다.
	// 그래서 우리만의 커스텀 키 "PlayerID"로 받아서, Super가 끝난 뒤 마지막으로 이름을 설정한다.
	const FString ParsedPlayerID = UGameplayStatics::ParseOption(Options, TEXT("PlayerID"));
	if (!ParsedPlayerID.IsEmpty())
	{
		ChangeName(NewPlayerController, ParsedPlayerID, false);
	}

	return ErrorMessage;
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

	const TArray<FMainSessionServerStatus> Servers = StatusRepository->GetAllServerStatuses();
	if (Servers.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("[ProjectRWW] 등록된 세션 서버가 없습니다."));
		return FString();
	}

	for (const FMainSessionServerStatus& Server : Servers)
	{
		if (Server.PlayerCount < Server.MaxPlayers)
		{
			return Server.Address;
		}
	}

	// 모든 서버가 꽉 참 — 인원이 가장 적은 서버로 보낸다.
	const FMainSessionServerStatus* LeastPlayerServer = &Servers[0];
	for (const FMainSessionServerStatus& Server : Servers)
	{
		if (Server.PlayerCount < LeastPlayerServer->PlayerCount)
		{
			LeastPlayerServer = &Server;
		}
	}

	return LeastPlayerServer->Address;
}
