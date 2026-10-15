#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GiftPanelSubsystem.generated.h"

class AGiftPanelHost;

/** Ensures a gift-panel host exists for game worlds. */
UCLASS()
class ONLYUP_API UGiftPanelSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
	UPROPERTY()
	TObjectPtr<AGiftPanelHost> HostActor;
};
