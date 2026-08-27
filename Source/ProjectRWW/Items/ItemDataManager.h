// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ItemData.h"
#include "ItemDataManager.generated.h"

/**
 * items.json 전용 매니저. 무기를 포함한 모든 아이템의 표시/시각 데이터(이름, 설명,
 * 레어도, 메쉬)만 다룬다. 게임플레이 스탯은 WeaponDataManager 등 별도 매니저의 책임이다.
 */
UCLASS()
class PROJECTRWW_API UItemDataManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "ItemData")
	bool GetItemData(FName Index, FItemData& OutData) const;

	UFUNCTION(BlueprintCallable, Category = "ItemData")
	bool IsDataLoaded() const { return bDataLoaded; }

private:
	bool LoadItems();

	UPROPERTY()
	bool bDataLoaded = false;

	TMap<FName, FItemData> ItemMap;

	static const FString ItemsJsonRelativePath;
};
