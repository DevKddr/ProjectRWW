#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WeaponData.h"
#include "WeaponDataManager.generated.h"

/**
 * weapons.json 전용 매니저. 무기 스탯/이름/설명만 다룬다.
 * 판매가처럼 등급에 종속된 값이 필요할 땐 이 매니저가 직접 계산하지 않고
 * URarityDataManager를 조회해서 위임한다 - 등급 데이터의 유일한 출처를 지키기 위함.
 */
UCLASS()
class PROJECTRWW_API UWeaponDataManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "WeaponData")
	bool GetWeaponData(FName WeaponIndex, FWeaponItem& OutData) const;

	/** 현재 언어(ko/en)로 무기 이름을 반환. */
	UFUNCTION(BlueprintCallable, Category = "WeaponData")
	FText GetWeaponDisplayName(FName WeaponIndex) const;

	/** 현재 언어(ko/en)로 무기 설명을 반환. */
	UFUNCTION(BlueprintCallable, Category = "WeaponData")
	FText GetWeaponDescription(FName WeaponIndex) const;

	/** "ko" 또는 "en". 다른 값을 넣으면 무시되고 현재 언어가 유지된다. */
	UFUNCTION(BlueprintCallable, Category = "WeaponData")
	void SetLanguage(const FString& LanguageCode);

	/** 등급(Rarity) 기반 판매가. RarityDataManager에게 위임해서 조회한다. */
	UFUNCTION(BlueprintCallable, Category = "WeaponData")
	bool GetWeaponSellValue(FName WeaponIndex, int32& OutSellValue) const;

	/** 완전 히트 시 총 데미지 = Damage x PelletCount. */
	UFUNCTION(BlueprintCallable, Category = "WeaponData")
	bool GetWeaponMaxTotalDamage(FName WeaponIndex, float& OutTotalDamage) const;

	UFUNCTION(BlueprintCallable, Category = "WeaponData")
	TArray<FWeaponItem> GetAllWeapons() const;

	UFUNCTION(BlueprintCallable, Category = "WeaponData")
	TArray<FWeaponItem> GetWeaponsByType(FName WeaponType) const;

	UFUNCTION(BlueprintCallable, Category = "WeaponData")
	bool IsDataLoaded() const { return bDataLoaded; }

private:
	bool LoadWeapons();

	UPROPERTY()
	bool bDataLoaded = false;

	FString CurrentLanguage = TEXT("ko");

	TMap<FName, FWeaponItem> WeaponMap;

	static const FString WeaponsJsonRelativePath;
};
