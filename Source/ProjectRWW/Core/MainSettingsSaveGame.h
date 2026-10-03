// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MainSettingsSaveGame.generated.h"

/**
 * 플레이어 개인 설정(감도 등)을 디스크 슬롯에 저장하는 SaveGame.
 * 로컬 머신에만 존재하며 서버와 동기화하지 않는다.
 * 값을 쓰는 쪽은 UMainGameInstance::SaveSettings, 읽는 쪽은 UMainGameInstance::LoadSettings 뿐이다.
 */
UCLASS()
class PROJECTRWW_API UMainSettingsSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	float LookSensitivity = 1.0f;

	UPROPERTY()
	float ADSSensitivityMultiplier = 1.0f;
};
