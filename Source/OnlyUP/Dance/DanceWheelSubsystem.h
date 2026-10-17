#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "DanceWheelSubsystem.generated.h"

class ADanceWheelHost;

UCLASS()
class ONLYUP_API UDanceWheelSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
	UPROPERTY()
	TObjectPtr<ADanceWheelHost> HostActor;
};
