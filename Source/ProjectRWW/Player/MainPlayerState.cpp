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
