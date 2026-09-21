// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemPickupActor.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class USphereComponent;

// 바닥에 놓여 플레이어가 F로 줍는 아이템의 공통 부모. 아이템마다 이 클래스를 상속한
// Blueprint(BP_Pickup_AR_1 등)를 만들어 메시, 판정 구, ItemIndex를 채운다.
// AItemActor(장착 시 손에 스폰되는 Actor)를 아이템마다 BP로 만드는 것과 같은 방식이다.
UCLASS(Blueprintable)
class PROJECTRWW_API AItemPickupActor : public AActor
{
	GENERATED_BODY()

public:
	AItemPickupActor();

	// 이 픽업을 주웠을 때 인벤토리에 들어갈 아이템의 Index. 리플리케이트하지 않는다 -
	// 클래스(BP) 자체가 모든 머신에 같은 기본값을 갖고 있고, 줍기 판정은 서버가 이
	// 값으로 하기 때문이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup")
	FName ItemIndex;

	// 이 픽업에 든 현재 탄약. -1이면 "한 번도 장착 안 한 새것"이라는 뜻으로, 인벤토리에
	// 들어간 뒤 처음 장착할 때 탄창을 가득 채운다(FInventorySlot::CurrentAmmo와 같은 규칙).
	// 리플리케이트하지 않는다 - 탄약은 줍는 순간 서버가 인벤토리로 옮길 때만 쓰이고,
	// 클라이언트 화면에는 표시할 곳이 없기 때문이다. 레벨에 손으로 놓은 총과 나중에 JSON
	// 스폰이 만든 새 총은 기본값(-1)이고, 플레이어가 버린 총은 스폰할 때 서버가 이 값을 채운다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup")
	int32 CurrentAmmo = -1;

	// 서버 검증/HUD 마커용: 판정 구의 월드 중심 좌표와 (스케일이 반영된) 반경.
	FVector GetPickupCenter() const;
	float GetPickupRadius() const;

protected:
	virtual void BeginPlay() override;

	// 메시와 PickupSphere의 공통 부모. 둘을 형제로 두어 서로의 스케일/위치가 영향을 주지
	// 않게 하고, 액터 원점(레벨에 놓는 기준점)을 어느 쪽에도 묶이지 않게 한다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<USceneComponent> SceneRoot;

	// 보이는 모습 전용(콜리전 없음). BP에서 이 컴포넌트의 Static Mesh를 지정한다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	// 줍기 판정 전용 구. 카메라 레이가 메시가 아니라 이 구에 맞는다 - 아이템마다 BP에서
	// 반경/위치를 직접 조절해서 "어디까지 조준하면 줍히는지"를 디자이너가 정한다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<USphereComponent> PickupSphere;
};
