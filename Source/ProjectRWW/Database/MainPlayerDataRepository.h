// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "MainPlayerRecord.h"
#include "MainPlayerDataRepository.generated.h"

// 로비 서버, 세션 서버 양쪽 빌드 타겟이 같은 ProjectRWW 모듈을 쓰므로,
// 이 클래스 하나로 양쪽에서 동일한 PlayerData.db 파일에 접근한다.
// 함수 호출마다 DB를 열고-바로 닫는다(이유는 MainSessionServerStatusRepository와 동일).
UCLASS()
class PROJECTRWW_API UMainPlayerDataRepository : public UObject
{
	GENERATED_BODY()

public:
	// 로비 서버·세션 서버가 공유하는 PlayerData.db의 경로.
	// 배포 환경이 바뀌면 이 함수 내부의 경로를 직접 수정해서 사용한다.
	static FString GetDefaultDatabasePath();

	// 해당 PlayerID의 행이 없으면 기본값으로 새 레코드를 만들어 반환한다.
	FMainPlayerRecord LoadPlayerData(const FString& PlayerID);
	bool SavePlayerData(const FMainPlayerRecord& Record);
};
