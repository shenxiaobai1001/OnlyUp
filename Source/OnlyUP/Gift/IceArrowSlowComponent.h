#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IceArrowSlowComponent.generated.h"

/**
 * Halves horizontal move speed while active. Refreshing only extends the timer (no stack).
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ONLYUP_API UIceArrowSlowComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UIceArrowSlowComponent();

	/** Activate or refresh the 3s slow window. Does not stack the 0.5x multiplier. */
	UFUNCTION(BlueprintCallable, Category = "Gift|Ice")
	void RefreshSlow(float DurationSeconds = 3.0f);

	UFUNCTION(BlueprintPure, Category = "Gift|Ice")
	bool IsSlowActive() const;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, Category = "Gift|Ice")
	float SpeedMultiplier = 0.5f;

private:
	float SlowEndTime = 0.0f;
};
