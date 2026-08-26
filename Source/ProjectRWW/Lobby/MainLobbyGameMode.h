// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MainLobbyGameMode.generated.h"

UCLASS()
class PROJECTRWW_API AMainLobbyGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Lobby")
	class UMainPlayerDataRepository* GetPlayerDataRepository() const { return PlayerDataRepository; }

	// 서버에서만 유효하다 — 클라이언트는 GameMode 자체가 없으므로 이 함수를 직접 호출할 수 없고,
	// PlayerController의 Server_RequestSessionList RPC를 거쳐야 한다.
	class UMainSessionServerStatusRepository* GetStatusRepository() const { return StatusRepository; }

	// 목록을 순서대로 훑어 아직 꽉 차지 않은 첫 번째 서버를 배정한다. 전부 꽉 찼으면 빈 문자열.
	FString AssignSessionServer();

protected:
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal = TEXT("")) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;

private:
	UPROPERTY()
	TObjectPtr<class UMainPlayerDataRepository> PlayerDataRepository;

	UPROPERTY()
	TObjectPtr<class UMainSessionServerStatusRepository> StatusRepository;
};
