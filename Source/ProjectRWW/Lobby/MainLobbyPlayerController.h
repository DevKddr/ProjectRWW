// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Database/MainPlayerRecord.h"
#include "MainLobbyPlayerController.generated.h"

UCLASS()
class PROJECTRWW_API AMainLobbyPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	void SetPlayerRecord(const FMainPlayerRecord& Record) { PlayerRecord = Record; }
	const FMainPlayerRecord& GetPlayerRecord() const { return PlayerRecord; }

	UFUNCTION(Exec)
	void RWW_SearchSession();

	UFUNCTION(Server, Reliable)
	void Server_SearchSession();

private:
	FMainPlayerRecord PlayerRecord;
};
