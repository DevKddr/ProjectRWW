// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MainCharacter.generated.h"

class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

UCLASS()
class PROJECTRWW_API AMainCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AMainCharacter();

	UPROPERTY(VisibleAnywhere, Category = "HP")
	TObjectPtr<class UMainHPComponent> HPComponent;

	UPROPERTY(VisibleAnywhere, Category = "Mana")
	TObjectPtr<class UMainManaComponent> ManaComponent;

	// PlayerBaseStat.json의 "WalkSpeed"/"RunSpeed"/"JumpPower"와 이름을 맞춘 이동 스탯값.
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float WalkSpeed = 300.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float RunSpeed = 600.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float JumpPower = 420.0f;

	UPROPERTY(VisibleAnywhere, Category = "Weapon")
	TObjectPtr<class UMainWeaponComponent> WeaponComponent;

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// HPComponent->OnDeath에 바인딩되는 콜백. 실제 처리는 GameMode에 위임한다.
	UFUNCTION()
	void OnDeath(AController* Killer);

	// IA_Move가 들어오는 동안 계속 호출
	void OnMove(const FInputActionValue& Value);
	// IA_Look이 들어오는 동안 계속 호출
	void OnLook(const FInputActionValue& Value);

	void OnSprintStart(const FInputActionValue& Value);
	void OnSprintStop(const FInputActionValue& Value);

	// 클라이언트 요청을 받아 서버 쪽 이동 속도를 실제로 바꾸는 함수. 속도 변경은 항상 이 함수를 거처야 한다.
	UFUNCTION(Server, Reliable)
	void ServerSetSprinting(bool bNewSprinting);

	void OnFire(const FInputActionValue& Value);
	void OnReload(const FInputActionValue& Value);

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> FireAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> ReloadAction;
};
