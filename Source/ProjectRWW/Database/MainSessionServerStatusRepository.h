// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "MainSessionServerStatusRepository.generated.h"

// 세션 서버 하나의 현재 상태(주소, 인원, 정원)를 담는 그릇.
// GetAllServerStatuses()가 DB에 등록된 모든 서버를 이 형태로 돌려준다.
USTRUCT()
struct FMainSessionServerStatus
{
	GENERATED_BODY()

	UPROPERTY()
	FString Address;

	UPROPERTY()
	int32 PlayerCount = 0;

	UPROPERTY()
	int32 MaxPlayers = 0;
};

// 세션 서버들이 각자 자기 상태(인원/정원)를 기록하고, 로비가 조회하는 용도.
// 함수 호출마다 DB를 열고-쓰고(또는 읽고)-바로 닫는다. 커넥션을 계속 들고 있으면
// 로비/세션 두 프로세스가 동시에 파일을 여는 상황에서 WAL 모드로도 disk I/O error가
// 발생하는 걸 확인했기 때문.
//
// 세션 서버 목록 자체를 더 이상 설정 파일(MainNetworkSettings)에 하드코딩하지 않는다.
// 세션 서버가 시작할 때 스스로 이 DB에 등록하고(UpdatePlayerCount), 로비는 DB에 실제로
// 등록된 서버들만 조회(GetAllServerStatuses)해서 배정 대상으로 삼는다.
UCLASS()
class PROJECTRWW_API UMainSessionServerStatusRepository : public UObject
{
	GENERATED_BODY()

public:
	static FString GetDefaultDatabasePath();

	// 해당 주소의 최신 인원수/정원을 기록한다(없으면 새로 생성).
	// 세션 서버가 시작 시점(인원 0)과 접속/퇴장 이벤트마다 호출한다.
	bool UpdatePlayerCount(const FString& Address, int32 PlayerCount, int32 MaxPlayers);

	// 현재 DB에 등록된 모든 세션 서버의 상태를 돌려준다.
	TArray<FMainSessionServerStatus> GetAllServerStatuses();

	// 정상 종료 시 자기 자신의 등록을 지운다.
	bool RemoveServer(const FString& Address);
};
