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
#include "Items/ItemActor.h"
#include "TimerManager.h"
#include "Engine/SkeletalMesh.h"
#include "Components/SkeletalMeshComponent.h"
#include "Player/MainCharacter.h"
#include "Player/PlayerMovementComponent.h"
#include "Player/MainPlayerState.h"
#include "GAS/GameplayEffects/GE_ManaCost.h"
#include "GAS/MainAttributeSet.h"
#include "GAS/MainGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "GAS/Abilities/MainGameplayAbility.h"

// 프레임/네트워크 지터로 발사 호출이 늦어져도, 그 지연이 이 한도 이하면 다음 발의 허용 시각을
// 뒤로 밀지 않는다. 늦은 호출 시각으로 일정을 다시 고정하면 바로 다음 정상 호출이 "너무 이르다"로
// 잘려서, 빠른 연사일수록 발사가 취소되는 문제가 있었다. 서버 검증과 클라 예측이 같이 쓴다.
static constexpr float MaxFireBacklogSeconds = 0.15f;

// 무기별 "원본 애니메이션에서 실제로 재장전이 끝나는 시점"(초, 고정값). weapons.json처럼
// 밸런스 목적으로 바뀌면 안 되는 값이라 데이터 파일이 아니라 코드에 직접 둔다.
// SG_1/SG_2(ReloadType: Single)은 재장전 방식이 완전히 달라서 이 필드 자체를 안 쓴다 -
// 표에서 빼서 조용히 폴백시키는 대신 0으로 명시해서, 실수로 호출되면 바로 티가 나게 한다.
struct FBaseReloadTime
{
	float Time = 0.0f;
	float TimeEmpty = 0.0f;
};

