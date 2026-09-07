// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainWeaponComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Net/UnrealNetwork.h"
#include "Weapons/WeaponDataManager.h"
#include "Items/ItemDataManager.h"
#include "Combat/MainManaComponent.h"
#include "TimerManager.h"
#include "Engine/SkeletalMesh.h"
#include "Components/SkeletalMeshComponent.h"
#include "Player/MainCharacter.h"

UMainWeaponComponent::UMainWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UMainWeaponComponent::BeginPlay()
{
	Super::BeginPlay();

	// 예전엔 여기서 디버그용으로 EquipWeapon(WeaponIndex, -1)을 무조건 호출했지만,
	// 이제 장착은 인벤토리(AMainPlayerController::OnPossess -> EquipItem)가 책임진다.
	// 스폰 직후엔 빈손이 맞는 상태다.
}

void UMainWeaponComponent::EquipVisual(UClass* ActorClass, TObjectPtr<AActor>& OutActiveActor)
{
	if (ActorClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetOwner();
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AActor* NewActor = GetWorld()->SpawnActor<AActor>(ActorClass, SpawnParams);
		if (NewActor)
		{
			if (USceneComponent* AttachTarget = WeaponMeshComponent->GetAttachParent())
			{
				NewActor->AttachToComponent(AttachTarget, FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("VB ik_hand_gun"));
			}

			if (OutActiveActor)
			{
				OutActiveActor->Destroy();
			}
			OutActiveActor = NewActor;
		}
	}
	else if (OutActiveActor)
	{
		OutActiveActor->Destroy();
		OutActiveActor = nullptr;
	}
}

