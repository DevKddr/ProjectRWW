// Copyright Epic Games, Inc. All Rights Reserved.

#include "ItemSkill_I_4_Projectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Engine/OverlapResult.h"
#include "Net/UnrealNetwork.h"
#include "DrawDebugHelpers.h"
#include "Player/MainPlayerController.h"

AItemSkill_I_4_Projectile::AItemSkill_I_4_Projectile()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	// 서버의 위치/회전 보정을 클라이언트에 보낸다. 주의: 처음 속도는 이 설정으로 전달되지 않는다(LaunchSpeed로 전달).
	SetReplicateMovement(true);
	// 클라이언트가 스스로 포물선을 계산하므로 서버 보정은 가끔만 보내도 된다.
	SetNetUpdateFrequency(20.0f);

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(12.0f);
	// 모든 것에 막힌다(바닥/벽/캐릭터). 알려진 부작용: 총알 트레이스도 ECC_Pawn 채널이라 날아가는 투사체에
	// 맞을 수 있다(작고 짧게 나는 물체라 일단 허용).
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Block);
	SetRootComponent(CollisionSphere);

	// 보이기만 한다. 메시/재질/크기는 BP에서 정한다.
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(CollisionSphere);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	// 닿으면 첫 충돌에서 멈추고 OnProjectileStop이 불린다(폭발 발동의 전제라 코드에서 고정한다).
	ProjectileMovement->bShouldBounce = false;
	// 중력은 엔진 기본값(1.0)을 쓰고, 배율 조절은 BP의 ProjectileMovement 컴포넌트에서 한다.
	ProjectileMovement->OnProjectileStop.AddDynamic(this, &AItemSkill_I_4_Projectile::OnProjectileStop);
}

void AItemSkill_I_4_Projectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AItemSkill_I_4_Projectile, LaunchSpeed, COND_InitialOnly);
}

void AItemSkill_I_4_Projectile::BeginPlay()
{
	Super::BeginPlay();

	// 서버와 클라이언트가 같은 값으로 같은 포물선을 각자 계산한다. 방향은 스폰 회전(액터 앞쪽)이다.
	// 클라이언트는 최초 복제 데이터(LaunchSpeed)를 받은 뒤에 BeginPlay가 불린다.
	ProjectileMovement->Velocity = GetActorForwardVector() * LaunchSpeed;

	// 던진 본인 캡슐에 발사 직후 걸려 멈추지 않도록 무시한다. 이건 투사체의 이동 충돌만 막는 것이고,
	// 폭발(ApplyBlast)은 본인도 대상에 포함한다.
	if (APawn* ThrowerPawn = GetInstigator())
	{
		CollisionSphere->IgnoreActorWhenMoving(ThrowerPawn, true);
	}

	// 수명은 서버에서만 건다. 서버가 Destroy하면 클라이언트 복사본도 같이 사라진다.
	if (HasAuthority())
	{
		SetLifeSpan(MaxFlightSeconds);
	}
}

void AItemSkill_I_4_Projectile::OnProjectileStop(const FHitResult& ImpactResult)
{
	// 멈추는 순간 모양을 없애고 충돌을 끈다 - 폭발 전 지연 동안 사람이 걸리거나 총알이 맞지 않게 한다.
	// 폭발 타이머를 이 액터가 들고 있어서 파괴하지 않고 보이지 않는 채로 남긴다.
	// 충돌 상태는 복제되지 않으므로, 서버와 클라이언트가 각자 멈추는 순간에 직접 처리한다.
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);

	// 폭발은 서버만 한다.
	if (!HasAuthority())
	{
		return;
	}

	// 무엇에 닿았든(바닥/벽/천장/캐릭터) 닿은 지점을 기억해 두고 FuseSeconds 뒤에 폭발한다.
	// 그동안 투사체는 닿은 자리에 멈춰 보인다.
	BlastCenter = ImpactResult.ImpactPoint;

	// 폭발 뒤 정리까지 1초를 남긴다 - 폭발 전에 MaxFlightSeconds 수명이 다해 사라지지 않게 하고,
	// 폭발 연출(멀티캐스트)이 전달될 시간도 준다.
	SetLifeSpan(FuseSeconds + 1.0f);

	if (FuseSeconds > 0.0f)
	{
		GetWorldTimerManager().SetTimer(FuseTimerHandle, this, &AItemSkill_I_4_Projectile::Detonate, FuseSeconds, false);
	}
	else
	{
		Detonate();
	}
}

void AItemSkill_I_4_Projectile::Detonate()
{
	ApplyBlast(BlastCenter);
	Multicast_DebugBlast(BlastCenter, BlastRadius);
	// 액터 자체는 남은 수명(1초)이 지나면 사라진다.
}

void AItemSkill_I_4_Projectile::ApplyBlast(const FVector& Center)
{
	// ECC_Pawn 오브젝트 타입으로 찾는다. 캐릭터는 캡슐뿐 아니라 메시(피격용 물리 바디)도 Pawn 타입일 수 있어
	// 한 캐릭터가 여러 번 잡힐 수 있다. 아래 Pushed로 중복을 거르니 이 처리를 지우면 안 된다.
	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(BlastRadius));

	TSet<ACharacter*> Pushed;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		ACharacter* Character = Cast<ACharacter>(Overlap.GetActor());
		if (!Character)
		{
			continue;
		}

		bool bAlreadyPushed = false;
		Pushed.Add(Character, &bAlreadyPushed);
		if (bAlreadyPushed)
		{
			continue;
		}

		// 죽은(또는 컨트롤러가 없는) 캐릭터는 밀지 않는다.
		const AMainPlayerController* Controller = Cast<AMainPlayerController>(Character->GetController());
		if (!Controller || Controller->bIsDead)
		{
			continue;
		}

		// 방향 = 폭발 중심 → 캐릭터. 거리는 한 번만 구한다. 중심과 정확히 겹쳤을 땐 위로 민다.
		const FVector Offset = Character->GetActorLocation() - Center;
		const float Distance = Offset.Size();
		FVector Direction = FMath::IsNearlyZero(Distance) ? FVector::UpVector : Offset / Distance;
		Direction.Z += UpwardBias;
		Direction = Direction.GetSafeNormal();

		// 세기는 중심에서 가까울수록 크고 반경 끝에서 EdgePushRatio배까지 선형으로 줄어든다.
		const float Alpha = FMath::Min(Distance / BlastRadius, 1.0f);
		const float PushSpeed = FMath::Lerp(MaxPushSpeed, MaxPushSpeed * EdgePushRatio, Alpha);

		// XY/Z 모두 덮어쓴다 - 달리던 속도나 낙하 속도와 섞이지 않고 항상 같은 세기로 밀려난다.
		Character->LaunchCharacter(Direction * PushSpeed, true, true);
	}
}

void AItemSkill_I_4_Projectile::Multicast_DebugBlast_Implementation(FVector_NetQuantize Center, float Radius)
{
#if !UE_BUILD_SHIPPING
	DrawDebugSphere(GetWorld(), Center, Radius, 24, FColor::Orange, false, 2.0f);
#endif
}
