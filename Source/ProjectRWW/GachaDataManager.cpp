#include "GachaDataManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "JsonObjectConverter.h"

const FString UGachaDataManager::RaritiesJsonRelativePath = TEXT("Data/output/rarities.json");
const FString UGachaDataManager::WeaponsJsonRelativePath = TEXT("Data/output/weapons.json");
const FString UGachaDataManager::GachaTablesJsonRelativePath = TEXT("Data/output/gacha_tables.json");

void UGachaDataManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const bool bRarityOk = LoadRarities();
	const bool bWeaponOk = LoadWeapons();
	const bool bGachaOk = LoadGachaTables();
	bDataLoaded = bRarityOk && bWeaponOk && bGachaOk;

	if (!bDataLoaded)
	{
		UE_LOG(LogTemp, Error, TEXT("[GachaDataManager] 데이터 로드 실패"));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[GachaDataManager] 로드 완료: 등급 %d, 무기 %d, 박스 %d"),
			RarityMap.Num(), WeaponMap.Num(), GachaTables.Boxes.Num());
	}
}

void UGachaDataManager::Deinitialize()
{
	WeaponMap.Empty();
	RarityMap.Empty();
	GachaTables = FGachaTables();
	Super::Deinitialize();
}

bool UGachaDataManager::LoadRarities()
{
	const FString FullPath = FPaths::Combine(FPaths::ProjectContentDir(), RaritiesJsonRelativePath);
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *FullPath))
	{
		UE_LOG(LogTemp, Error, TEXT("[GachaDataManager] 파일 없음: %s"), *FullPath);
		return false;
	}

	TArray<FRarityData> Parsed;
	if (!FJsonObjectConverter::JsonArrayStringToUStruct(JsonString, &Parsed, 0, 0))
	{
		UE_LOG(LogTemp, Error, TEXT("[GachaDataManager] rarities.json 파싱 실패"));
		return false;
	}

	RarityMap.Empty();
	for (const FRarityData& Entry : Parsed)
	{
		RarityMap.Add(Entry.RarityId, Entry);
	}
	return true;
}

bool UGachaDataManager::LoadWeapons()
{
	const FString FullPath = FPaths::Combine(FPaths::ProjectContentDir(), WeaponsJsonRelativePath);
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *FullPath))
	{
		UE_LOG(LogTemp, Error, TEXT("[GachaDataManager] 파일 없음: %s"), *FullPath);
		return false;
	}

	TArray<FWeaponItem> Parsed;
	if (!FJsonObjectConverter::JsonArrayStringToUStruct(JsonString, &Parsed, 0, 0))
	{
		UE_LOG(LogTemp, Error, TEXT("[GachaDataManager] weapons.json 파싱 실패"));
		return false;
	}

	WeaponMap.Empty();
	for (const FWeaponItem& Entry : Parsed)
	{
		WeaponMap.Add(Entry.Index, Entry);
	}
	return true;
}

bool UGachaDataManager::LoadGachaTables()
{
	// gacha_tables.json은 배열이 아니라 객체 하나이므로 JsonObjectStringToUStruct 사용.
	const FString FullPath = FPaths::Combine(FPaths::ProjectContentDir(), GachaTablesJsonRelativePath);
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *FullPath))
	{
		UE_LOG(LogTemp, Error, TEXT("[GachaDataManager] 파일 없음: %s"), *FullPath);
		return false;
	}

	if (!FJsonObjectConverter::JsonObjectStringToUStruct(JsonString, &GachaTables, 0, 0))
	{
		UE_LOG(LogTemp, Error, TEXT("[GachaDataManager] gacha_tables.json 파싱 실패"));
		return false;
	}
	return true;
}

bool UGachaDataManager::GetWeaponData(FName WeaponIndex, FWeaponItem& OutData) const
{
	if (const FWeaponItem* Found = WeaponMap.Find(WeaponIndex))
	{
		OutData = *Found;
		return true;
	}
	return false;
}

bool UGachaDataManager::GetRarityData(FName RarityId, FRarityData& OutData) const
{
	if (const FRarityData* Found = RarityMap.Find(RarityId))
	{
		OutData = *Found;
		return true;
	}
	return false;
}

void UGachaDataManager::SetLanguage(const FString& LanguageCode)
{
	if (LanguageCode == TEXT("ko") || LanguageCode == TEXT("en"))
	{
		CurrentLanguage = LanguageCode;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[GachaDataManager] 지원하지 않는 언어 코드: %s (ko/en만 지원)"), *LanguageCode);
	}
}

