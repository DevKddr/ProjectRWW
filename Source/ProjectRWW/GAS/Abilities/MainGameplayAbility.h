#pragma once
#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "Combat/MainWeaponComponent.h"
#include "MainGameplayAbility.generated.h"

struct FItemData;

// 아이템 스킬이 전부 이 클래스를 상속한다. 쿨다운은 스킬마다 새 GameplayEffect를 만들지
// 않고, 여기서 공용 UGE_Cooldown에 각자의 CooldownTag를 동적으로 얹어 적용한다.
UCLASS(Abstract)
class PROJECTRWW_API UMainGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UMainGameplayAbility()
	{
		InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
		NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	}

	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

	virtual const FGameplayTagContainer* GetCooldownTags() const override;

	// ActivateAbility()는 CanActivateAbility()가 이미 통과된 뒤에만 호출된다 - 즉 쿨다운/
	// 코스트/태그 요구사항 등으로 인한 발동 거부는 ActivateAbility() 안에서는 절대 못 잡는다
	// (그 안에 로그를 넣어봤자 이 경우엔 아예 도달하지 않는다). 진짜 거부 지점인 여기서
	// 로그를 남긴다.
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

protected:
	FGameplayTag CooldownTag;

	// GetCooldownTags()가 매번 새로 채워서 반환하는 버퍼 - 인스턴스마다 하나씩 소유한다.
	// static이었을 때는 모든 스킬/모든 플레이어가 이 버퍼 하나를 공유해서, 서로 다른
	// 인스턴스의 GetCooldownTags() 호출이 겹치면 내용이 덮어써질 위험이 있었다.
	mutable FGameplayTagContainer CachedCooldownTags;

	// 이 스킬이 Primary/Secondary 중 어느 입력 슬롯용인지 - 각 스킬 서브클래스 생성자에서
	// 고정값으로 설정한다. EquipTime처럼 "클래스 자체가 정하는 값"이라 서버/클라이언트가
	// 항상 같은 기본값을 컴파일해서 갖고 있고, 런타임에 주입할 필요가 없다.
	EMainAbilityInputID SkillSlot = EMainAbilityInputID::Primary;

	// 지금 장착된 아이템의 쿨다운/마나 비용을 그 자리에서 조회한다 - EquipTime/ReloadTime과
	// 완전히 같은 원리(MainWeaponComponent::ActiveItemIndex가 이미 서버/클라이언트/원격
	// 클라이언트 모두에 정확히 리플리케이트돼있으므로, 값 자체를 어빌리티에 주입할 필요가 없다).
	bool GetOwnerItemData(const FGameplayAbilityActorInfo* ActorInfo, FItemData& OutItemData) const;
	float GetCurrentSkillCooldownDuration(const FGameplayAbilityActorInfo* ActorInfo) const;
	float GetCurrentSkillManaCost(const FGameplayAbilityActorInfo* ActorInfo) const;

	// 소비하지 않고 확인만 한다 - 쿨다운 커밋 전에 먼저 불러서, 마나 부족이면 쿨다운을
	// 아예 걸지 않고 취소할 수 있게 한다.
	bool HasEnoughMana(const FGameplayAbilityActorInfo* ActorInfo) const;

	// 실제로 마나를 깎는다. 반드시 CommitAbility()가 성공한 "다음"에만 불러야 한다.
	// ApplyCooldown()과 똑같이 Handle/ActivationInfo를 받아서 ApplyGameplayEffectSpecToOwner()에
	// 넘긴다 - 이래야 예측 키가 붙어서, 서버가 발동을 거부했을 때 GAS가 이 마나 소모도
	// 정확히 롤백 대상으로 추적할 수 있다.
	void ApplyManaCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const;

	// 아이템을 장착한 직후(Draw 애니메이션 재생 중)엔 스킬을 못 쓰게 막는다 - Fire/ADS/
	// Reload가 이미 쓰고 있는 EquippedTimeSeconds/EquipTime 가드를 스킬에도 그대로 적용.
	bool IsStillEquipping(const FGameplayAbilityActorInfo* ActorInfo) const;

	// IsStillEquipping -> HasEnoughMana -> CommitAbility 3단 가드를 통과하면 ApplyManaCost()까지
	// 적용하고 true를 반환한다. 실패하면 알맞은 로그를 남기고 EndAbility()까지 처리한 뒤
	// false를 반환한다 - 모든 아이템 스킬의 ActivateAbility()가 완전히 동일하게 반복하던
	// 코드를 여기 한 곳으로 모았다.
	bool TryCommitSkillActivation(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo);
};
