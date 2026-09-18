// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainHotbarSlotWidget.h"
#include "GAS/Abilities/MainGameplayAbility.h"
#include "Player/MainPlayerState.h"
#include "AbilitySystemComponent.h"
#include "MainInventoryComponent.h"

void UMainHotbarSlotWidget::SetSlotData(int32 InSlotIndex, const FInventorySlot& SlotData)
{
	Super::SetSlotData(InSlotIndex, SlotData);

	// 아이템이 바뀌었거나, 아직 구독이 안 걸린 상태(PlayerState가 그때 안 준비됐었음)면
	// 지금 다시 시도한다. 이 함수 자체엔 반복문이 없다 - "재시도"는 인벤토리가 바뀔
	// 때마다 이 SetSlotData()가 반복 호출된다는 사실에서 나온다.
	if (SlotData.ItemIndex != LastCheckedItemIndex || !bIsSubscribed)
	{
		RefreshSkillCooldownBindings(SlotData.ItemIndex);
		LastCheckedItemIndex = SlotData.ItemIndex;
	}
}

void UMainHotbarSlotWidget::RefreshSkillCooldownBindings(FName ItemIndex)
{
	// 기존 구독부터 해제 - 아이템이 바뀌었으면 옛 아이템의 태그 감시를 끊어야 하고,
	// 애초에 구독 성공 못 했던 상태였다면 핸들이 무효라 그냥 안전하게 지나간다.
	if (UAbilitySystemComponent* OldASC = CachedASC.Get())
	{
		if (PrimaryCooldownTag.IsValid())
		{
			OldASC->UnregisterGameplayTagEvent(PrimaryTagHandle, PrimaryCooldownTag);
		}
		if (SecondaryCooldownTag.IsValid())
		{
			OldASC->UnregisterGameplayTagEvent(SecondaryTagHandle, SecondaryCooldownTag);
		}
	}
	PrimaryCooldownTag = FGameplayTag();
	SecondaryCooldownTag = FGameplayTag();

	// 아이템이 바뀌면 옛 태그 구독은 끊기지만, 그 태그 자체는 ASC에 계속 남아있을 수 있어
	// (다른 슬롯에서 여전히 진행 중인 쿨다운이므로) OnCooldownTagChanged가 저절로 안 불린다.
	// 그래서 여기서 일단 둘 다 강제로 숨겨서 이전 아이템의 잔상이 안 남게 하고, 새 아이템이
	// 실제로 쿨다운 중이면 아래쪽 로직이 다시 보여준다.
	OnSkillCooldownEnded(PrimarySkillSlot);
	OnSkillCooldownEnded(SecondarySkillSlot);
	bIsSubscribed = false;

	const AMainPlayerState* PS = GetOwningPlayerState<AMainPlayerState>();
	UAbilitySystemComponent* ASC = PS ? PS->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		return; // PlayerState/ASC 아직 준비 안 됨 - 다음 SetSlotData() 호출 때 다시 시도됨
	}
	CachedASC = ASC;

	UClass* PrimaryClass = FindObject<UClass>(nullptr,
		*FString::Printf(TEXT("/Script/ProjectRWW.GA_ItemSkill_%s_Primary"), *ItemIndex.ToString()));
	UClass* SecondaryClass = FindObject<UClass>(nullptr,
		*FString::Printf(TEXT("/Script/ProjectRWW.GA_ItemSkill_%s_Secondary"), *ItemIndex.ToString()));

	if (const UMainGameplayAbility* PrimaryCDO = PrimaryClass ? PrimaryClass->GetDefaultObject<UMainGameplayAbility>() : nullptr)
	{
		const FGameplayTagContainer* Tags = PrimaryCDO->GetCooldownTags();
		if (Tags && Tags->Num() > 0)
		{
			PrimaryCooldownTag = Tags->First();
			PrimarySkillSlot = PrimaryCDO->GetSkillSlot();
			PrimaryTagHandle = ASC->RegisterGameplayTagEvent(PrimaryCooldownTag, EGameplayTagEventType::NewOrRemoved)
				.AddUObject(this, &UMainHotbarSlotWidget::OnCooldownTagChanged);
		}
	}

	if (const UMainGameplayAbility* SecondaryCDO = SecondaryClass ? SecondaryClass->GetDefaultObject<UMainGameplayAbility>() : nullptr)
	{
		const FGameplayTagContainer* Tags = SecondaryCDO->GetCooldownTags();
		if (Tags && Tags->Num() > 0)
		{
			SecondaryCooldownTag = Tags->First();
			SecondarySkillSlot = SecondaryCDO->GetSkillSlot();
			SecondaryTagHandle = ASC->RegisterGameplayTagEvent(SecondaryCooldownTag, EGameplayTagEventType::NewOrRemoved)
				.AddUObject(this, &UMainHotbarSlotWidget::OnCooldownTagChanged);
		}
	}

	bIsSubscribed = true;

	// 구독을 막 걸었을 뿐인데 이미 쿨다운 중일 수 있다(예: 다른 슬롯에서 쓰던 아이템을
	// 이 슬롯으로 옮겨온 경우) - 놓치지 않도록 지금 상태를 한 번 직접 확인해서 반영한다.
	if (PrimaryCooldownTag.IsValid())
	{
		OnCooldownTagChanged(PrimaryCooldownTag, ASC->GetTagCount(PrimaryCooldownTag));
	}
	if (SecondaryCooldownTag.IsValid())
	{
		OnCooldownTagChanged(SecondaryCooldownTag, ASC->GetTagCount(SecondaryCooldownTag));
	}
}

