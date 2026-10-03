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

public:
	// --- 인게임 감도 설정 ---
	// 서버 권위 대상이 아니다: 각 머신의 GameInstance에만 있는 로컬 입력 설정이며 복제하지 않고
	// 서버 RPC로도 보내지 않는다. 서버는 감도가 적용된 "최종 컨트롤 회전"만 받는다.
	// 값은 반드시 Set 함수로 바꾼다(범위 보장). 변수는 BP에서 읽기 전용이다.

	// 허용 범위. 설정 위젯의 슬라이더 Min/Max도 이 값을 읽어서 맞춘다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings")
	float MinSensitivity = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings")
	float MaxSensitivity = 3.0f;

	// 평소 시점 감도 배율. 1.0이 기존 속도와 동일하다.
	UPROPERTY(BlueprintReadOnly, Category = "Settings")
	float LookSensitivity = 1.0f;

	// 조준(ADS) 중 추가 배율. 줌 보정과는 별개로 곱해진다. 1.0이면 추가 보정 없음.
	UPROPERTY(BlueprintReadOnly, Category = "Settings")
	float ADSSensitivityMultiplier = 1.0f;

	UFUNCTION(BlueprintPure, Category = "Settings")
	float GetLookSensitivity() const { return LookSensitivity; }

	UFUNCTION(BlueprintPure, Category = "Settings")
	float GetADSSensitivityMultiplier() const { return ADSSensitivityMultiplier; }

	// Min~Max로 클램프해서 저장한다. NaN/무한대는 무시한다.
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SetLookSensitivity(float NewValue);

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SetADSSensitivityMultiplier(float NewValue);

	// 슬롯 파일에서 설정을 읽는다. 파일이 없거나 깨졌으면 기본값(1.0)을 유지한다.
	// 전용 서버에서는 아무것도 하지 않는다. Init()에서 자동 호출되므로 보통 직접 부를 일은 없다.
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void LoadSettings();

	// 현재 설정을 슬롯 파일에 쓴다. 성공하면 true. 전용 서버에서는 false.
	// 슬라이더를 놓을 때/닫기 버튼에서 부르고, 값이 바뀔 때마다 부르지는 않는다(디스크 쓰기 절약).
	UFUNCTION(BlueprintCallable, Category = "Settings")
	bool SaveSettings();

	// 감도를 기본값(1.0)으로 되돌린다. 저장은 하지 않는다.
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void ResetSettingsToDefault();

protected:
	virtual void Init() override;

	// 클라이언트가 서버 접속을 시도했다가 실패했을 때(서버가 없음, 타임아웃 등) 호출된다.
	// 타이틀->로비, 로비->세션 서버 등 어떤 이동이든 이 GameInstance 하나가 전부 잡는다 —
	// GameInstance는 레벨 이동에도 파괴되지 않는 유일한 오브젝트이기 때문이다.
	void HandleConnectionNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);

	// 접속 자체는 됐지만 맵/레벨 이동 처리 중 실패했을 때 호출된다.
	void HandleConnectionTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString);

private:
	// 설정 SaveGame 슬롯 이름. Saved/SaveGames/Settings.sav로 저장된다.
	static const FString SettingsSlotName;
};
