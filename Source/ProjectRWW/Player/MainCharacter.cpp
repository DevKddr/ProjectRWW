// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Combat/MainWeaponComponent.h"
#include "Core/MainGameMode.h"
#include "Net/UnrealNetwork.h"
#include "Player/MainPlayerState.h"
#include "GAS/MainAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GAS/GameplayEffects/GE_Damage.h"
#include "GAS/GameplayEffects/GE_DamagedTag.h"
#include "GAS/MainGameplayTags.h"
#include "PlayerBaseStat/PlayerStatManager.h"

AMainCharacter::AMainCharacter()
{
	// ACharacter는 기본적으로 bReplicates = true이지만, 서버 권위 캐릭터임을 명시적으로 표시한다.
	bReplicates = true;
	GetCharacterMovement()->SetIsReplicated(true);

	WeaponComponent = CreateDefaultSubobject<UMainWeaponComponent>(TEXT("WeaponComponent"));
}

void AMainCharacter::OnFire(const FInputActionValue& Value)
{
	if (WeaponComponent)
	{
		WeaponComponent->StartFire();
	}
}

void AMainCharacter::OnStopFire(const FInputActionValue& Value)
{
	if (WeaponComponent)
	{
		// bCancelBurst=false: 트리거를 놓았다는 사실(OnFireReleased 등) 자체는 발사모드와
		// 무관하게 항상 Kinemation에 알려야 하지만, 진행 중인 버스트의 남은 발
		// 애니메이션(ClientBurstTimerHandle)은 여기서 끊지 않는다 - Burst는 트리거를
		// 일찍 떼도 이미 시작된 발이 끝까지 나가야 하는데(총기 자체가 그렇게 동작함),
		// 서버(FireBurstShot의 BurstTimerHandle)는 이 함수와 무관하게 계속 돌아 실제
		// 발사가 그대로 이어지는 반면 본인 화면만 여기서 끊기면 "남에게는 버스트가
		// 끝까지 보이는데 본인 화면만 끊긴다"는 불일치가 생긴다.
		WeaponComponent->StopFire(false);
		WeaponComponent->Server_StopFire();
	}
}

void AMainCharacter::OnReload(const FInputActionValue& Value)
{
	if (WeaponComponent)
	{
		WeaponComponent->RequestReload();
	}
}

void AMainCharacter::OnADSStart(const FInputActionValue& Value)
{
	if (WeaponComponent)
	{
		WeaponComponent->StartADS();
	}
}

void AMainCharacter::OnADSStop(const FInputActionValue& Value)
{
	if (WeaponComponent)
	{
		WeaponComponent->StopADS();
	}
}

void AMainCharacter::OnSprintStart(const FInputActionValue& Value)
{
	// 로컬 예측: 서버 응답을 기다리지 않고 즉시 반응.
	SyncMovementSpeedFromAttributes(true);
	ServerSetSprinting(true);

	if (bIsMoving)
	{
		ReceiveMovementChange(2.0f);
	}
}

void AMainCharacter::OnSprintStop(const FInputActionValue& Value)
{
	SyncMovementSpeedFromAttributes(false);
	ServerSetSprinting(false);

	if (bIsMoving)
	{
		ReceiveMovementChange(1.0f);
	}
}

void AMainCharacter::OnMoveStopped(const FInputActionValue& Value)
{
	bIsMoving = false;
	ReceiveMovementChange(0.0f);
}

void AMainCharacter::ServerSetSprinting_Implementation(bool bNewSprinting)
{
	bSprintRequested = bNewSprinting;
	SyncMovementSpeedFromAttributes(bNewSprinting);
}

void AMainCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority())
	{
		return;
	}

	const bool bMoving = GetVelocity().SizeSquared() > KINDA_SMALL_NUMBER;
	const EMovementStatus NewStatus = !bMoving ? EMovementStatus::Idle
		: (bSprintRequested ? EMovementStatus::Sprint : EMovementStatus::Walk);

	if (NewStatus != MovementStatus)
	{
		MovementStatus = NewStatus;
	}
}

void AMainCharacter::OnRep_MovementChange()
{
	if (!HasLocalNetOwner())
	{
		ReceiveMovementChange(static_cast<float>(MovementStatus));
	}
}

void AMainCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMainCharacter, MovementStatus);
}

void AMainCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 이동 스탯은 이제 PlayerState의 AttributeSet이 관리한다(InitAbilitySystem에서
	// ResetStatsToFull 호출 후 동기화됨) - 혹시 몰라 한 번 더 동기화만 해준다.
	SyncMovementSpeedFromAttributes(false);

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}

		// 타이틀 화면에서 UIOnly로 전환한 입력 모드가 서버 이동 후에도 유지되므로,
		// 실제 캐릭터를 조작하는 시점에 게임 입력 모드로 되돌려준다.
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
	}
}

void AMainCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitAbilitySystem();
}

void AMainCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	InitAbilitySystem();
}

UMainAttributeSet* AMainCharacter::GetMainAttributeSet() const
{
	const AMainPlayerState* MainPS = GetPlayerState<AMainPlayerState>();
	return MainPS ? MainPS->GetMainAttributeSet() : nullptr;
}

bool AMainCharacter::IsSprinting() const
{
	const UMainAttributeSet* AttrSet = GetMainAttributeSet();
	return AttrSet && GetCharacterMovement()->MaxWalkSpeed >= AttrSet->GetRunSpeed();
}

void AMainCharacter::InitAbilitySystem()
{
	AMainPlayerState* MainPS = GetPlayerState<AMainPlayerState>();
	if (!MainPS)
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = MainPS->GetAbilitySystemComponent())
	{
		ASC->InitAbilityActorInfo(MainPS, this);
	}

	if (UMainAttributeSet* AttrSet = MainPS->GetMainAttributeSet())
	{
		AttrSet->OnDeath.RemoveAll(this);
		AttrSet->OnDeath.AddUObject(this, &AMainCharacter::HandleAttributeDeath);

		AttrSet->OnMovementAttributesChanged.RemoveAll(this);
		AttrSet->OnMovementAttributesChanged.AddUObject(this, &AMainCharacter::HandleMovementAttributesChanged);
	}

	if (HasAuthority())
	{
		MainPS->ResetStatsToFull();

		// 재빙의 시 중복 등록 방지를 위해 먼저 제거 후 다시 건다 (AttrSet->OnDeath와 동일 패턴).
		OnTakeAnyDamage.RemoveDynamic(this, &AMainCharacter::OnTakeAnyDamage_GAS);
		OnTakeAnyDamage.AddDynamic(this, &AMainCharacter::OnTakeAnyDamage_GAS);
	}

	SyncMovementSpeedFromAttributes(bSprintRequested);
}

void AMainCharacter::SyncMovementSpeedFromAttributes(bool bSprinting)
{
	UMainAttributeSet* AttrSet = GetMainAttributeSet();
	if (!AttrSet)
	{
		return;
	}

	GetCharacterMovement()->MaxWalkSpeed = bSprinting ? AttrSet->GetRunSpeed() : AttrSet->GetWalkSpeed();
	GetCharacterMovement()->JumpZVelocity = AttrSet->GetJumpPower();
}

void AMainCharacter::HandleMovementAttributesChanged()
{
	SyncMovementSpeedFromAttributes(bSprintRequested);
}

void AMainCharacter::HandleAttributeDeath(AActor* Avatar, AController* Killer)
{
	if (!HasAuthority())
	{
		return;
	}

	if (AMainGameMode* GameMode = GetWorld()->GetAuthGameMode<AMainGameMode>())
	{
		GameMode->HandlePlayerDeath(Cast<APlayerController>(GetController()), Killer);
	}
}

void AMainCharacter::OnTakeAnyDamage_GAS(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (Damage <= 0.0f)
	{
		return;
	}

	AMainPlayerState* MainPS = GetPlayerState<AMainPlayerState>();
	UAbilitySystemComponent* ASC = MainPS ? MainPS->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		return;
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddInstigator(InstigatedBy, DamageCauser);

	FGameplayEffectSpecHandle DamageSpec = ASC->MakeOutgoingSpec(UGE_Damage::StaticClass(), 1.0f, Context);
	DamageSpec.Data->SetSetByCallerMagnitude(MainGameplayTags::Data_Damage.GetTag(), Damage);
	ASC->ApplyGameplayEffectSpecToSelf(*DamageSpec.Data);

	// 회복 지연 태그 - HPRegenDelay초 뒤 자동 만료된다. 연속으로 맞으면 매번 새 인스턴스가
	// 걸리는데(스택 안 함), 각자 독립적으로 만료되므로 결과적으로 "마지막 피격 후
	// HPRegenDelay초"가 유지되는 것과 동일하다.
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UPlayerStatManager* StatManager = GameInstance->GetSubsystem<UPlayerStatManager>())
		{
			FGameplayEffectSpecHandle TagSpec = ASC->MakeOutgoingSpec(UGE_DamagedTag::StaticClass(), 1.0f, Context);
			TagSpec.Data->SetSetByCallerMagnitude(MainGameplayTags::Data_HPRegenDelay.GetTag(), StatManager->GetBaseStat().HPRegenDelay);
			ASC->ApplyGameplayEffectSpecToSelf(*TagSpec.Data);
		}
	}

	const FString KillerName = (InstigatedBy && InstigatedBy->PlayerState)
		? InstigatedBy->PlayerState->GetPlayerName()
		: TEXT("Unknown");
	const AController* VictimController = GetController();
	const FString VictimName = (VictimController && VictimController->PlayerState)
		? VictimController->PlayerState->GetPlayerName()
		: GetNameSafe(this);

	UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] %s -> %s: %.1f damage (GAS)"), *KillerName, *VictimName, Damage);
}

void AMainCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMainCharacter::OnMove);
			EnhancedInput->BindAction(MoveAction, ETriggerEvent::Completed, this, &AMainCharacter::OnMoveStopped);
		}
		if (LookAction)
		{
			EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMainCharacter::OnLook);
		}
		if (JumpAction)
		{
			EnhancedInput->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ACharacter::Jump);
			EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		}
		if (SprintAction)
		{
			EnhancedInput->BindAction(SprintAction, ETriggerEvent::Started, this, &AMainCharacter::OnSprintStart);
			EnhancedInput->BindAction(SprintAction, ETriggerEvent::Completed, this, &AMainCharacter::OnSprintStop);
		}
		if (FireAction)
		{
			EnhancedInput->BindAction(FireAction, ETriggerEvent::Started, this, &AMainCharacter::OnFire);
			EnhancedInput->BindAction(FireAction, ETriggerEvent::Completed, this, &AMainCharacter::OnStopFire);
		}
		if (ReloadAction)
		{
			EnhancedInput->BindAction(ReloadAction, ETriggerEvent::Started, this, &AMainCharacter::OnReload);
		}
		if (ADSAction)
		{
			EnhancedInput->BindAction(ADSAction, ETriggerEvent::Started, this, &AMainCharacter::OnADSStart);
			EnhancedInput->BindAction(ADSAction, ETriggerEvent::Completed, this, &AMainCharacter::OnADSStop);
		}
	}
}

void AMainCharacter::OnMove(const FInputActionValue& Value)
{
	const FVector2D MoveVector = Value.Get<FVector2D>();

	if (Controller)
	{
		// 월드 좌표가 아니라 카메라가 보는 방향 기준이어야 하므로
		// 컨트롤러의 Yaw만 가져와서 앞/오른쪽 방향 벡터를 계산한다.
		const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// AddMovementInput 호출 시 클라이언트는 즉시 예측 이동하고, 같은 입력이 서버로 전달되어
		// 서버가 계산한 진짜 위치로 자동 보정된다 (CharacterMovementComponent 내장 기능).
		AddMovementInput(ForwardDirection, MoveVector.Y);
		AddMovementInput(RightDirection, MoveVector.X);

		// 멈춰있다가 방금 움직이기 시작한 순간에만 이벤트를 호출한다.
		if (!bIsMoving)
		{
			bIsMoving = true;
			ReceiveMovementChange(IsSprinting() ? 2.0f : 1.0f);
		}

		// 디버그용: 각 축 속도 확인
		//const FVector Velocity = GetVelocity();
		//UE_LOG(LogTemp, Log, TEXT("[ProjectRWW] %s velocity: X=%.1f Y=%.1f Z=%.1f (Speed=%.1f)"), *GetNameSafe(this), Velocity.X, Velocity.Y, Velocity.Z, Velocity.Size());
	}
}

void AMainCharacter::OnLook(const FInputActionValue& Value)
{
	const FVector2D LookVector = Value.Get<FVector2D>();

	if (Controller)
	{
		// 컨트롤러 회전은 서버-클라이언트 간 자동 동기화된다.
		AddControllerYawInput(LookVector.X);
		AddControllerPitchInput(LookVector.Y);
	}
}

AActor* AMainCharacter::GetMainItem() const
{
	// ActiveHandActor는 무기/아이템 공용 슬롯이라, 지금 무기가 장착 중이 아닐 때만
	// "아이템 용도"로 취급해서 반환한다 - GetMainWeapon()(WeaponComponent::
	// GetActiveWeaponActor())과 정확히 대칭되는 필터링이다.
	if (!WeaponComponent || WeaponComponent->HasWeaponEquipped())
	{
		return nullptr;
	}
	return WeaponComponent->ActiveHandActor;
}