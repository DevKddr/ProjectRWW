// Copyright Epic Games, Inc. All Rights Reserved.

#include "ItemSkill_I_5_Zone.h"
#include "AbilitySystemComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Character.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GAS/MainAttributeSet.h"
#include "GAS/MainGameplayTags.h"
#include "NativeGameplayTags.h"
#include "Player/MainPlayerController.h"
#include "Player/MainPlayerState.h"

// 이 파일 안에서만 쓰는 태그 - MainGameplayTags에 안 넣는다 (다른 파일에서 참조할 일이 없음).
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Data_ItemSkill_I_5_Duration, "Data.ItemSkill.I_5.Duration");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Data_ItemSkill_I_5_MoveSpeedMultiplier, "Data.ItemSkill.I_5.MoveSpeedMultiplier");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Data_ItemSkill_I_5_FireRateMultiplier, "Data.ItemSkill.I_5.FireRateMultiplier");

UGE_ItemSkill_I_5_Effect::UGE_ItemSkill_I_5_Effect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FSetByCallerFloat DurationSetByCaller;
	DurationSetByCaller.DataTag = TAG_Data_ItemSkill_I_5_Duration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(DurationSetByCaller);

	FSetByCallerFloat MoveSpeedMagnitude;
	MoveSpeedMagnitude.DataTag = TAG_Data_ItemSkill_I_5_MoveSpeedMultiplier;

	FGameplayModifierInfo WalkModifier;
	WalkModifier.Attribute = UMainAttributeSet::GetWalkSpeedAttribute();
	WalkModifier.ModifierOp = EGameplayModOp::Multiplicitive;
	WalkModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(MoveSpeedMagnitude);
	Modifiers.Add(WalkModifier);

	FGameplayModifierInfo RunModifier = WalkModifier;
	RunModifier.Attribute = UMainAttributeSet::GetRunSpeedAttribute();
	Modifiers.Add(RunModifier);

	FSetByCallerFloat FireRateMagnitude;
	FireRateMagnitude.DataTag = TAG_Data_ItemSkill_I_5_FireRateMultiplier;

	FGameplayModifierInfo FireRateModifier;
	FireRateModifier.Attribute = UMainAttributeSet::GetFireRateMultiplierAttribute();
	FireRateModifier.ModifierOp = EGameplayModOp::Multiplicitive;
	FireRateModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FireRateMagnitude);
	Modifiers.Add(FireRateModifier);

	// 같은 효과(장판이 갱신하거나 여러 장판이 겹침)는 쌓이지 않고 한 번만 적용되며, 다시 적용되면 지속 시간만 새로 갱신된다.
	// 시전자와 무관하게 대상 하나에 하나만 걸리도록 AggregateByTarget을 쓴다.
	// 5.7에서 StackingType 직접 접근은 deprecated이고 대안인 SetStackingType은 에디터 빌드에만 있어서(WITH_EDITOR),
	// 패키지/서버 빌드에서도 컴파일되도록 경고만 끄고 직접 대입한다.
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	StackingType = EGameplayEffectStackingType::AggregateByTarget;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	StackLimitCount = 1;
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;
}

void UGE_ItemSkill_I_5_Effect::PostInitProperties()
{
	Super::PostInitProperties();

	// 사망 시 이 버프를 자동으로 제거하도록 정리 대상 태그를 붙인다. 생성자에서 FindOrAddComponent를 부르면
	// 크래시가 나서(GE_DamagedTag와 같은 이유) 생성자 체인이 끝난 직후인 PostInitProperties에서 한다.
	UTargetTagsGameplayEffectComponent& TargetTagsComponent = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer TagChanges;
	TagChanges.AddTag(MainGameplayTags::Effect_ClearOnDeath);
	TargetTagsComponent.SetAndApplyTargetTagChanges(TagChanges);
}

AItemSkill_I_5_Zone::AItemSkill_I_5_Zone()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	// 설치된 뒤 움직이지 않는다. 스폰 시점의 위치는 처음 복제될 때 한 번에 전달된다.
	SetReplicateMovement(false);
	SetNetUpdateFrequency(1.0f);
	SetMinNetUpdateFrequency(0.5f);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	// 이 메시가 보이는 모양이자 실제 판정 범위다. 메시/재질은 BP에서 정하고, 크기와 위치는 BeginPlay에서 ZoneRadius/ZoneHeight에 맞춘다.
	// 캐릭터(Pawn)와의 겹침만 받는다(막지 않고, 총알도 가로채지 않는다).
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(SceneRoot);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	MeshComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	MeshComponent->SetGenerateOverlapEvents(true);
}

