// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "MainPlayerState.generated.h"

class UAbilitySystemComponent;
class UMainAttributeSet;

UCLASS()
class PROJECTRWW_API AMainPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AMainPlayerState();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystemComponent; }

	UFUNCTION(BlueprintPure, Category = "GAS")
	UMainAttributeSet* GetMainAttributeSet() const { return AttributeSet; }

	// Health/Mana/이동속도/점프력을 PlayerBaseStat.json 값으로 다시 채운다. 여러 번 불러도
	// 안전하다(단순 대입) - 첫 스폰과 매 리스폰마다 AMainCharacter::PossessedBy()가 호출한다.
	// 서버에서만 호출되어야 한다.
	void ResetStatsToFull();

protected:
	virtual void BeginPlay() override;

	// 상시 회복 GameplayEffect(Health/Mana) 2개를 건다. raid 동안 딱 한 번만 호출되어야
	// 한다 - 두 번 부르면 회복 이펙트가 중복으로 쌓여 회복 속도가 배로 늘어난다.
	void SetupRegenEffects();

	UPROPERTY(VisibleAnywhere, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, Category = "GAS")
	TObjectPtr<UMainAttributeSet> AttributeSet;
};
