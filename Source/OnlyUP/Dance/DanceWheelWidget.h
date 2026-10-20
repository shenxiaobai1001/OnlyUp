#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DanceWheelWidget.generated.h"

class UCanvasPanel;
class UDanceSectorWidget;
class UPanelWidget;
class USoundBase;

DECLARE_MULTICAST_DELEGATE(FOnDanceWheelClosed);

/**
 * Dance wheel. Sector layout lives on the widget blueprint under Panel_Sectors.
 * Selection follows the mouse direction from that panel's center.
 */
UCLASS()
class ONLYUP_API UDanceWheelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UDanceWheelWidget(const FObjectInitializer& ObjectInitializer);
	UFUNCTION(BlueprintCallable, Category = "Dance")
	void OpenWheel();

	UFUNCTION(BlueprintCallable, Category = "Dance")
	void CloseWheel();

	UFUNCTION(BlueprintPure, Category = "Dance")
	bool IsWheelOpen() const;

	UFUNCTION(BlueprintPure, Category = "Dance")
	bool ShouldShowCursorWhileOpen() const { return bShowCursorWhileOpen; }

	/** Left click while open. Plays the current sector, or does nothing inside the dead zone. */
	void ConfirmSelection();

	FOnDanceWheelClosed OnWheelClosed;

	UFUNCTION(BlueprintNativeEvent, Category = "Dance")
	void PlayDance(int32 DanceIndex);

	UFUNCTION(BlueprintImplementableEvent, Category = "Dance")
	void OnSectorSelected(int32 Index);

	UFUNCTION(BlueprintImplementableEvent, Category = "Dance")
	void OnSectorDeselected(int32 Index);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void PlayDance_Implementation(int32 DanceIndex);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dance")
	TObjectPtr<USoundBase> HoverSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dance", meta = (ClampMin = "0.0"))
	float HoverSoundStartTime = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dance")
	TObjectPtr<USoundBase> ClickSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dance", meta = (ClampMin = "0.0"))
	float ClickSoundStartTime = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dance", meta = (ClampMin = "0.0"))
	float CursorInnerRadius = 80.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dance", meta = (ClampMin = "0.0"))
	float CursorOuterRadius = 280.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dance", meta = (ClampMin = "1"))
	int32 SectorCount = 8;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dance", meta = (ClampMin = "0.0"))
	float SectorRadius = 200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dance", meta = (ClampMin = "0.0"))
	float SectorScale = 1.f;

	/** Degrees clockwise from straight up. 0 places sector 0 at the top. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dance")
	float FirstSectorAngle = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dance")
	bool bShowCursorWhileOpen = true;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> Panel_Sectors;

private:
	void GenerateSectors();
	void StopNavigation(UWidget* Widget);
	void UpdateSelection();
	void SetSelectedIndex(int32 NewIndex);
	void PlayUiSound(USoundBase* Sound, float StartTime) const;
	bool TryGetAbsoluteCenter(UWidget* Widget, FVector2D& OutCenter) const;
	FVector2D LastRingDirection = FVector2D(0.f, -1.f);

	UPROPERTY()
	TArray<TObjectPtr<UDanceSectorWidget>> SectorWidgets;

	int32 SelectedIndex = INDEX_NONE;
	bool bLoggedMissingPanel = false;
	bool bSectorsGenerated = false;
};
