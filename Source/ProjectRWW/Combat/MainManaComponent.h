// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MainManaComponent.generated.h"

// 사용 방법:
//   UMainManaComponent* Mana = Character->ManaComponent;
//   if (Mana->ConsumeMana(RequiredAmount))
//   {
//       // 소비 성공 - 장전/스킬 등 진행
//   }
//   else
//   {
//       // 마나 부족 - 실패 처리
//   }
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTRWW_API UMainManaComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMainManaComponent();

	UFUNCTION(BlueprintPure, Category = "Mana")
	float GetCurrentMana() const { return CurrentMana; }

	UFUNCTION(BlueprintPure, Category = "Mana")
	float GetMaxMana() const { return MaxMana; }

	// 마나가 충분하면 차감하고 true, 부족하면 아무것도 안 바꾸고 false.
	// 서버 권위 로직 안에서만 호출되어야 한다 — 클라이언트가 직접 부르면 안 됨.
	UFUNCTION(BlueprintCallable, Category = "Mana")
	bool ConsumeMana(float Amount);

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// CurrentMana가 클라이언트에 복제되어 도착하면 자동 호출된다.
	UFUNCTION()
	void OnRep_CurrentMana();

	// PlayerBaseStat.json의 "MaxMana"와 이름을 맞춘 최대 마나값.
	UPROPERTY(EditDefaultsOnly, Replicated, Category = "Mana")
	float MaxMana = 1000.0f;

	// PlayerBaseStat.json의 "ManaRegen"과 이름을 맞춘 초당 회복량.
	UPROPERTY(EditDefaultsOnly, Category = "Mana")
	float ManaRegen = 10.0f;

	// MaxMana보다 뒤에 선언되어 있어서, 그 초기화된 값을 그대로 가져다 쓸 수 있다.
	UPROPERTY(ReplicatedUsing = OnRep_CurrentMana, VisibleAnywhere, Category = "Mana")
	float CurrentMana = MaxMana;

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
