// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventorySlot.h"
#include "MainSlotContainerComponent.generated.h"

// 슬롯 배열을 가진 모든 컨테이너(인벤토리, 창고, 나중에 생길 필드 상자 등)의 공용 부모.
// 장착 같은 특수 부수효과가 필요 없는 컨테이너는 TakeSlot/PlaceSlot/AddItem을
// 오버라이드하지 않고 그대로 상속받아 쓰면 된다. Abstract가 아니므로, 그냥 고정
// 크기 상자가 필요하면 이 클래스를 코드 없이 바로 붙이고 Columns만 설정하면 된다.
UCLASS(BlueprintType, meta = (BlueprintSpawnableComponent))
class PROJECTRWW_API UMainSlotContainerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMainSlotContainerComponent();

	// 서버 권위 데이터지만 소유 클라이언트 본인에게는 실시간으로 보여줘야 해서 리플리케이트한다.
	UPROPERTY(ReplicatedUsing = OnRep_Slots)
	TArray<FInventorySlot> Slots;

	// 그리드로 그릴 때 가로 몇 칸마다 다음 줄로 넘길지. 세로 줄 수는 저장하지 않고
	// 위젯이 항상 Slots.Num() / Columns로 계산한다 - 데이터와 그리드 모양이
	// 어긋날 일이 구조적으로 없게 하기 위함. EditAnywhere인 이유는 창고처럼
	// 액터 인스턴스마다(등급 초기값 등) 다르게 줄 수도 있기 때문.
	UPROPERTY(EditAnywhere, Category = "Inventory")
	int32 Columns = 9;

	// 가로 칸 수를 물어본다. 기본 구현은 Columns를 그대로 반환하지만, 창고처럼
	// 조건에 따라 매번 다시 계산해야 하는 컨테이너는 C++이나 BP에서 오버라이드한다.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Inventory")
	int32 GetColumnCount() const;
	virtual int32 GetColumnCount_Implementation() const;

	// 런타임에 가로 칸 수를 바꾼다. 지금은 아무도 안 부르지만, 나중에 플레이어
	// 인벤토리가 동적으로 커지거나 사망 시 그 크기 그대로 상자를 만들 때 쓸 용도.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetGridSize(int32 NewColumns);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSlotsChanged);
	UPROPERTY(BlueprintAssignable)
	FOnSlotsChanged OnSlotsChanged;

	// 빈 슬롯을 찾아 넣는다. 반환값은 채워진 슬롯 번호 - 꽉 차서 실패하면 INDEX_NONE(-1).
	// bool이 아니라 슬롯 번호를 돌려주는 이유는, 자식(UMainInventoryComponent)이 "방금
	// 채운 자리가 지금 장착 중인 슬롯인지" 판단하려면 위치 정보가 필요하기 때문이다.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	virtual int32 AddItem(FName ItemIndex);

	// 지정 슬롯의 내용을 꺼내고 그 자리를 비운다. 빈 슬롯/잘못된 인덱스면 빈 값 반환.
	// 브로드캐스트하지 않는다 - Server_MoveSlot이 최종적으로 한 번씩 처리한다.
	virtual FInventorySlot TakeSlot(int32 SlotIndex);

	// 지정 슬롯에 값을 채워 넣는다. 이미 차있거나 인덱스가 잘못되면 false.
	// 이것도 브로드캐스트하지 않는다 - 이유는 위와 동일.
	virtual bool PlaceSlot(int32 SlotIndex, const FInventorySlot& SlotData);

	// 클라이언트 -> 서버: 슬롯 하나를 옮긴다. DestComponent를 생략하면(nullptr) 자기 자신으로
	// 취급되어 같은 컨테이너 안에서 이동한다. 두 경우 모두 TakeSlot+TakeSlot+PlaceSlot+PlaceSlot
	// 조합으로 동일하게 처리되며, 목적지가 이미 차있으면 두 아이템이 서로 스왑된다.
	UFUNCTION(Server, Reliable)
	void Server_MoveSlot(int32 FromSlot, int32 ToSlot, UMainSlotContainerComponent* DestComponent = nullptr);

	// 클라이언트 -> 서버: 디버그 명령어 등이 이걸 통해서만 아이템을 넣는다.
	UFUNCTION(Server, Reliable)
	void Server_AddItem(FName ItemIndex);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	// Slots가 리플리케이트되어 도착하면 호출된다. OnSlotsChanged를 브로드캐스트해서
	// 구독 중인 위젯들이 각자 자기 몫을 다시 그리게 한다.
	UFUNCTION()
	void OnRep_Slots();
};
