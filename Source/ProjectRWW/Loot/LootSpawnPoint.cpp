// Copyright Epic Games, Inc. All Rights Reserved.

#include "LootSpawnPoint.h"
#include "Components/SceneComponent.h"
#include "Components/ArrowComponent.h"

ALootSpawnPoint::ALootSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	// 서버가 아이템을 만들 자리만 알려 주는 데이터라서 클라이언트에는 필요 없다.
	bNetLoadOnClient = false;

#if WITH_EDITORONLY_DATA
	// World Partition 맵에서 서버는 플레이어(스트리밍 소스)가 없는 레이드 시작 시점에 스폰을 끝내므로,
	// 공간 로딩 액터면 이 데이터가 아직 로드되지 않아 스폰이 0개가 된다. 항상 로드되게 한다.
	bIsSpatiallyLoaded = false;
#endif

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

#if WITH_EDITORONLY_DATA
	EditorArrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("EditorArrow"));
	if (EditorArrow)
	{
		EditorArrow->SetupAttachment(SceneRoot);
		EditorArrow->ArrowColor = FColor(255, 170, 0);
		EditorArrow->ArrowSize = 0.6f;
		EditorArrow->SetHiddenInGame(true);
		// 위에서 아래로(바닥을 가리키게) 보이도록. 위치 기준점이 바닥이라는 표시다.
		EditorArrow->SetRelativeRotation(FRotator(-90.0, 0.0, 0.0));
	}
#endif
}
