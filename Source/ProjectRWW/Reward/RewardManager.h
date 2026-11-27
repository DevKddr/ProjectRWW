// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameReward.h"
#include "RewardManager.generated.h"

class AMainPlayerController;
class UMainPlayerDataRepository;

// rewards.json 전체와 1:1 대응. 키가 RewardId다.
USTRUCT()
struct FRewardTable
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<FString, FGameReward> Rewards;
};

// 보상 지급의 단일 창구. 호출부는 RewardId만 말하고, 내용 조회와 지급은 여기서 한다.
// 어떤 상황의 보상인지와 화면 표시는 모른다. 서버 전용이다.
UCLASS()
class PROJECTRWW_API URewardManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// 표에 이 RewardId가 있는지. 없는 것이 정상인 경우(연속 킬)에 먼저 확인한다.
	bool HasReward(FName RewardId) const;

	// RewardIds의 보상을 합쳐 한 번에 지급하고, 실제로 지급된 것을 돌려준다.
	// 상자는 추첨 결과가 Items에 담기고 GachaBoxIds는 비어 있다.
	// 인벤토리가 가득 차 있으면 아이템은 유실된다(월드 드랍 없음).
	FGameReward GrantReward(AMainPlayerController* Player, const TArray<FName>& RewardIds);

private:
	void LoadRewardTable();

	FRewardTable Table;

	UPROPERTY()
	TObjectPtr<UMainPlayerDataRepository> PlayerDataRepository;
};
