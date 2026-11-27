// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "KillNotice.generated.h"

// 서버 -> 킬한 사람: 이번 처치 한 번의 결과. 합산과 목록은 받는 쪽 팝업이 한다.
USTRUCT()
struct FKillNotice
{
	GENERATED_BODY()

	UPROPERTY()
	int32 KillStreak = 0;

	UPROPERTY()
	FString VictimName;

	// 이번 처치로 실제 받은 Credit(기본 + 연속 보상).
	UPROPERTY()
	int32 Credit = 0;
};
