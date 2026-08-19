#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GachaData.h"
#include "GachaDataManager.generated.h"

/**
 * rarities.json / weapons.json / gacha_tables.json 을 로드하고,
 * gacha_draw_example.py의 2단계 가중치 추첨 로직을 그대로 구현한 매니저.
 *
 * 사용 예:
 *   UGachaDataManager* Mgr = GetGameInstance()->GetSubsystem<UGachaDataManager>();
 *   FName OutCategory, OutIndex, OutRarity;
 *   if (Mgr->DrawFromBox(FName("WeaponBox_Default"), OutCategory, OutIndex, OutRarity))
 *   {
 *       FWeaponItem Weapon;
 *       Mgr->GetWeaponData(OutIndex, Weapon);
 *   }
 */
UCLASS()
class PROJECTRWW_API UGachaDataManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "GachaData")
	bool GetWeaponData(FName WeaponIndex, FWeaponItem& OutData) const;

	UFUNCTION(BlueprintCallable, Category = "GachaData")
	bool GetRarityData(FName RarityId, FRarityData& OutData) const;

	/** 현재 언어(ko/en)로 무기 이름을 반환. */
	UFUNCTION(BlueprintCallable, Category = "GachaData")
	FText GetWeaponDisplayName(FName WeaponIndex) const;

	/** 현재 언어(ko/en)로 무기 설명을 반환. */
	UFUNCTION(BlueprintCallable, Category = "GachaData")
	FText GetWeaponDescription(FName WeaponIndex) const;

	/** "ko" 또는 "en". 다른 값을 넣으면 무시되고 현재 언어가 유지된다. */
	UFUNCTION(BlueprintCallable, Category = "GachaData")
	void SetLanguage(const FString& LanguageCode);

	/** 등급(Rarity) 기반 판매가. 무기 개별 판매가는 없음. */
	UFUNCTION(BlueprintCallable, Category = "GachaData")
	bool GetWeaponSellValue(FName WeaponIndex, int32& OutSellValue) const;

	/** 완전 히트 시 총 데미지 = Damage x PelletCount. */
	UFUNCTION(BlueprintCallable, Category = "GachaData")
	bool GetWeaponMaxTotalDamage(FName WeaponIndex, float& OutTotalDamage) const;

	/**
	 * 랜덤박스 2단계 가중치 추첨.
	 * 1단계: 그 박스에서 아이템이 있는 등급들을 RarityDropWeight로 하나 선택.
	 * 2단계: 그 등급 풀 안에서 ItemDropWeight(=Weight)로 아이템 하나 선택.
	 * 실패(박스 없음/뽑을 아이템 없음) 시 false.
	 */
	UFUNCTION(BlueprintCallable, Category = "GachaData")
	bool DrawFromBox(FName BoxId, FName& OutCategory, FName& OutItemIndex, FName& OutRarityId) const;

	UFUNCTION(BlueprintCallable, Category = "GachaData")
	bool IsDataLoaded() const { return bDataLoaded; }

private:
	bool LoadRarities();
	bool LoadWeapons();
	bool LoadGachaTables();

	UPROPERTY()
	bool bDataLoaded = false;

	FString CurrentLanguage = TEXT("ko");

	TMap<FName, FWeaponItem> WeaponMap;
	TMap<FName, FRarityData> RarityMap;
	FGachaTables GachaTables;

	static const FString RaritiesJsonRelativePath;
	static const FString WeaponsJsonRelativePath;
	static const FString GachaTablesJsonRelativePath;
};
