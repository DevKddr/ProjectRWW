// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LootSpawnPoint.generated.h"

class USceneComponent;
class UArrowComponent;

// 아이템이 놓일 자리 하나. 레벨 디자이너는 이 액터를 바닥에 놓기만 하면 된다 -
// 어떤 아이템이 나올지는 이 포인트가 들어 있는 ALootRegion(없으면 Default)이 정한다.
//
// 액터 원점이 곧 아이템이 놓이는 위치이므로 바닥 높이에 맞춰 둘 것(화살표가 바닥을 향하는 기준).
// ALootRegion과 마찬가지로 서버 시작 시 한 번만 읽히는 데이터 액터다.
UCLASS()
class PROJECTRWW_API ALootSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	ALootSpawnPoint();

	// false면 이 포인트는 스폰에서 제외된다(임시로 끄고 싶을 때).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	bool bEnabled = true;

	// 지역과 상관없이 이 포인트만 특정 테이블을 쓰게 하고 싶을 때만 채운다. 비워 두면 지역을 따른다.
	// loot_tables.json의 TableId와 같은 이름이어야 한다(없는 이름이면 경고 후 지역을 따름).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	FName TableOverride;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Loot")
	TObjectPtr<USceneComponent> SceneRoot;

#if WITH_EDITORONLY_DATA
	// 에디터에서 자리를 눈으로 확인하기 위한 표시. 패키지/서버에는 포함되지 않는다.
	UPROPERTY()
	TObjectPtr<UArrowComponent> EditorArrow;
#endif
};
