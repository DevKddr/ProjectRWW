// Copyright Epic Games, Inc. All Rights Reserved.

#include "KillStreakRewardTable.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

bool UKillStreakRewardTable::Load()
{
	RewardByStreak.Empty();

	const FString FullPath = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data/output/kill_streak_rewards.json"));
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *FullPath))
	{
		UE_LOG(LogTemp, Error, TEXT("[ProjectRWW] kill_streak_rewards.json 로드 실패: %s"), *FullPath);
		return false;
	}

	TArray<FKillStreakReward> Entries;
	if (!FJsonObjectConverter::JsonArrayStringToUStruct(JsonString, &Entries, 0, 0))
	{
		UE_LOG(LogTemp, Error, TEXT("[ProjectRWW] kill_streak_rewards.json 파싱 실패"));
		return false;
	}

	for (const FKillStreakReward& Entry : Entries)
	{
		// 손으로 편집하는 파일이라, 잘못된 줄은 경고를 남기고 건너뛴다.
		const bool bEmptyReward = Entry.Reward.Credit == 0 && Entry.Reward.Items.IsEmpty() && Entry.Reward.GachaBoxIds.IsEmpty();
		if (Entry.KillStreak <= 0 || Entry.Reward.Credit < 0 || bEmptyReward || RewardByStreak.Contains(Entry.KillStreak))
		{
			UE_LOG(LogTemp, Warning, TEXT("[ProjectRWW] 연속 킬 보상 항목 무시(잘못된 값 또는 중복): killStreak=%d"), Entry.KillStreak);
			continue;
		}
		RewardByStreak.Add(Entry.KillStreak, Entry.Reward);
	}

	UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] 연속 킬 보상 표 로드: %d개"), RewardByStreak.Num());
	return true;
}

bool UKillStreakRewardTable::FindReward(int32 KillStreak, FGameReward& OutReward) const
{
	if (const FGameReward* Found = RewardByStreak.Find(KillStreak))
	{
		OutReward = *Found;
		return true;
	}
	return false;
}