void UMainHotbarSlotWidget::OnCooldownTagChanged(FGameplayTag Tag, int32 NewCount)
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	if (!ASC)
	{
		return;
	}

	const EMainAbilityInputID SkillSlot = (Tag == PrimaryCooldownTag) ? PrimarySkillSlot : SecondarySkillSlot;

	if (NewCount > 0)
	{
		const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(Tag));

		// BP 이벤트에 알려줄 Duration은 "남은 시간"이 아니라 이 쿨다운 이펙트의 실제
		// 적용된 총 시간이어야 한다 - 나중에 쿨다운 감소 효과가 생겨도 항상 정확하다.
		float Duration = 0.0f;
		for (const float D : ASC->GetActiveEffectsDuration(Query))
		{
			Duration = FMath::Max(Duration, D);
		}

		OnSkillCooldownStarted(SkillSlot, Duration);
	}
	else
	{
		OnSkillCooldownEnded(SkillSlot);
	}
}

float UMainHotbarSlotWidget::GetPrimaryCooldownPercent() const
{
	return CalculateCooldownPercent(PrimaryCooldownTag);
}

float UMainHotbarSlotWidget::GetSecondaryCooldownPercent() const
{
	return CalculateCooldownPercent(SecondaryCooldownTag);
}

bool UMainHotbarSlotWidget::IsEquippedSlot() const
{
	const UMainInventoryComponent* InventoryComp = Cast<UMainInventoryComponent>(OwningComponent.Get());
	return InventoryComp && InventoryComp->EquippedSlotIndex == SlotIndex;
}

float UMainHotbarSlotWidget::CalculateCooldownPercent(const FGameplayTag& Tag) const
{
	const UAbilitySystemComponent* ASC = CachedASC.Get();
	if (!ASC || !Tag.IsValid())
	{
		return 0.0f;
	}

	const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(Tag));

	float Remaining = 0.0f;
	for (const float Time : ASC->GetActiveEffectsTimeRemaining(Query))
	{
		Remaining = FMath::Max(Remaining, Time);
	}

	float TotalDuration = 0.0f;
	for (const float D : ASC->GetActiveEffectsDuration(Query))
	{
		TotalDuration = FMath::Max(TotalDuration, D);
	}

	if (TotalDuration <= 0.0f)
	{
		return 0.0f;
	}

	return FMath::Clamp(1.0f - (Remaining / TotalDuration), 0.0f, 1.0f);
}

void UMainHotbarSlotWidget::NativeDestruct()
{
	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		if (PrimaryCooldownTag.IsValid())
		{
			ASC->UnregisterGameplayTagEvent(PrimaryTagHandle, PrimaryCooldownTag);
		}
		if (SecondaryCooldownTag.IsValid())
		{
			ASC->UnregisterGameplayTagEvent(SecondaryTagHandle, SecondaryCooldownTag);
		}
	}

	// 구독을 해제했으니 반드시 false로 되돌려야 한다 - 안 그러면 이 위젯이 나중에
	// 다시 보여질 때(SetSlotData가 같은 아이템으로 다시 불려도) "이미 구독됨"으로
	// 착각해서 재구독을 영영 시도하지 않게 된다.
	bIsSubscribed = false;

	Super::NativeDestruct();
}
