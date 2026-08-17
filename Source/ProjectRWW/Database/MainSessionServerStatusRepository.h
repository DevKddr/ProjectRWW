// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Templates/PimplPtr.h"
#include "MainSessionServerStatusRepository.generated.h"

// 세션 서버들이 각자 자기 인원수를 기록하고, 로비가 조회하는 용도.
// PlayerData.db와 별도 파일을 쓴다 — 여러 프로세스가 동시에 쓰는 파일과
// 로비만 쓰는 파일을 분리해 불필요한 락 경합을 피하기 위함.
UCLASS()
class PROJECTRWW_API UMainSessionServerStatusRepository : public UObject
{
	GENERATED_BODY()

public:
	static FString GetDefaultDatabasePath();

	bool Open(const FString& DatabaseFilePath);
	void Close();

	// 해당 주소의 최신 인원수를 기록한다(없으면 새로 생성).
	bool ReportHeartbeat(const FString& Address, int32 PlayerCount);

	// 해당 주소의 마지막으로 보고된 인원수. 기록이 없으면 0.
	int32 GetPlayerCount(const FString& Address);

private:
	TPimplPtr<class FSQLiteDatabase> Database;
};
