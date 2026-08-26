// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainGameInstance.h"
#include "Engine/Engine.h"

void UMainGameInstance::Init()
{
	Super::Init();

	// UEngine의 델리게이트에 직접 등록한다 — GameInstance 자체의 HandleNetworkError/
	// HandleTravelError는 BlueprintImplementableEvent라 C++에서 오버라이드할 수 없다.
	GEngine->OnNetworkFailure().AddUObject(this, &UMainGameInstance::HandleConnectionNetworkFailure);
	GEngine->OnTravelFailure().AddUObject(this, &UMainGameInstance::HandleConnectionTravelFailure);
}

void UMainGameInstance::HandleConnectionNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red,
			FString::Printf(TEXT("[접속 실패] 서버에 연결할 수 없습니다: %s"), *ErrorString));
	}
}

void UMainGameInstance::HandleConnectionTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red,
			FString::Printf(TEXT("[이동 실패] %s"), *ErrorString));
	}
}
