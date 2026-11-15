// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemSkill_I_4_Projectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;

// I_4 스킬이 던지는 투사체. 서버에서 스폰되어 클라이언트로 복제되고, 서버와 클라이언트가 같은 처음 속도로 각자
// 포물선을 계산한다(서버가 위치를 매 프레임 보내지 않는다). 무언가에 닿아 멈추면 서버가 FuseSeconds 뒤에
// 닿은 지점을 중심으로 반경 안의 모든 캐릭터(던진 본인 포함)를 바깥쪽으로 날려 보낸다.
// 모양(메시/재질/이펙트)과 중력 배율은 이 클래스를 부모로 한 Blueprint(BP_I_4_Projectile)에서 지정한다.
UCLASS(Blueprintable)
class PROJECTRWW_API AItemSkill_I_4_Projectile : public AActor
{
	GENERATED_BODY()

public:
	AItemSkill_I_4_Projectile();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 서버가 스폰 직후(FinishSpawning 전에) 호출해 처음 속도(cm/s)를 정한다. 방향은 스폰 회전(액터 앞쪽)이다.
	void SetLaunchSpeed(float InSpeed) { LaunchSpeed = InSpeed; }

protected:
	virtual void BeginPlay() override;

	// 바운스가 꺼져 있어서 무언가에 막혀 멈추는 순간 불린다. 서버만 처리한다.
	UFUNCTION()
	void OnProjectileStop(const FHitResult& ImpactResult);

	// 닿은 지점에서 지연 뒤 실제로 폭발한다. 서버만 호출한다.
	void Detonate();

	// 닿은 지점 중심으로 반경 안의 모든 캐릭터(던진 본인 포함)를 바깥쪽으로 날려 보낸다. 서버만 호출한다.
	void ApplyBlast(const FVector& Center);

	// [임시 디버그] 폭발 범위를 모든 화면에 표시한다. 이펙트가 생기면 제거.
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_DebugBlast(FVector_NetQuantize Center, float Radius);

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<USphereComponent> CollisionSphere;

	// 모양은 BP에서 StaticMesh를 지정한다. C++에서는 비워 둔다.
	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	// 처음 속도(cm/s). 충전량마다 달라서 스폰 때 한 번만 복제한다(COND_InitialOnly).
	// 클라이언트의 투사체 컴포넌트는 서버가 정한 InitialSpeed를 알 수 없으므로, 이 값으로 BeginPlay에서 속도를 직접 정한다.
	UPROPERTY(Replicated)
	float LaunchSpeed = 0.0f;

	// 폭발 지연 타이머와 폭발 중심(닿은 지점). 서버만 쓴다.
	FTimerHandle FuseTimerHandle;
	FVector BlastCenter = FVector::ZeroVector;

	// 아무것에도 안 닿고 계속 날아갈 때 서버가 지우는 시간(초). 이 경우 폭발은 없다.
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	float MaxFlightSeconds = 6.0f;

	// 밸런싱용 (임시 초안)
	// 폭발 반경(cm). 0으로 나누지 않도록 최소값을 둔다.
	UPROPERTY(EditDefaultsOnly, Category = "Blast", meta = (ClampMin = "1.0"))
	float BlastRadius = 750.0f;

	// 폭발 중심에서의 밀어내는 속도(cm/s). 충전량과 무관하게 같다.
	UPROPERTY(EditDefaultsOnly, Category = "Blast")
	float MaxPushSpeed = 1500.0f;

	// 반경 끝에서의 세기 비율(중심 대비). 그 사이는 선형으로 줄어든다.
	UPROPERTY(EditDefaultsOnly, Category = "Blast", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float EdgePushRatio = 0.2f;

	// 닿은 뒤 폭발까지의 지연(초). 0이면 닿는 즉시 폭발한다.
	UPROPERTY(EditDefaultsOnly, Category = "Blast", meta = (ClampMin = "0.0"))
	float FuseSeconds = 0.3f;

	// 방향에 더하는 위쪽 성분 - 바닥에 서 있는 사람도 낮게 미끄러지지 않고 떠오르게 한다.
	UPROPERTY(EditDefaultsOnly, Category = "Blast")
	float UpwardBias = 0.35f;
};
