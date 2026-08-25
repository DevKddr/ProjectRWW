// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainCrosshairWidget.h"
#include "Player/MainCharacter.h"
#include "Combat/MainWeaponComponent.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Camera/PlayerCameraManager.h"

void UMainCrosshairWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const AMainCharacter* OwningCharacter = Cast<AMainCharacter>(GetOwningPlayerPawn());
	if (!OwningCharacter)
	{
		return;
	}

	if (const UMainWeaponComponent* WeaponComp = OwningCharacter->WeaponComponent)
	{
		CurrentSpreadDegrees = WeaponComp->GetCurrentSpreadDegrees();
		MaxSpreadDegrees = WeaponComp->GetMaxSpreadDegrees();
		bIsAiming = WeaponComp->IsAiming();
	}

	const APlayerController* PC = GetOwningPlayer();
	if (!PC || !PC->PlayerCameraManager)
	{
		return;
	}

	const float HorizontalFOVRad = FMath::DegreesToRadians(PC->PlayerCameraManager->GetFOVAngle());
	const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(this);
	if (ViewportSize.Y <= 0.0f)
	{
		return;
	}

	// 언리얼 카메라의 FOV는 가로 기준이라, 세로 FOV는 화면 비율로 역산해야 한다.
	const float AspectRatio = ViewportSize.X / ViewportSize.Y;
	const float VerticalFOVRad = 2.0f * FMath::Atan(FMath::Tan(HorizontalFOVRad * 0.5f) / AspectRatio);

	// tan(산포각) / tan(FOV의 절반)은 "화면 절반 크기 대비 얼마나 벗어났는지"의 비율이다.
	// CurrentSpreadDegrees는 이미 BaseSpread가 포함된 실제 산포각이므로 별도 정규화 없이 바로 쓴다.
	const float SpreadRad = FMath::DegreesToRadians(CurrentSpreadDegrees);
	OffsetX = FMath::Tan(SpreadRad) / FMath::Tan(HorizontalFOVRad * 0.5f) * (ViewportSize.X * 0.5f);
	OffsetY = FMath::Tan(SpreadRad) / FMath::Tan(VerticalFOVRad * 0.5f) * (ViewportSize.Y * 0.5f);
}
