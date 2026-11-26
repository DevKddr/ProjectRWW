// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameReward.generated.h"

// 보상 한 묶음. 어떤 조건으로 주는지는 모르고 "무엇을 얼마나"만 담는다.
// 새 재화를 추가하면 여기에 필드를 넣고 AMainGameMode::GrantKillStreakReward에서 지급한다.
USTRUCT()
struct FGameReward
{
	GENERATED_BODY()

	UPROPERTY()
	int32 Credit = 0;

	// 지급할 아이템 Index(items.json). 같은 아이템 2개는 두 번 적는다.
	UPROPERTY()
	TArray<FName> Items;

	// 추첨할 가챠 상자의 BoxId(gacha_tables.json). 항목 하나당 1회 추첨한다.
	UPROPERTY()
	TArray<FName> GachaBoxIds;
};
