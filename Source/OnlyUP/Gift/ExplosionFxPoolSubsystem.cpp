#include "ExplosionFxPoolSubsystem.h"

#include "Engine/World.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ExplosionFxPoolSubsystem)

void UExplosionFxPoolSubsystem::Deinitialize()
{
	ExplosionSystem = nullptr;
	Super::Deinitialize();
}

void UExplosionFxPoolSubsystem::EnsureSystem()
{
	if (!ExplosionSystem)
	{
		ExplosionSystem = LoadObject<UNiagaraSystem>(
			nullptr,
			TEXT("/Game/ModAsset/GrimzaFX_Explosions/Niagara/Explosion/NS_Explosion_12.NS_Explosion_12"));
		if (!ExplosionSystem)
		{
			UE_LOG(LogTemp, Error, TEXT("[TaiQiu] Failed to load NS_Explosion_12"));
		}
	}
}

void UExplosionFxPoolSubsystem::Prewarm()
{
	EnsureSystem();
	if (!ExplosionSystem || !GetWorld())
	{
		return;
	}

	// Warm Niagara's AutoRelease pool with one off-screen play.
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(),
		ExplosionSystem,
		FVector(0.0f, 0.0f, -100000.0f),
		FRotator::ZeroRotator,
		FVector(0.01f),
		true,
		true,
		ENCPoolMethod::AutoRelease,
		true);
}

void UExplosionFxPoolSubsystem::PlayExplosion(const FVector& WorldLocation, float Scale)
{
	EnsureSystem();
	if (!ExplosionSystem || !GetWorld())
	{
		UE_LOG(LogTemp, Warning, TEXT("[TaiQiu] Explosion play failed — system/world missing."));
		return;
	}

	UNiagaraComponent* Comp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(),
		ExplosionSystem,
		WorldLocation,
		FRotator::ZeroRotator,
		FVector(FMath::Max(Scale, 0.1f)),
		true,
		true,
		ENCPoolMethod::AutoRelease,
		true);

	if (!Comp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TaiQiu] SpawnSystemAtLocation returned null at %s"),
			*WorldLocation.ToCompactString());
		return;
	}

	Comp->SetHiddenInGame(false);
	Comp->SetVisibility(true, true);
	UE_LOG(LogTemp, Log, TEXT("[TaiQiu] Explosion played at %s scale=%.2f"),
		*WorldLocation.ToCompactString(), Scale);
}
