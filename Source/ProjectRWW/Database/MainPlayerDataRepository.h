// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "MainPlayerRecord.h"
#include "MainPlayerDataRepository.generated.h"

// 로비 서버, 세션 서버 양쪽 빌드 타겟이 같은 ProjectRWW 모듈을 쓰므로,
// 이 클래스 하나로 양쪽에서 동일한 PlayerData.db 파일에 접근한다.
// 함수 호출마다 DB를 열고-바로 닫는다(이유는 MainSessionServerStatusRepository와 동일).
// WAL 모드를 쓰지만, 연결을 오래 들고 있으면 로비/세션 두 프로세스가 동시에 파일을 열 때
// disk I/O error가 난 적이 있어서 "쓰고 나면 즉시 Close()"를 정책으로 한다. 새 함수를
// 추가할 때도 연결을 멤버로 들고 있지 말고, Statement를 블록으로 끝낸 뒤 Close()한다.
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

	// 해당 PlayerID의 Credit만 Amount만큼 올린다(다른 열은 건드리지 않는다).
	// 증가분 한 문장이라 여러 프로세스가 동시에 써도 값이 합쳐진다. 대상 행이 없으면 false.
	// 메모리의 PlayerRecord는 호출부가 직접 올린다.
	bool AddCredit(const FString& PlayerID, int32 Amount);
};
