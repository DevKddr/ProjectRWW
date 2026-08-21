// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MainHealthComponent.generated.h"

// 사망 시 1회만 브로드캐스트된다. Killer는 OnTakeAnyDamage의 InstigatedBy를 그대로 전달한 것 —
// 자살/환경 데미지 등으로 죽으면 nullptr일 수 있다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMainOnDeath, AController*, Killer);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTRWW_API UMainHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMainHealthComponent();

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHp() const { return CurrentHp; }

	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsDead() const { return CurrentHp <= 0.0f; }

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FMainOnDeath OnDeath;

protected:
	virtual void BeginPlay() override;

	// AActor::OnTakeAnyDamage 델리게이트에 등록되는 콜백. 서버에서만 등록한다(HasAuthority 체크).
	UFUNCTION()
	void OnTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

	// CurrentHp가 클라이언트에 복제되어 도착하면 자동 호출된다.
	UFUNCTION()
	void OnRep_CurrentHp();

	// PlayerBaseStat.json의 "Hp"와 이름을 맞춘 최대 체력값.
	UPROPERTY(EditDefaultsOnly, Category = "Health")
	float Hp = 100.0f;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentHp, VisibleAnywhere, Category = "Health")
	float CurrentHp = 0.0f;

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
