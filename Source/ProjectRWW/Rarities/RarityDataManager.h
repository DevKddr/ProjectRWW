#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RarityData.h"
#include "RarityDataManager.generated.h"

/**
 * rarities.json 전용 매니저. 등급 정의(DropWeight/SellValue/Color 등)만 다룬다.
 * 다른 매니저(WeaponDataManager, GachaManager 등)가 등급 정보가 필요하면
 * 이 매니저를 통해서만 조회한다 - 등급 데이터를 복제해서 들고 있지 않는다.
 */
UCLASS()
class PROJECTRWW_API URarityDataManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "RarityData")
	bool GetRarityData(FName RarityId, FRarityData& OutData) const;

	UFUNCTION(BlueprintCallable, Category = "RarityData")
	TArray<FRarityData> GetAllRarities() const;

	UFUNCTION(BlueprintCallable, Category = "RarityData")
	bool IsDataLoaded() const { return bDataLoaded; }

private:
	bool LoadRarities();

	UPROPERTY()
	bool bDataLoaded = false;

	TMap<FName, FRarityData> RarityMap;

	static const FString RaritiesJsonRelativePath;
};
