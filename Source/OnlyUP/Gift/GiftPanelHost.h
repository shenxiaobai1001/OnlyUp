#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GiftPanelHost.generated.h"

class UGiftPanelWidget;

/**
 * Auto-spawned helper that binds Alt+R and owns the gift panel widget.
 */
UCLASS(NotPlaceable, Transient)
class ONLYUP_API AGiftPanelHost : public AActor
{
	GENERATED_BODY()

public:
	AGiftPanelHost();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	void BindInputIfNeeded();
	void TogglePanel();
	void EnsureWidget();
	void HandleGiftTriggered(FName GiftId, int32 Count);

	UPROPERTY()
	TObjectPtr<UGiftPanelWidget> PanelWidget;

	bool bInputBound = false;
};
