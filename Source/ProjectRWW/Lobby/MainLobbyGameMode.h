// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MainLobbyGameMode.generated.h"

struct FMainSessionServerEntry
{
	FString Address; // 예: "127.0.0.1:7778"
	int32 MaxPlayers = 20;
};

UCLASS()
class PROJECTRWW_API AMainLobbyGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Lobby")
	class UMainPlayerDataRepository* GetPlayerDataRepository() const { return PlayerDataRepository; }

	// 목록을 순서대로 훑어 아직 꽉 차지 않은 첫 번째 서버를 배정한다. 전부 꽉 찼으면 빈 문자열.
	// 서브프로젝트 2 스펙: 나중에 "꽉 찼을 때 새 서버를 띄우는" 로직으로 확장할 지점.
	FString AssignSessionServer();

protected:
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY()
	TObjectPtr<class UMainPlayerDataRepository> PlayerDataRepository;

	UPROPERTY()
	TObjectPtr<class UMainSessionServerStatusRepository> StatusRepository;

	// 세션 서버 목록. 서버를 추가/변경할 땐 이 배열을 직접 수정하고 다시 빌드한다.
	TArray<FMainSessionServerEntry> SessionServers = {
		{ TEXT("127.0.0.1:7778"), 20 }
	};
};
