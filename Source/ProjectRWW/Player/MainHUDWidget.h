// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainHUDWidget.generated.h"

UCLASS()
class PROJECTRWW_API UMainHUDWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	float CurrentHP = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	float MaxHP = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	float CurrentMana = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	float MaxMana = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	int32 CurrentAmmo = 0;

	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	int32 MagazineSize = 0;

	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	bool bIsReloading = false;

	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	float ReloadProgress = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	FName WeaponIndex;

	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	float CurrentSpreadDegrees = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	float MaxSpreadDegrees = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	bool bIsAiming = false;

	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	bool bIsExtracting = false;

	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	float ExtractionDuration = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	float ExtractionStartTimeSeconds = 0.0f;

	// [타격자] 내가 쏜 총알이 캐릭터에 맞았을 때(발사당 한 번). 크로스헤어 히트마커와 소리는 BP 구현부가 재생한다.
	// bHeadshot이면 헤드샷용 이미지/소리를 쓴다.
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void ReceiveHitConfirmed(bool bHeadshot);

	// [피격자] 내가 맞았을 때(발사당 한 번). SourceLocation은 쏜 사람의 위치라 피격 방향 표시에 쓸 수 있고,
	// Damage는 이번 발사에서 맞은 피해 합계다. 피격 연출과 소리는 BP 구현부가 재생한다.
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void ReceiveDamaged(FVector SourceLocation, float Damage, bool bHeadshot);

	// [타격자] 내가 죽인 상대가 사망했을 때(킬 확인). 킬 마커와 소리는 BP 구현부가 재생한다.
	// 같은 발사의 일반 히트마커(ReceiveHitConfirmed)보다 먼저 도착할 수 있으니, BP에서 킬 연출이 재생 중이면
	// 일반 히트마커가 덮어쓰지 않게 막아 둔다.
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void ReceiveKillConfirmed();
};
