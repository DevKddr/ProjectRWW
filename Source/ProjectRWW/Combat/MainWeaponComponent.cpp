// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainWeaponComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Net/UnrealNetwork.h"
#include "Weapons/WeaponDataManager.h"
#include "Combat/MainManaComponent.h"

UMainWeaponComponent::UMainWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UMainWeaponComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		EquipWeapon(WeaponIndex, -1);
	}
}

void UMainWeaponComponent::EquipWeapon(FName NewWeaponIndex, int32 SavedAmmo)
{
	WeaponIndex = NewWeaponIndex;

	if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
	{
		if (UWeaponDataManager* WeaponMgr = GameInstance->GetSubsystem<UWeaponDataManager>())
		{
			FWeaponItem WeaponData;
			if (WeaponMgr->GetWeaponData(WeaponIndex, WeaponData))
			{
				WeaponType = WeaponData.WeaponType;
				Rarity = WeaponData.Rarity;
				Name = WeaponData.Name;
				Description = WeaponData.Description;

				const FWeaponStats& Stats = WeaponData.Stats;
				Damage = Stats.Damage;
				PelletCount = Stats.PelletCount;
				HeadshotMultiplier = Stats.HeadshotMultiplier;
				FireRate_RPS = Stats.FireRate_RPS;
				FireMode = Stats.FireMode;
				BurstCount = Stats.BurstCount;
				BurstShotInterval = Stats.BurstShotInterval;
				MagazineSize = Stats.MagazineSize;
				ReloadTime = Stats.ReloadTime;
				CanReload = Stats.CanReload;
				WeaponReqMana = Stats.WeaponReqMana;
				ManaPerAmmo = Stats.ManaPerAmmo;
				MaxRange = Stats.MaxRange;
				DamageFalloffStart = Stats.DamageFalloffStart;
				DamageFalloffEnd = Stats.DamageFalloffEnd;
				DamageFalloffMin = Stats.DamageFalloffMin;
				IsHitscan = Stats.IsHitscan;
				ProjectileSpeed = Stats.ProjectileSpeed;
				CanADS = Stats.CanADS;
				ScopeZoomLevel = Stats.ScopeZoomLevel;
				SpreadHipfire = Stats.SpreadHipfire;
				SpreadADS = Stats.SpreadADS;
				SpreadIncreasePerShot = Stats.SpreadIncreasePerShot;
				MaxSpreadBloomHipfire = Stats.MaxSpreadBloomHipfire;
				MaxSpreadBloomADS = Stats.MaxSpreadBloomADS;
				SpreadRecoveryDelay = Stats.SpreadRecoveryDelay;
				SpreadRecoveryRate = Stats.SpreadRecoveryRate;
				RecoilVertical = Stats.RecoilVertical;
				RecoilHorizontal = Stats.RecoilHorizontal;
				ADSSpeed = Stats.ADSSpeed;
				ADSMoveSpeedMultiplier = Stats.ADSMoveSpeedMultiplier;
				MoveSpeedMultiplier = Stats.MoveSpeedMultiplier;
				EquipTime = Stats.EquipTime;
				PelletSpreadAngle = Stats.PelletSpreadAngle;
			}
		}
	}

	CurrentAmmo = (SavedAmmo < 0) ? MagazineSize : FMath::Clamp(SavedAmmo, 0, MagazineSize);
	EquippedTimeSeconds = FPlatformTime::Seconds();
}

void UMainWeaponComponent::StartFire()
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn || !OwnerPawn->GetController())
	{
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	OwnerPawn->GetController()->GetPlayerViewPoint(ViewLocation, ViewRotation);

	ServerFire(ViewLocation, ViewRotation.Vector());
}

void UMainWeaponComponent::RequestReload()
{
	Server_Reload();
}

void UMainWeaponComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceStart, const FVector_NetQuantizeNormal& TraceDirection)
{
	const double Now = FPlatformTime::Seconds();
	const float FireIntervalSeconds = FireRate_RPS > 0.0f ? (1.0f / FireRate_RPS) : 0.0f;

	if (Now - LastFireTimeSeconds < FireIntervalSeconds)
	{
		return;
	}

	if (Now - EquippedTimeSeconds < EquipTime)
	{
		return;
	}

	if (CurrentAmmo <= 0)
	{
		return;
	}

	LastFireTimeSeconds = Now;
	--CurrentAmmo;

	const FVector TraceEnd = TraceStart + TraceDirection * MaxRange;

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());

	const bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Pawn, QueryParams);

	if (bHit && HitResult.GetActor())
	{
		const float HitDistance = FVector::Dist(TraceStart, HitResult.ImpactPoint);
		UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] %s hit %s at distance %.1f"), *GetNameSafe(GetOwner()), *GetNameSafe(HitResult.GetActor()), HitDistance);

		UGameplayStatics::ApplyDamage(HitResult.GetActor(), Damage, Cast<APawn>(GetOwner())->GetController(), GetOwner(), UDamageType::StaticClass());
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] %s fired and missed"), *GetNameSafe(GetOwner()));
	}

	MulticastPlayFireEffects(TraceStart, bHit ? HitResult.ImpactPoint : TraceEnd, bHit);
}

bool UMainWeaponComponent::ServerFire_Validate(const FVector_NetQuantize& TraceStart, const FVector_NetQuantizeNormal& TraceDirection)
{
	return true;
}

void UMainWeaponComponent::Server_Reload_Implementation()
{
	if (!CanReload || CurrentAmmo >= MagazineSize)
	{
		return;
	}

	const int32 MissingAmmo = FMath::Max(MagazineSize - CurrentAmmo, 0);
	const float RequiredMana = WeaponReqMana - static_cast<float>(MissingAmmo) * ManaPerAmmo;

	UMainManaComponent* ManaComp = GetOwner() ? GetOwner()->FindComponentByClass<UMainManaComponent>() : nullptr;
	if (!ManaComp || !ManaComp->ConsumeMana(RequiredMana))
	{
		return;
	}

	CurrentAmmo = MagazineSize;
}

void UMainWeaponComponent::MulticastPlayFireEffects_Implementation(const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& TraceEnd, bool bHit)
{
	DrawDebugLine(GetWorld(), TraceStart, TraceEnd, bHit ? FColor::Green : FColor::Red, false, 1.0f, 0, 1.5f);
}

void UMainWeaponComponent::OnRep_CurrentAmmo()
{
	// TODO: 탄약 UI 갱신 등, 클라이언트 반응 로직이 필요해지면 여기에 추가
}

void UMainWeaponComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UMainWeaponComponent, CurrentAmmo, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UMainWeaponComponent, MagazineSize, COND_OwnerOnly);
}