static const TMap<FName, FBaseReloadTime> BaseReloadTimeTable = {
	{ TEXT("P_1"),   { 2.5f, 3.3f } },
	{ TEXT("P_2"),   { 4.1f, 4.1f } },
	{ TEXT("AR_1"),  { 2.9f, 3.7f } },
	{ TEXT("AR_2"),  { 3.1f, 3.9f } },
	{ TEXT("SR_1"),  { 3.0f, 5.0f } },
	{ TEXT("SMG_1"), { 3.2f, 3.6f } },
	{ TEXT("SMG_2"), { 3.2f, 3.6f } },
	{ TEXT("SG_1"),  { 0.0f, 0.0f } },
	{ TEXT("SG_2"),  { 0.0f, 0.0f } },
	{ TEXT("DMR_1"), { 3.2f, 4.5f } },
	{ TEXT("MG_1"),  { 6.8f, 8.0f } },
};

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
			// AItemActor 파생 액터는 자기 ItemVisualData에서 부착 소켓/오프셋을 직접
			// 정의할 수 있다 - 무기 액터(AItemActor가 아님)는 이 분기를 안 타서
			// 기존 하드코딩 값("VB ik_hand_gun_pivot", 오프셋 0) 그대로 유지된다.
			FName SocketName = TEXT("VB ik_hand_gun_pivot");
			FVector OffsetLocation = FVector::ZeroVector;
			FRotator OffsetRotation = FRotator::ZeroRotator;
			if (const AItemActor* ItemActor = Cast<AItemActor>(NewActor))
			{
				SocketName = ItemActor->GetAttachSocketName();
				OffsetLocation = ItemActor->GetAttachOffsetLocation();
				OffsetRotation = ItemActor->GetAttachOffsetRotation();
			}

			if (USceneComponent* AttachTarget = WeaponMeshComponent->GetAttachParent())
			{
				// 스케일은 소켓 것을 따라가지 않게 한다 - IncludingScale로 부착하면
				// BP에서 설정한 아이템 메시 스케일이 매번 부착 시점에 소켓 스케일로
				// 덮어써져서 무시된다.
				NewActor->AttachToComponent(AttachTarget, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
				NewActor->AddActorLocalOffset(OffsetLocation);
				NewActor->AddActorLocalRotation(OffsetRotation);
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

	// 무기(또는 아이템)가 바뀌면 조준은 항상 풀린다. ApplyWeaponStats()가 ADSSpeed를 바꾸기 전에
	// 해제해야 ADSAlphaAtChange가 이전 무기 기준으로 계산된다.
	// 이 함수는 서버·오너 클라이언트·원격 클라이언트 모두에서 돌기 때문에 실행 주체별로 나눈다.
	//  - 서버/오너: SetAiming(false)를 직접 한다(서버 값은 복제된다).
	//  - 원격 클라이언트: 로컬로 바꾸지 않는다 - 서버 복제가 OnRep_IsAiming을 발동시켜야
	//    ReceiveADSChange(false) 연출이 나온다(값이 이미 같으면 OnRep이 안 돈다).
	//  - 오너: OnRep_IsAiming이 ReceiveADSChange를 부르지 않으므로 직접 호출한다.
	if (bIsAiming)
	{
		AMainCharacter* OwningCharacter = Cast<AMainCharacter>(GetOwner());
		const bool bLocallyControlled = OwningCharacter && OwningCharacter->IsLocallyControlled();

		if (GetOwner()->HasAuthority() || bLocallyControlled)
		{
			SetAiming(false);
		}
		if (bLocallyControlled)
		{
			if (UPlayerMovementComponent* Movement = OwningCharacter->GetPlayerMovement())
			{
				Movement->SetWantsToAim(false);
			}
			OwningCharacter->ReceiveADSChange(false);
		}
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BurstTimerHandle);
		World->GetTimerManager().ClearTimer(ClientBurstTimerHandle);
		World->GetTimerManager().ClearTimer(ReloadTimerHandle);
		World->GetTimerManager().ClearTimer(ReloadAmmoUpTimerHandle);
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
	ReloadTime_Empty = Stats.ReloadTime_Empty;
	ReloadTime_EmptyAmmoUp = Stats.ReloadTime_EmptyAmmoUp;
	ReloadType = Stats.ReloadType;
	ReloadTime_Start = Stats.ReloadTime_Start;
	ReloadTime_Loop = Stats.ReloadTime_Loop;
	ReloadTime_LoopAmmoUp = Stats.ReloadTime_LoopAmmoUp;
	ReloadTime_End = Stats.ReloadTime_End;
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
	// 무기와 아이템은 항상 배타적이라, 지금 뭐가 진짜 활성 상태인지(WeaponIndex)만
	// 보고 그쪽 함수 하나만 재실행하면 된다 - 두 트리거가 따로 있을 때는 어느 게
	// 먼저 도착하느냐에 따라 방금 스폰된 걸 반대쪽이 지워버리는 레이스가 있었는데,
	// 트리거를 하나로 합치면 그 레이스 자체가 없어진다(재실행이 항상 한 번만,
	// 항상 최종 상태 기준으로 일어나므로).
	if (HasWeaponEquipped())
	{
		EquipWeapon(WeaponIndex, -2, EquippedSlotIndex);
	}
	else
	{
		EquipItemVisual(ActiveItemIndex);
	}
}

void UMainWeaponComponent::EquipItemVisual(FName ItemIndex)
{
	// 원격 클라이언트에게 "장착이 다시 일어났다"는 걸 반드시 알리기 위한 트리거 -
	// EquipWeapon()과 공유하는 ReplicationSequence 선언부 주석 참고.
	++ReplicationSequence;

	ActiveItemIndex = ItemIndex;

	// 무기가 아닌 아이템(맨손 포함)을 들면 이동 배율은 항상 1.0이다. 서버는 UnequipWeapon()이 스탯을 리셋하지만
	// 클라이언트는 이 함수(EquipItemVisual)만 타서 옛 무기의 배율이 남으므로, 여기서 직접 맞춘다.
	MoveSpeedMultiplier = 1.0f;
	ADSMoveSpeedMultiplier = 1.0f;

	// EquipWeapon()은 이 시각을 기록하는데 여기는 빠져있었다 - 그래서 아이템 장착 시
	// IsStillEquipping()이 "마지막으로 무기를 장착했던 시각" 같은 엉뚱한 기준으로
	// 판단해버려 장착 가드가 무력화되는 버그가 있었다. 무기든 아이템이든 "언제
	// 장착했는지"는 똑같이 이 시각 하나로 관리해야 한다.
	EquippedTimeSeconds = FPlatformTime::Seconds();

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
		// EquipWeapon()은 이 값을 채우는데 여기는 빠져있었다 - 그래서 아이템 장착 시
		// WeaponComponent::EquipTime이 갱신 안 되고 마지막 무기 값(또는 0)에 머물러
		// 있었다. IsStillEquipping()이 이 값을 참조하는데, 갱신이 안 되니 장착 가드가
		// 무력화되는 버그가 있었다.
		EquipTime = ItemData.EquipTime;
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

void UMainWeaponComponent::GrantItemSkillAbility(TSubclassOf<UGameplayAbility> SkillClass, EMainAbilityInputID InputID)
{
	FGameplayAbilitySpecHandle& TargetHandle = (InputID == EMainAbilityInputID::Primary)
		? GrantedPrimarySkillHandle
		: GrantedSecondarySkillHandle;

	AMainCharacter* OwnerCharacter = Cast<AMainCharacter>(GetOwner());
	AMainPlayerState* MainPS = OwnerCharacter ? OwnerCharacter->GetPlayerState<AMainPlayerState>() : nullptr;
	UAbilitySystemComponent* ASC = MainPS ? MainPS->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		return;
	}

	if (TargetHandle.IsValid())
	{
		ASC->ClearAbility(TargetHandle);
		TargetHandle = FGameplayAbilitySpecHandle();
	}

	if (!SkillClass)
	{
		// 이 슬롯에 스킬이 없는 아이템 - 기존 것만 회수하고 끝.
		return;
	}

	// 쿨다운/마나 값은 더 이상 여기서 주입하지 않는다 - 어빌리티가 ActiveItemIndex를 보고
	// 스스로 조회한다(EquipTime/ReloadTime과 같은 패턴). 서버 인스턴스에만 값이 꽂히고
	// 클라이언트 예측 인스턴스는 못 받는 문제(Finding #1)가 이 방식 자체를 없애서 해결된다.
	TargetHandle = ASC->GiveAbility(FGameplayAbilitySpec(SkillClass, 1, static_cast<int32>(InputID), this));
}

void UMainWeaponComponent::RevokeItemSkillAbilities()
{
	AMainCharacter* OwnerCharacter = Cast<AMainCharacter>(GetOwner());
	AMainPlayerState* MainPS = OwnerCharacter ? OwnerCharacter->GetPlayerState<AMainPlayerState>() : nullptr;
	UAbilitySystemComponent* ASC = MainPS ? MainPS->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		return;
	}

	if (GrantedPrimarySkillHandle.IsValid())
	{
		ASC->ClearAbility(GrantedPrimarySkillHandle);
		GrantedPrimarySkillHandle = FGameplayAbilitySpecHandle();
	}
	if (GrantedSecondarySkillHandle.IsValid())
	{
		ASC->ClearAbility(GrantedSecondarySkillHandle);
		GrantedSecondarySkillHandle = FGameplayAbilitySpecHandle();
	}
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
	if (FireMode == TEXT("FullAuto"))
	{
		if (UWorld* World = GetWorld())
		{
			const float FireIntervalSeconds = FireRate_RPS > 0.0f ? (1.0f / FireRate_RPS) : 0.0f;
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
	bool bJustCancelledSingleReload = false;
	if (bIsReloading)
	{
		// Single 타입은 ReloadStart 이후 언제든 발사가 재장전을 취소할 수 있다 - 로컬 예측
		// 타이머 체인만 끊고(EndSingleReload 없이) 아래로 흘러가 정상 발사 로직을 그대로 탄다.
		if (ReloadType == TEXT("Single"))
		{
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().ClearTimer(ClientSingleReloadTimerHandle);
			}
			bIsReloading = false;
			bJustCancelledSingleReload = true; // 탄약이 아직 0이어도 서버에 요청을 보내야 서버도 취소한다.

			// 재장전 애니메이션(특히 Loop)이 중간에 기울어진 자세로 끝나도록 만들어져 있어서,
			// 그냥 끊으면 그 자세로 고정된다 - ReloadEnd를 재생해서 정자세로 풀어준다.
			if (AMainCharacter* OwningCharacter = Cast<AMainCharacter>(GetOwner()))
			{
				OwningCharacter->ReceiveReloadEnd();
			}
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("[ProjectRWW][발사취소] 클라: 재장전 중이라 발사 안 함 (%s)"), *GetNameSafe(GetOwner()));
			return;
		}
	}

	AMainCharacter* OwningCharacter = Cast<AMainCharacter>(GetOwner());

	// 달리기 중에는 발사할 수 없다.
	if (OwningCharacter && OwningCharacter->IsSprinting())
	{
		UE_LOG(LogTemp, Log, TEXT("[ProjectRWW][발사취소] 클라: 달리기 중 (%s)"), *GetNameSafe(GetOwner()));
		return;
	}

	// 장착 시간이 아직 안 지났으면 예측도 하지 않는다 - 서버(ServerFire_Implementation)가
	// 어차피 거부하는데 여기서 먼저 애니메이션을 재생하면, 그 사이의 "쐈는데 서버는
	// 무시함" 불일치가 다른 조건들과 똑같이 재현된다.
	if (FPlatformTime::Seconds() - EquippedTimeSeconds < EquipTime)
	{
		UE_LOG(LogTemp, Log, TEXT("[ProjectRWW][발사취소] 클라: 장착 직후 (%s)"), *GetNameSafe(GetOwner()));
		return;
	}

	if (CurrentAmmo > 0)
	{
		bAmmoEmptyNotified = false;

		const double Now = FPlatformTime::Seconds();
		const float FireIntervalSeconds = FireRate_RPS > 0.0f ? (1.0f / FireRate_RPS) : 0.0f;

		// 서버가 허용하는 실제 발사 간격보다 빠른 클릭은 애니메이션도 재생하지 않는다 -
		// 안 그러면 Semi에서 연타할 때 애니메이션만 매번 나오고 실제 발사(탄 소모/피격)는
		// 서버 쪽 간격 제한에 걸려 뒤처지는 불일치가 생긴다. 다만 이건 순전히 로컬
		// 예측(애니메이션)용 값이라 서버 판정과는 무관하므로, 프레임 지터로 인한
		// 오탐 스킵을 줄이려고 여기도 서버와 같은 비율만큼 살짝 일찍 허용한다.
		constexpr float ClientFireRateToleranceRatio = 0.1f;
		if (Now >= NextAllowedPredictedFireTimeSeconds - FireIntervalSeconds * ClientFireRateToleranceRatio)
		{
			// 다음 허용 시각을 "지금"이 아니라 "이전 허용 시각 + 간격"으로 고정한다 -
			// 프레임 지터로 이번 체크가 살짝 늦게 들어와도 오차가 누적되지 않고, 한 번
			// 밀린다고 다음 발까지 통째로 한 텀 더 밀리지 않는다. 다만 오래 안 쐈다가
			// 다시 쏘는 경우(트리거 첫 입력 포함)까지 밀린 발수만큼 몰아 쏘면 안 되므로
			// Now보다 과거로는 안 잡는다.
			NextAllowedPredictedFireTimeSeconds = FMath::Max(NextAllowedPredictedFireTimeSeconds, Now - MaxFireBacklogSeconds) + FireIntervalSeconds;

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
		else
		{
			// 예측(애니메이션)만 생략하고 아래 ServerFire는 그대로 보내므로, 서버 로그의
			// "연사 간격 위반"과 짝을 맞춰 볼 수 있다.
			UE_LOG(LogTemp, Log, TEXT("[ProjectRWW][발사취소] 클라: 예측 스킵(서버 요청은 전송) (%s) 부족한 시간=%.1fms"),
				*GetNameSafe(GetOwner()), (NextAllowedPredictedFireTimeSeconds - Now) * 1000.0);
		}

		// 탄약이 있을 때만 서버에 발사 요청을 보낸다 - 어차피 서버(ServerFire_Implementation)가
		// CurrentAmmo<=0이면 거부하지만, 그건 RPC가 이미 나간 뒤라 대역폭 낭비다. FullAuto로
		// 빈 탄창에서 방아쇠를 계속 누르고 있으면 이 낭비가 매 간격마다 반복된다.
		FVector ViewLocation;
		FRotator ViewRotation;
		OwnerPawn->GetController()->GetPlayerViewPoint(ViewLocation, ViewRotation);

		ServerFire(ViewLocation, ViewRotation.Vector());
	}
	else if (bJustCancelledSingleReload)
	{
		// 실제로 쏠 탄약은 없지만, 서버가 자기 쪽 Single 재장전 상태 머신을 취소할 수
		// 있게 요청은 그대로 보낸다 - 안 보내면 클라만 끊기고 서버는 계속 재장전을 진행한다.
		FVector ViewLocation;
		FRotator ViewRotation;
		OwnerPawn->GetController()->GetPlayerViewPoint(ViewLocation, ViewRotation);

		ServerFire(ViewLocation, ViewRotation.Vector());
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
}

float UMainWeaponComponent::UpdateSpread()
{
	const UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;

	const float ADSAlpha = GetADSAlpha();
	const float BaseSpread = FMath::Lerp(SpreadHipfire, SpreadADS, ADSAlpha);
	const float MaxBloom = FMath::Lerp(MaxSpreadBloomHipfire, MaxSpreadBloomADS, ADSAlpha);

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
	const float ADSAlpha = GetADSAlpha();
	const float BaseSpread = FMath::Lerp(SpreadHipfire, SpreadADS, ADSAlpha);
	const float MaxBloom = FMath::Lerp(MaxSpreadBloomHipfire, MaxSpreadBloomADS, ADSAlpha);

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
	const float ADSAlpha = GetADSAlpha();
	const float BaseSpread = FMath::Lerp(SpreadHipfire, SpreadADS, ADSAlpha);
	const float MaxBloom = FMath::Lerp(MaxSpreadBloomHipfire, MaxSpreadBloomADS, ADSAlpha);
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

	// 재장전 중엔 조준 상태일 수 없다 - 이미 조준 중이었다면 풀어준다.
	if (bIsAiming)
	{
		StopADS();
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

		// Single 타입은 서버와 같은 타이밍으로 로컬 예측 재생 체인을 시작한다.
		if (ReloadType == TEXT("Single"))
		{
			// 서버 BeginLoopStep()의 Empty Start 1발 선반영과 예측 횟수를 맞춘다.
			int32 EffectiveStartAmmo = CurrentAmmo;
			if (EffectiveStartAmmo == 0 && ReloadTime_EmptyAmmoUp >= 0.0f)
			{
				++EffectiveStartAmmo;
			}
			ReloadAmmoRemaining = MagazineSize - EffectiveStartAmmo;
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().SetTimer(ClientSingleReloadTimerHandle, this, &UMainWeaponComponent::PlayClientReloadLoopStep, GetReloadStartTime(), false);
			}
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

	// 재장전 중에는 조준할 수 없다.
	if (bIsReloading)
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
	SetAiming(true);
	Server_SetAiming(true);

	if (OwningCharacter)
	{
		// 이동 속도 계산에 쓰이는 조준 의도. StartADS의 모든 조기 반환(재장전/스프린트/장착 시간)을 통과했을 때만 켜진다.
		if (UPlayerMovementComponent* Movement = OwningCharacter->GetPlayerMovement())
		{
			Movement->SetWantsToAim(true);
		}
		OwningCharacter->ReceiveADSChange(true);
	}
}

void UMainWeaponComponent::StopADS()
{
	SetAiming(false);
	Server_SetAiming(false);

	if (AMainCharacter* OwningCharacter = Cast<AMainCharacter>(GetOwner()))
	{
		if (UPlayerMovementComponent* Movement = OwningCharacter->GetPlayerMovement())
		{
			Movement->SetWantsToAim(false);
		}
		OwningCharacter->ReceiveADSChange(false);
	}
}

void UMainWeaponComponent::Server_SetAiming_Implementation(bool bNewAiming)
{
	// 조준을 켜려는 요청이면 달리기 중인지, 장착 시간이 지났는지 확인한다. 조준 해제(false)는 항상 허용.
	if (bNewAiming)
	{
		if (bIsReloading)
		{
			return;
		}

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

	SetAiming(bNewAiming);
}

void UMainWeaponComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceStart, const FVector_NetQuantizeNormal& TraceDirection)
{
	// [발사취소] 로그는 발사가 어디서 잘리는지 추적하는 진단용이다. 원인이 확정되면 제거한다.

	// 재장전 중에는 발사를 거부한다 - 단, Single 타입은 재장전 취소가 먼저다. 탄약이
	// 아직 0(Empty Start 도중)이어도 취소 자체는 일어나야 하므로 탄약 체크보다 앞에 둔다.
	if (bIsReloading)
	{
		// 클라이언트 RequestFire()와 같은 정책 - Single 타입만 발사로 재장전을 취소할 수 있다.
		// 지금까지 채운 CurrentAmmo는 그대로 두고 서버 권위 타이머만 끊는다.
		if (ReloadType == TEXT("Single"))
		{
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().ClearTimer(ReloadTimerHandle);
				World->GetTimerManager().ClearTimer(ReloadAmmoUpTimerHandle);
			}
			bIsReloading = false;
			bAmmoEmptyNotified = false;
			MulticastReloadEnd(); // 원격 클라이언트도 ReloadEnd로 기울어진 자세를 풀어준다.
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("[ProjectRWW][발사취소] 서버: 재장전 중 (%s)"), *GetNameSafe(GetOwner()));
			return;
		}
	}

	if (CurrentAmmo <= 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[ProjectRWW][발사취소] 서버: 탄약 없음 (%s)"), *GetNameSafe(GetOwner()));
		return;
	}

	// 달리기 중에는 발사할 수 없다.
	if (AMainCharacter* OwningCharacter = Cast<AMainCharacter>(GetOwner()))
	{
		if (OwningCharacter->IsSprinting())
		{
			UE_LOG(LogTemp, Log, TEXT("[ProjectRWW][발사취소] 서버: 달리기 중 (%s)"), *GetNameSafe(GetOwner()));
			return;
		}
	}

	const double Now = FPlatformTime::Seconds();
	const float FireIntervalSeconds = FireRate_RPS > 0.0f ? (1.0f / FireRate_RPS) : 0.0f;

	// 네트워크 지연/지터로 클라이언트의 발사 요청이 서버에 도착하는 시각이 몇 ms
	// 늦어질(또는 일찍 도착할) 수 있다. 검사할 때만 간격의 10%만큼 살짝 일찍
	// 허용해서 지터로 인한 오탐 거부를 줄인다 - 다음 허용 시각(NextAllowedFireTimeSeconds)은
	// 항상 정확한 FireIntervalSeconds로 갱신되므로, 이 여유가 연사 속도 자체를
	// 앞당기지는 않는다.
	constexpr float ServerFireRateToleranceRatio = 0.1f;
	if (Now < NextAllowedFireTimeSeconds - FireIntervalSeconds * ServerFireRateToleranceRatio)
	{
		// "너무 이른 정도"가 허용오차를 넘은 만큼이 도착 지터/뭉침의 크기다.
		UE_LOG(LogTemp, Log, TEXT("[ProjectRWW][발사취소] 서버: 연사 간격 위반 (%s) 너무 이른 정도=%.1fms, 간격=%.1fms, 허용오차=%.1fms"),
			*GetNameSafe(GetOwner()),
			(NextAllowedFireTimeSeconds - Now) * 1000.0,
			FireIntervalSeconds * 1000.0,
			FireIntervalSeconds * ServerFireRateToleranceRatio * 1000.0);
		return;
	}

	if (Now - EquippedTimeSeconds < EquipTime)
	{
		UE_LOG(LogTemp, Log, TEXT("[ProjectRWW][발사취소] 서버: 장착 직후 (%s)"), *GetNameSafe(GetOwner()));
		return;
	}

	// 다음 허용 시각을 "지금"이 아니라 "이전 허용 시각 + 간격"으로 고정한다 -
	// 이유는 RequestFire()의 같은 패턴 주석 참고.
	NextAllowedFireTimeSeconds = FMath::Max(NextAllowedFireTimeSeconds, Now - MaxFireBacklogSeconds) + FireIntervalSeconds;
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

	AMainCharacter* OwnerCharacter = Cast<AMainCharacter>(GetOwner());
	AMainPlayerState* MainPS = OwnerCharacter ? OwnerCharacter->GetPlayerState<AMainPlayerState>() : nullptr;
	UAbilitySystemComponent* ASC = MainPS ? MainPS->GetAbilitySystemComponent() : nullptr;
	UMainAttributeSet* AttrSet = MainPS ? MainPS->GetMainAttributeSet() : nullptr;

	if (!ASC || !AttrSet || AttrSet->GetMana() < RequiredMana)
	{
		return;
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	FGameplayEffectSpecHandle CostSpec = ASC->MakeOutgoingSpec(UGE_ManaCost::StaticClass(), 1.0f, Context);
	CostSpec.Data->SetSetByCallerMagnitude(MainGameplayTags::Data_ManaCost.GetTag(), -RequiredMana);
	ASC->ApplyGameplayEffectSpecToSelf(*CostSpec.Data);

	bIsReloading = true;
	SetAiming(false); // 재장전 중엔 조준 상태일 수 없다 - 서버 권위로 강제 해제
	                  // (리플리케이트되어 OnRep_IsAiming이 원격 클라이언트 조준 해제 연출도 처리한다).
	ReloadStartTimeSeconds = GetWorld()->GetTimeSeconds();

	if (ReloadType == TEXT("Single"))
	{
		int32 EffectiveStartAmmo = CurrentAmmo;
		if (EffectiveStartAmmo == 0)
		{
			++EffectiveStartAmmo;
		}
		ReloadTotalDuration = GetReloadStartTime() + (MagazineSize - EffectiveStartAmmo) * ReloadTime_Loop;
	}
	else
	{
		ReloadTotalDuration = GetEffectiveReloadTime();
	}

	if (UWorld* World = GetWorld())
	{
		// 진행 중이던 버스트의 남은 발을 여기서 확실히 끊는다 - 안 그러면
		// bIsReloading이 true가 된 뒤에도 FireBurstShot()이 이 값을 확인하지
		// 않아서 서버가 남은 발을 계속 실제로 쏴버린다(대미지/탄약 소모 포함).
		World->GetTimerManager().ClearTimer(BurstTimerHandle);
		PendingBurstShotsRemaining = 0;

		// 탄창이 완전히 빈 상태였다면(재장전 시작 시점 기준) 더 긴 ReloadTime_Empty를 쓴다 -
		// BP_TacticalShooterWeapon::OnReload의 GetEffectiveReloadTime() 호출과 반드시 같은
		// 값을 써야 애니메이션 도중에 탄창이 채워지는 어긋남이 생기지 않는다(로직은
		// GetEffectiveReloadTime()에 한 곳으로 모아 중복을 없앴다).
		if (ReloadType == TEXT("Single"))
		{
			StartSingleReload();
		}
		else
		{
			World->GetTimerManager().SetTimer(ReloadTimerHandle, this, &UMainWeaponComponent::CompleteReload, GetEffectiveReloadTime(), false);
		}
	}
}

void UMainWeaponComponent::CompleteReload()
{
	bIsReloading = false;
	CurrentAmmo = MagazineSize;
	bAmmoEmptyNotified = false;
}

void UMainWeaponComponent::StartSingleReload()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(ReloadTimerHandle, this, &UMainWeaponComponent::BeginLoopStep, GetReloadStartTime(), false);

		// Empty Start가 탄약을 선반영하는 무기만(ReloadTime_EmptyAmmoUp >= 0) Start 애니메이션
		// 도중 별도 타이밍에 첫 발을 채운다. -1이면 이 무기는 Empty Start에 장전 동작이 없다는
		// 뜻이라 아무것도 안 하고, 첫 발은 그냥 첫 Loop에서 채워진다.
		if (CurrentAmmo == 0 && ReloadTime_EmptyAmmoUp >= 0.0f)
		{
			World->GetTimerManager().SetTimer(ReloadAmmoUpTimerHandle, this, &UMainWeaponComponent::IncrementReloadAmmo, ReloadTime_EmptyAmmoUp, false);
		}
	}
}

