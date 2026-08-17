// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainMapWidget.h"
#include "MainMapMarkerSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"

void UMainMapWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RefreshMarkers();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(MarkerRefreshTimerHandle, this, &UMainMapWidget::RefreshMarkers, MarkerRefreshIntervalSeconds, true);
	}
}

void UMainMapWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MarkerRefreshTimerHandle);
	}

	Super::NativeDestruct();
}

void UMainMapWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (const APawn* OwningPawn = GetOwningPlayerPawn())
	{
		PlayerWorldLocation = OwningPawn->GetActorLocation();
		PlayerMapPosition = WorldToMapUV(PlayerWorldLocation);
		PlayerMapRotationDegrees = OwningPawn->GetActorRotation().Yaw;
	}
}

FVector2D UMainMapWidget::WorldToMapUV(const FVector& WorldLocation) const
{
	// WorldMinX/MaxX(또는 Y)가 실수로 같은 값으로 설정되면 0으로 나누게 되어 NaN이 발생한다.
	const float RangeX = WorldMaxX - WorldMinX;
	const float RangeY = WorldMaxY - WorldMinY;

	const float U = FMath::IsNearlyZero(RangeX) ? 0.0f : (WorldLocation.X - WorldMinX) / RangeX;
	const float V = FMath::IsNearlyZero(RangeY) ? 0.0f : (WorldLocation.Y - WorldMinY) / RangeY;
	return FVector2D(FMath::Clamp(U, 0.0f, 1.0f), FMath::Clamp(V, 0.0f, 1.0f));
}

void UMainMapWidget::RefreshMarkers()
{
	Markers.Reset();

	UGameInstance* GameInstance = GetGameInstance();
	UMainMapMarkerSubsystem* Subsystem = GameInstance ? GameInstance->GetSubsystem<UMainMapMarkerSubsystem>() : nullptr;
	if (!Subsystem)
	{
		return;
	}

	for (const TWeakObjectPtr<UMainMapMarkerComponent>& WeakMarker : Subsystem->GetMarkers())
	{
		UMainMapMarkerComponent* Marker = WeakMarker.Get();
		if (!Marker || !Marker->bVisibleOnMap || !Marker->GetOwner())
		{
			continue;
		}

		FMainMapMarkerViewData ViewData;
		ViewData.MapPosition = WorldToMapUV(Marker->GetOwner()->GetActorLocation());
		ViewData.MarkerType = Marker->MarkerType;
		ViewData.DisplayName = Marker->DisplayName;
		ViewData.IconTint = Marker->IconTint;

		Markers.Add(ViewData);
	}

	OnMapDataRefreshed();
}
