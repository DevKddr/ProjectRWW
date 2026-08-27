// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InventorySlot.generated.h"

// 인벤토리 한 칸의 상태. AMainPlayerController::InventorySlots 배열의 원소로 쓰인다.
USTRUCT(BlueprintType)
struct FInventorySlot
{
	GENERATED_BODY()

	// 이 슬롯에 든 아이템의 Index(ItemDataManager 조회 키). 빈 슬롯이면 NAME_None.
	UPROPERTY(BlueprintReadOnly)
	FName ItemIndex = NAME_None;

	// Category가 "Weapon"인 아이템일 때만 의미 있음. -1 = 아직 한 번도 장착 안 해서
	// 다음 장착 시 탄창을 가득 채운 채로 시작해야 한다는 뜻(EquipWeapon의 SavedAmmo 규칙과 동일).
	UPROPERTY(BlueprintReadOnly)
	int32 CurrentAmmo = -1;

	// ItemIndex가 NAME_None이면(아무 아이템도 없으면) true. 코드 여기저기서
	// "== NAME_None"을 직접 비교하는 대신 이걸 써서 의도를 명확히 한다.
	bool IsEmpty() const { return ItemIndex == NAME_None; }
};