void UMainWeaponComponent::EquipWeapon(FName NewWeaponIndex, int32 SavedAmmo, int32 SlotIndex)
{
	// 원격 클라이언트에게 "장착이 다시 일어났다"는 걸 반드시 알리기 위한 트리거 -
	// ReplicationSequence 선언부 주석 참고.
	++ReplicationSequence;

	// 이전 무기가 예약해둔 발사(FullAuto 연사 타이머, 버스트 잔탄)를 정리한다 —
	// 안 하면 새 무기 스탯으로 이전 무기의 남은 발사가 뒤섞여 나갈 수 있다.
	StopFire();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BurstTimerHandle);
		World->GetTimerManager().ClearTimer(ClientBurstTimerHandle);
		World->GetTimerManager().ClearTimer(ReloadTimerHandle);
	}
	PendingBurstShotsRemaining = 0;
	ClientBurstShotsRemaining = 0;
	bIsReloading = false;
	bAmmoEmptyNotified = false;

	WeaponIndex = NewWeaponIndex;
	EquippedSlotIndex = SlotIndex;

	if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
	{
		if (UWeaponDataManager* WeaponDataManager = GameInstance->GetSubsystem<UWeaponDataManager>())
		{
			FWeaponItem WeaponData;
			if (WeaponDataManager->GetWeaponData(WeaponIndex, WeaponData))
			{
				WeaponType = WeaponData.WeaponType;
				ApplyWeaponStats(WeaponData.Stats);

				if (AMainCharacter* OwningCharacter = Cast<AMainCharacter>(GetOwner()))
				{
					OwningCharacter->ReceiveFireModeChange(FireMode);
				}
			}
			else
			{
				WeaponType = NAME_None;
				ApplyWeaponStats(FWeaponStats());
			}
		}
	}

	// -2는 OnRep_ReplicationSequence()가 클라이언트 재동기화용으로 넘기는 특수값이다 - 이때는
	// CurrentAmmo를 건드리지 않는다. CurrentAmmo는 자기 자신의 리플리케이션으로
	// 이미 정확한 값이 도착해 있어서, 여기서 다시 세팅하면 그 값을 덮어써버린다.
	if (SavedAmmo != -2)
	{
		CurrentAmmo = (SavedAmmo < 0) ? MagazineSize : FMath::Clamp(SavedAmmo, 0, MagazineSize);
	}
	EquippedTimeSeconds = FPlatformTime::Seconds();

	// 무기 Actor 스폰은 ItemDataManager(items.json) 담당 - WeaponDataManager는 스탯만 안다.
	// OnRep_ReplicationSequence를 통해 이 함수가 모든 클라이언트에서도 재실행되므로,
	// 여기서 세팅하면 다른 플레이어 화면에도 자연스럽게 반영된다.
	// WeaponMeshComponent는 블루프린트(BP_MainCharacter)가 컨스트럭션 스크립트에서
	// 채워줘야 하는 값이라, 아직 안 채워졌으면(초기화 순서 문제 등) 조용히 건너뛴다.
	if (WeaponMeshComponent)
	{
		if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
		{
			if (UItemDataManager* ItemDataManager = GameInstance->GetSubsystem<UItemDataManager>())
			{
				FItemData ItemData;
				if (ItemDataManager->GetItemData(WeaponIndex, ItemData))
				{
					// EquipTime은 이제 weapons.json이 아니라 items.json 쪽 값이다 -
					// 장착 애니메이션 재생 시간이자, 아래 IsEquipping() 계열 가드가
					// 참조하는 값이라 스폰 성공 여부와 무관하게 항상 갱신해야 한다.
					EquipTime = ItemData.EquipTime;

					// 무기 Actor 스폰/부착/헌것파괴는 EquipVisual()에 위임한다
					// (EquipItemVisual()과 공용).
					EquipVisual(ItemData.ActorClassPath.LoadSynchronous(), ActiveHandActor);
				}
				else if (ActiveHandActor)
				{
					ActiveHandActor->Destroy();
					ActiveHandActor = nullptr;
				}
			}
		}
	}

	// 무기 Actor 스폰이 끝난 뒤에 호출해야 BP의 ReceiveWeaponEquip 구현부에서
	// GetMainWeapon()으로 방금 스폰된 새 무기 Actor를 정확히 가져올 수 있다.
	// 무기 조회 성공 여부와 무관하게 항상 호출한다 - BP ReceiveWeaponEquip이 자체
	// Branch로 성공/실패(=빈손)를 다시 판단해서 각각 다른 TacticalViewSettings를
	// 적용해야 하는데, 여기서 실패 시 호출을 건너뛰면 빈손 분기가 아예 실행될
	// 기회가 없다.
	if (AMainCharacter* OwningCharacter = Cast<AMainCharacter>(GetOwner()))
	{
		OwningCharacter->ReceiveWeaponEquip(WeaponIndex);
	}
}

void UMainWeaponComponent::ApplyWeaponStats(const FWeaponStats& Stats)
{
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
	ADSSpeed = Stats.ADSSpeed;
	ADSMoveSpeedMultiplier = Stats.ADSMoveSpeedMultiplier;
	MoveSpeedMultiplier = Stats.MoveSpeedMultiplier;
	PelletSpreadAngle = Stats.PelletSpreadAngle;
}

void UMainWeaponComponent::UnequipWeapon()
{
	// 타이머 정리 + WeaponType/스탯/메쉬 리셋을 EquipWeapon()의 조회 실패(else) 분기에
	// 그대로 위임한다 - 여기서 중복으로 다시 안 써도 됨. -2라 CurrentAmmo만 안 건드리므로
	// 그건 아래에서 직접 처리한다.
	EquipWeapon(NAME_None, -2);
	CurrentAmmo = 0;
	CurrentSpreadDegrees = 0.0f;
}

void UMainWeaponComponent::OnRep_ReplicationSequence()
{
	EquipWeapon(WeaponIndex, -2, EquippedSlotIndex);
}

