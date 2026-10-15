// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Items/ItemPickupActor.h"
#include "LootSettings.generated.h"

// 루트 스폰 시스템이 "어떤 아이템을 어떤 픽업 BP로 월드에 만들지" 고르는 설정.
// Project Settings > ProjectRWW Loot Settings 에서 편집한다(Config/DefaultGame.ini에 저장됨).
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "ProjectRWW Loot Settings"))
class PROJECTRWW_API ULootSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	// 아이템 Index -> 그 아이템을 바닥에 놓을 때 쓰는 픽업 BP(메시/판정 구가 설정된 BP_Pickup_xxx).
	// 아이템마다 BP를 만드는 기존 방식을 그대로 쓰기 위한 매핑이다.
	UPROPERTY(Config, EditAnywhere, Category = "Loot")
	TMap<FName, TSoftClassPtr<AItemPickupActor>> PickupClassByItem;

	// 위 매핑에 없는 아이템에 쓸 픽업 BP. 비워 두면 매핑이 없는 아이템은 스폰하지 않고 경고만 남긴다.
	// 주의: 이 BP에는 아이템별 메시가 없으므로 임시 표시용(예: 상자 모양)으로만 쓸 것.
	UPROPERTY(Config, EditAnywhere, Category = "Loot")
	TSoftClassPtr<AItemPickupActor> DefaultPickupClass;
};