void UMainWeaponComponent::BeginLoopStep()
{
	// 발사로 이미 취소됐는데 타이머가 남아있던 경우의 방어 코드.
	if (!bIsReloading)
	{
		return;
	}

	MulticastReloadLoop(); // 원격 클라이언트에게 지금 이 루프 애니메이션을 재생하라고 알린다.

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(ReloadAmmoUpTimerHandle, this, &UMainWeaponComponent::IncrementReloadAmmo, ReloadTime_LoopAmmoUp, false);
		World->GetTimerManager().SetTimer(ReloadTimerHandle, this, &UMainWeaponComponent::EndLoopStep, ReloadTime_Loop, false);
	}
}

void UMainWeaponComponent::EndLoopStep()
{
	if (!bIsReloading)
	{
		return;
	}

	if (CurrentAmmo >= MagazineSize)
	{
		// End는 Loop와 달리 여기서 더 대기할 이유가 없다 - 마지막 탄이 약실에 들어간
		// 순간 발사 가능해야 하고(Loop처럼 발사로 끊겨도 되는 후속 연출), End 애니메이션도
		// 그 즉시 재생돼야 한다(대기 후 재생하면 그 사이 아무 것도 안 보이는 공백이 생김).
		EndSingleReload();
	}
	else
	{
		// 다음 루프 애니메이션을 즉시 시작한다 - 여기서 또 ReloadTime_Loop초를 기다리면
		// 한 발당 ReloadTime_Loop가 두 번(이 대기 + BeginLoopStep이 여는 대기) 들어가서
		// 실제 재장전 속도가 의도한 것의 절반으로 느려진다.
		BeginLoopStep();
	}
}

