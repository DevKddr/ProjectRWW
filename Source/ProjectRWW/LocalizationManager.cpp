#include "LocalizationManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

const FString ULocalizationManager::DefaultLanguage = TEXT("ko");
const FString ULocalizationManager::LocalizationDirRelativePath = TEXT("Data/output");

void ULocalizationManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	bDataLoaded = SetLanguage(DefaultLanguage);

	if (!bDataLoaded)
	{
		UE_LOG(LogTemp, Error, TEXT("[LocalizationManager] 기본 언어(%s) 로드 실패"), *DefaultLanguage);
	}
}

void ULocalizationManager::Deinitialize()
{
	LocalizedTextMap.Empty();
	Super::Deinitialize();
}

bool ULocalizationManager::SetLanguage(const FString& LanguageCode)
{
	if (!LoadLanguageFile(LanguageCode))
	{
		return false;
	}
	CurrentLanguage = LanguageCode;
	bDataLoaded = true;
	OnLanguageChanged.Broadcast();
	return true;
}

bool ULocalizationManager::LoadLanguageFile(const FString& LanguageCode)
{
	// Content/Data/output/Localization_{lang}.json
	const FString FileName = FString::Printf(TEXT("Localization_%s.json"), *LanguageCode);
	const FString FullPath = FPaths::Combine(FPaths::ProjectContentDir(), LocalizationDirRelativePath, FileName);

	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *FullPath))
	{
		UE_LOG(LogTemp, Error, TEXT("[LocalizationManager] 파일을 찾을 수 없음: %s"), *FullPath);
		return false;
	}

	// Localization_{lang}.json은 배열이 아니라 { "Key": "Text", ... } 형태의 순수 JSON 객체이므로
	// FJsonObjectConverter 대신 FJsonSerializer로 직접 파싱한다.
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[LocalizationManager] JSON 파싱 실패: %s"), *FullPath);
		return false;
	}

	TMap<FName, FText> NewMap;
	NewMap.Reserve(JsonObject->Values.Num());
	for (const auto& Pair : JsonObject->Values)
	{
		FString Value;
		if (Pair.Value.IsValid() && Pair.Value->TryGetString(Value))
		{
			NewMap.Add(FName(*Pair.Key), FText::FromString(Value));
		}
	}

	LocalizedTextMap = MoveTemp(NewMap);
	UE_LOG(LogTemp, Log, TEXT("[LocalizationManager] '%s' 로드 완료: %d개 키"), *LanguageCode, LocalizedTextMap.Num());
	return true;
}

FText ULocalizationManager::GetLocalizedText(FName Key) const
{
	if (const FText* Found = LocalizedTextMap.Find(Key))
	{
		return *Found;
	}
	UE_LOG(LogTemp, Warning, TEXT("[LocalizationManager] 키를 찾을 수 없음: %s"), *Key.ToString());
	return FText::FromString(FString::Printf(TEXT("[%s]"), *Key.ToString()));
}
