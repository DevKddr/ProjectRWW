// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Combat/MainWeaponComponent.h"
#include "Player/PlayerMovementComponent.h"
#include "Items/InteractionComponent.h"
#include "Core/MainGameMode.h"
#include "Core/MainGameInstance.h"
#include "Net/UnrealNetwork.h"
#include "Player/MainPlayerState.h"
#include "Player/MainPlayerController.h"
#include "GAS/MainAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GAS/GameplayEffects/GE_Damage.h"
#include "GAS/GameplayEffects/GE_DamagedTag.h"
#include "GAS/MainGameplayTags.h"
#include "PlayerBaseStat/PlayerStatManager.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "HAL/IConsoleManager.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"

// [ScopeAfterADS] 1이면 스코프 PIP 캡처를 ADS가 완료된 뒤에만 켠다(기본 1). 0이면 항상 캡처(원래 동작).
static TAutoConsoleVariable<int32> CVarScopeAfterADS(
	TEXT("rww.ScopeAfterADS"), 1,
	TEXT("1이면 스코프 캡처를 ADS 완료 후에만 켠다 (기본 1)."),
	ECVF_Default);

// [FirstPersonRender] 본인 화면에서 총/몸이 벽을 뚫고 보이지 않게 하는 First Person 렌더링 사용 여부.
// 1이면 켜짐(기본), 0이면 원래 렌더링. 값을 바꾸면 Tick에서 감지해 바로 반영한다.
static TAutoConsoleVariable<int32> CVarFirstPersonRendering(
	TEXT("rww.FirstPersonRendering"), 1,
	TEXT("1이면 로컬 플레이어의 FirstPersonMesh와 무기를 First Person 렌더링으로 그린다 (기본 1)."),
	ECVF_Default);

AMainCharacter::AMainCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UPlayerMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	// ACharacter는 기본적으로 bReplicates = true이지만, 서버 권위 캐릭터임을 명시적으로 표시한다.
	bReplicates = true;
	GetCharacterMovement()->SetIsReplicated(true);

	WeaponComponent = CreateDefaultSubobject<UMainWeaponComponent>(TEXT("WeaponComponent"));
	InteractionComponent = CreateDefaultSubobject<UInteractionComponent>(TEXT("InteractionComponent"));
}

void AMainCharacter::OnFire(const FInputActionValue& Value)
{
	if (WeaponComponent)
	{
		// 달리는 중에 발사를 누르면 스프린트를 풀고 바로 쏜다. 무기가 없을 때는 쏘지도 못하므로 스프린트를 유지한다.
		if (WeaponComponent->HasWeaponEquipped() && IsSprinting())
		{
			CancelSprint();
		}
		WeaponComponent->StartFire();
	}
}

void AMainCharacter::OnStopFire(const FInputActionValue& Value)
{
	// 맨손/아이템 상태에서는 "발사 멈춤"을 로컬 연출로도, 서버(→원격 클라이언트 멀티캐스트)로도 보내지 않는다.
	// 무기 교체 시점의 정리는 EquipWeapon()이 StopFire()로 직접 처리하므로 여기서 막아도 상태가 남지 않는다.
	if (WeaponComponent && WeaponComponent->HasWeaponEquipped())
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
		// 달리는 중에 조준을 누르면 스프린트를 풀고 바로 조준한다.
		if (WeaponComponent->HasWeaponEquipped() && IsSprinting())
		{
			CancelSprint();
		}
		WeaponComponent->StartADS();
	}
}

void AMainCharacter::OnADSStop(const FInputActionValue& Value)
{
	// 맨손/아이템 상태에서는 조준 해제 연출(ReceiveADSChange(false))을 부르지 않는다.
	// 무기 교체로 조준이 풀리는 경우는 EquipWeapon()이 직접 해제하므로 여기서 막아도 문제없다.
	if (WeaponComponent && WeaponComponent->HasWeaponEquipped())
	{
		WeaponComponent->StopADS();
	}
}

