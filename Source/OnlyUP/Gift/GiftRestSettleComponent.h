#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GiftRestSettleComponent.generated.h"

class UPrimitiveComponent;

/**
 * Watches a simulating primitive; once it lands / slows (or max roll time),
 * freezes it as a solid non-physics obstacle (collision on, simulate off).
 */
UCLASS(ClassGroup = (Gift), meta = (BlueprintSpawnableComponent))
class ONLYUP_API UGiftRestSettleComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGiftRestSettleComponent();

	/** Create+register on Owner and begin watching Prim. */
	static UGiftRestSettleComponent* AttachAndWatch(
		AActor* Owner,
		UPrimitiveComponent* Prim,
		float InMaxRollSeconds = 3.0f,
		float InRestSpeed = 55.0f);

	void StartWatching(UPrimitiveComponent* Prim, float InMaxRollSeconds = 3.0f, float InRestSpeed = 55.0f);

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void FreezeAsFixedObstacle();

	TWeakObjectPtr<UPrimitiveComponent> WatchedPrim;

	float MaxRollSeconds = 3.0f;
	float RestSpeedThreshold = 55.0f;
	float RestHoldSeconds = 0.3f;
	float MinAirSeconds = 0.35f;

	float Elapsed = 0.0f;
	float RestAccum = 0.0f;
	bool bSettled = false;
};