void AItemSkill_I_5_Zone::BeginPlay()
{
	Super::BeginPlay();

	// 판정은 서버만 한다. 클라이언트는 모양만 보이면 되므로 충돌(겹침 추적)을 꺼서 비용을 없앤다.
	if (!HasAuthority())
	{
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// 메시를 ZoneRadius/ZoneHeight 크기의 원기둥에 맞춘다. 서버와 클라이언트가 같은 클래스 기본값을 쓰므로
	// 따로 복제하지 않아도 같은 크기로 보인다. 메시가 없으면(BP에서 아직 지정 안 함) 건너뛴다.
	if (const UStaticMesh* Mesh = MeshComponent->GetStaticMesh())
	{
		const FBoxSphereBounds Bounds = Mesh->GetBounds();
		const FVector MeshSize = Bounds.BoxExtent * 2.0;
		if (MeshSize.X > UE_KINDA_SMALL_NUMBER && MeshSize.Y > UE_KINDA_SMALL_NUMBER && MeshSize.Z > UE_KINDA_SMALL_NUMBER)
		{
			const FVector Scale(ZoneRadius * 2.0f / MeshSize.X, ZoneRadius * 2.0f / MeshSize.Y, ZoneHeight / MeshSize.Z);
			MeshComponent->SetRelativeScale3D(Scale);
			// 메시의 바닥면이 원기둥 바닥(이 액터의 위치)에 오도록 올린다.
			const float MeshBottomZ = (Bounds.Origin.Z - Bounds.BoxExtent.Z) * Scale.Z;
			MeshComponent->SetRelativeLocation(FVector(0.0f, 0.0f, -MeshBottomZ));
		}
	}

	// 수명과 버프 갱신은 서버에서만 돈다. 서버가 Destroy하면 클라이언트 복사본도 같이 사라진다.
	if (HasAuthority())
	{
		SetLifeSpan(ZoneLifeSeconds);
		UE_LOG(LogTemp, Log, TEXT("[ProjectRWW][I_5 장판] 생성 %s (유지 %.1f초)"), *GetActorLocation().ToString(), ZoneLifeSeconds);

		RefreshBuffs();
		GetWorldTimerManager().SetTimer(RefreshTimerHandle, this, &AItemSkill_I_5_Zone::RefreshBuffs, RefreshIntervalSeconds, true);
	}
}

void AItemSkill_I_5_Zone::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority())
	{
		UE_LOG(LogTemp, Log, TEXT("[ProjectRWW][I_5 장판] 소멸"));
	}
	Super::EndPlay(EndPlayReason);
}

void AItemSkill_I_5_Zone::RefreshBuffs()
{
	// 장판 메시와 겹친 캐릭터를 엔진이 추적해 준다(같은 캐릭터가 여러 번 나오지 않는다).
	TArray<AActor*> Overlapping;
	MeshComponent->GetOverlappingActors(Overlapping, ACharacter::StaticClass());

	for (AActor* Actor : Overlapping)
	{
		const ACharacter* Character = Cast<ACharacter>(Actor);
		const AMainPlayerController* Controller = Character ? Cast<AMainPlayerController>(Character->GetController()) : nullptr;
		if (!Controller || Controller->bIsDead)
		{
			continue; // 컨트롤러가 없거나 죽은 캐릭터는 건너뛴다.
		}

		const AMainPlayerState* PlayerState = Character->GetPlayerState<AMainPlayerState>();
		UAbilitySystemComponent* ASC = PlayerState ? PlayerState->GetAbilitySystemComponent() : nullptr;
		if (!ASC)
		{
			continue;
		}

		// 대상 자신의 ASC에서 스펙을 만들어 적용한다(PlayerState가 회복 효과를 거는 것과 같은 방식).
		FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(UGE_ItemSkill_I_5_Effect::StaticClass(), 1.0f, ASC->MakeEffectContext());
		if (!SpecHandle.IsValid())
		{
			continue;
		}
		SpecHandle.Data->SetSetByCallerMagnitude(TAG_Data_ItemSkill_I_5_Duration, BuffLingerSeconds);
		SpecHandle.Data->SetSetByCallerMagnitude(TAG_Data_ItemSkill_I_5_MoveSpeedMultiplier, MoveSpeedMultiplier);
		SpecHandle.Data->SetSetByCallerMagnitude(TAG_Data_ItemSkill_I_5_FireRateMultiplier, FireRateMultiplier);
		ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
	}
}