void AMainCharacter::OnSprintStart(const FInputActionValue& Value)
{
	// 달리기 시작하면 조준 상태일 수 없다 - 이미 조준 중이었다면 풀어준다.
	if (WeaponComponent && WeaponComponent->IsAiming())
	{
		WeaponComponent->StopADS();
	}

	// 스프린트 의도를 이동 컴포넌트에 알린다. 이동 패킷의 플래그로 서버에 전달되므로 별도 RPC가 필요 없다.
	if (UPlayerMovementComponent* Movement = GetPlayerMovement())
	{
		Movement->SetWantsToSprint(true);
	}

	if (bIsMoving)
	{
		ReceiveMovementChange(2.0f);
	}
}

void AMainCharacter::OnSprintStop(const FInputActionValue& Value)
{
	CancelSprint();
}

void AMainCharacter::CancelSprint()
{
	if (UPlayerMovementComponent* Movement = GetPlayerMovement())
	{
		Movement->SetWantsToSprint(false);
	}

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

void AMainCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// [FirstPersonRender] 콘솔 변수가 바뀌었으면 다시 적용한다. 로컬 조종 캐릭터만 대상이라 비용이 거의 없다.
	if (IsLocallyControlled() && (CVarFirstPersonRendering.GetValueOnGameThread() != 0) != bFirstPersonRenderApplied)
	{
		ApplyFirstPersonRendering();
	}

	if (IsLocallyControlled())
	{
		UpdateScopeCapture(); // [ScopeAfterADS]
	}

	if (!HasAuthority())
	{
		return;
	}

	const bool bMoving = GetVelocity().SizeSquared() > KINDA_SMALL_NUMBER;
	const EMovementStatus NewStatus = !bMoving ? EMovementStatus::Idle
		: (IsSprinting() ? EMovementStatus::Sprint : EMovementStatus::Walk);

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

// [FirstPersonRender] begin
void AMainCharacter::ApplyFirstPersonRendering()
{
	// 로컬 조종 캐릭터에서만 켠다. Controller가 아직 없는 클라이언트 초기에는 네트워크 역할(자율 프록시)로도 판별한다.
	const bool bIsLocalView = IsLocallyControlled() || GetLocalRole() == ROLE_AutonomousProxy;
	const bool bEnable = bIsLocalView && CVarFirstPersonRendering.GetValueOnGameThread() != 0;

	// FirstPersonMesh는 BP에서 만든 컴포넌트라 이름으로 찾는다(MainWeaponComponent의 HitMeshComponentName과 같은 이름).
	USkeletalMeshComponent* FirstPersonMesh = nullptr;
	TArray<USkeletalMeshComponent*> SkeletalMeshes;
	GetComponents<USkeletalMeshComponent>(SkeletalMeshes);
	for (USkeletalMeshComponent* SkelMesh : SkeletalMeshes)
	{
		if (SkelMesh && SkelMesh->GetFName() == TEXT("FirstPersonMesh"))
		{
			FirstPersonMesh = SkelMesh;
			break;
		}
	}
	if (!FirstPersonMesh)
	{
		return;
	}

	// 카메라: 스케일만 켠다. FirstPersonFieldOfView는 켜지 않아서 ADS 줌 등 카메라 FOV 변화를 그대로 따라간다.
	if (UCameraComponent* Camera = FindComponentByClass<UCameraComponent>())
	{
		Camera->SetEnableFirstPersonScale(bEnable);
		if (bEnable)
		{
			Camera->SetFirstPersonScale(FirstPersonRenderScale);
		}
	}

	const EFirstPersonPrimitiveType Type = bEnable ? EFirstPersonPrimitiveType::FirstPerson : EFirstPersonPrimitiveType::None;

	FirstPersonMesh->SetFirstPersonPrimitiveType(Type);

	// 손에 붙은 무기 액터와 그 부착물/이펙트는 FirstPersonMesh의 자손 컴포넌트다.
	TArray<USceneComponent*> Descendants;
	FirstPersonMesh->GetChildrenComponents(true, Descendants);
	for (USceneComponent* Child : Descendants)
	{
		if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Child))
		{
			if (Primitive->FirstPersonPrimitiveType != Type)
			{
				Primitive->SetFirstPersonPrimitiveType(Type);
			}
		}
	}

	// [ScopeAfterADS] 손에 든 무기에 달린 스코프 캡처를 다시 모은다.
	ScopeCaptures.Reset();
	for (USceneComponent* Child : Descendants)
	{
		if (USceneCaptureComponent2D* Capture = Cast<USceneCaptureComponent2D>(Child))
		{
			ScopeCaptures.Add(Capture);
		}
	}

	bFirstPersonRenderApplied = bEnable;
}
// [FirstPersonRender] end

