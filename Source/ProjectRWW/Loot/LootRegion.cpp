// Copyright Epic Games, Inc. All Rights Reserved.

#include "LootRegion.h"
#include "LootTableManager.h"
#include "Components/BoxComponent.h"

ALootRegion::ALootRegion()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	// 서버가 스폰 위치/테이블을 정하는 데이터일 뿐이라 클라이언트에는 필요 없다.
	bNetLoadOnClient = false;

#if WITH_EDITORONLY_DATA
	// World Partition 맵에서 서버는 플레이어(스트리밍 소스)가 없는 레이드 시작 시점에 스폰을 끝내므로,
	// 공간 로딩 액터면 이 데이터가 아직 로드되지 않아 스폰이 0개가 된다. 항상 로드되게 한다.
	bIsSpatiallyLoaded = false;
#endif

	Volume = CreateDefaultSubobject<UBoxComponent>(TEXT("Volume"));
	SetRootComponent(Volume);
	Volume->InitBoxExtent(FVector(500.0, 500.0, 200.0));
	// 포함 여부는 수학으로만 검사한다. 콜리전을 켜면 오버랩 비용만 생긴다.
	Volume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Volume->SetGenerateOverlapEvents(false);
	// 에디터에서 지역 크기가 보이도록 선을 그리되 게임 화면에는 안 나오게 한다.
	Volume->SetHiddenInGame(true);
	Volume->ShapeColor = FColor(255, 170, 0);
}

FName ALootRegion::GetEffectiveRegionId() const
{
	return RegionId.IsNone() ? ULootTableManager::DefaultRegionId : RegionId;
}

bool ALootRegion::ContainsPoint(const FVector& WorldPoint) const
{
	// 회전만 되돌린 로컬 좌표(스케일 미적용)를 스케일 반영된 반치수와 비교한다.
	const FVector Local = Volume->GetComponentTransform().InverseTransformPositionNoScale(WorldPoint);
	const FVector Extent = Volume->GetScaledBoxExtent();
	return FMath::Abs(Local.X) <= Extent.X
		&& FMath::Abs(Local.Y) <= Extent.Y
		&& FMath::Abs(Local.Z) <= Extent.Z;
}

double ALootRegion::GetVolume() const
{
	const FVector Extent = Volume->GetScaledBoxExtent();
	return 8.0 * Extent.X * Extent.Y * Extent.Z;
}
