// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ItemData.generated.h"

// items.json 각 항목과 1:1 대응. 무기를 포함한 모든 카테고리의 아이템이 여기 들어간다.
// 게임플레이 스탯(데미지 등)은 다루지 않고, 표시/시각적인 데이터만 담당한다 -
// 무기의 실제 스탯은 같은 Index로 WeaponDataManager를 따로 조회해야 한다.
USTRUCT(BlueprintType)
struct FItemData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FName Index;

	// "Weapon", "Item" 등. EquipItem()이 이 값으로 분기한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FName Category;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FString Description;

	// RarityDataManager 조회용 키.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FName RarityId;

	// 인벤토리 슬롯 UI에 그려질 2D 아이콘. MeshPath(3D 손모델)와는 별개 용도.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FSoftObjectPath IconPath;

	// 이 아이템을 장착했을 때 스폰할 Actor 클래스(무기는 BP_AK105, 비무기는 BP_HealKit 등).
	// Category로 이미 무기/비무기가 구분되므로 필드를 따로 나눌 필요가 없다 - 무기든
	// 아이템이든 "장착 시 스폰할 Actor" 하나로 통일. items.json의 "actorClassPath"와
	// 이름을 맞춰야 JSON->구조체 변환기가 이 필드를 채워준다 - "Path" 접미사를 빼먹으면
	// 조용히 매칭 실패해서 항상 빈 값으로 남는다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TSoftClassPtr<AActor> ActorClassPath;

	// 장착(Draw) 애니메이션 재생 시간(초). 원래 weapons.json의 무기 전투 스탯 쪽에
	// 있었는데, 이 값을 쓰는 곳(Draw() 애니메이션 재생 시간, 장착 중 발사/재장전/ADS
	// 잠그는 가드)이 전부 "무기냐 아니냐"와 무관한 장착 자체의 속성이라 여기로
	// 옮겼다 - 비무기 아이템도 장착 시간이 필요하다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	float EquipTime = 0.0f;
};
