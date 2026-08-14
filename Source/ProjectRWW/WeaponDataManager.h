#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WeaponData.h"
#include "WeaponDataManager.generated.h"

/**
 * 게임 인스턴스 수명 동안 유지되는 무기 데이터 매니저.
 * Weapon.json / WeaponTypeData.json / Rarity.json을 로드해 조회 API를 제공한다.
 *
 * 접근 예시:
 *   UWeaponDataManager* Mgr = GetGameInstance()->GetSubsystem<UWeaponDataManager>();
 *   FText DisplayName = Mgr->GetWeaponDisplayName(FName("AR_1")); // 현재 언어로 자동 변환됨
 *   int32 SellPrice;
 *   Mgr->GetWeaponSellValue(FName("AR_2"), SellPrice); // Rarity 기반 판매가
 */
UCLASS()
class PROJECTRWW_API UWeaponDataManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "WeaponData")
	bool GetWeaponData(FName WeaponIndex, FWeaponData& OutData) const;

	UFUNCTION(BlueprintCallable, Category = "WeaponData")
	bool GetWeaponTypeData(FName TypeID, FWeaponTypeData& OutData) const;

	UFUNCTION(BlueprintCallable, Category = "WeaponData")
	bool GetRarityData(FName RarityID, FRarityData& OutData) const;

	/** Weapon.Rarity를 조회해서 그 등급의 SellValue를 반환한다 (무기 개별 판매가는 없음). */
	UFUNCTION(BlueprintCallable, Category = "WeaponData")
	bool GetWeaponSellValue(FName WeaponIndex, int32& OutSellValue) const;

	/** 완전 히트 시 총 데미지 = Damage x PelletCount (일반 무기는 PelletCount=1이라 Damage와 동일). */
	UFUNCTION(BlueprintCallable, Category = "WeaponData")
	bool GetWeaponMaxTotalDamage(FName WeaponIndex, float& OutTotalDamage) const;

	/** WeaponData.NameKey를 ULocalizationManager로 넘겨 현재 언어의 표시 이름을 바로 반환. */
	UFUNCTION(BlueprintCallable, Category = "WeaponData")
	FText GetWeaponDisplayName(FName WeaponIndex) const;

	/** WeaponData.DescKey를 ULocalizationManager로 넘겨 현재 언어의 설명 텍스트를 바로 반환. */
	UFUNCTION(BlueprintCallable, Category = "WeaponData")
	FText GetWeaponDescription(FName WeaponIndex) const;

	UFUNCTION(BlueprintCallable, Category = "WeaponData")
	TArray<FWeaponData> GetAllWeapons() const;

	UFUNCTION(BlueprintCallable, Category = "WeaponData")
	TArray<FWeaponData> GetWeaponsByType(FName TypeID) const;

	UFUNCTION(BlueprintCallable, Category = "WeaponData")
	bool IsDataLoaded() const { return bDataLoaded; }

private:
	bool LoadWeaponTypeData();
	bool LoadRarityData();
	bool LoadWeaponData();

	UPROPERTY()
	bool bDataLoaded = false;

	TMap<FName, FWeaponData> WeaponMap;
	TMap<FName, FWeaponTypeData> WeaponTypeMap;
	TMap<FName, FRarityData> RarityMap;

	static const FString WeaponJsonRelativePath;
	static const FString WeaponTypeJsonRelativePath;
	static const FString RarityJsonRelativePath;
};
