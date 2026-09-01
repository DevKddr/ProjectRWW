// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MainSlotContainerComponent.h"
#include "MainStorageComponent.generated.h"

struct FStorageTierData;

// AMainLobbyPlayerController에 붙는 창고 시스템. 슬롯 배열/이동 자체는 부모
// (UMainSlotContainerComponent)가 담당하고, 여기서는 "등급에 따라 크기가 커진다"는
// 창고 고유의 개념만 추가한다. 장착이라는 동작이 없어서 오버라이드할 것도 없다.
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTRWW_API UMainStorageComponent : public UMainSlotContainerComponent
{
	GENERATED_BODY()

public:
	// 지금 창고 등급. 서버에서만 설정되며(등급 상승 조건은 별도 과제), 리플리케이트해서
	// 클라이언트 UI가 참고할 수 있게 한다.
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Storage")
	int32 StorageTier = 0;

	// StorageTier를 설정하고, storage_tiers.json에서 조회한 가로x세로 칸 수로
	// Slots 크기를 맞춘다. 로드 직후(역직렬화 다음) 호출한다.
	void ApplyTier(int32 NewTier);

	// 지금 등급의 가로 칸 수. StorageTier로부터 매번 다시 조회하는 파생값이라 별도로
	// 저장/리플리케이트하지 않는다. UI가 그리드 배치(줄바꿈 위치)를 계산할 때 쓴다.
	UFUNCTION(BlueprintPure, Category = "Storage")
	int32 GetColumnCount() const;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	// GameInstance -> UStorageTierDataManager 서브시스템 -> GetTierData 순으로 조회하는
	// 과정을 ApplyTier/GetColumnCount가 각자 반복하지 않도록 한 곳에 모아둔다.
	bool GetTierData(int32 Tier, FStorageTierData& OutData) const;
};
