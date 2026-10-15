#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HomingArrowMotorComponent.generated.h"

class UNiagaraSystem;

/**
 * Drives a spawned arrow BP toward the player torso, then sticks as a child.
 * Ice arrows also play burst FX, spawn foot buff FX, and refresh ice slow.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ONLYUP_API UHomingArrowMotorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHomingArrowMotorComponent();

	void StartHoming(AActor* InTarget, bool bInIceArrow);

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void ApplyGuidedMotion(float DeltaSeconds);
	void StickToTarget();
	void HandleIceImpact();
	void PlayIcePathBurst();
	void SpawnIceBuffArea();
	FVector GetAimPoint() const;
	bool IsCloseEnoughToHit() const;

	UPROPERTY(EditAnywhere, Category = "Gift|Arrow")
	float GuidedSpeed = 2560.0f;

	UPROPERTY(EditAnywhere, Category = "Gift|Arrow")
	float HitAcceptDistance = 40.0f;

	UPROPERTY(EditAnywhere, Category = "Gift|Arrow|Ice")
	float IceSlowDuration = 3.0f;

private:
	TWeakObjectPtr<AActor> TargetActor;
	bool bIceArrow = false;
	bool bGuided = true;
	bool bStuck = false;

	UPROPERTY()
	TObjectPtr<UNiagaraSystem> IceBuffAreaSystem;
};
