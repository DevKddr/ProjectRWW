#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WeaponData.h"
#include "WeaponDataManager.generated.h"

struct IConsoleCommand;

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

	/**
	 * 장전 시 필요한 마나 계산.
	 * ReqReloadMana = WeaponReqMana - (MaxAmmo - CurAmmo) x ManaPerAmmo
	 * (MaxAmmo = Stats.MagazineSize. CurAmmo는 런타임 상태값이라 무기 액터가 들고 있다가 넘겨줘야 함)
	 * CanReload=FALSE인 무기에 대해 호출하면 false를 반환한다 (장전 자체가 불가능하므로).
	 */
	UFUNCTION(BlueprintCallable, Category = "WeaponData")
	bool GetRequiredReloadMana(FName WeaponIndex, int32 CurAmmo, float& OutRequiredMana) const;

	UFUNCTION(BlueprintCallable, Category = "WeaponData")
	TArray<FWeaponItem> GetAllWeapons() const;

	UFUNCTION(BlueprintCallable, Category = "WeaponData")
	TArray<FWeaponItem> GetWeaponsByType(FName WeaponType) const;

	UFUNCTION(BlueprintCallable, Category = "WeaponData")
	bool IsDataLoaded() const { return bDataLoaded; }

	/** Output Log에 로드된 모든 무기의 핵심 스탯을 한 줄씩 출력. 파싱 검증용. */
	UFUNCTION(BlueprintCallable, Category = "WeaponData|Debug")
	void DebugPrintAllWeapons() const;

private:
	bool LoadWeapons();

	UPROPERTY()
	bool bDataLoaded = false;

	FString CurrentLanguage = TEXT("ko");

	TMap<FName, FWeaponItem> WeaponMap;

	static const FString WeaponsJsonRelativePath;

	// 콘솔 명령어("WeaponData.PrintAll") 등록/해제용 핸들.
	static IConsoleCommand* DebugPrintWeaponsCommand;

	// 지금 살아있는 인스턴스 수. GameInstanceSubsystem이라 PIE 멀티플레이어 테스트 시
	// 여러 인스턴스가 동시에 존재할 수 있는데, 콘솔 명령어는 프로세스 전역 자원이라
	// 첫 인스턴스가 생길 때만 등록하고 마지막 인스턴스가 사라질 때만 해제해야 한다.
	static int32 ActiveInstanceCount;
};