// [ScopeAfterADS] begin
void AMainCharacter::UpdateScopeCapture()
{
	if (ScopeCaptures.Num() == 0)
	{
		return;
	}

	// 렌즈의 페이드 값 = ADS 진행도(0~1). ADS를 시작하면 0에서 1로 올라가며 서서히 나타나고, 풀면 1에서 0으로
	// 내려가며 서서히 사라진다. 진행도가 0보다 큰 동안에만 캡처를 켠다. 이 기능을 끄면(0) 항상 켠다(페이드 1).
	const float Fade = CVarScopeAfterADS.GetValueOnGameThread() == 0
		? 1.0f
		: (WeaponComponent ? FMath::Clamp(WeaponComponent->GetADSAlpha(), 0.0f, 1.0f) : 0.0f);
	const bool bReady = Fade > KINDA_SMALL_NUMBER;

	for (const TWeakObjectPtr<USceneCaptureComponent2D>& Weak : ScopeCaptures)
	{
		if (USceneCaptureComponent2D* Capture = Weak.Get())
		{
			if (Capture->bCaptureEveryFrame != bReady)
			{
				Capture->bCaptureEveryFrame = bReady;
				Capture->bCaptureOnMovement = bReady;
			}

			// 렌즈 면(Reticle)과 십자선(ScopeCross)은 머티리얼의 ScopeFade 파라미터(0~1)를 ADS 진행도로 올려서
			// 페이드 인/아웃한다. 진행도가 0이면 아예 숨긴다. 머티리얼에 ScopeFade 파라미터가 없으면 아무 일도 없고
			// 숨김/표시만 바뀐다. 새로 장착한 스코프에도 매 프레임 맞춰진다.
			if (const AActor* ScopeActor = Capture->GetOwner())
			{
				TArray<UStaticMeshComponent*> LensMeshes;
				ScopeActor->GetComponents<UStaticMeshComponent>(LensMeshes);
				for (UStaticMeshComponent* LensMesh : LensMeshes)
				{
					const FName LensName = LensMesh->GetFName();
					if (LensName == TEXT("Reticle") || LensName == TEXT("ScopeCross"))
					{
						LensMesh->SetVisibility(bReady);
						if (bReady)
						{
							UMaterialInstanceDynamic* LensMaterial = Cast<UMaterialInstanceDynamic>(LensMesh->GetMaterial(0));
							if (!LensMaterial)
							{
								LensMaterial = LensMesh->CreateAndSetMaterialInstanceDynamic(0);
							}
							if (LensMaterial)
							{
								LensMaterial->SetScalarParameterValue(TEXT("ScopeFade"), Fade);
							}
						}
					}
				}
			}
		}
	}

	// 켜진 상태에서 꺼질 때 한 번만 렌더 타깃을 비운다(공유 타깃이라 매 프레임 비우면 안 된다).
	if (bScopeCaptureOn && !bReady)
	{
		for (const TWeakObjectPtr<USceneCaptureComponent2D>& Weak : ScopeCaptures)
		{
			if (const USceneCaptureComponent2D* Capture = Weak.Get())
			{
				if (Capture->TextureTarget)
				{
					UKismetRenderingLibrary::ClearRenderTarget2D(this, Capture->TextureTarget, FLinearColor::Black);
				}
			}
		}
	}
	bScopeCaptureOn = bReady;
}
// [ScopeAfterADS] end

