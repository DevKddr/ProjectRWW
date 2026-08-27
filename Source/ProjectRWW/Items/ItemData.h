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

	// "Weapon", "Misc" 등. EquipItem()이 이 값으로 분기한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FName Category;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FString Description;

	// RarityDataManager 조회용 키.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FName RarityId;

	// 손에 들었을 때 보여줄 스켈레탈 메쉬 에셋 경로. 비어있으면 시각적 표현 없음(Misc 등).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FSoftObjectPath MeshPath;

	// MeshPath로 로드한 메쉬에 적용할 균일 스케일. 에셋마다 원본 크기가 달라서
	// FirstPersonMesh 컴포넌트 하나로 여러 무기를 돌려쓰려면 아이템별로 보정이 필요하다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	float MeshScale = 1.0f;

	// 인벤토리 슬롯 UI에 그려질 2D 아이콘. MeshPath(3D 손모델)와는 별개 용도.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FSoftObjectPath IconPath;
};
