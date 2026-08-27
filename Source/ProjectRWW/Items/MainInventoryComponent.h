// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventorySlot.h"
#include "MainInventoryComponent.generated.h"

// AMainPlayerController에 붙는 인벤토리 시스템. PlayerRecord(단순 데이터)와 달리
// 리플리케이션 + 여러 동작(장착/이동)을 갖는 시스템이라, MainWeaponComponent 등과
// 같은 패턴으로 별도 컴포넌트로 분리했다.
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTRWW_API UMainInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMainInventoryComponent();

	// 인벤토리는 항상 이 크기로 고정된다(4행x9열, 앞 9칸이 핫바).
	static constexpr int32 InventorySlotCount = 36;

	// 서버 권위 데이터지만, 소유 클라이언트(본인)에게는 실시간으로 보여줘야 해서
	// 리플리케이트한다. COND_OwnerOnly라 다른 플레이어에게는 절대 전송되지 않는다.
	UPROPERTY(ReplicatedUsing = OnRep_InventorySlots)
	TArray<FInventorySlot> InventorySlots;

	// 지금 손에 장착 중인 아이템이 InventorySlots의 몇 번 슬롯에서 왔는지.
	// -1이면 빈손. 서버 전용 값이라 리플리케이트 안 한다.
	int32 EquippedSlotIndex = -1;

	// 인벤토리가 바뀔 때(OnRep_InventorySlots)마다 브로드캐스트된다.
	// UI 위젯들이 이걸 구독해서 갱신한다.
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);
	UPROPERTY(BlueprintAssignable)
	FOnInventoryChanged OnInventoryChanged;

	// 빈 슬롯을 찾아 넣는다. 꽉 차면 false.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItem(FName ItemIndex);

	// 두 슬롯을 서로 바꾼다. 둘 중 하나가 장착 중인 슬롯이면, 스왑 후 그 슬롯 번호에
	// 새로 들어온 아이템을 다시 장착한다(핫바의 "그 자리"가 계속 활성 상태를 유지).
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool MoveItem(int32 FromSlot, int32 ToSlot);

	// 지정한 슬롯의 아이템을 장착한다. Category에 따라 분기 - 지금은 Weapon만 실제 동작.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void EquipItem(int32 SlotIndex);

	// 지금 장착 중인 아이템을 손에서 내린다.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void UnequipItem();

	// 클라이언트 -> 서버: 핫바 숫자키 등으로 슬롯의 아이템을 장착 요청.
	UFUNCTION(Server, Reliable)
	void Server_EquipItem(int32 SlotIndex);

	// 클라이언트 -> 서버: 인벤토리 창에서 두 슬롯을 서로 바꾸는 요청.
	UFUNCTION(Server, Reliable)
	void Server_MoveItem(int32 FromSlot, int32 ToSlot);

	// 클라이언트 -> 서버: 디버그 명령어(RWW_AddItem)가 이걸 통해서만 아이템을 넣는다.
	// AddItem()을 직접 부르면 리슨 서버가 아닌 클라이언트에서는 로컬 사본만 바뀌고
	// 서버의 진짜 데이터에는 반영되지 않는다.
	UFUNCTION(Server, Reliable)
	void Server_AddItem(FName ItemIndex);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	// InventorySlots가 리플리케이트되어 도착하면 호출된다. OnInventoryChanged를
	// 브로드캐스트해서 인벤토리 창/핫바 HUD가 각자 자기 몫을 다시 그리게 한다.
	UFUNCTION()
	void OnRep_InventorySlots();

private:
	// 소유자(PlayerController)가 지금 빙의한 폰의 MainWeaponComponent를 찾아 반환한다.
	// 폰이 없거나 타입이 다르면 nullptr. 사망 등으로 폰이 계속 교체되니 캐싱하지 않는다.
	class UMainWeaponComponent* GetWeaponComponent() const;
};
