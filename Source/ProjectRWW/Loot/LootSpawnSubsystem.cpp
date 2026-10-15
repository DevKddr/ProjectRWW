// Copyright Epic Games, Inc. All Rights Reserved.

#include "LootSpawnSubsystem.h"
#include "LootRegion.h"
#include "LootSpawnPoint.h"
#include "LootSettings.h"
#include "LootTableManager.h"
#include "Items/ItemPickupActor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"

namespace
{
	// 콘솔 명령은 현재 월드의 서브시스템에 전달만 한다.
	void RespawnCommand(UWorld* World)
	{
		if (World)
		{
			if (ULootSpawnSubsystem* Subsystem = World->GetSubsystem<ULootSpawnSubsystem>())
			{
				Subsystem->RespawnAll();
			}
		}
	}

	void DumpCommand(UWorld* World)
	{
		if (World)
		{
			if (const ULootSpawnSubsystem* Subsystem = World->GetSubsystem<ULootSpawnSubsystem>())
			{
				Subsystem->DumpStats();
			}
		}
	}

	FAutoConsoleCommandWithWorld GLootRespawnCommand(
		TEXT("rww.Loot.Respawn"),
		TEXT("서버에서 루트 픽업을 모두 지우고 스폰 포인트 기준으로 다시 스폰한다."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&RespawnCommand));

	FAutoConsoleCommandWithWorld GLootDumpCommand(
		TEXT("rww.Loot.Dump"),
		TEXT("루트 스폰 결과(지역별 포인트 수/스폰 수)를 로그로 출력한다."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&DumpCommand));
}

bool ULootSpawnSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer))
	{
		return false;
	}
	// 에디터 프리뷰/에셋 에디터 월드에서는 만들지 않는다. 네트워크 모드는 이 시점에 확정이 안 될 수 있어서
	// 클라이언트 제외는 OnWorldBeginPlay에서 한다.
	const UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld();
}

void ULootSpawnSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (InWorld.GetNetMode() == NM_Client)
	{
		return;
	}
	SpawnAll();
}

void ULootSpawnSubsystem::Deinitialize()
{
	SpawnedPickups.Empty();
	Stats.Empty();
	Super::Deinitialize();
}

void ULootSpawnSubsystem::RespawnAll()
{
	UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_Client)
	{
		return;
	}
	ClearSpawned();
	SpawnAll();
}

void ULootSpawnSubsystem::ClearSpawned()
{
	for (const TWeakObjectPtr<AItemPickupActor>& Pickup : SpawnedPickups)
	{
		if (Pickup.IsValid())
		{
			Pickup->Destroy();
		}
	}
	SpawnedPickups.Empty();
	Stats.Empty();
}

