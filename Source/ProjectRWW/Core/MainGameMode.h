#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MainGameMode.generated.h"

UCLASS()
class PROJECTRWW_API AMainGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMainGameMode();

protected:
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 맵 이름 → 정원 매핑. 여기 없는 맵은 DefaultMaxPlayers를 씀.
	UPROPERTY(EditDefaultsOnly, Category = "Session")
	TMap<FString, int32> MapMaxPlayers;

	UPROPERTY(EditDefaultsOnly, Category = "Session")
	int32 DefaultMaxPlayers = 20;

private:
	void ReportHeartbeat();

	// InitGame에서 MapMaxPlayers/DefaultMaxPlayers 중 이번 맵에 실제로 적용되는 값으로 확정해둔다.
	int32 CurrentMaxPlayers = 20;

	UPROPERTY()
	TObjectPtr<class UMainSessionServerStatusRepository> StatusRepository;

	// -serveraddress= 인자로 받는 "이 서버의 주소". 안 주어지면 127.0.0.1:포트로 기본 동작(로컬 테스트용).
	FString MySessionServerAddress;

	FTimerHandle HeartbeatTimerHandle;
};