void AMainCharacter::BeginPlay()
{
	Super::BeginPlay();

	ApplyFirstPersonRendering(); // [FirstPersonRender]

	// 폰 BeginPlay 전에 도착해 건너뛴 장착 BP 이벤트를 지금 보낸다 - BP BeginPlay는 방금 끝났다.
	// 전용 서버처럼 장착이 BeginPlay 이후에 오는 폰은 아직 장착 상태가 없어 아무것도 보내지 않는다.
	if (WeaponComponent)
	{
		WeaponComponent->SendCurrentEquipStateToBlueprint();
	}

	// 점프력은 PlayerState의 AttributeSet이 관리한다(InitAbilitySystem에서 ResetStatsToFull 호출 후 동기화됨) -
	// 혹시 몰라 한 번 더 동기화만 해준다.
	SyncJumpPowerFromAttributes();

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

	// 소유 클라이언트의 BeginPlay가 끝났음을 서버에 알린다. 이 신호를 받은 뒤에야
	// 서버가 보상 지급/장착을 실행한다. 이 시점엔 클라이언트에 Controller가 아직
	// 없을 수 있어 IsLocallyControlled() 대신 네트워크 역할로 판별한다.
	if (GetLocalRole() == ROLE_AutonomousProxy)
	{
		if (AMainPlayerController* MainPC = GetWorld()->GetFirstPlayerController<AMainPlayerController>())
		{
			MainPC->Server_NotifyPawnReady(this);
		}
	}
}

void AMainCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitAbilitySystem();
	ApplyFirstPersonRendering(); // [FirstPersonRender]
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

UPlayerMovementComponent* AMainCharacter::GetPlayerMovement() const
{
	return Cast<UPlayerMovementComponent>(GetCharacterMovement());
}

bool AMainCharacter::IsSprinting() const
{
	const UPlayerMovementComponent* Movement = GetPlayerMovement();
	return Movement && Movement->IsSprintingEffective();
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

	SyncJumpPowerFromAttributes();
}

void AMainCharacter::SyncJumpPowerFromAttributes()
{
	UMainAttributeSet* AttrSet = GetMainAttributeSet();
	if (!AttrSet)
	{
		return;
	}

	GetCharacterMovement()->JumpZVelocity = AttrSet->GetJumpPower();
}

void AMainCharacter::HandleMovementAttributesChanged()
{
	SyncJumpPowerFromAttributes();
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

	// 체력이 바뀐 이 프레임에 바로 클라이언트로 복제하도록 요청한다(복제 빈도 설정과 별개의 안전망).
	MainPS->ForceNetUpdate();

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
		if (ItemSkillPrimaryAction)
		{
			EnhancedInput->BindAction(ItemSkillPrimaryAction, ETriggerEvent::Started, this, &AMainCharacter::OnItemSkillPrimary);
			// Completed는 정상적으로 뗐을 때, Canceled는 Hold/Tap 같은 트리거가 중간에 취소됐을 때. 둘 다 "뗌"으로 처리한다.
			EnhancedInput->BindAction(ItemSkillPrimaryAction, ETriggerEvent::Completed, this, &AMainCharacter::OnItemSkillPrimaryReleased);
			EnhancedInput->BindAction(ItemSkillPrimaryAction, ETriggerEvent::Canceled, this, &AMainCharacter::OnItemSkillPrimaryReleased);
		}
		if (ItemSkillSecondaryAction)
		{
			EnhancedInput->BindAction(ItemSkillSecondaryAction, ETriggerEvent::Started, this, &AMainCharacter::OnItemSkillSecondary);
			EnhancedInput->BindAction(ItemSkillSecondaryAction, ETriggerEvent::Completed, this, &AMainCharacter::OnItemSkillSecondaryReleased);
			EnhancedInput->BindAction(ItemSkillSecondaryAction, ETriggerEvent::Canceled, this, &AMainCharacter::OnItemSkillSecondaryReleased);
		}
		if (InteractAction)
		{
			EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, this, &AMainCharacter::OnInteract);
		}

		// SetupPlayerInputComponent는 로컬 조종 폰에서만 호출되므로, 다른 플레이어 폰은
		// 트레이스 타이머를 아예 돌리지 않는다.
		if (InteractionComponent)
		{
			InteractionComponent->StartScanning();
		}
	}
}

void AMainCharacter::OnInteract(const FInputActionValue& Value)
{
	if (InteractionComponent)
	{
		InteractionComponent->TryInteract();
	}
}

void AMainCharacter::OnItemSkillPrimary(const FInputActionValue& Value)
{
	AMainPlayerState* MainPS = GetPlayerState<AMainPlayerState>();
	if (UAbilitySystemComponent* ASC = MainPS ? MainPS->GetAbilitySystemComponent() : nullptr)
	{
		UE_LOG(LogTemp, Log, TEXT("[ProjectRWW][진단] OnItemSkillPrimary 입력 수신"));
		ASC->AbilityLocalInputPressed(static_cast<int32>(EMainAbilityInputID::Primary));
	}
}

