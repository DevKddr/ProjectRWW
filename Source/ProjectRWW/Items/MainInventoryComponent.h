// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MainSlotContainerComponent.h"
#include "MainInventoryComponent.generated.h"

// AMainPlayerController에 붙는 인벤토리 시스템. 슬롯 배열/이동 자체는 부모
// (UMainSlotContainerComponent)가 담당하고, 여기서는 "장착"이라는 인벤토리 고유의
// 개념만 추가한다.
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTRWW_API UMainInventoryComponent : public UMainSlotContainerComponent
{
	GENERATED_BODY()

public:
	// 인벤토리는 항상 이 크기로 고정된다(4행x9열, 앞 9칸이 핫바).
	static constexpr int32 InventorySlotCount = 36;

	// 지금 손에 장착 중인 아이템이 Slots의 몇 번 슬롯에서 왔는지.
	// -1이면 빈손. 서버 전용 값이라 리플리케이트 안 한다.
	int32 EquippedSlotIndex = -1;

	virtual int32 AddItem(FName ItemIndex) override;
	virtual FInventorySlot TakeSlot(int32 SlotIndex) override;
	virtual bool PlaceSlot(int32 SlotIndex, const FInventorySlot& SlotData) override;

	// 지정한 슬롯의 아이템을 장착한다. Category에 따라 분기 - 지금은 Weapon만 실제 동작.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void EquipItem(int32 SlotIndex);

	// 지금 장착 중인 아이템을 손에서 내린다.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void UnequipItem();

	// 클라이언트 -> 서버: 핫바 숫자키 등으로 슬롯의 아이템을 장착 요청.
	UFUNCTION(Server, Reliable)
	void Server_EquipItem(int32 SlotIndex);

private:
	// 소유자(PlayerController)가 지금 빙의한 폰의 MainWeaponComponent를 찾아 반환한다.
	// 폰이 없거나 타입이 다르면 nullptr. 사망 등으로 폰이 계속 교체되니 캐싱하지 않는다.
	class UMainWeaponComponent* GetWeaponComponent() const;
};