FText UGachaDataManager::GetWeaponDisplayName(FName WeaponIndex) const
{
	const FWeaponItem* Weapon = WeaponMap.Find(WeaponIndex);
	if (!Weapon)
	{
		return FText::FromString(FString::Printf(TEXT("[Unknown Weapon: %s]"), *WeaponIndex.ToString()));
	}
	const FString& Text = (CurrentLanguage == TEXT("en")) ? Weapon->Name.En : Weapon->Name.Ko;
	return FText::FromString(Text);
}

FText UGachaDataManager::GetWeaponDescription(FName WeaponIndex) const
{
	const FWeaponItem* Weapon = WeaponMap.Find(WeaponIndex);
	if (!Weapon)
	{
		return FText::GetEmpty();
	}
	const FString& Text = (CurrentLanguage == TEXT("en")) ? Weapon->Description.En : Weapon->Description.Ko;
	return FText::FromString(Text);
}

bool UGachaDataManager::GetWeaponSellValue(FName WeaponIndex, int32& OutSellValue) const
{
	const FWeaponItem* Weapon = WeaponMap.Find(WeaponIndex);
	if (!Weapon) return false;
	const FRarityData* Rarity = RarityMap.Find(Weapon->Rarity);
	if (!Rarity) return false;
	OutSellValue = Rarity->SellValue;
	return true;
}

bool UGachaDataManager::GetWeaponMaxTotalDamage(FName WeaponIndex, float& OutTotalDamage) const
{
	const FWeaponItem* Weapon = WeaponMap.Find(WeaponIndex);
	if (!Weapon) return false;
	OutTotalDamage = Weapon->Stats.Damage * static_cast<float>(Weapon->Stats.PelletCount);
	return true;
}

bool UGachaDataManager::DrawFromBox(FName BoxId, FName& OutCategory, FName& OutItemIndex, FName& OutRarityId) const
{
	const FGachaBox* Box = GachaTables.Boxes.Find(BoxId.ToString());
	if (!Box)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GachaDataManager] BoxID를 찾을 수 없음: %s"), *BoxId.ToString());
		return false;
	}

	// ---------- 1단계: 등급 추첨 (RarityDropWeight, 아이템이 있는 등급만) ----------
	float RarityTotal = 0.f;
	TArray<TPair<FString, float>> RarityPairs;
	for (const auto& PoolPair : Box->Pools)
	{
		if (PoolPair.Value.Items.Num() > 0)
		{
			RarityPairs.Add(TPair<FString, float>(PoolPair.Key, PoolPair.Value.RarityDropWeight));
			RarityTotal += PoolPair.Value.RarityDropWeight;
		}
	}
	if (RarityPairs.Num() == 0 || RarityTotal <= 0.f)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GachaDataManager] BoxID '%s'에 뽑을 수 있는 등급이 없음"), *BoxId.ToString());
		return false;
	}

	float R = FMath::FRandRange(0.f, RarityTotal);
	float Upto = 0.f;
	FString ChosenRarity = RarityPairs.Last().Key; // 부동소수 오차 대비 fallback
	for (const auto& Pair : RarityPairs)
	{
		Upto += Pair.Value;
		if (Upto >= R)
		{
			ChosenRarity = Pair.Key;
			break;
		}
	}

	// ---------- 2단계: 아이템 추첨 (선택된 등급 내 ItemDropWeight) ----------
	const FGachaRarityPool* Pool = Box->Pools.Find(ChosenRarity);
	if (!Pool || Pool->Items.Num() == 0)
	{
		return false;
	}

	float ItemTotal = 0.f;
	for (const FGachaPoolItem& Item : Pool->Items)
	{
		ItemTotal += Item.Weight;
	}
	if (ItemTotal <= 0.f)
	{
		return false;
	}

	float R2 = FMath::FRandRange(0.f, ItemTotal);
	float Upto2 = 0.f;
	const FGachaPoolItem* ChosenItem = &Pool->Items.Last(); // fallback
	for (const FGachaPoolItem& Item : Pool->Items)
	{
		Upto2 += Item.Weight;
		if (Upto2 >= R2)
		{
			ChosenItem = &Item;
			break;
		}
	}

	OutCategory = ChosenItem->Category;
	OutItemIndex = ChosenItem->Index;
	OutRarityId = FName(*ChosenRarity);
	return true;
}
