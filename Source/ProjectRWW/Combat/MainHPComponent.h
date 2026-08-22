// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MainHPComponent.generated.h"

// 사망 시 1회만 브로드캐스트된다. Killer는 OnTakeAnyDamage의 InstigatedBy를 그대로 전달한 것 —
// 자살/환경 데미지 등으로 죽으면 nullptr일 수 있다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMainOnDeath, AController*, Killer);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTRWW_API UMainHPComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMainHPComponent();

	UFUNCTION(BlueprintPure, Category = "HP")
	float GetCurrentHP() const { return CurrentHP; }

	UFUNCTION(BlueprintPure, Category = "HP")
	float GetMaxHP() const { return MaxHP; }

	UFUNCTION(BlueprintPure, Category = "HP")
	bool IsDead() const { return CurrentHP <= 0.0f; }

	UPROPERTY(BlueprintAssignable, Category = "HP")
	FMainOnDeath OnDeath;

protected:
	virtual void BeginPlay() override;

	// AActor::OnTakeAnyDamage 델리게이트에 등록되는 콜백. 서버에서만 등록한다(HasAuthority 체크).
	UFUNCTION()
	void OnTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

	// CurrentHP가 클라이언트에 복제되어 도착하면 자동 호출된다.
	UFUNCTION()
	void OnRep_CurrentHP();

	// PlayerBaseStat.json의 "MaxHP"와 이름을 맞춘 최대 체력값.
	UPROPERTY(EditDefaultsOnly, Replicated, Category = "HP")
	float MaxHP = 100.0f;

	// MaxHP보다 뒤에 선언되어 있어서, 그 초기화된 값을 그대로 가져다 쓸 수 있다.
	UPROPERTY(ReplicatedUsing = OnRep_CurrentHP, VisibleAnywhere, Category = "HP")
	float CurrentHP = MaxHP;

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
