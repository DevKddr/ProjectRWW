// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Combat/MainHealthComponent.h"
#include "Combat/MainWeaponComponent.h"

AMainCharacter::AMainCharacter()
{
	// ACharacter는 기본적으로 bReplicates = true이지만, 서버 권위 캐릭터임을 명시적으로 표시한다.
	bReplicates = true;
	GetCharacterMovement()->SetIsReplicated(true);

	HealthComponent = CreateDefaultSubobject<UMainHealthComponent>(TEXT("HealthComponent"));
	WeaponComponent = CreateDefaultSubobject<UMainWeaponComponent>(TEXT("WeaponComponent"));
}

void AMainCharacter::OnFire(const FInputActionValue& Value)
{
	if (WeaponComponent)
	{
		WeaponComponent->StartFire();
	}
}

void AMainCharacter::OnSprintStart(const FInputActionValue& Value)
{
	// 로컬 예측: 서버 응답을 기다리지 않고 즉시 반응.
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
	ServerSetSprinting(true);
}

void AMainCharacter::OnSprintStop(const FInputActionValue& Value)
{
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	ServerSetSprinting(false);
}

void AMainCharacter::ServerSetSprinting_Implementation(bool bNewSprinting)
{
	GetCharacterMovement()->MaxWalkSpeed = bNewSprinting ? SprintSpeed : WalkSpeed;
}

void AMainCharacter::BeginPlay()
{
	Super::BeginPlay();

	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

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