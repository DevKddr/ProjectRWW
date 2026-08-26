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

	// 리슨 서버 호스트는 이 오브젝트가 곧 서버 권위 오브젝트이기도 해서 FireShot()에서
	// 이미 UpdateSpread()를 호출한다. 여기서 또 부르면 이중 계산되므로, 권한이 없을 때만
	// (원격 클라이언트일 때만) 크로스헤어 예측용으로 미리 계산한다.
	if (!OwnerPawn->HasAuthority())
	{
		UpdateSpread();
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	OwnerPawn->GetController()->GetPlayerViewPoint(ViewLocation, ViewRotation);

	ServerFire(ViewLocation, ViewRotation.Vector());
}

float UMainWeaponComponent::UpdateSpread()
{
	const UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;

	const float BaseSpread = bIsAiming ? SpreadADS : SpreadHipfire;
	const float MaxBloom = bIsAiming ? MaxSpreadBloomADS : MaxSpreadBloomHipfire;

	const double TimeSinceLastShot = Now - LastSpreadUpdateTimeSeconds;
	if (TimeSinceLastShot > SpreadRecoveryDelay)
	{
		const float RecoveryAmount = static_cast<float>(TimeSinceLastShot - SpreadRecoveryDelay) * SpreadRecoveryRate;
		CurrentSpreadDegrees = FMath::Max(CurrentSpreadDegrees - RecoveryAmount, 0.0f);
	}
	LastSpreadUpdateTimeSeconds = Now;

	const float TotalSpreadDegrees = FMath::Clamp(BaseSpread + CurrentSpreadDegrees, 0.0f, BaseSpread + MaxBloom);

	CurrentSpreadDegrees = FMath::Min(CurrentSpreadDegrees + SpreadIncreasePerShot, MaxBloom);

	return TotalSpreadDegrees;
}

float UMainWeaponComponent::GetCurrentSpreadDegrees() const
{
	const float BaseSpread = bIsAiming ? SpreadADS : SpreadHipfire;
	const float MaxBloom = bIsAiming ? MaxSpreadBloomADS : MaxSpreadBloomHipfire;

	// UpdateSpread()가 실제 탄 궤적에 쓰는 것과 동일한 회복 계산. BaseSpread를 더해야
	// GetMaxSpreadDegrees()와 같은 기준(0 ~ BaseSpread+MaxBloom)이 되어 UI에서
	// Current/Max 비율이 실제 산포와 일치한다.
	float RecoveredBloom = CurrentSpreadDegrees;
	if (const UWorld* World = GetWorld())
	{
		const double TimeSinceLastShot = World->GetTimeSeconds() - LastSpreadUpdateTimeSeconds;
		if (TimeSinceLastShot > SpreadRecoveryDelay)
		{
			const float RecoveryAmount = static_cast<float>(TimeSinceLastShot - SpreadRecoveryDelay) * SpreadRecoveryRate;
			RecoveredBloom = FMath::Max(CurrentSpreadDegrees - RecoveryAmount, 0.0f);
		}
	}

	return FMath::Clamp(BaseSpread + RecoveredBloom, 0.0f, BaseSpread + MaxBloom);
}

float UMainWeaponComponent::GetMaxSpreadDegrees() const
{
	const float BaseSpread = bIsAiming ? SpreadADS : SpreadHipfire;
	const float MaxBloom = bIsAiming ? MaxSpreadBloomADS : MaxSpreadBloomHipfire;
	return BaseSpread + MaxBloom;
}

void UMainWeaponComponent::RequestReload()
{
	Server_Reload();
}

void UMainWeaponComponent::RequestSwitchWeapon()
{
	Server_SwitchWeapon();
}

void UMainWeaponComponent::Server_SwitchWeapon_Implementation()
{
	UGameInstance* GameInstance = GetWorld()->GetGameInstance();
	UWeaponDataManager* WeaponDataManager = GameInstance ? GameInstance->GetSubsystem<UWeaponDataManager>() : nullptr;
	if (!WeaponDataManager)
	{
		return;
	}

	const TArray<FWeaponItem> AllWeapons = WeaponDataManager->GetAllWeapons();
	if (AllWeapons.Num() == 0)
	{
		return;
	}

	const int32 CurrentIndex = AllWeapons.IndexOfByPredicate([this](const FWeaponItem& Item) { return Item.Index == WeaponIndex; });
	const int32 NextIndex = (CurrentIndex == INDEX_NONE) ? 0 : (CurrentIndex + 1) % AllWeapons.Num();

	EquipWeapon(AllWeapons[NextIndex].Index, -1);
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
	// 1단계: 무기 전체의 정확도(반동/블룸)로 "이번 발의 중심 방향"을 정한다. 한 발(트리거 한 번)당
	// 딱 한 번만 계산해야 한다 — 펠릿마다 다시 계산하면 블룸이 펠릿 수만큼 잘못 누적된다.
	const float SpreadDegrees = UpdateSpread();
	const FVector AimDirection = FMath::VRandCone(TraceDirection, FMath::DegreesToRadians(SpreadDegrees));

	AController* InstigatorController = Cast<APawn>(GetOwner())->GetController();
	bool bAnyHit = false;

	// 2단계: 그 중심 방향을 기준으로 펠릿마다 PelletSpreadAngle만큼 추가로 흩뿌려서 쏜다.
	// 일반 무기는 PelletCount=1, PelletSpreadAngle=0이라 루프가 한 번만 돌고 AimDirection
	// 그대로 나가므로, 기존 단발 무기 동작과 완전히 동일하게 자연스럽게 처리된다.
	for (int32 PelletIndex = 0; PelletIndex < PelletCount; ++PelletIndex)
	{
		const FVector PelletDirection = FMath::VRandCone(AimDirection, FMath::DegreesToRadians(PelletSpreadAngle));
		const FVector TraceEnd = TraceStart + PelletDirection * MaxRange;

		FHitResult HitResult;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(GetOwner());

		const bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Pawn, QueryParams) && HitResult.GetActor();
		if (bHit)
		{
			const float HitDistance = FVector::Dist(TraceStart, HitResult.ImpactPoint);
			UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] %s hit %s at distance %.1f (pellet %d/%d)"),
				*GetNameSafe(GetOwner()), *GetNameSafe(HitResult.GetActor()), HitDistance, PelletIndex + 1, PelletCount);

			UGameplayStatics::ApplyDamage(HitResult.GetActor(), Damage, InstigatorController, GetOwner(), UDamageType::StaticClass());
			bAnyHit = true;
		}

		// 펠릿마다 각자의 궤적을 그려야 샷건 특유의 흩어지는 모습이 보인다 — 마지막 한 번만
		// 보내면 나머지 펠릿의 시각 효과가 전부 사라져 보인다.
		MulticastPlayFireEffects(TraceStart, bHit ? HitResult.ImpactPoint : TraceEnd, bHit);
	}

	if (!bAnyHit)
	{
		UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] %s fired and missed"), *GetNameSafe(GetOwner()));
	}
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
	DrawDebugLine(GetWorld(), TraceStart, TraceEnd, bHit ? FColor::Green : FColor::Red, false, 20.0f, 0, 0.5f);
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
