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
#include "TimerManager.h"

UMainWeaponComponent::UMainWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UMainWeaponComponent::BeginPlay()
{
	Super::BeginPlay();

	// weapons.json은 서버/클라이언트 모두에 동일하게 존재하는 공유 파일이라, 각자 독립적으로
	// 읽어도 같은 값을 얻는다 — WalkSpeed/RunSpeed/JumpPower와 같은 정책.
	EquipWeapon(WeaponIndex, -1);
}

void UMainWeaponComponent::EquipWeapon(FName NewWeaponIndex, int32 SavedAmmo)
{
	// 이전 무기가 예약해둔 발사(FullAuto 연사 타이머, 버스트 잔탄)를 정리한다 —
	// 안 하면 새 무기 스탯으로 이전 무기의 남은 발사가 뒤섞여 나갈 수 있다.
	StopFire();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BurstTimerHandle);
		World->GetTimerManager().ClearTimer(ReloadTimerHandle);
	}
	PendingBurstShotsRemaining = 0;
	bIsReloading = false;

	WeaponIndex = NewWeaponIndex;

	if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
	{
		if (UWeaponDataManager* WeaponDataManager = GameInstance->GetSubsystem<UWeaponDataManager>())
		{
			FWeaponItem WeaponData;
			if (WeaponDataManager->GetWeaponData(WeaponIndex, WeaponData))
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

void UMainWeaponComponent::OnRep_WeaponIndex()
{
	EquipWeapon(WeaponIndex, -1);
}

void UMainWeaponComponent::StartFire()
{
	RequestFire();

	// FullAuto는 버튼을 뗄 때까지 FireRate_RPS 간격으로 계속 쏴야 하므로 반복 타이머를 건다.
	// 첫 발은 위에서 이미 쐈으니, 타이머의 첫 실행은 한 박자 뒤로 미룬다(중복 발사 방지).
	UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] StartFire called. FireMode=%s, FireRate_RPS=%.2f"), *FireMode.ToString(), FireRate_RPS);

	if (FireMode == TEXT("FullAuto"))
	{
		if (UWorld* World = GetWorld())
		{
			const float FireIntervalSeconds = FireRate_RPS > 0.0f ? (1.0f / FireRate_RPS) : 0.0f;
			UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] FullAuto timer armed, interval=%.3f"), FireIntervalSeconds);
			World->GetTimerManager().SetTimer(AutoFireTimerHandle, this, &UMainWeaponComponent::RequestFire, FireIntervalSeconds, true, FireIntervalSeconds);
		}
	}
}

void UMainWeaponComponent::StopFire()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoFireTimerHandle);
	}
}

void UMainWeaponComponent::RequestFire()
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

void UMainWeaponComponent::StartADS()
{
	// 로컬 예측: 서버 응답을 기다리지 않고 즉시 조준 연출을 시작할 수 있게 한다.
	bIsAiming = true;
	Server_SetAiming(true);
}

void UMainWeaponComponent::StopADS()
{
	bIsAiming = false;
	Server_SetAiming(false);
}

void UMainWeaponComponent::Server_SetAiming_Implementation(bool bNewAiming)
{
	bIsAiming = bNewAiming;
}

void UMainWeaponComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceStart, const FVector_NetQuantizeNormal& TraceDirection)
{
	if (CurrentAmmo <= 0)
	{
		return;
	}

	// 재장전 중에 방아쇠를 당기면 재장전을 취소하고 바로 발사를 시도한다. 이미 소비한 마나는 환불하지 않는다.
	if (bIsReloading)
	{
		bIsReloading = false;
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(ReloadTimerHandle);
		}
	}

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

	LastFireTimeSeconds = Now;
	--CurrentAmmo;
	FireShot(TraceStart, TraceDirection);

	// Burst는 첫 발 이후 나머지 (BurstCount - 1)발을 BurstShotInterval 간격으로 이어서 쏜다.
	// 조준 방향은 저장해두지 않고, FireBurstShot에서 매번 그 순간의 실시간 방향을 다시 읽는다.
	if (FireMode == TEXT("Burst") && BurstCount > 1)
	{
		if (UWorld* World = GetWorld())
		{
			PendingBurstShotsRemaining = BurstCount - 1;

			const float Interval = BurstShotInterval > 0.0f ? BurstShotInterval : 0.05f;
			World->GetTimerManager().SetTimer(BurstTimerHandle, this, &UMainWeaponComponent::FireBurstShot, Interval, true);
		}
	}
}

void UMainWeaponComponent::FireBurstShot()
{
	UWorld* World = GetWorld();
	if (!World || PendingBurstShotsRemaining <= 0 || CurrentAmmo <= 0)
	{
		if (World)
		{
			World->GetTimerManager().ClearTimer(BurstTimerHandle);
		}
		return;
	}

	--PendingBurstShotsRemaining;
	--CurrentAmmo;

	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn && OwnerPawn->GetController())
	{
		FVector ViewLocation;
		FRotator ViewRotation;
		OwnerPawn->GetController()->GetPlayerViewPoint(ViewLocation, ViewRotation);
		FireShot(ViewLocation, ViewRotation.Vector());
	}

	if (PendingBurstShotsRemaining <= 0)
	{
		World->GetTimerManager().ClearTimer(BurstTimerHandle);
	}
}

void UMainWeaponComponent::FireShot(const FVector_NetQuantize& TraceStart, const FVector_NetQuantizeNormal& TraceDirection)
{
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
	if (!CanReload || CurrentAmmo >= MagazineSize || bIsReloading)
	{
		return;
	}

	UGameInstance* GameInstance = GetWorld()->GetGameInstance();
	UWeaponDataManager* WeaponDataManager = GameInstance ? GameInstance->GetSubsystem<UWeaponDataManager>() : nullptr;

	float RequiredMana = 0.0f;
	if (!WeaponDataManager || !WeaponDataManager->GetRequiredReloadMana(WeaponIndex, CurrentAmmo, RequiredMana))
	{
		return;
	}

	UMainManaComponent* ManaComp = GetOwner() ? GetOwner()->FindComponentByClass<UMainManaComponent>() : nullptr;
	if (!ManaComp || !ManaComp->ConsumeMana(RequiredMana))
	{
		return;
	}

	bIsReloading = true;
	ReloadStartTimeSeconds = GetWorld()->GetTimeSeconds();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(ReloadTimerHandle, this, &UMainWeaponComponent::CompleteReload, ReloadTime, false);
	}
}

void UMainWeaponComponent::CompleteReload()
{
	bIsReloading = false;
	CurrentAmmo = MagazineSize;
}

void UMainWeaponComponent::OnRep_IsReloading()
{
	if (bIsReloading)
	{
		if (UWorld* World = GetWorld())
		{
			ReloadStartTimeSeconds = World->GetTimeSeconds();
		}
	}
}

float UMainWeaponComponent::GetReloadProgress() const
{
	if (!bIsReloading || ReloadTime <= 0.0f)
	{
		return 0.0f;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.0f;
	}

	const float ElapsedTime = World->GetTimeSeconds() - ReloadStartTimeSeconds;
	return FMath::Clamp(ElapsedTime / ReloadTime, 0.0f, 1.0f);
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
	DOREPLIFETIME_CONDITION(UMainWeaponComponent, bIsReloading, COND_OwnerOnly);
	DOREPLIFETIME(UMainWeaponComponent, WeaponIndex);
}
