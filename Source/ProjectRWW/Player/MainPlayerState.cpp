// Copyright Epic Games, Inc. All Rights Reserved.
#include "MainPlayerState.h"
#include "AbilitySystemComponent.h"
#include "GAS/MainAttributeSet.h"
#include "GAS/MainGameplayTags.h"
#include "GAS/GameplayEffects/GE_HealthRegen.h"
#include "GAS/GameplayEffects/GE_ManaRegen.h"
#include "PlayerBaseStat/PlayerStatManager.h"

AMainPlayerState::AMainPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UMainAttributeSet>(TEXT("AttributeSet"));

	// APlayerState의 기본 복제 빈도는 초당 1회라서, 체력(AttributeSet)이 서버에서 바뀌어도 클라이언트 HUD에
	// 최대 1초 늦게 반영된다. GAS의 ASC를 PlayerState에 두는 구조에서는 이 값을 올려 두는 것이 일반적이다.
	// 값이 바뀐 속성만 전송되므로 빈도를 올려도 대역폭 부담은 크지 않다.
	SetNetUpdateFrequency(100.f);
	SetMinNetUpdateFrequency(33.f);
}

void AMainPlayerState::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		SetupRegenEffects();
	}
}

void AMainPlayerState::SetupRegenEffects()
{
	UGameInstance* GameInstance = GetGameInstance();
	UPlayerStatManager* StatManager = GameInstance ? GameInstance->GetSubsystem<UPlayerStatManager>() : nullptr;
	if (!StatManager || !AbilitySystemComponent)
	{
		return;
	}

	const FPlayerBaseStat& Stat = StatManager->GetBaseStat();
	const FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();

	FGameplayEffectSpecHandle HealthRegenSpec = AbilitySystemComponent->MakeOutgoingSpec(UGE_HealthRegen::StaticClass(), 1.0f, Context);
	HealthRegenSpec.Data->SetSetByCallerMagnitude(MainGameplayTags::Data_HPRegen.GetTag(), Stat.HPRegen);
	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*HealthRegenSpec.Data);

	FGameplayEffectSpecHandle ManaRegenSpec = AbilitySystemComponent->MakeOutgoingSpec(UGE_ManaRegen::StaticClass(), 1.0f, Context);
	ManaRegenSpec.Data->SetSetByCallerMagnitude(MainGameplayTags::Data_ManaRegen.GetTag(), Stat.ManaRegen);
	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*ManaRegenSpec.Data);
}

void AMainPlayerState::ResetStatsToFull()
{
	UGameInstance* GameInstance = GetGameInstance();
	UPlayerStatManager* StatManager = GameInstance ? GameInstance->GetSubsystem<UPlayerStatManager>() : nullptr;
	if (!StatManager || !AttributeSet)
	{
		return;
	}

	const FPlayerBaseStat& Stat = StatManager->GetBaseStat();

	AttributeSet->InitMaxHealth(Stat.MaxHP);
	AttributeSet->InitHealth(Stat.MaxHP);
	AttributeSet->InitMaxMana(Stat.MaxMana);
	AttributeSet->InitMana(Stat.MaxMana);
	AttributeSet->InitWalkSpeed(Stat.WalkSpeed);
	AttributeSet->InitRunSpeed(Stat.RunSpeed);
	AttributeSet->InitJumpPower(Stat.JumpPower);
}
