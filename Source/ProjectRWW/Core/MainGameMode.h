#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MainGameMode.generated.h"

class AMainPlayerController;

UCLASS()
class PROJECTRWW_API AMainGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMainGameMode();

	// 사망 정산의 진입점. Character의 OnDeath 콜백, 그리고 살아있는 채로 로비 복귀를
	// 요청한 경우(PlayerController) 양쪽에서 호출된다.
	void HandlePlayerDeath(APlayerController* Victim, AController* Killer);

	// 탈출 성공 정산의 진입점. AMainExtractionZone의 타이머 완료 콜백에서 호출된다.
	// 사망과 달리 인벤토리를 보존한 채 저장한다.
	void HandleExtraction(APlayerController* Player);

	// OnPossess()에서 호출된다(최초 접속과 매 리스폰 모두). 최초 접속의 DB 로드/인벤토리 복원,
	// 스폰 보상 지급, 1번 슬롯 장착을 이 함수 하나가 전부 처리한다.
	void HandlePlayerSpawned(AMainPlayerController* PC);

	// gacha_tables.json의 BoxId로 1회 추첨해서 TargetController의 인벤토리에 바로 지급한다.
	// 어떤 상황(스폰/킬 등)에 어떤 박스를 쓸지는 호출부가 BoxId로 정한다 - 이 함수 자체는
	// 상황을 모른다. 인벤토리가 가득 차 있으면 보상은 조용히 유실된다(월드 드랍 없음).
	void GrantGachaBoxReward(APlayerController* TargetController, FName BoxId);

	// 스폰(최초 접속 포함)/킬마다 지급할 박스. 같은 박스를 쓰거나, 나중에 gacha_tables.json에
	// 박스를 더 추가해서 각각 다르게 지정할 수 있다.
	UPROPERTY(EditDefaultsOnly, Category = "Gacha")
	FName SpawnRewardBoxId = TEXT("WeaponBox_Default");

	UPROPERTY(EditDefaultsOnly, Category = "Gacha")
	FName KillRewardBoxId = TEXT("WeaponBox_Default");

protected:
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal = TEXT("")) override;
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// 접속/퇴장 이벤트가 있을 때마다 지금 인원수를 DB에 갱신한다.
	void UpdatePlayerCount();

	// -maxplayers= 커맨드라인 인자로 받는다. 안 주면 기본값(20)을 쓴다.
	int32 CurrentMaxPlayers = 20;

	UPROPERTY()
	TObjectPtr<class UMainSessionServerStatusRepository> StatusRepository;

	// 로비 GameMode와 동일한 리포지토리 — 세션 서버도 같은 PlayerData.db를 직접 읽고 쓴다.
	UPROPERTY()
	TObjectPtr<class UMainPlayerDataRepository> PlayerDataRepository;

	// -serveraddress= 인자로 받는 "이 서버의 주소". 안 주어지면 127.0.0.1:포트로 기본 동작(로컬 테스트용).
	FString MySessionServerAddress;
};
