#include "ItemSkill_I_5_Primary.h"
#include "Components/CapsuleComponent.h"
#include "GAS/Abilities/ItemSkill_I_5_Zone.h"
#include "NativeGameplayTags.h"
#include "Player/MainCharacter.h"

// 이 파일 안에서만 쓰는 태그 - MainGameplayTags에 안 넣는다 (다른 파일에서 참조할 일이 없음).
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Cooldown_ItemSkill_I_5_Primary, "Cooldown.ItemSkill.I_5.Primary");

UGA_ItemSkill_I_5_Primary::UGA_ItemSkill_I_5_Primary()
{
	CooldownTag = TAG_Cooldown_ItemSkill_I_5_Primary;
	SkillSlot = EMainAbilityInputID::Primary;

	// BP 경로 끝의 "_C"는 Blueprint가 만드는 실제 클래스를 가리키는 규칙이다.
	ZoneClassPath = TSoftClassPtr<AItemSkill_I_5_Zone>(
		FSoftObjectPath(TEXT("/Game/Items/I_5/BP_I_5_Zone.BP_I_5_Zone_C")));
}

void UGA_ItemSkill_I_5_Primary::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!TryCommitSkillActivation(Handle, ActorInfo, ActivationInfo))
	{
		return;
	}

	// 장판은 서버에서만 만든다(복제되어 클라이언트에 복사본이 생긴다).
	if (ActorInfo && ActorInfo->IsNetAuthority())
	{
		AMainCharacter* Character = Cast<AMainCharacter>(ActorInfo->AvatarActor.Get());
		UWorld* World = Character ? Character->GetWorld() : nullptr;
		if (World)
		{
			TSubclassOf<AItemSkill_I_5_Zone> ZoneClass = ZoneClassPath.LoadSynchronous();
			if (!ZoneClass)
			{
				// BP가 아직 없거나 경로가 틀렸다. 장판 기능은 동작하지만 모양이 없으니 눈치채도록 경고를 남긴다.
				UE_LOG(LogTemp, Warning, TEXT("[ProjectRWW][I_5] 장판 BP를 찾지 못해 기본 클래스로 대체합니다: %s"), *ZoneClassPath.ToString());
				ZoneClass = AItemSkill_I_5_Zone::StaticClass();
			}

			// 시전자 발밑(캡슐 절반 높이만큼 아래)에 만든다. 공중에서 쓰면 그 높이에 떠 있게 된다.
			const FVector FeetLocation = Character->GetActorLocation() - FVector(0.0f, 0.0f, Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());

			FActorSpawnParameters SpawnParams;
			SpawnParams.Instigator = Character;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			World->SpawnActor<AItemSkill_I_5_Zone>(ZoneClass, FeetLocation, FRotator::ZeroRotator, SpawnParams);
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}
