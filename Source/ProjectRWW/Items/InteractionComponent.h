// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractionComponent.generated.h"

class AItemPickupActor;

// 바라보는 픽업이 바뀔 때(잡힘/놓침/교체/가득 참 여부 변화)만 브로드캐스트한다.
// HUD가 구독해서 마커를 켜고 끄고 색을 바꾼다 - 매 프레임 폴링/바인딩을 피하기 위함.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInteractFocusChanged, AItemPickupActor*, FocusedPickup, bool, bBlocked);

// 캐릭터에 붙어서 "지금 바라보는 줍기 대상"을 찾고, F 입력을 서버 줍기 요청으로 바꾼다.
// 감지(트레이스)는 로컬 플레이어에서만 돌고, 실제 줍기 판정은 서버가 다시 검증한다.
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTRWW_API UInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractionComponent();

	// 카메라 정면으로 트레이스를 쏘는 최대 거리(cm).
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	float InteractionRange = 300.0f;

	// 서버 거리 검증에 더해주는 여유(cm). 클라이언트 카메라 위치와 서버가 아는 눈 높이가
	// 조금 다르고 네트워크 지연으로 위치가 어긋나도, 정상 플레이어가 거절당하지 않게 한다.
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	float ServerRangeTolerance = 150.0f;

	// 트레이스 주기(초). Tick 대신 타이머를 쓴다(Lyra 상호작용 시스템과 같은 방식).
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	float ScanIntervalSeconds = 0.1f;

	UPROPERTY(BlueprintAssignable)
	FOnInteractFocusChanged OnFocusChanged;

	// 로컬 플레이어의 폰에서만 호출된다(SetupPlayerInputComponent). 원격 폰은 스캔하지 않는다.
	void StartScanning();

	// F 입력. 바라보는 대상이 있고 인벤토리에 빈 칸이 있을 때만 서버에 요청한다.
	void TryInteract();

	// HUD가 (재)구독하는 순간 지금 상태를 바로 맞출 수 있게 현재 값을 노출한다. 델리게이트는
	// "바뀔 때"만 오므로, 구독 이전에 이미 일어난 변화는 이 getter로 따라잡아야 한다.
	// (구현은 cpp에 둔다 - 헤더에는 AItemPickupActor의 전방 선언뿐이라 약참조의 Get()이
	// 완전한 타입을 요구하기 때문이다.)
	AItemPickupActor* GetFocusedPickup() const;
	bool IsFocusBlocked() const { return bFocusBlocked; }

	// 클라이언트 -> 서버: 이 픽업을 줍겠다는 요청. 서버가 거리/시야/빈 칸을 전부 다시 검증한다.
	UFUNCTION(Server, Reliable)
	void Server_PickupItem(AItemPickupActor* Pickup);

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// 타이머가 0.1초마다 호출한다. 트레이스 결과를 이전 상태와 비교해 바뀔 때만 알린다.
	void Scan();

	void SetFocus(AItemPickupActor* NewFocus, bool bNewBlocked);

	FTimerHandle ScanTimerHandle;

	// 파괴된 픽업을 가리킬 수 있어서 약참조로 든다.
	TWeakObjectPtr<AItemPickupActor> FocusedPickup;

	// FocusedPickup이 파괴돼 무효가 된 경우도 "바뀜"으로 잡기 위한 별도 플래그.
	bool bHasFocus = false;

	// 지금 잡힌 대상이 "인벤토리가 가득 차서 못 줍는" 상태인지.
	bool bFocusBlocked = false;
};