void ULootSpawnSubsystem::SpawnAll()
{
	UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	ULootTableManager* TableMgr = GameInstance ? GameInstance->GetSubsystem<ULootTableManager>() : nullptr;
	if (!TableMgr || !TableMgr->IsDataLoaded())
	{
		UE_LOG(LogTemp, Warning, TEXT("[LootSpawn] 루트 테이블 데이터가 없어 스폰하지 않음"));
		return;
	}

	TArray<ALootRegion*> Regions;
	for (TActorIterator<ALootRegion> It(World); It; ++It)
	{
		Regions.Add(*It);
	}

	Stats.Empty();
	int32 TotalPoints = 0;
	int32 TotalSpawned = 0;

	for (TActorIterator<ALootSpawnPoint> It(World); It; ++It)
	{
		const ALootSpawnPoint* Point = *It;
		if (!Point->bEnabled)
		{
			continue;
		}
		++TotalPoints;

		// 포인트의 TableOverride가 유효하면 지역보다 우선한다. 없는 이름이면 경고하고 지역을 따른다.
		FName TableId = NAME_None;
		float SpawnChance = 1.f;
		FName RegionUsed = ULootTableManager::DefaultRegionId;

		if (!Point->TableOverride.IsNone() && TableMgr->HasTable(Point->TableOverride))
		{
			TableId = Point->TableOverride;
			RegionUsed = FName(*FString::Printf(TEXT("Override:%s"), *Point->TableOverride.ToString()));
		}
		else
		{
			if (!Point->TableOverride.IsNone())
			{
				UE_LOG(LogTemp, Warning, TEXT("[LootSpawn] %s: TableOverride '%s'이(가) loot_tables.json에 없어 지역을 따름"),
					*Point->GetName(), *Point->TableOverride.ToString());
			}

			const FName RegionId = FindRegionIdForPoint(Point->GetActorLocation(), Regions);
			if (!TableMgr->ResolveRegion(RegionId, RegionUsed, TableId, SpawnChance))
			{
				UE_LOG(LogTemp, Error, TEXT("[LootSpawn] %s: Default 지역 데이터가 없어 건너뜀"), *Point->GetName());
				continue;
			}
		}

		FRegionStats& RegionStats = Stats.FindOrAdd(RegionUsed);
		++RegionStats.Points;

		if (SpawnChance < 1.f && FMath::FRand() > SpawnChance)
		{
			continue;
		}

		FName ItemIndex;
		if (!TableMgr->DrawItem(TableId, ItemIndex))
		{
			continue;
		}

		UClass* PickupClass = ResolvePickupClass(ItemIndex);
		if (!PickupClass)
		{
			continue;
		}

		if (SpawnPickup(PickupClass, ItemIndex, Point->GetActorTransform()))
		{
			++RegionStats.Spawned;
			++TotalSpawned;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[LootSpawn] 스폰 완료: 포인트 %d개 중 픽업 %d개 (지역 볼륨 %d개)"), TotalPoints, TotalSpawned, Regions.Num());
	DumpStats();
}

FName ULootSpawnSubsystem::FindRegionIdForPoint(const FVector& Location, const TArray<ALootRegion*>& Regions) const
{
	const ALootRegion* Best = nullptr;
	for (const ALootRegion* Region : Regions)
	{
		if (!Region->ContainsPoint(Location))
		{
			continue;
		}
		// 겹치면 Priority가 높은 쪽, 같으면 부피가 작은(더 구체적인) 쪽.
		if (!Best
			|| Region->Priority > Best->Priority
			|| (Region->Priority == Best->Priority && Region->GetVolume() < Best->GetVolume()))
		{
			Best = Region;
		}
	}

	// 어느 지역에도 안 들어 있으면 None -> ResolveRegion이 Default로 처리한다.
	return Best ? Best->GetEffectiveRegionId() : NAME_None;
}

UClass* ULootSpawnSubsystem::ResolvePickupClass(FName ItemIndex)
{
	const ULootSettings* Settings = GetDefault<ULootSettings>();
	if (const TSoftClassPtr<AItemPickupActor>* Mapped = Settings->PickupClassByItem.Find(ItemIndex))
	{
		if (UClass* Loaded = Mapped->LoadSynchronous())
		{
			return Loaded;
		}
	}
	if (UClass* Fallback = Settings->DefaultPickupClass.LoadSynchronous())
	{
		return Fallback;
	}

	if (!WarnedMissingPickupClass.Contains(ItemIndex))
	{
		WarnedMissingPickupClass.Add(ItemIndex);
		UE_LOG(LogTemp, Warning, TEXT("[LootSpawn] 아이템 '%s'의 픽업 BP가 없어 스폰하지 못함 (Project Settings > ProjectRWW Loot Settings)"), *ItemIndex.ToString());
	}
	return nullptr;
}

AItemPickupActor* ULootSpawnSubsystem::SpawnPickup(UClass* PickupClass, FName ItemIndex, const FTransform& PointTransform)
{
	UWorld* World = GetWorld();
	// 위치와 회전만 쓰고 스케일은 1로 둔다 - 포인트의 스케일이 픽업 판정 구에 영향을 주지 않게.
	const FTransform SpawnTransform(PointTransform.GetRotation(), PointTransform.GetLocation(), FVector::OneVector);

	// Deferred로 만들어서 BeginPlay/최초 리플리케이션 전에 ItemIndex를 채운다.
	AItemPickupActor* Pickup = World->SpawnActorDeferred<AItemPickupActor>(
		PickupClass, SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Pickup)
	{
		return nullptr;
	}

	Pickup->ItemIndex = ItemIndex;
	Pickup->FinishSpawning(SpawnTransform);
	SpawnedPickups.Add(Pickup);
	return Pickup;
}

void ULootSpawnSubsystem::DumpStats() const
{
	for (const TPair<FName, FRegionStats>& Pair : Stats)
	{
		UE_LOG(LogTemp, Log, TEXT("[LootSpawn]   지역 %-24s 포인트 %3d / 스폰 %3d"), *Pair.Key.ToString(), Pair.Value.Points, Pair.Value.Spawned);
	}
}
