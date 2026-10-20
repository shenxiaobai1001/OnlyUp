#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DanceWheelHost.generated.h"

class UDanceWheelWidget;

/**
 * Binds Tab and owns the dance wheel widget.
 */
UCLASS()
class ONLYUP_API ADanceWheelHost : public AActor
{
	GENERATED_BODY()

public:
	ADanceWheelHost();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void BindInputIfNeeded();
	void EnsureWidget();
	void OpenFromTab();
	void CloseFromTab();
	void ConfirmFromClick();
	void ApplyInputMode(bool bWheelOpen);
	void SetLookIgnored(bool bIgnore);
	void UpdateClickConsume();

	UFUNCTION()
	void HandleWheelClosedFromWidget();

	UPROPERTY()
	TObjectPtr<UDanceWheelWidget> WheelWidget;

	bool bInputBound = false;
	bool bWheelOpen = false;
	bool bClosedByClickWhileHeld = false;
	bool bCloseRequestedByTab = false;
	bool bLookIgnored = false;
};
