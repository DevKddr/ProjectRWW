// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Combat/MainHPComponent.h"
#include "Combat/MainManaComponent.h"
#include "Combat/MainWeaponComponent.h"
#include "Combat/MainEffectTickComponent.h"
#include "Core/MainGameMode.h"
#include "PlayerBaseStat/PlayerStatManager.h"

AMainCharacter::AMainCharacter()
{
	// ACharacter는 기본적으로 bReplicates = true이지만, 서버 권위 캐릭터임을 명시적으로 표시한다.
	bReplicates = true;
	GetCharacterMovement()->SetIsReplicated(true);

	HPComponent = CreateDefaultSubobject<UMainHPComponent>(TEXT("HPComponent"));
	ManaComponent = CreateDefaultSubobject<UMainManaComponent>(TEXT("ManaComponent"));
	WeaponComponent = CreateDefaultSubobject<UMainWeaponComponent>(TEXT("WeaponComponent"));
	EffectTickComponent = CreateDefaultSubobject<UMainEffectTickComponent>(TEXT("EffectTickComponent"));
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
		WeaponComponent->StopFire();
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

void AMainCharacter::OnSwitchWeapon(const FInputActionValue& Value)
{
	if (WeaponComponent)
	{
		WeaponComponent->RequestSwitchWeapon();
	}
}

void AMainCharacter::OnSprintStart(const FInputActionValue& Value)
{
	// 로컬 예측: 서버 응답을 기다리지 않고 즉시 반응.
	GetCharacterMovement()->MaxWalkSpeed = RunSpeed;
	ServerSetSprinting(true);
}

void AMainCharacter::OnSprintStop(const FInputActionValue& Value)
{
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	ServerSetSprinting(false);
}

void AMainCharacter::ServerSetSprinting_Implementation(bool bNewSprinting)
{
	GetCharacterMovement()->MaxWalkSpeed = bNewSprinting ? RunSpeed : WalkSpeed;
}

void AMainCharacter::BeginPlay()
{
	Super::BeginPlay();

	// PlayerBaseStat.json에서 이동 스탯을 채운다. 로드 실패 시 0으로 남아 눈에 띄게
	// 망가지는 쪽을 택했다(조용한 폴백 없음) — MainHPComponent와 동일한 정책.
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UPlayerStatManager* StatManager = GameInstance->GetSubsystem<UPlayerStatManager>())
		{
			const FPlayerBaseStat& Stat = StatManager->GetBaseStat();
			WalkSpeed = Stat.WalkSpeed;
			RunSpeed = Stat.RunSpeed;
			JumpPower = Stat.JumpPower;
		}
	}

	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	GetCharacterMovement()->JumpZVelocity = JumpPower;

	// 서버에서만 등록 — HPComponent 자체의 델리게이트 등록과 같은 이유(서버 권위 유지).
	// 클라이언트에도 등록하면 사망 정산(HandlePlayerDeath)이 클라이언트에서도 시도되어 위험하다.
	if (HasAuthority() && HPComponent)
	{
		HPComponent->OnDeath.AddDynamic(this, &AMainCharacter::OnDeath);
	}

	// 회복 등 주기적 이펙트 처리 — EffectTickComponent가 신호를 보내면 HP/Mana가 각자 반응한다.
	if (HasAuthority() && EffectTickComponent)
	{
		if (HPComponent)
		{
			EffectTickComponent->OnEffectTick.AddDynamic(HPComponent, &UMainHPComponent::OnEffectTick);
		}
		if (ManaComponent)
		{
			EffectTickComponent->OnEffectTick.AddDynamic(ManaComponent, &UMainManaComponent::OnEffectTick);
		}
	}

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

void AMainCharacter::OnDeath(AController* Killer)
{
	if (AMainGameMode* GameMode = GetWorld()->GetAuthGameMode<AMainGameMode>())
	{
		GameMode->HandlePlayerDeath(Cast<APlayerController>(GetController()), Killer);
	}
}

void AMainCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMainCharacter::OnMove);
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
		if (SwitchWeaponAction)
		{
			EnhancedInput->BindAction(SwitchWeaponAction, ETriggerEvent::Started, this, &AMainCharacter::OnSwitchWeapon);
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