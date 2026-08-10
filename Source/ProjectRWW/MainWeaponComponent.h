// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MainWeaponComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTRWW_API UMainWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMainWeaponComponent();

	// 캐릭터(소유자)가 발사 입력을 받으면 이 함수를 호출한다.
	void StartFire();

protected:
	// 클라이언트 -> 서버 요청: "이 방향으로 쐈다". 실제 판정은 서버가 한다.
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerFire(const FVector_NetQuantize& TraceStart, const FVector_NetQuantizeNormal& TraceDirection);

	// 서버 -> 전체 클라이언트: 판정 결과에 따른 이펙트만 통보.
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayFireEffects(const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& TraceEnd, bool bHit);

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float Damage = 20.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float Range = 10000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float FireIntervalSeconds = 0.15f;

	double LastFireTimeSeconds = 0.0;
};
