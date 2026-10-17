#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DanceWheelWidget.generated.h"

class UButton;
class USoundBase;

DECLARE_MULTICAST_DELEGATE(FOnDanceWheelClosed);

/**
 * Fixed 8-sector dance wheel. Layout and hover style live on the widget blueprint.
 */
UCLASS()
class ONLYUP_API UDanceWheelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Dance")
	void OpenWheel();

	UFUNCTION(BlueprintCallable, Category = "Dance")
	void CloseWheel();

	UFUNCTION(BlueprintPure, Category = "Dance")
	bool IsWheelOpen() const;

	FOnDanceWheelClosed OnWheelClosed;

	UFUNCTION(BlueprintNativeEvent, Category = "Dance")
	void PlayDance(int32 DanceIndex);

protected:
	virtual void NativeConstruct() override;
	virtual void PlayDance_Implementation(int32 DanceIndex);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dance")
	TObjectPtr<USoundBase> HoverSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dance", meta = (ClampMin = "0.0"))
	float HoverSoundStartTime = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dance")
	TObjectPtr<USoundBase> ClickSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dance", meta = (ClampMin = "0.0"))
	float ClickSoundStartTime = 0.f;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_Dismiss;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_Dance_0;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_Dance_1;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_Dance_2;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_Dance_3;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_Dance_4;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_Dance_5;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_Dance_6;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_Dance_7;

private:
	void BindButtons();
	void HandleDanceClicked(int32 DanceIndex);
	void HandleDanceHovered(int32 DanceIndex);
	void PlayUiSound(USoundBase* Sound, float StartTime) const;

	UFUNCTION()
	void HandleDismissClicked();

	UFUNCTION()
	void HandleDanceClicked0();
	UFUNCTION()
	void HandleDanceClicked1();
	UFUNCTION()
	void HandleDanceClicked2();
	UFUNCTION()
	void HandleDanceClicked3();
	UFUNCTION()
	void HandleDanceClicked4();
	UFUNCTION()
	void HandleDanceClicked5();
	UFUNCTION()
	void HandleDanceClicked6();
	UFUNCTION()
	void HandleDanceClicked7();

	UFUNCTION()
	void HandleDanceHovered0();
	UFUNCTION()
	void HandleDanceHovered1();
	UFUNCTION()
	void HandleDanceHovered2();
	UFUNCTION()
	void HandleDanceHovered3();
	UFUNCTION()
	void HandleDanceHovered4();
	UFUNCTION()
	void HandleDanceHovered5();
	UFUNCTION()
	void HandleDanceHovered6();
	UFUNCTION()
	void HandleDanceHovered7();

	bool bButtonsBound = false;
};
