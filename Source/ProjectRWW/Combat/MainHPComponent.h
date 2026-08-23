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

	// MainEffectTickComponent의 OnEffectTick에 등록되는 콜백. MainCharacter가 연결한다.
	UFUNCTION()
	void OnEffectTick();

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

	// PlayerBaseStat.json의 "HPRegen"과 이름을 맞춘 1회 회복량.
	UPROPERTY(EditDefaultsOnly, Category = "HP")
	float HPRegen = 0.0f;

	// PlayerBaseStat.json의 "HPRegenDelay"와 이름을 맞춘, 피격 후 회복이 재개되기까지의 지연시간(초).
	UPROPERTY(EditDefaultsOnly, Category = "HP")
	float HPRegenDelay = 0.0f;

	// MaxHP보다 뒤에 선언되어 있어서, 그 초기화된 값을 그대로 가져다 쓸 수 있다.
	UPROPERTY(ReplicatedUsing = OnRep_CurrentHP, VisibleAnywhere, Category = "HP")
	float CurrentHP = MaxHP;

	// 마지막으로 데미지를 받은 서버 월드 시각. 회복 재개 여부 판단에만 쓰는 서버 로컬 값이라 복제하지 않는다.
	double LastDamageTimeSeconds = 0.0;

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
