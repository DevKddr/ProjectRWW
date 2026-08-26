// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "MainGameInstance.generated.h"

class UNetDriver;

UCLASS()
class PROJECTRWW_API UMainGameInstance : public UGameInstance
{
	GENERATED_BODY()

protected:
	virtual void Init() override;

	// 클라이언트가 서버 접속을 시도했다가 실패했을 때(서버가 없음, 타임아웃 등) 호출된다.
	// 타이틀->로비, 로비->세션 서버 등 어떤 이동이든 이 GameInstance 하나가 전부 잡는다 —
	// GameInstance는 레벨 이동에도 파괴되지 않는 유일한 오브젝트이기 때문이다.
	void HandleConnectionNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);

	// 접속 자체는 됐지만 맵/레벨 이동 처리 중 실패했을 때 호출된다.
	void HandleConnectionTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString);
};
