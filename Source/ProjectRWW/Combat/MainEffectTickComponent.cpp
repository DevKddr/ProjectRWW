// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainEffectTickComponent.h"
#include "GameFramework/Actor.h"

UMainEffectTickComponent::UMainEffectTickComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UMainEffectTickComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	const double CurrentTimeSeconds = GetWorld()->GetTimeSeconds();
	if (CurrentTimeSeconds - LastTickTimeSeconds < EffectTickInterval)
	{
		return;
	}

	LastTickTimeSeconds = CurrentTimeSeconds;
	OnEffectTick.Broadcast();
}