void AMainCharacter::OnItemSkillSecondary(const FInputActionValue& Value)
{
	AMainPlayerState* MainPS = GetPlayerState<AMainPlayerState>();
	if (UAbilitySystemComponent* ASC = MainPS ? MainPS->GetAbilitySystemComponent() : nullptr)
	{
		UE_LOG(LogTemp, Log, TEXT("[ProjectRWW][진단] OnItemSkillSecondary 입력 수신"));
		ASC->AbilityLocalInputPressed(static_cast<int32>(EMainAbilityInputID::Secondary));
	}
}

void AMainCharacter::OnItemSkillPrimaryReleased(const FInputActionValue& Value)
{
	AMainPlayerState* MainPS = GetPlayerState<AMainPlayerState>();
	if (UAbilitySystemComponent* ASC = MainPS ? MainPS->GetAbilitySystemComponent() : nullptr)
	{
		ASC->AbilityLocalInputReleased(static_cast<int32>(EMainAbilityInputID::Primary));
	}
}

void AMainCharacter::OnItemSkillSecondaryReleased(const FInputActionValue& Value)
{
	AMainPlayerState* MainPS = GetPlayerState<AMainPlayerState>();
	if (UAbilitySystemComponent* ASC = MainPS ? MainPS->GetAbilitySystemComponent() : nullptr)
	{
		ASC->AbilityLocalInputReleased(static_cast<int32>(EMainAbilityInputID::Secondary));
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

namespace
{
	// 플레이어가 설정한 시점 감도 배율을 계산한다. 이 값은 이 PC의 GameInstance에만 있는 로컬 입력 설정이며
	// 서버 권위 대상이 아니다(서버는 감도가 적용된 "최종 컨트롤 회전"만 받는다).
	//
	//   배율 = LookSensitivity
	//          x Lerp(1, 1/ScopeZoomLevel, ADSAlpha)            ... 줌 보정: 줌한 화면에서 같은 마우스 이동량이 비슷한 속도로 느껴지게
	//          x Lerp(1, ADSSensitivityMultiplier, ADSAlpha)    ... 플레이어가 설정한 조준 감도 배율
	//
	// ADSAlpha(0~1)로 섞기 때문에 ADS 전환 중에도 감도가 부드럽게 변해 시점이 튀지 않는다.
	// 무기를 안 들었거나 조준 중이 아니면(ADSAlpha = 0) LookSensitivity만 적용된다.
	float ComputeLookSensitivityScale(const UGameInstance* GameInstance, const UMainWeaponComponent* Weapon)
	{
		const UMainGameInstance* Settings = Cast<UMainGameInstance>(GameInstance);
		if (!Settings)
		{
			return 1.0f;
		}

		float Scale = Settings->GetLookSensitivity();

		if (Weapon)
		{
			const float ADSAlpha = Weapon->GetADSAlpha();
			if (ADSAlpha > 0.0f)
			{
				const float ZoomLevel = FMath::Max(1.0f, Weapon->GetScopeZoomLevel());
				const float ZoomCorrection = FMath::Lerp(1.0f, 1.0f / ZoomLevel, ADSAlpha);
				const float ADSMultiplier = FMath::Lerp(1.0f, Settings->GetADSSensitivityMultiplier(), ADSAlpha);
				Scale *= ZoomCorrection * ADSMultiplier;
			}
		}

		return Scale;
	}
}

void AMainCharacter::OnLook(const FInputActionValue& Value)
{
	const FVector2D LookVector = Value.Get<FVector2D>();

	if (Controller)
	{
		// 감도는 로컬 플레이어의 입력에만 적용한다. 다른 머신의 GameInstance 값을 읽는 일이 없도록
		// 로컬 조종이 아니면 배율 1.0(보정 없음)으로 둔다.
		const float Scale = IsLocallyControlled()
			? ComputeLookSensitivityScale(GetGameInstance(), WeaponComponent)
			: 1.0f;

		// 컨트롤러 회전은 서버-클라이언트 간 자동 동기화된다.
		AddControllerYawInput(LookVector.X * Scale);
		AddControllerPitchInput(LookVector.Y * Scale);
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