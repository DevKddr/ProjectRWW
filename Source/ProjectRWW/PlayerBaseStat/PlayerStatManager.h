// PlayerStatManager.h
// 책임: PlayerBaseStat.json을 읽어 FPlayerBaseStat 하나를 보관하고, 그걸 돌려주는 것. 이것뿐이다.
// - 파싱/검증은 Python 컨버터 단계에서 이미 끝났으므로 여기서는 JSON 역직렬화만 한다.
// - DataTable/RowName 개념을 쓰지 않는다 (모든 플레이어가 공유하는 단일 설정이므로).
//
// 준비물 (Build.cs):
//   PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine",
//       "Json", "JsonUtilities" });
//
// 사용 방법:
//   1) PlayerBaseStat.json을 Content/Data/output/PlayerBaseStat.json 경로에 둔다.
//      (패키징 시 포함되도록 Project Settings > Packaging >
//       Additional Non-Asset Directories to Package 에 "Data" 폴더를 추가해야 한다)
//   2) 어디서든 다음처럼 사용:
//        UPlayerStatManager* Manager = GetGameInstance()->GetSubsystem<UPlayerStatManager>();
//        const FPlayerBaseStat& Stat = Manager->GetBaseStat();

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PlayerBaseStat.h"
#include "PlayerStatManager.generated.h"

// 전용 로그 카테고리. Output Log에서 "LogPlayerStat"으로 필터링하면
// 파싱 성공/실패 로그만 따로 모아볼 수 있다.
DECLARE_LOG_CATEGORY_EXTERN(LogPlayerStat, Log, All);

UCLASS(BlueprintType, Blueprintable)
class PROJECTRWW_API UPlayerStatManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** 로드된 기본 스탯을 반환한다. 로드에 실패했다면 0으로 초기화된 값이 반환된다. */
	UFUNCTION(BlueprintCallable, Category = "PlayerStat")
	const FPlayerBaseStat& GetBaseStat() const { return BaseStat; }

	/** JSON을 정상적으로 읽어서 파싱까지 끝냈는지 여부. 실행 중 언제든 확인 가능. */
	UFUNCTION(BlueprintCallable, Category = "PlayerStat")
	bool IsLoaded() const { return bLoaded; }

protected:
	// Content 폴더 기준 상대 경로. 필요하면 BP_PlayerStatManager 등에서 값만 바꿔 재사용 가능.
	// 현재는 파이썬 컨버터의 기본 출력 폴더(output)를 그대로 유지하는 경로로 맞춰뒀다.
	UPROPERTY(EditDefaultsOnly, Category = "PlayerStat")
	FString JsonRelativePath = TEXT("Data/output/PlayerBaseStat.json");

private:
	bool LoadFromJson();

	UPROPERTY()
	FPlayerBaseStat BaseStat;

	bool bLoaded = false;
};

