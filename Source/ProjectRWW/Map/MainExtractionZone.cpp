// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainExtractionZone.h"
#include "MainMapMarkerComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Player/MainCharacter.h"
#include "Player/MainPlayerController.h"
#include "Core/MainGameMode.h"

AMainExtractionZone::AMainExtractionZone()
{
	bReplicates = true;

	CollisionMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CollisionMesh"));
	CollisionMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionMesh->SetGenerateOverlapEvents(true);
	RootComponent = CollisionMesh;

	MarkerComponent = CreateDefaultSubobject<UMainMapMarkerComponent>(TEXT("MarkerComponent"));
	MarkerComponent->MarkerType = EMainMapMarkerType::Extraction;

	CollisionMesh->OnComponentBeginOverlap.AddDynamic(this, &AMainExtractionZone::OnZoneBeginOverlap);
	CollisionMesh->OnComponentEndOverlap.AddDynamic(this, &AMainExtractionZone::OnZoneEndOverlap);
}

void AMainExtractionZone::OnZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority())
	{
		return;
	}

	const AMainCharacter* Character = Cast<AMainCharacter>(OtherActor);
	AMainPlayerController* Controller = Character ? Cast<AMainPlayerController>(Character->GetController()) : nullptr;
	if (!Controller || Controller->bIsDead)
	{
		return;
	}

	Controller->bIsExtracting = true;
	Controller->ExtractionDuration = ExtractionDuration;

	FTimerHandle NewTimer;
	TWeakObjectPtr<AMainPlayerController> WeakController(Controller);
	GetWorld()->GetTimerManager().SetTimer(NewTimer, FTimerDelegate::CreateUObject(this, &AMainExtractionZone::CompleteExtraction, WeakController), ExtractionDuration, false);
	ActiveTimers.Add(WeakController, NewTimer);
}

void AMainExtractionZone::OnZoneEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!HasAuthority())
	{
		return;
	}

	const AMainCharacter* Character = Cast<AMainCharacter>(OtherActor);
	AMainPlayerController* Controller = Character ? Cast<AMainPlayerController>(Character->GetController()) : nullptr;
	if (!Controller)
	{
		return;
	}

	TWeakObjectPtr<AMainPlayerController> WeakController(Controller);
	if (FTimerHandle* Timer = ActiveTimers.Find(WeakController))
	{
		GetWorld()->GetTimerManager().ClearTimer(*Timer);
		ActiveTimers.Remove(WeakController);
	}

	Controller->bIsExtracting = false;
}

void AMainExtractionZone::CompleteExtraction(TWeakObjectPtr<AMainPlayerController> WeakController)
{
	ActiveTimers.Remove(WeakController);

	AMainPlayerController* Controller = WeakController.Get();
	if (!Controller || !Controller->bIsExtracting)
	{
		return;
	}

	if (AMainGameMode* GameMode = GetWorld()->GetAuthGameMode<AMainGameMode>())
	{
		GameMode->HandleExtraction(Controller);
	}
}
