// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainMapMarkerSubsystem.h"
#include "MainMapMarkerComponent.h"

void UMainMapMarkerSubsystem::AddMarker(UMainMapMarkerComponent* Marker)
{
	if (Marker)
	{
		Markers.AddUnique(Marker);
	}
}

void UMainMapMarkerSubsystem::RemoveMarker(UMainMapMarkerComponent* Marker)
{
	Markers.RemoveSingleSwap(Marker);
}
