// PlayerStatManager.cpp

#include "PlayerStatManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "JsonObjectConverter.h"

DEFINE_LOG_CATEGORY(LogPlayerStat);

void UPlayerStatManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	bLoaded = LoadFromJson();

	if (!bLoaded)
	{
		UE_LOG(LogPlayerStat, Error,
			TEXT("PlayerBaseStat.json 로드/파싱에 실패했습니다. "
			     "기본값(0)으로 초기화된 상태로 계속 진행합니다."));
	}
}

bool UPlayerStatManager::LoadFromJson()
{
	const FString FullPath = FPaths::ProjectContentDir() / JsonRelativePath;

	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *FullPath))
	{
		UE_LOG(LogPlayerStat, Error, TEXT("파일을 찾을 수 없습니다: %s"), *FullPath);
		return false;
	}

	if (!FJsonObjectConverter::JsonObjectStringToUStruct<FPlayerBaseStat>(JsonString, &BaseStat, 0, 0))
	{
		UE_LOG(LogPlayerStat, Error, TEXT("JSON 파싱에 실패했습니다: %s"), *FullPath);
		return false;
	}

	// 성공 로그: 실제로 파싱된 값을 그대로 찍어서, 엑셀에서 넣은 값과 육안으로 대조할 수 있게 한다.
	UE_LOG(LogPlayerStat, Log,
		TEXT("PlayerBaseStat.json 파싱 성공 (%s) - Hp=%d, WalkSpeed=%.1f, RunSpeed=%.1f, MaxMana=%d, ManaRegen=%.1f, JumpPower=%.1f"),
		*FullPath, BaseStat.Hp, BaseStat.WalkSpeed, BaseStat.RunSpeed,
		BaseStat.MaxMana, BaseStat.ManaRegen, BaseStat.JumpPower);

	return true;
}

