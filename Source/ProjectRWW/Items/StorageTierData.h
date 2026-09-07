// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "StorageTierData.generated.h"

// storage_tiers.json 각 항목과 1:1 대응.
USTRUCT(BlueprintType)
struct FStorageTierData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Storage")
	int32 Tier = 0;

	// 전체 칸 수. 세로 줄 수는 저장하지 않고 위젯이 SlotCount / Cols로 계산한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Storage")
	int32 SlotCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Storage")
	int32 Cols = 0;
};
