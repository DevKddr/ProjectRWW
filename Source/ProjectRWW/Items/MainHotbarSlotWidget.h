// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MainInventorySlotWidget.h"
#include "GameplayTagContainer.h"
#include "Combat/MainWeaponComponent.h" // EMainAbilityInputID
#include "MainHotbarSlotWidget.generated.h"

class UAbilitySystemComponent;

// 핫바 전용 슬롯 위젯. 부모(UMainInventorySlotWidget)의 아이콘/드래그앤드랍 로직은
// 그대로 물려받고, 여기에 스킬 쿨다운 진행률 표시 기능만 추가한다. 인벤토리 화면 등
// 다른 곳에서는 이 클래스를 안 쓰므로, 쿨다운 UI는 핫바에서만 보인다.
UCLASS()
class PROJECTRWW_API UMainHotbarSlotWidget : public UMainInventorySlotWidget
{
	GENERATED_BODY()

public:
	virtual void SetSlotData(int32 InSlotIndex, const FInventorySlot& SlotData) override;

	// BP의 ProgressBar Percent 바인드용. MainWeaponComponent::GetReloadProgress()와 같은
	// 원리 - 남은 시간을 ASC에서 매번 다시 조회해서 계산하므로, 나중에 쿨다운 감소 효과가
	// 생겨도 항상 정확하다. UMG가 함수 바인드는 매 프레임 알아서 다시 호출해주므로 Tick이
	// 따로 필요 없다(HPBar/ManaBar의 Get_HPBar_Percent와 동일 패턴).
	UFUNCTION(BlueprintPure, Category = "Hotbar")
	float GetPrimaryCooldownPercent() const;

	UFUNCTION(BlueprintPure, Category = "Hotbar")
	float GetSecondaryCooldownPercent() const;

	// BP의 하이라이트 위젯 Visibility 바인드용. OwningComponent를 UMainInventoryComponent로
	// 캐스팅해서 EquippedSlotIndex와 이 슬롯의 SlotIndex가 같은지만 비교한다. EquippedSlotIndex는
	// OnRep이 없는 리플리케이트 전용 프로퍼티라 이벤트로 알림받을 수 없는데, 바인드는 매 프레임
	// 값을 다시 확인해주므로 이벤트 없이도 항상 정확하다.
	UFUNCTION(BlueprintPure, Category = "Hotbar")
	bool IsEquippedSlot() const;

protected:
	virtual void NativeDestruct() override;

	// 쿨다운이 시작되면 호출된다 - BP가 이 이벤트를 받아서 SkillSlot에 맞는 프로그레스
	// 바를 보이게 한다. 진행률 자체는 위 GetXCooldownPercent()를 바인드해서 표시하므로
	// 여기선 Visibility만 처리하면 된다.
	UFUNCTION(BlueprintImplementableEvent, Category = "Hotbar")
	void OnSkillCooldownStarted(EMainAbilityInputID SkillSlot, float Duration);

	// 쿨다운이 끝나면(또는 조기 종료되면) 호출된다 - BP가 해당 바를 즉시 숨긴다.
	UFUNCTION(BlueprintImplementableEvent, Category = "Hotbar")
	void OnSkillCooldownEnded(EMainAbilityInputID SkillSlot);

private:
	// 이 슬롯의 아이템이 바뀌었거나 아직 구독이 안 걸린 상태일 때 호출된다.
	// 기존 구독을 해제하고, 새 아이템의 Primary/Secondary 어빌리티 클래스를 찾아
	// 각각의 CooldownTag/SkillSlot을 CDO에서 읽어온 뒤 ASC에 다시 구독을 건다.
	void RefreshSkillCooldownBindings(FName ItemIndex);

	// RegisterGameplayTagEvent가 태그 개수 변화를 감지할 때마다 자동으로 호출한다.
	// Primary/Secondary 구독 둘 다 이 함수 하나를 공유한다 - 어느 쪽 태그가 바뀐 건지는
	// 저장해둔 PrimaryCooldownTag와 직접 비교해서 판단한다.
	void OnCooldownTagChanged(FGameplayTag Tag, int32 NewCount);

	// GetPrimaryCooldownPercent()/GetSecondaryCooldownPercent()의 공용 계산 로직.
	// Tag가 유효하지 않거나 Duration이 0 이하면(쿨다운 대상이 없는 슬롯) 0을 반환한다.
	float CalculateCooldownPercent(const FGameplayTag& Tag) const;

	// RefreshSkillCooldownBindings()에서 구독을 걸 때 찾아둔 ASC를 재사용하기 위해
	// 캐싱해둔다 - OnCooldownTagChanged()/NativeDestruct()에서 PlayerState를 다시
	// 거치지 않고 바로 쓴다. 약한 참조라 ASC가 파괴돼도 안전하다.
	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;

	FGameplayTag PrimaryCooldownTag;
	FGameplayTag SecondaryCooldownTag;
	EMainAbilityInputID PrimarySkillSlot = EMainAbilityInputID::Primary;
	EMainAbilityInputID SecondarySkillSlot = EMainAbilityInputID::Secondary;
	FDelegateHandle PrimaryTagHandle;
	FDelegateHandle SecondaryTagHandle;
	FName LastCheckedItemIndex;
	bool bIsSubscribed = false;
};
