#include "ItemSkill_I_4_Primary.h"
#include "NativeGameplayTags.h"
#include "GameFramework/Controller.h"
#include "Player/MainCharacter.h"
#include "GAS/Abilities/ItemSkill_I_4_Projectile.h"

// 이 파일 안에서만 쓰는 태그 - MainGameplayTags에 안 넣는다 (다른 파일에서 참조할 일이 없음).
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Cooldown_ItemSkill_I_4_Primary, "Cooldown.ItemSkill.I_4.Primary");

UGA_ItemSkill_I_4_Primary::UGA_ItemSkill_I_4_Primary()
{
	CooldownTag = TAG_Cooldown_ItemSkill_I_4_Primary;
	SkillSlot = EMainAbilityInputID::Primary;

	// BP 경로 끝의 "_C"는 Blueprint가 만드는 실제 클래스를 가리키는 규칙이다.
	ProjectileClassPath = TSoftClassPtr<AItemSkill_I_4_Projectile>(
		FSoftObjectPath(TEXT("/Game/Items/I_4/BP_I_4_Projectile.BP_I_4_Projectile_C")));
}

void UGA_ItemSkill_I_4_Primary::OnChargeReleased(float ChargeRatio)
{
	// 서버에서만 호출된다(UMainChargeGameplayAbility). 마나와 쿨다운은 베이스가 이미 처리했다.
	AMainCharacter* Character = CurrentActorInfo ? Cast<AMainCharacter>(CurrentActorInfo->AvatarActor.Get()) : nullptr;
	AController* Controller = Character ? Character->GetController() : nullptr;
	UWorld* World = Character ? Character->GetWorld() : nullptr;
	if (!Controller || !World)
	{
		return;
	}

	TSubclassOf<AItemSkill_I_4_Projectile> ProjectileClass = ProjectileClassPath.LoadSynchronous();
	if (!ProjectileClass)
	{
		// BP가 아직 없거나 경로가 틀렸다. 투사체 기능은 동작하지만 모양이 없으니 눈치채도록 경고를 남긴다.
		UE_LOG(LogTemp, Warning, TEXT("[ProjectRWW][I_4] 투사체 BP를 찾지 못해 기본 클래스로 대체합니다: %s"), *ProjectileClassPath.ToString());
		ProjectileClass = AItemSkill_I_4_Projectile::StaticClass();
	}

	// 던지는 방향 = 시전자의 시점 방향. 총 발사(MainWeaponComponent)와 같은 방식으로 서버가 직접 구한다.
	FVector ViewLocation;
	FRotator ViewRotation;
	Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
	const FVector Direction = ViewRotation.Vector();
	const FTransform SpawnTransform(ViewRotation, ViewLocation + Direction * SpawnForwardOffset);

	// 충전량 0~1에 따라 처음 속도가 MinSpeed~MaxSpeed로 달라진다.
	const float Speed = FMath::Lerp(MinSpeed, MaxSpeed, ChargeRatio);

	// Deferred 스폰: 속도를 먼저 넣은 뒤 FinishSpawning으로 확정한다. 이래야 처음 복제될 때부터 올바른 값이 전달된다.
	// Instigator에 던진 본인을 넣는다(투사체가 본인 캡슐에 걸리지 않게 하는 데 쓴다).
	AItemSkill_I_4_Projectile* Projectile = World->SpawnActorDeferred<AItemSkill_I_4_Projectile>(
		ProjectileClass, SpawnTransform, nullptr, Character, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Projectile)
	{
		Projectile->SetLaunchSpeed(Speed);
		Projectile->FinishSpawning(SpawnTransform);
	}
}
