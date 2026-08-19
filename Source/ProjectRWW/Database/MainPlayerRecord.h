// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MainPlayerRecord.generated.h"

USTRUCT(BlueprintType)
struct FMainPlayerRecord
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString PlayerID;

	UPROPERTY(BlueprintReadOnly)
	int32 KillCount = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 DeathCount = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 Currency = 0;

	// 아이템 스키마는 인벤토리 서브프로젝트에서 확정 전까지 JSON 문자열로만 다룬다.
	UPROPERTY(BlueprintReadOnly)
	FString ItemsJson = TEXT("[]");
};
