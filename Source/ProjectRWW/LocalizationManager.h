#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LocalizationManager.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnLanguageChanged);

/**
 * Localization_{lang}.json (예: Localization_ko.json)을 로드해서
 * NameKey/DescKey 같은 키로 실제 표시 문자열을 조회하는 매니저.
 *
 * Weapon.json 등의 데이터는 텍스트를 직접 담지 않고 키만 담고 있으므로,
 * 실제 UI 표시 문자열은 항상 이 매니저를 거쳐서 얻는다.
 *
 * 사용 예:
 *   ULocalizationManager* Loc = GetGameInstance()->GetSubsystem<ULocalizationManager>();
 *   FText Name = Loc->GetLocalizedText(WeaponData.NameKey);
 */
UCLASS()
class PROJECTRWW_API ULocalizationManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** 키로 현재 언어의 텍스트를 조회. 키가 없으면 "[Key]" 형태로 반환(눈에 띄게 하기 위함). */
	UFUNCTION(BlueprintCallable, Category = "Localization")
	FText GetLocalizedText(FName Key) const;

	/** 언어를 전환하고 해당 Localization_{lang}.json을 다시 로드한다. 예: "ko", "en" */
	UFUNCTION(BlueprintCallable, Category = "Localization")
	bool SetLanguage(const FString& LanguageCode);

	UFUNCTION(BlueprintCallable, Category = "Localization")
	FString GetCurrentLanguage() const { return CurrentLanguage; }

	/** 언어가 바뀔 때마다 UI가 텍스트를 다시 갱신할 수 있도록 알림. */
	FOnLanguageChanged OnLanguageChanged;

	UFUNCTION(BlueprintCallable, Category = "Localization")
	bool IsDataLoaded() const { return bDataLoaded; }

private:
	bool LoadLanguageFile(const FString& LanguageCode);

	UPROPERTY()
	bool bDataLoaded = false;

	FString CurrentLanguage;
	TMap<FName, FText> LocalizedTextMap;

	// 게임 시작 시 기본으로 로드할 언어. 필요하면 시스템 로케일 감지 로직으로 교체 가능.
	static const FString DefaultLanguage;
	static const FString LocalizationDirRelativePath; // Content/Data
};
