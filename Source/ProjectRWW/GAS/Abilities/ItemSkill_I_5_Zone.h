// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GameFramework/Actor.h"
#include "ItemSkill_I_5_Zone.generated.h"

class USceneComponent;
class UStaticMeshComponent;

// 장판 액터 하나에서만 쓰이는 1:1 전용 이펙트라 같은 파일에 둔다. Duration,
// WalkSpeed/RunSpeed/FireRateMultiplier에 SetByCaller 배율만큼 Multiplicative(곱하기).
// 사망 시 자동 제거되고, 같은 효과는 쌓이지 않으며 다시 적용되면 지속 시간만 갱신된다.
UCLASS()
class PROJECTRWW_API UGE_ItemSkill_I_5_Effect : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_ItemSkill_I_5_Effect();
	virtual void PostInitProperties() override;
};

// I_5 스킬이 만드는 원기둥 모양 장판. 서버에서 스폰되어 클라이언트로 복제된다(모양 표시용). 서버만 일정 간격으로
// 장판 메시의 충돌과 겹친 살아 있는 모든 캐릭터에게 짧은 지속 시간의 버프(이동속도/연사속도 상승)를 걸거나 갱신하고,
// 수명이 지나면 스스로 사라진다. 나가거나 사라진 뒤에는 마지막으로 건 버프가 BuffLingerSeconds 안에 저절로 풀린다.
// 보이는 모양이 곧 판정 범위다: 메시/재질은 이 클래스를 부모로 한 Blueprint(BP_I_5_Zone)에서 지정하고,
// 메시 크기는 ZoneRadius/ZoneHeight에 자동으로 맞춘다. 판정이 되려면 지정한 메시에 충돌 모양이 있어야 한다.
UCLASS(Blueprintable)
class PROJECTRWW_API AItemSkill_I_5_Zone : public AActor
{
	GENERATED_BODY()

public:
	AItemSkill_I_5_Zone();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 장판 메시와 겹친 캐릭터에게 버프를 건다/갱신한다. 서버만 호출한다.
	void RefreshBuffs();

	// 루트(위치 기준점). 장판이 생긴 자리가 원기둥의 바닥 중심이다.
	UPROPERTY(VisibleAnywhere, Category = "Zone")
	TObjectPtr<USceneComponent> SceneRoot;

	// 보이는 모양이자 실제 판정 범위인 메시. 메시/재질은 BP에서 지정하고, 크기와 위치는 BeginPlay에서 ZoneRadius/ZoneHeight에 맞춘다.
	// 캐릭터(Pawn)와의 겹침만 받는다(막지 않고 총알도 가로채지 않는다). 판정은 서버에서만 한다.
	UPROPERTY(VisibleAnywhere, Category = "Zone")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	// 장판 메시를 맞출 크기(cm). 바닥 중심은 이 액터의 위치이고 위로 ZoneHeight만큼 올라간다.
	// 판정 수치가 아니라 메시의 크기를 정하는 값이고, 실제 판정은 그 메시의 충돌을 따른다.
	UPROPERTY(EditDefaultsOnly, Category = "Zone", meta = (ClampMin = "1.0"))
	float ZoneRadius = 400.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Zone", meta = (ClampMin = "1.0"))
	float ZoneHeight = 300.0f;

	// 밸런싱용 - 장판 유지 시간(초).
	UPROPERTY(EditDefaultsOnly, Category = "Zone", meta = (ClampMin = "0.1"))
	float ZoneLifeSeconds = 10.0f;

	// 버프를 갱신하는 간격(초).
	UPROPERTY(EditDefaultsOnly, Category = "Zone", meta = (ClampMin = "0.05"))
	float RefreshIntervalSeconds = 0.25f;

	// 한 번 건 버프의 지속 시간(초). 갱신 간격보다 충분히 길어야 끊김 없이 이어진다(장판을 나가면 이 시간 안에 풀림).
	UPROPERTY(EditDefaultsOnly, Category = "Zone", meta = (ClampMin = "0.1"))
	float BuffLingerSeconds = 0.6f;

	// 이동속도/연사속도 배율. Multiplicative라서 현재값에 곱해진다(다른 스킬의 속도 효과와는 증가분끼리 합산된다).
	UPROPERTY(EditDefaultsOnly, Category = "Zone")
	float MoveSpeedMultiplier = 1.3f;

	UPROPERTY(EditDefaultsOnly, Category = "Zone")
	float FireRateMultiplier = 1.3f;

private:
	FTimerHandle RefreshTimerHandle;
};
