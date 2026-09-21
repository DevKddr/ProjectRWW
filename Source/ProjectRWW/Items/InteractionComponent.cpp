// Copyright Epic Games, Inc. All Rights Reserved.

#include "InteractionComponent.h"
#include "ItemPickupActor.h"
#include "MainInventoryComponent.h"
#include "Player/MainPlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "CollisionQueryParams.h"

UInteractionComponent::UInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	// Server_PickupItem RPC를 받으려면 컴포넌트가 리플리케이트돼야 한다.
	SetIsReplicatedByDefault(true);
}

void UInteractionComponent::StartScanning()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(ScanTimerHandle, this, &UInteractionComponent::Scan, ScanIntervalSeconds, true);
	}
}

void UInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ScanTimerHandle);
	}

	// 사망/폰 교체 시 HUD 마커가 남지 않게 마지막으로 "대상 없음"을 알린다.
	SetFocus(nullptr, false);

	Super::EndPlay(EndPlayReason);
}

void UInteractionComponent::Scan()
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (!PC)
	{
		SetFocus(nullptr, false);
		return;
	}

	// 실제 화면 중앙(카메라)에서 쏜다 - 1인칭 카메라가 소켓에 붙어 있어서 액터 위치와 다르다.
	FVector ViewLocation;
	FRotator ViewRotation;
	PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
	const FVector TraceEnd = ViewLocation + ViewRotation.Vector() * InteractionRange;

	// 자기 자신과, 손에 붙은 무기/아이템 액터는 무시한다 - 손에 든 것이 시야를 막아
	// 앞의 픽업이 안 잡히는 일을 막기 위함.
	FCollisionQueryParams Params(SCENE_QUERY_STAT(InteractionScan), false, GetOwner());
	TArray<AActor*> AttachedActors;
	GetOwner()->GetAttachedActors(AttachedActors, true);
	Params.AddIgnoredActors(AttachedActors);

	AItemPickupActor* NewFocus = nullptr;
	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, ViewLocation, TraceEnd, ECC_Visibility, Params))
	{
		NewFocus = Cast<AItemPickupActor>(Hit.GetActor());
	}

	// Slots가 소유 클라이언트에 리플리케이트되므로 클라이언트에서도 정확하다.
	const AMainPlayerController* MainPC = Cast<AMainPlayerController>(PC);
	const bool bHasRoom = MainPC && MainPC->InventoryComponent && MainPC->InventoryComponent->HasEmptySlot();

	SetFocus(NewFocus, NewFocus != nullptr && !bHasRoom);
}

void UInteractionComponent::SetFocus(AItemPickupActor* NewFocus, bool bNewBlocked)
{
	const bool bNewHasFocus = NewFocus != nullptr;
	if (bHasFocus == bNewHasFocus && FocusedPickup.Get() == NewFocus && bFocusBlocked == bNewBlocked)
	{
		return;  // 아무것도 안 바뀌었으면 HUD를 건드리지 않는다
	}

	bHasFocus = bNewHasFocus;
	FocusedPickup = NewFocus;
	bFocusBlocked = bNewBlocked;
	OnFocusChanged.Broadcast(NewFocus, bNewBlocked);
}

AItemPickupActor* UInteractionComponent::GetFocusedPickup() const
{
	return FocusedPickup.Get();
}

void UInteractionComponent::TryInteract()
{
	AItemPickupActor* Target = FocusedPickup.Get();
	if (!Target || bFocusBlocked)
	{
		return;  // 대상이 없거나 인벤토리가 가득 참 - RPC 자체를 보내지 않는다
	}
	Server_PickupItem(Target);
}

void UInteractionComponent::Server_PickupItem_Implementation(AItemPickupActor* Pickup)
{
	// 클라이언트가 보낸 값은 전부 검증한다. 위 TryInteract의 검사는 UX용일 뿐 보안이 아니다.
	const APawn* Pawn = Cast<APawn>(GetOwner());
	AMainPlayerController* PC = Pawn ? Cast<AMainPlayerController>(Pawn->GetController()) : nullptr;
	if (!IsValid(Pickup) || !PC || PC->bIsDead || !PC->InventoryComponent)
	{
		return;
	}

	// 거리: 클라이언트 레이가 구 "표면"에 맞으므로, 표면까지의 거리로 판정한다.
	const FVector Eyes = Pawn->GetPawnViewLocation();
	const FVector Center = Pickup->GetPickupCenter();
	const float DistanceToSurface = FMath::Max(0.0f, FVector::Dist(Eyes, Center) - Pickup->GetPickupRadius());
	if (DistanceToSurface > InteractionRange + ServerRangeTolerance)
	{
		return;
	}

	// 시야: 눈에서 구 중심으로 쏴서 처음 맞는 게 그 픽업이어야 한다(벽 뒤 줍기 방지).
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PickupLineOfSight), false, GetOwner());
	TArray<AActor*> AttachedActors;
	GetOwner()->GetAttachedActors(AttachedActors, true);
	Params.AddIgnoredActors(AttachedActors);

	FHitResult Hit;
	if (!GetWorld()->LineTraceSingleByChannel(Hit, Eyes, Center, ECC_Visibility, Params) || Hit.GetActor() != Pickup)
	{
		return;
	}

	// 가득 참: 클라이언트 값이 오래됐거나 조작됐어도 서버가 최종으로 막는다.
	if (!PC->InventoryComponent->HasEmptySlot())
	{
		return;
	}

	// AddItem이 실패하면(INDEX_NONE) 픽업을 파괴하지 않는다 - 아이템이 사라지는 일이 없게.
	if (PC->InventoryComponent->AddItem(Pickup->ItemIndex, Pickup->CurrentAmmo) == INDEX_NONE)
	{
		return;
	}

	// 두 명이 동시에 눌러도 서버는 한 번에 하나씩 처리한다. 먼저 온 쪽이 Destroy하면
	// 다음 요청의 Pickup은 IsValid 검사에서 걸러진다.
	Pickup->Destroy();
}
