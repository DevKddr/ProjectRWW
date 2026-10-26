#include "GCN_ItemSkillHold.h"
#include "GAS/MainGameplayTags.h"
#include "Items/ItemActor.h"
#include "Player/MainCharacter.h"

AGCN_ItemSkillHold::AGCN_ItemSkillHold()
{
	bAutoDestroyOnRemove = true;
}

AItemActor* AGCN_ItemSkillHold::GetItemActor(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	// 이 프로젝트의 ASC는 PlayerState에 있어서 MyTarget이 캐릭터가 아닐 수 있다 - 기존 즉시형 큐(GC_ItemSkillUse_*)와
	// 같이 Parameters의 Instigator(어빌리티가 아바타 캐릭터를 실어 보낸다)를 먼저 쓰고, 없으면 MyTarget을 시도한다.
	const AMainCharacter* Character = Cast<AMainCharacter>(Parameters.Instigator.Get());
	if (!Character)
	{
		Character = Cast<AMainCharacter>(MyTarget);
	}
	return Character ? Cast<AItemActor>(Character->GetMainItem()) : nullptr;
}

void AGCN_ItemSkillHold::StartHold(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if (AItemActor* Item = GetItemActor(MyTarget, Parameters))
	{
		const bool bPrimary = GameplayCueTag.MatchesTagExact(MainGameplayTags::GameplayCue_ItemSkill_Hold_Primary.GetTag());
		// 재생 속도는 서버가 RawMagnitude로 실어 보낸다(지금은 항상 1.0).
		Item->PlayHoldStart(bPrimary, Parameters.RawMagnitude);
	}
}

bool AGCN_ItemSkillHold::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	StartHold(MyTarget, Parameters);
	return true;
}

bool AGCN_ItemSkillHold::WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	StartHold(MyTarget, Parameters);
	return true;
}

bool AGCN_ItemSkillHold::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if (AItemActor* Item = GetItemActor(MyTarget, Parameters))
	{
		Item->StopHold();
	}
	return true;
}
