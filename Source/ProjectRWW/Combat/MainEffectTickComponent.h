// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MainEffectTickComponent.generated.h"

// 회복/상태이상 등, 일정 간격(초)마다 반복되는 로직들이 공유하는 틱 신호.
// EffectTickInterval초마다 브로드캐스트된다. 서버에서만 동작한다(회복/상태이상 판정은 서버 권위 로직이므로).
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMainOnEffectTick);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTRWW_API UMainEffectTickComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMainEffectTickComponent();

	UPROPERTY(BlueprintAssignable, Category = "Effect")
	FMainOnEffectTick OnEffectTick;

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 몇 초 간격으로 OnEffectTick을 브로드캐스트할지. json에는 없는 값이라 하드코딩 기본값을
	// 쓰며, 추후 DataAsset으로 옮겨 여러 캐릭터가 공유하게 할 계획이다.
	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	float EffectTickInterval = 1.0f;

	// 마지막으로 틱을 브로드캐스트한 서버 월드 시각.
	double LastTickTimeSeconds = 0.0;
};
