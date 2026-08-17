// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MainPlayerRecord.generated.h"

USTRUCT()
struct FMainPlayerRecord
{
	GENERATED_BODY()

	UPROPERTY()
	FString PlayerID;

	UPROPERTY()
	int32 KillCount = 0;

	UPROPERTY()
	int32 DeathCount = 0;

	UPROPERTY()
	int32 Currency = 0;

	// 아이템 스키마는 인벤토리 서브프로젝트에서 확정 전까지 JSON 문자열로만 다룬다.
	UPROPERTY()
	FString ItemsJson = TEXT("[]");
};