void UMainWeaponComponent::EquipItemVisual(FName ItemIndex)
{
	ActiveItemIndex = ItemIndex;

	// 무기 Actor 스폰 로직(EquipWeapon())과 완전히 같은 패턴 - 새 걸 먼저 붙이고
	// 헌 걸 나중에 지워서 손이 비는 순간을 없앤다. WeaponMeshComponent는 무기든
	// 아이템이든 공통으로 쓰는 부착 대상(FirstPersonMesh)이다.
	if (!WeaponMeshComponent)
	{
		return;
	}

	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	UItemDataManager* ItemDataManager = GameInstance ? GameInstance->GetSubsystem<UItemDataManager>() : nullptr;
	if (!ItemDataManager)
	{
		return;
	}

	FItemData ItemData;
	if (ItemDataManager->GetItemData(ItemIndex, ItemData))
	{
		EquipVisual(ItemData.ActorClassPath.LoadSynchronous(), ActiveHandActor);
	}
	else if (ActiveHandActor)
	{
		ActiveHandActor->Destroy();
		ActiveHandActor = nullptr;
	}

	// 스폰이 끝난 뒤에 호출해야 BP의 ReceiveItemEquip 구현부에서 GetMainItem()으로
	// 방금 스폰된 새 아이템 Actor를 정확히 가져올 수 있다.
	if (AMainCharacter* OwningCharacter = Cast<AMainCharacter>(GetOwner()))
	{
		OwningCharacter->ReceiveItemEquip(ItemIndex);
	}
}

void UMainWeaponComponent::OnRep_ActiveItemIndex()
{
	EquipItemVisual(ActiveItemIndex);
}

void UMainWeaponComponent::Multicast_DestroyWeaponActor_Implementation()
{
	if (ActiveHandActor)
	{
		ActiveHandActor->Destroy();
		ActiveHandActor = nullptr;
	}
}

void UMainWeaponComponent::StartFire()
{
	if (!HasWeaponEquipped())
	{
		return;
	}

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

void UMainWeaponComponent::StopFire(bool bCancelBurst)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoFireTimerHandle);

		if (bCancelBurst)
		{
			World->GetTimerManager().ClearTimer(ClientBurstTimerHandle);
		}
	}

	if (AMainCharacter* OwningCharacter = Cast<AMainCharacter>(GetOwner()))
	{
		OwningCharacter->ReceiveWeaponFireStopped();
	}
}