void UMainWeaponComponent::IncrementReloadAmmo()
{
	if (!bIsReloading)
	{
		return;
	}

	++CurrentAmmo;
}

void UMainWeaponComponent::MulticastReloadLoop_Implementation()
{
	if (GetOwner() && !GetOwner()->HasLocalNetOwner())
	{
		if (AMainCharacter* OwningCharacter = Cast<AMainCharacter>(GetOwner()))
		{
			OwningCharacter->ReceiveReloadLoop();
		}
	}
}

void UMainWeaponComponent::EndSingleReload()
{
	bIsReloading = false;
	bAmmoEmptyNotified = false;
	MulticastReloadEnd();
}

void UMainWeaponComponent::PlayClientReloadLoopStep()
{
	if (!bIsReloading)
	{
		return;
	}

	if (AMainCharacter* OwningCharacter = Cast<AMainCharacter>(GetOwner()))
	{
		OwningCharacter->ReceiveReloadLoop();
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	--ReloadAmmoRemaining;
	if (ReloadAmmoRemaining > 0)
	{
		World->GetTimerManager().SetTimer(ClientSingleReloadTimerHandle, this, &UMainWeaponComponent::PlayClientReloadLoopStep, ReloadTime_Loop, false);
	}
	else
	{
		// 마지막 루프 애니메이션이 다 재생될 때까지(ReloadTime_Loop) 기다린 뒤 End를 즉시
		// 재생한다 - 서버(EndLoopStep -> EndSingleReload 즉시 호출)와 같은 타이밍.
		World->GetTimerManager().SetTimer(ClientSingleReloadTimerHandle, this, &UMainWeaponComponent::PlayClientReloadEndStep, ReloadTime_Loop, false);
	}
}

void UMainWeaponComponent::PlayClientReloadEndStep()
{
	if (AMainCharacter* OwningCharacter = Cast<AMainCharacter>(GetOwner()))
	{
		OwningCharacter->ReceiveReloadEnd();
	}
}

void UMainWeaponComponent::MulticastReloadEnd_Implementation()
{
	if (GetOwner() && !GetOwner()->HasLocalNetOwner())
	{
		if (AMainCharacter* OwningCharacter = Cast<AMainCharacter>(GetOwner()))
		{
			OwningCharacter->ReceiveReloadEnd();
		}
	}
}

void UMainWeaponComponent::OnRep_IsReloading()
{
	if (bIsReloading)
	{
		if (UWorld* World = GetWorld())
		{
			ReloadStartTimeSeconds = World->GetTimeSeconds();
		}

		if (ReloadType == TEXT("Single"))
		{
			int32 EffectiveStartAmmo = CurrentAmmo;
			if (EffectiveStartAmmo == 0)
			{
				++EffectiveStartAmmo;
			}
			ReloadTotalDuration = GetReloadStartTime() + (MagazineSize - EffectiveStartAmmo) * ReloadTime_Loop;
		}
		else
		{
			ReloadTotalDuration = GetEffectiveReloadTime();
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
	if (!bIsReloading || ReloadTotalDuration <= 0.0f)
	{
		return 0.0f;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.0f;
	}

	const float ElapsedTime = World->GetTimeSeconds() - ReloadStartTimeSeconds;
	return FMath::Clamp(ElapsedTime / ReloadTotalDuration, 0.0f, 1.0f);
}

float UMainWeaponComponent::GetBaseReloadTime() const
{
	if (const FBaseReloadTime* Found = BaseReloadTimeTable.Find(WeaponIndex))
	{
		return Found->Time;
	}
	return ReloadTime; // 표에 없는 무기는 비율 1.0(원래 속도)으로 폴백
}

float UMainWeaponComponent::GetBaseReloadTime_Empty() const
{
	if (const FBaseReloadTime* Found = BaseReloadTimeTable.Find(WeaponIndex))
	{
		return Found->TimeEmpty;
	}
	return ReloadTime_Empty;
}

float UMainWeaponComponent::GetEffectiveReloadTime() const
{
	return (CurrentAmmo == 0 && ReloadTime_Empty > 0.0f) ? ReloadTime_Empty : ReloadTime;
}

float UMainWeaponComponent::GetReloadPlayRate() const
{
	const float BaseTime = (CurrentAmmo == 0 && ReloadTime_Empty > 0.0f) ? GetBaseReloadTime_Empty() : GetBaseReloadTime();
	return BaseTime / FMath::Max(GetEffectiveReloadTime(), 0.01f);
}

float UMainWeaponComponent::GetReloadStartTime() const
{
	return (CurrentAmmo == 0 && ReloadTime_Empty > 0.0f) ? ReloadTime_Empty : ReloadTime_Start;
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
	// 복제로 이미 새 값이 덮어써진 상태라, 이전 상태 기준으로 전환 시점의 진행도를 복원한다.
	ADSAlphaAtChange = CalcADSAlpha(!bIsAiming);
	if (const UWorld* World = GetWorld())
	{
		ADSChangeTimeSeconds = World->GetTimeSeconds();
	}

	if (AMainCharacter* OwningCharacter = Cast<AMainCharacter>(GetOwner()))
	{
		if (!GetOwner()->HasLocalNetOwner())
		{
			OwningCharacter->ReceiveADSChange(bIsAiming);
		}
		else if (!bIsAiming)
		{
			// 서버가 강제로 조준을 풀었을 때(재장전 시작 등) 오너도 의도 플래그를 꺼야 다음 이동 패킷이 "조준 아님"을 싣는다.
			if (UPlayerMovementComponent* Movement = OwningCharacter->GetPlayerMovement())
			{
				Movement->SetWantsToAim(false);
			}
		}
	}
}

float UMainWeaponComponent::CalcADSAlpha(bool bAimingState) const
{
	const float Target = bAimingState ? 1.0f : 0.0f;
	const UWorld* World = GetWorld();
	if (!World || ADSSpeed <= 0.0f)
	{
		return Target; // 시간이 0이면 즉시 전환
	}

	const float Delta = static_cast<float>(World->GetTimeSeconds() - ADSChangeTimeSeconds) / ADSSpeed;
	return bAimingState
		? FMath::Min(1.0f, ADSAlphaAtChange + Delta)
		: FMath::Max(0.0f, ADSAlphaAtChange - Delta);
}

float UMainWeaponComponent::GetADSAlpha() const
{
	return CalcADSAlpha(bIsAiming);
}

void UMainWeaponComponent::SetAiming(bool bNewAiming)
{
	if (bIsAiming == bNewAiming)
	{
		return;
	}

	ADSAlphaAtChange = GetADSAlpha(); // 값이 바뀌기 전에 계산해야 한다
	if (const UWorld* World = GetWorld())
	{
		ADSChangeTimeSeconds = World->GetTimeSeconds();
	}
	bIsAiming = bNewAiming;
}

void UMainWeaponComponent::OnRep_CurrentAmmo()
{
	// TODO: 탄약 UI 갱신 등, 클라이언트 반응 로직이 필요해지면 여기에 추가
	// (Single 타입 ReloadLoop 애니메이션 신호는 이제 MulticastReloadLoop()가 전담한다 -
	// 탄약은 그 루프가 끝난 뒤에야 바뀌므로 더 이상 여기서 트리거하면 안 된다.)
}

void UMainWeaponComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UMainWeaponComponent, CurrentAmmo);
	DOREPLIFETIME(UMainWeaponComponent, MagazineSize);

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
