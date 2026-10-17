#include "DanceWheelSubsystem.h"

#include "DanceWheelHost.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DanceWheelSubsystem)

bool UDanceWheelSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (const UWorld* World = Cast<UWorld>(Outer))
	{
		return World->IsGameWorld();
	}
	return false;
}

void UDanceWheelSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (HostActor || InWorld.GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.Name = TEXT("DanceWheelHost");
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	HostActor = InWorld.SpawnActor<ADanceWheelHost>(ADanceWheelHost::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
}