void UMainWeaponComponent::RequestFire()
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn || !OwnerPawn->GetController())
	{
		return;
	}

	// 재장전 중에는 발사 예측(애니메이션)도 하지 않는다 - 서버가 어차피 거부한다.
	if (bIsReloading)
	{
		return;
	}

	AMainCharacter* OwningCharacter = Cast<AMainCharacter>(GetOwner());

	// 달리기 중에는 발사할 수 없다.
	if (OwningCharacter && OwningCharacter->IsSprinting())
	{
		return;
	}

	// 장착 시간이 아직 안 지났으면 예측도 하지 않는다 - 서버(ServerFire_Implementation)가
	// 어차피 거부하는데 여기서 먼저 애니메이션을 재생하면, 그 사이의 "쐈는데 서버는
	// 무시함" 불일치가 다른 조건들과 똑같이 재현된다.
	if (FPlatformTime::Seconds() - EquippedTimeSeconds < EquipTime)
	{
		return;
	}

	if (CurrentAmmo > 0)
	{
		bAmmoEmptyNotified = false;

		const double Now = FPlatformTime::Seconds();
		const float FireIntervalSeconds = FireRate_RPS > 0.0f ? (1.0f / FireRate_RPS) : 0.0f;

		// 서버가 허용하는 실제 발사 간격보다 빠른 클릭은 애니메이션도 재생하지 않는다 -
		// 안 그러면 Semi에서 연타할 때 애니메이션만 매번 나오고 실제 발사(탄 소모/피격)는
		// 서버 쪽 간격 제한에 걸려 뒤처지는 불일치가 생긴다.
		if (Now >= NextAllowedPredictedFireTimeSeconds)
		{
			// 다음 허용 시각을 "지금"이 아니라 "이전 허용 시각 + 간격"으로 고정한다 -
			// 프레임 지터로 이번 체크가 살짝 늦게 들어와도 오차가 누적되지 않고, 한 번
			// 밀린다고 다음 발까지 통째로 한 텀 더 밀리지 않는다. 다만 오래 안 쐈다가
			// 다시 쏘는 경우(트리거 첫 입력 포함)까지 밀린 발수만큼 몰아 쏘면 안 되므로
			// Now보다 과거로는 안 잡는다.
			NextAllowedPredictedFireTimeSeconds = FMath::Max(NextAllowedPredictedFireTimeSeconds, Now) + FireIntervalSeconds;

			// 리슨 서버 호스트는 이 오브젝트가 곧 서버 권위 오브젝트이기도 해서 FireShot()에서
			// 이미 UpdateSpread()를 호출한다. 여기서 또 부르면 이중 계산되므로, 권한이 없을 때만
			// (원격 클라이언트일 때만) 크로스헤어 예측용으로 미리 계산한다.
			if (!OwnerPawn->HasAuthority())
			{
				UpdateSpread();

				// 로컬 예측: 서버의 실제 차감(--CurrentAmmo, ServerFire_Implementation)을
				// 기다리지 않고 즉시 반영한다. 안 그러면 마지막 한 발을 쏜 뒤 서버의 "탄
				// 없음" 소식이 리플리케이션으로 돌아오기 전까지, 클라이언트가 여전히
				// CurrentAmmo > 0으로 착각해서 애니메이션을 계속 재생해버린다. 나중에
				// 리플리케이션으로 서버의 진짜 값이 도착하면 그걸로 덮어써지므로 오차는
				// 자동 보정된다.
				--CurrentAmmo;
			}

			if (OwningCharacter)
			{
				OwningCharacter->ReceiveWeaponFire();
			}

			// FullAuto는 클라이언트 자신의 반복 입력(AutoFireTimerHandle)이 매 발마다
			// RequestFire()를 다시 불러서 애니메이션도 자연히 반복되지만, Burst는
			// RequestFire()가 트리거당 한 번뿐이라 나머지 발의 애니메이션을 놓친다.
			// 서버의 FireBurstShot()과 같은 타이밍으로 여기서 따로 재생해준다.
			if (FireMode == TEXT("Burst") && BurstCount > 1)
			{
				if (UWorld* World = GetWorld())
				{
					ClientBurstShotsRemaining = BurstCount - 1;

					const float Interval = BurstShotInterval > 0.0f ? BurstShotInterval : 0.05f;
					World->GetTimerManager().SetTimer(ClientBurstTimerHandle, this, &UMainWeaponComponent::PlayClientBurstShot, Interval, true);
				}
			}
		}
	}
	else if (!bAmmoEmptyNotified)
	{
		bAmmoEmptyNotified = true;

		// 탄약이 떨어진 순간에도 Stop()을 호출해야 한다 - BP 쪽 RecoilComponent의
		// IsLooping이 켜진 채로 남아있으면, 우리가 더 이상 ReceiveWeaponFire()를 안
		// 불러도 Tick 기반 반동 애니메이션이 스스로 계속 재생된다. 버튼을 뗄 때만
		// 호출하던 ReceiveWeaponFireStopped()를 여기서도 호출해서 IsLooping을 꺼준다.
		if (OwningCharacter)
		{
			OwningCharacter->ReceiveWeaponFireStopped();
		}
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
	if (!HasWeaponEquipped())
	{
		return;
	}

	// 장착 시간이 아직 안 지났으면 재장전 예측도 하지 않는다 - 서버(Server_Reload_Implementation)가
	// 어차피 거부한다.
	if (FPlatformTime::Seconds() - EquippedTimeSeconds < EquipTime)
	{
		return;
	}

	// 재장전을 시작하면 진행 중이던 연사를 확실히 끊는다 - 안 그러면 탄약 0일 때와
	// 같은 이유(IsLooping이 안 꺼짐)로 반동 애니메이션이 재장전 중에도 계속 남아있는다.
	StopFire();

	// 로컬 예측: 서버 응답을 기다리지 않고 즉시 재장전 연출을 시작한다.
	// Server_Reload_Implementation()의 진짜 가드와 맞춰서, 애니메이션만
	// 재생되고 실제로는 거부되는 상황을 줄인다.
	if (CanReload && CurrentAmmo < MagazineSize && !bIsReloading)
	{
		if (AMainCharacter* OwningCharacter = Cast<AMainCharacter>(GetOwner()))
		{
			OwningCharacter->ReceiveReloadStart();
		}
	}

	Server_Reload();
}

