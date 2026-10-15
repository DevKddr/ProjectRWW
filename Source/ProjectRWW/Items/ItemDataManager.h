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

	// 로드된 모든 아이템을 복사해 돌려준다(순서 없음). 루트 테이블 추첨처럼 "후보 전체"가 필요할 때 쓴다.
	void GetAllItems(TArray<FItemData>& OutItems) const;

	// 아이템 이름/설명을 "현재 언어(culture)" 기준 FText로 돌려준다. 문장은 StringTable(ST_Items)에서 읽고 번역은
	// Localization Dashboard가 관리하므로, UI는 이 함수만 호출하면 언어를 바꿀 때 자동으로 따라간다.
	// 아이템이 없거나 키가 비어 있으면 빈 텍스트를 돌려준다.
	UFUNCTION(BlueprintPure, Category = "ItemData")
	FText GetItemNameText(FName Index) const;

	UFUNCTION(BlueprintPure, Category = "ItemData")
	FText GetItemDescriptionText(FName Index) const;

private:
	bool LoadItems();

	UPROPERTY()
	bool bDataLoaded = false;

	TMap<FName, FItemData> ItemMap;

	static const FString ItemsJsonRelativePath;

	// 아이템 이름/설명 문장이 들어 있는 StringTable 에셋 경로(ST_Items.csv를 임포트해서 만든다).
	static const FName ItemStringTableId;
};
