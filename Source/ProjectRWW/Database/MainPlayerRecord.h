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

	// 화면에 표시되는 닉네임. PlayerID(식별자)와 분리되어 있어, 나중에 PlayerID가
	// 사용자 입력이 아닌 값(예: Steam UniqueId)으로 바뀌어도 이 필드는 그대로 쓸 수 있다.
	UPROPERTY(BlueprintReadOnly)
	FString PlayerName;

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
