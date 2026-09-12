// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainHUDWidget.h"
#include "Player/MainCharacter.h"
#include "GAS/MainAttributeSet.h"
#include "Combat/MainWeaponComponent.h"
#include "Player/MainPlayerController.h"

void UMainHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const AMainCharacter* OwningCharacter = Cast<AMainCharacter>(GetOwningPlayerPawn());
	if (!OwningCharacter)
	{
		return;
	}

	if (const UMainAttributeSet* AttrSet = OwningCharacter->GetMainAttributeSet())
	{
		CurrentHP = AttrSet->GetHealth();
		MaxHP = AttrSet->GetMaxHealth();
		CurrentMana = AttrSet->GetMana();
		MaxMana = AttrSet->GetMaxMana();
	}

	if (const UMainWeaponComponent* WeaponComp = OwningCharacter->WeaponComponent)
	{
		CurrentAmmo = WeaponComp->GetCurrentAmmo();
		MagazineSize = WeaponComp->GetMagazineSize();
		bIsReloading = WeaponComp->IsReloading();
		ReloadProgress = WeaponComp->GetReloadProgress();
		WeaponIndex = WeaponComp->GetWeaponIndex();
		CurrentSpreadDegrees = WeaponComp->GetCurrentSpreadDegrees();
		MaxSpreadDegrees = WeaponComp->GetMaxSpreadDegrees();
		bIsAiming = WeaponComp->IsAiming();
	}

	if (const AMainPlayerController* PC = Cast<AMainPlayerController>(GetOwningPlayer()))
	{
		bIsExtracting = PC->bIsExtracting;
		ExtractionDuration = PC->ExtractionDuration;
		ExtractionStartTimeSeconds = PC->ExtractionStartTimeSeconds;
	}
}
