// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "MainAttributeSet.generated.h"

#define MAIN_ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

// 사망 시 1회만 브로드캐스트된다. 기존 UMainHPComponent::OnDeath와 같은 계약을 유지한다 -
// Killer는 데미지 GameplayEffect의 컨텍스트에서 그대로 전달된 것이라 nullptr일 수 있다.
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnMainDeath, AActor* /*Avatar*/, AController* /*Killer*/);

DECLARE_MULTICAST_DELEGATE(FOnMainMovementAttributesChanged);

UCLASS()
class PROJECTRWW_API UMainAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UMainAttributeSet();

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Health")
	FGameplayAttributeData Health;
	MAIN_ATTRIBUTE_ACCESSORS(UMainAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "Health")
	FGameplayAttributeData MaxHealth;
	MAIN_ATTRIBUTE_ACCESSORS(UMainAttributeSet, MaxHealth)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Mana, Category = "Mana")
	FGameplayAttributeData Mana;
	MAIN_ATTRIBUTE_ACCESSORS(UMainAttributeSet, Mana)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxMana, Category = "Mana")
	FGameplayAttributeData MaxMana;
	MAIN_ATTRIBUTE_ACCESSORS(UMainAttributeSet, MaxMana)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_WalkSpeed, Category = "Movement")
	FGameplayAttributeData WalkSpeed;
	MAIN_ATTRIBUTE_ACCESSORS(UMainAttributeSet, WalkSpeed)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_RunSpeed, Category = "Movement")
	FGameplayAttributeData RunSpeed;
	MAIN_ATTRIBUTE_ACCESSORS(UMainAttributeSet, RunSpeed)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_JumpPower, Category = "Movement")
	FGameplayAttributeData JumpPower;
	MAIN_ATTRIBUTE_ACCESSORS(UMainAttributeSet, JumpPower)

	// 메타 Attribute - 복제되지 않는다. GE_Damage가 여기에 Additive로 값을 넣으면
	// PostGameplayEffectExecute가 즉시 Health로 옮기고 0으로 되돌린다(Lyra/ARPG 샘플과 동일 패턴).
	UPROPERTY(BlueprintReadOnly, Category = "Damage")
	FGameplayAttributeData Damage;
	MAIN_ATTRIBUTE_ACCESSORS(UMainAttributeSet, Damage)

	FOnMainDeath OnDeath;

	// WalkSpeed/RunSpeed/JumpPower가 리플리케이트되어 도착할 때마다 브로드캐스트된다.
	// 클라이언트에서 이 값들이 아직 안 도착한 상태로 CharacterMovementComponent를
	// 동기화했다면(초기 타이밍 레이스), 이 델리게이트가 도착 시점에 재동기화를 트리거해
	// 스스로 회복되게 한다.
	FOnMainMovementAttributesChanged OnMovementAttributesChanged;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

protected:
	UFUNCTION() void OnRep_Health(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_Mana(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_MaxMana(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_WalkSpeed(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_RunSpeed(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_JumpPower(const FGameplayAttributeData& OldValue);
};