void UMainWeaponComponent::StartADS()
{
	if (!HasWeaponEquipped())
	{
		return;
	}

	AMainCharacter* OwningCharacter = Cast<AMainCharacter>(GetOwner());

	// 달리기 중에는 조준할 수 없다.
	if (OwningCharacter && OwningCharacter->IsSprinting())
	{
		return;
	}

	// 장착 시간이 아직 안 지났으면 조준 예측도 하지 않는다 - 서버(Server_SetAiming_Implementation)가
	// 어차피 거부한다.
	if (FPlatformTime::Seconds() - EquippedTimeSeconds < EquipTime)
	{
		return;
	}

	// 로컬 예측: 서버 응답을 기다리지 않고 즉시 조준 연출을 시작할 수 있게 한다.
	bIsAiming = true;
	Server_SetAiming(true);

	if (OwningCharacter)
	{
		OwningCharacter->ReceiveADSChange(true);
	}
}

void UMainWeaponComponent::StopADS()
{
	bIsAiming = false;
	Server_SetAiming(false);

	if (AMainCharacter* OwningCharacter = Cast<AMainCharacter>(GetOwner()))
	{
		OwningCharacter->ReceiveADSChange(false);
	}
}

void UMainWeaponComponent::Server_SetAiming_Implementation(bool bNewAiming)
{
	// 조준을 켜려는 요청이면 달리기 중인지, 장착 시간이 지났는지 확인한다. 조준 해제(false)는 항상 허용.
	if (bNewAiming)
	{
		if (AMainCharacter* OwningCharacter = Cast<AMainCharacter>(GetOwner()))
		{
			if (OwningCharacter->IsSprinting())
			{
				return;
			}
		}

		if (FPlatformTime::Seconds() - EquippedTimeSeconds < EquipTime)
		{
			return;
		}
	}

	bIsAiming = bNewAiming;
}

void UMainWeaponComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceStart, const FVector_NetQuantizeNormal& TraceDirection)
{
	if (CurrentAmmo <= 0)
	{
		return;
	}

	// 재장전 중에는 발사를 거부한다.
	if (bIsReloading)
	{
		return;
	}

	// 달리기 중에는 발사할 수 없다.
	if (AMainCharacter* OwningCharacter = Cast<AMainCharacter>(GetOwner()))
	{
		if (OwningCharacter->IsSprinting())
		{
			return;
		}
	}

	const double Now = FPlatformTime::Seconds();
	const float FireIntervalSeconds = FireRate_RPS > 0.0f ? (1.0f / FireRate_RPS) : 0.0f;

	if (Now < NextAllowedFireTimeSeconds)
	{
		return;
	}

	if (Now - EquippedTimeSeconds < EquipTime)
	{
		return;
	}

	// 다음 허용 시각을 "지금"이 아니라 "이전 허용 시각 + 간격"으로 고정한다 -
	// 이유는 RequestFire()의 같은 패턴 주석 참고.
	NextAllowedFireTimeSeconds = FMath::Max(NextAllowedFireTimeSeconds, Now) + FireIntervalSeconds;
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

void UMainWeaponComponent::PlayClientBurstShot()
{
	UWorld* World = GetWorld();
	if (!World || ClientBurstShotsRemaining <= 0)
	{
		if (World)
		{
			World->GetTimerManager().ClearTimer(ClientBurstTimerHandle);
		}
		return;
	}

	--ClientBurstShotsRemaining;

	if (AMainCharacter* OwningCharacter = Cast<AMainCharacter>(GetOwner()))
	{
		OwningCharacter->ReceiveWeaponFire();
	}

	if (ClientBurstShotsRemaining <= 0)
	{
		World->GetTimerManager().ClearTimer(ClientBurstTimerHandle);
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

	if (FPlatformTime::Seconds() - EquippedTimeSeconds < EquipTime)
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
	bAmmoEmptyNotified = false;
}

void UMainWeaponComponent::OnRep_IsReloading()
{
	if (bIsReloading)
	{
		if (UWorld* World = GetWorld())
		{
			ReloadStartTimeSeconds = World->GetTimeSeconds();
		}

		// 본인 클라이언트는 RequestReload()에서 이미 로컬 예측으로 애니메이션을
		// 시작했으니 여기서 또 부르면 안 된다 - 다른 플레이어의 재장전을 보는
		// 원격 클라이언트를 위한 경로다.
		if (GetOwner() && !GetOwner()->HasLocalNetOwner())
		{
			if (AMainCharacter* OwningCharacter = Cast<AMainCharacter>(GetOwner()))
			{
				OwningCharacter->ReceiveReloadStart();
			}
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

	// 본인은 RequestFire()에서 이미 로컬 예측으로 재생했으니 원격 클라이언트일 때만
	// 재생한다. Single/Burst/FullAuto 전부 FireShot()을 거쳐 여기로 오므로, 실제
	// 발사 횟수와 정확히 맞아떨어진다.
	if (GetOwner() && !GetOwner()->HasLocalNetOwner())
	{
		if (AMainCharacter* OwningCharacter = Cast<AMainCharacter>(GetOwner()))
		{
			OwningCharacter->ReceiveWeaponFire();
		}
	}
}

void UMainWeaponComponent::Server_StopFire_Implementation()
{
	MulticastWeaponFireStopped();
}

void UMainWeaponComponent::MulticastWeaponFireStopped_Implementation()
{
	if (GetOwner() && !GetOwner()->HasLocalNetOwner())
	{
		if (AMainCharacter* OwningCharacter = Cast<AMainCharacter>(GetOwner()))
		{
			OwningCharacter->ReceiveWeaponFireStopped();
		}
	}
}

void UMainWeaponComponent::OnRep_IsAiming()
{
	if (AMainCharacter* OwningCharacter = Cast<AMainCharacter>(GetOwner()))
	{
		if (!GetOwner()->HasLocalNetOwner())
		{
			OwningCharacter->ReceiveADSChange(bIsAiming);
		}
	}
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

	// 재장전/조준 애니메이션은 남에게도 보여야 하므로(OnRep_IsReloading/OnRep_IsAiming의
	// 원격 클라이언트 경로), 탄약 수치와 달리 전체 공개로 리플리케이트한다.
	DOREPLIFETIME(UMainWeaponComponent, bIsReloading);
	DOREPLIFETIME(UMainWeaponComponent, bIsAiming);

	DOREPLIFETIME(UMainWeaponComponent, WeaponIndex);

	// 읽기용 - 트리거 역할은 아래 ReplicationSequence가 담당한다.
	DOREPLIFETIME(UMainWeaponComponent, EquippedSlotIndex);
	DOREPLIFETIME(UMainWeaponComponent, ReplicationSequence);

	DOREPLIFETIME(UMainWeaponComponent, ActiveItemIndex);
}
