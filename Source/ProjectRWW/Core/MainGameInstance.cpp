// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainGameInstance.h"
#include "MainSettingsSaveGame.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"

const FString UMainGameInstance::SettingsSlotName = TEXT("Settings");

void UMainGameInstance::Init()
{
	Super::Init();

	// UEngine의 델리게이트에 직접 등록한다 — GameInstance 자체의 HandleNetworkError/
	// HandleTravelError는 BlueprintImplementableEvent라 C++에서 오버라이드할 수 없다.
	GEngine->OnNetworkFailure().AddUObject(this, &UMainGameInstance::HandleConnectionNetworkFailure);
	GEngine->OnTravelFailure().AddUObject(this, &UMainGameInstance::HandleConnectionTravelFailure);

	// 감도는 로컬 입력 설정이라 전용 서버에는 필요 없다(LoadSettings 내부에서도 걸러진다).
	LoadSettings();
}

void UMainGameInstance::SetLookSensitivity(float NewValue)
{
	// 깨진 SaveGame 등에서 NaN/무한대가 들어와도 시점 입력이 망가지지 않게 막는다.
	if (!FMath::IsFinite(NewValue))
	{
		return;
	}
	LookSensitivity = FMath::Clamp(NewValue, MinSensitivity, MaxSensitivity);
}

void UMainGameInstance::SetADSSensitivityMultiplier(float NewValue)
{
	if (!FMath::IsFinite(NewValue))
	{
		return;
	}
	ADSSensitivityMultiplier = FMath::Clamp(NewValue, MinSensitivity, MaxSensitivity);
}

void UMainGameInstance::LoadSettings()
{
	// 서버 권위 대상이 아닌 로컬 설정이므로 전용 서버에서는 파일을 건드리지 않는다.
	// IsRunningDedicatedServer()는 에디터 PIE의 서버 월드에서는 false라서, 월드 컨텍스트로 판별하는
	// IsDedicatedServerInstance()를 쓴다.
	if (IsDedicatedServerInstance())
	{
		return;
	}

	// 먼저 기본값으로 되돌려서, 파일이 없거나 깨졌을 때도 항상 유효한 값을 갖게 한다.
	ResetSettingsToDefault();

	if (!UGameplayStatics::DoesSaveGameExist(SettingsSlotName, 0))
	{
		return;
	}

	const UMainSettingsSaveGame* Saved = Cast<UMainSettingsSaveGame>(UGameplayStatics::LoadGameFromSlot(SettingsSlotName, 0));
	if (!Saved)
	{
		return;
	}

	SetLookSensitivity(Saved->LookSensitivity);
	SetADSSensitivityMultiplier(Saved->ADSSensitivityMultiplier);
}

bool UMainGameInstance::SaveSettings()
{
	if (IsDedicatedServerInstance())
	{
		return false;
	}

	UMainSettingsSaveGame* SaveObject = Cast<UMainSettingsSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UMainSettingsSaveGame::StaticClass()));
	if (!SaveObject)
	{
		return false;
	}

	SaveObject->LookSensitivity = LookSensitivity;
	SaveObject->ADSSensitivityMultiplier = ADSSensitivityMultiplier;

	return UGameplayStatics::SaveGameToSlot(SaveObject, SettingsSlotName, 0);
}

void UMainGameInstance::ResetSettingsToDefault()
{
	SetLookSensitivity(1.0f);
	SetADSSensitivityMultiplier(1.0f);
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
