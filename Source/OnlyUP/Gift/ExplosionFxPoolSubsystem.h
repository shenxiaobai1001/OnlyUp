#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ExplosionFxPoolSubsystem.generated.h"

class UNiagaraSystem;

/**
 * Thin wrapper around Niagara's built-in AutoRelease component pool.
 * Grows/recycles dynamically — no fixed size.
 */
UCLASS()
class ONLYUP_API UExplosionFxPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	void Prewarm();
	void PlayExplosion(const FVector& WorldLocation, float Scale = 1.5f);

private:
	void EnsureSystem();

	UPROPERTY()
	TObjectPtr<UNiagaraSystem> ExplosionSystem = nullptr;
};
