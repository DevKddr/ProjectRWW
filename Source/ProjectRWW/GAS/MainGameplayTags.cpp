// Copyright Epic Games, Inc. All Rights Reserved.
#include "MainGameplayTags.h"

namespace MainGameplayTags
{
	// State.Damaged - 피격 후 HPRegenDelay초 동안 붙어있는 태그. 회복 GameplayEffect가
	// 이 태그를 IgnoreTags로 걸어서, 붙어있는 동안 회복이 자동으로 멈춘다.
	UE_DEFINE_GAMEPLAY_TAG(State_Damaged, "State.Damaged");

	// 아래 Data.* 태그들은 전부 GameplayEffect의 SetByCaller 매그니튜드 키로만 쓰인다 -
	// JSON에서 읽은 값(HPRegen 등)을 GameplayEffect 애셋에 하드코딩하지 않고 런타임에
	// 주입하기 위한 용도.
	UE_DEFINE_GAMEPLAY_TAG(Data_Damage, "Data.Damage");
	UE_DEFINE_GAMEPLAY_TAG(Data_HPRegen, "Data.HPRegen");
	UE_DEFINE_GAMEPLAY_TAG(Data_ManaRegen, "Data.ManaRegen");
	UE_DEFINE_GAMEPLAY_TAG(Data_HPRegenDelay, "Data.HPRegenDelay");
	UE_DEFINE_GAMEPLAY_TAG(Data_ManaCost, "Data.ManaCost");

	// 이 태그가 붙은 GameplayEffect는 소유자가 사망하는 순간 전부 강제 제거된다
	// (UMainAttributeSet::PostGameplayEffectExecute 참고). "이 효과가 뭘 하는지"를
	// 설명하는 태그가 아니라, "사망 시 정리 대상 그룹"에 속한다는 걸 표시하는 분류용
	// 태그다 - 실제 효과를 설명하는 다른 태그와 같이 부여해서 쓴다.
	UE_DEFINE_GAMEPLAY_TAG(Effect_ClearOnDeath, "Effect.ClearOnDeath");

	// UGE_Cooldown(범용)이 SetByCaller로 지속시간을 받는 키 - 모든 스킬의 쿨다운이 공유한다.
	// 실제로 "어떤 스킬의 쿨다운인지"는 이 값이 아니라 DynamicGrantedTags로 붙는
	// Cooldown.ItemSkill.* 태그(각 스킬 파일 안에 로컬 정의됨)가 구분한다. 아이템 스킬
	// 쿨다운은 리스폰해도 유지되도록 의도했으므로 Effect_ClearOnDeath는 안 붙인다.
	UE_DEFINE_GAMEPLAY_TAG(Data_Cooldown_Duration, "Data.Cooldown.Duration");
}
