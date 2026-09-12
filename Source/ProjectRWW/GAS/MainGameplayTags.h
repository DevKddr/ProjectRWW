// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "NativeGameplayTags.h"

namespace MainGameplayTags
{
	PROJECTRWW_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Damaged);
	PROJECTRWW_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Damage);
	PROJECTRWW_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_HPRegen);
	PROJECTRWW_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_ManaRegen);
	PROJECTRWW_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_HPRegenDelay);
	PROJECTRWW_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_ManaCost);
	PROJECTRWW_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_ClearOnDeath);

	// UGE_Cooldown(범용)이 SetByCaller로 지속시간을 받는 키 - 모든 스킬의 쿨다운이 공유한다.
	// 실제로 "어떤 스킬의 쿨다운인지"는 이 값이 아니라 DynamicGrantedTags로 붙는
	// Cooldown.ItemSkill.* 태그(각 스킬 파일 안에 로컬 정의됨)가 구분한다.
	PROJECTRWW_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Cooldown_Duration);
}
