#include "GiftPanelSubsystem.h"

#include "Engine/World.h"
#include "GiftPanelHost.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GiftPanelSubsystem)

bool UGiftPanelSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (const UWorld* World = Cast<UWorld>(Outer))
	{
		return World->IsGameWorld();
	}
	return false;
}

void UGiftPanelSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (HostActor || InWorld.GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.Name = TEXT("GiftPanelHost");
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	HostActor = InWorld.SpawnActor<AGiftPanelHost>(AGiftPanelHost::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
}
