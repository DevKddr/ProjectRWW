// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainMapMarkerComponent.h"
#include "MainMapMarkerSubsystem.h"
#include "Net/UnrealNetwork.h"

UMainMapMarkerComponent::UMainMapMarkerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UMainMapMarkerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		if (UMainMapMarkerSubsystem* Subsystem = GameInstance->GetSubsystem<UMainMapMarkerSubsystem>())
		{
			Subsystem->AddMarker(this);
		}
	}
}

void UMainMapMarkerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		if (UMainMapMarkerSubsystem* Subsystem = GameInstance->GetSubsystem<UMainMapMarkerSubsystem>())
		{
			Subsystem->RemoveMarker(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void UMainMapMarkerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UMainMapMarkerComponent, MarkerType);
	DOREPLIFETIME(UMainMapMarkerComponent, DisplayName);
	DOREPLIFETIME(UMainMapMarkerComponent, IconTint);
	DOREPLIFETIME(UMainMapMarkerComponent, Radius);
	DOREPLIFETIME(UMainMapMarkerComponent, bVisibleOnMap);
}
