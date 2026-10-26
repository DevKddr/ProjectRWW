#pragma once
#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "GCN_ItemSkillHold.generated.h"

class AItemActor;

// 홀드형 아이템 스킬의 연출 큐. 에디터의 BP 자식은 GameplayCueTag만 지정한다(Primary/Secondary 각각).
//  - OnActive/WhileActive: 시작(WhileActive는 중간에 접속한 클라이언트가 이미 진행 중인 큐를 받을 때)
//  - OnRemove: 정상 종료(다음 섹션을 End로). 강제 종료(아이템 교체/사망)는 AItemActor::EndPlay가 처리한다.
// 큐 제거(RemoveGameplayCue)에는 파라미터를 실을 수 없어서 정상/강제 정보를 큐로 전달하지 않는다.
UCLASS(Abstract)
class PROJECTRWW_API AGCN_ItemSkillHold : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	AGCN_ItemSkillHold();

	virtual bool OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;

private:
	AItemActor* GetItemActor(AActor* MyTarget, const FGameplayCueParameters& Parameters) const;
	void StartHold(AActor* MyTarget, const FGameplayCueParameters& Parameters);
};
