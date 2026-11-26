// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameReward.h"
#include "KillStreakRewardTable.generated.h"

// kill_streak_rewards.json 각 항목과 1:1 대응.
USTRUCT()
struct FKillStreakReward
{
	GENERATED_BODY()

	// 이번 목숨의 몇 번째 킬인지.
	UPROPERTY()
	int32 KillStreak = 0;

	// 그 킬에 기본 보상 위에 더해 줄 보상.
	UPROPERTY()
	FGameReward Reward;
};

// 연속 킬 보상 표. 서버의 GameMode만 만들어 쓴다(클라이언트는 로드하지 않는다).
UCLASS()
class PROJECTRWW_API UKillStreakRewardTable : public UObject
{
	GENERATED_BODY()

public:
	// JSON을 읽어 표를 채운다. 실패하면 false - 이 경우 기본 킬 보상만 지급된다.
	bool Load();

	// 표에 정확히 이 연속 킬 수가 있으면 OutReward에 담고 true. 없으면 OutReward는 건드리지 않는다.
	bool FindReward(int32 KillStreak, FGameReward& OutReward) const;

private:
	TMap<int32, FGameReward> RewardByStreak;
};
