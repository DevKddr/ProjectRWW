#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GachaTablesData.h"
#include "GachaManager.generated.h"

/**
 * gacha_tables.json 전용 매니저. 랜덤박스 추첨(2단계 가중치 뽑기)만 책임진다.
 * 뽑힌 결과(카테고리+인덱스)로 실제 아이템 데이터를 조회하는 건 이 매니저의 역할이 아니다 -
 * 호출부가 결과를 받아서 WeaponDataManager(또는 향후 SkinDataManager 등)에 물어봐야 한다.
 *
 * 사용 예:
 *   UGachaManager* GachaMgr = GetGameInstance()->GetSubsystem<UGachaManager>();
 *   FName Category, ItemIndex, RarityId;
 *   if (GachaMgr->DrawFromBox(FName("WeaponBox_Default"), Category, ItemIndex, RarityId))
 *   {
 *       UWeaponDataManager* WeaponMgr = GetGameInstance()->GetSubsystem<UWeaponDataManager>();
 *       FWeaponItem Weapon;
 *       WeaponMgr->GetWeaponData(ItemIndex, Weapon);
 *   }
 */
UCLASS()
class PROJECTRWW_API UGachaManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * 랜덤박스 2단계 가중치 추첨.
	 * 1단계: 그 박스에서 아이템이 있는 등급들을 RarityDropWeight로 하나 선택.
	 * 2단계: 그 등급 풀 안에서 ItemDropWeight(=Weight)로 아이템 하나 선택.
	 * 실패(박스 없음/뽑을 아이템 없음) 시 false.
	 */
	UFUNCTION(BlueprintCallable, Category = "Gacha")
	bool DrawFromBox(FName BoxId, FName& OutCategory, FName& OutItemIndex, FName& OutRarityId) const;

	UFUNCTION(BlueprintCallable, Category = "Gacha")
	bool IsDataLoaded() const { return bDataLoaded; }

private:
	bool LoadGachaTables();

	UPROPERTY()
	bool bDataLoaded = false;

	FGachaTables GachaTables;

	static const FString GachaTablesJsonRelativePath;
};
