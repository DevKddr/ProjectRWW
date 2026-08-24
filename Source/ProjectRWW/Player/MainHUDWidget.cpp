// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainHUDWidget.h"
#include "Player/MainCharacter.h"
#include "Combat/MainHPComponent.h"
#include "Combat/MainManaComponent.h"
#include "Combat/MainWeaponComponent.h"

void UMainHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const AMainCharacter* OwningCharacter = Cast<AMainCharacter>(GetOwningPlayerPawn());
	if (!OwningCharacter)
	{
		return;
	}

	if (const UMainHPComponent* HPComp = OwningCharacter->HPComponent)
	{
		CurrentHP = HPComp->GetCurrentHP();
		MaxHP = HPComp->GetMaxHP();
	}

	if (const UMainManaComponent* ManaComp = OwningCharacter->ManaComponent)
	{
		CurrentMana = ManaComp->GetCurrentMana();
		MaxMana = ManaComp->GetMaxMana();
	}

	if (const UMainWeaponComponent* WeaponComp = OwningCharacter->WeaponComponent)
	{
		CurrentAmmo = WeaponComp->GetCurrentAmmo();
		MagazineSize = WeaponComp->GetMagazineSize();
		bIsReloading = WeaponComp->IsReloading();
		ReloadProgress = WeaponComp->GetReloadProgress();
	}
}
