// Copyright Epic Games, Inc. All Rights Reserved.

#include "ItemPickupActor.h"
#include "ItemDataManager.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

AItemPickupActor::AItemPickupActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(SceneRoot);
	// 메시는 보이기만 한다. 판정은 PickupSphere가 전담한다.
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
	PickupSphere->SetupAttachment(SceneRoot);
	PickupSphere->InitSphereRadius(50.0f);  // 기본값. 아이템별 BP에서 조절한다.
	// Visibility(카메라 시야 트레이스)만 막고 나머지는 전부 무시한다. 총알은 ECC_Pawn을
	// 쓰므로 이 구가 총알을 가로채지 않고, 플레이어가 걸려 넘어지지도 않는다. 벽 뒤
	// 아이템은 벽이 먼저 Visibility를 막아서 자동으로 안 잡힌다.
	PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupSphere->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}

void AItemPickupActor::BeginPlay()
{
	Super::BeginPlay();

	// BP의 ItemIndex와 메시가 서로 안 맞게 들어가는 실수는 코드로 막을 수 없지만, 최소한
	// "items.json에 없는 Index를 넣었거나 비워둔" 실수는 서버 시작 로그에서 바로 잡는다.
	if (HasAuthority())
	{
		const UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
		const UItemDataManager* ItemDataManager = GameInstance ? GameInstance->GetSubsystem<UItemDataManager>() : nullptr;
		FItemData Unused;
		if (ItemDataManager && !ItemDataManager->GetItemData(ItemIndex, Unused))
		{
			UE_LOG(LogTemp, Warning, TEXT("[ProjectRWW][픽업] %s: items.json에 없는 ItemIndex '%s'"), *GetName(), *ItemIndex.ToString());
		}
	}
}

FVector AItemPickupActor::GetPickupCenter() const
{
	return PickupSphere->GetComponentLocation();
}

float AItemPickupActor::GetPickupRadius() const
{
	return PickupSphere->GetScaledSphereRadius();
}
