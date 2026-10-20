#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DanceSectorWidget.generated.h"

class UButton;

/**
 * One dance-wheel sector. Override OnSectorHovered and OnSectorUnhovered
 * in the widget blueprint to play selection animations.
 */
UCLASS()
class ONLYUP_API UDanceSectorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "Dance")
	void OnSectorHovered();

	UFUNCTION(BlueprintNativeEvent, Category = "Dance")
	void OnSectorUnhovered();

	UFUNCTION(BlueprintNativeEvent, Category = "Dance")
	void OnWheelUIOpened();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dance")
	int32 Index = 0;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Dance")
	TObjectPtr<UButton> DanceSectorButton;

	void SetSectorIndex(int32 InIndex);

	void SetButtonScale(float Scale);

	void ResetToDefault();

protected:
	virtual void NativeConstruct() override;
	virtual void OnSectorHovered_Implementation();
	virtual void OnSectorUnhovered_Implementation();
	virtual void OnWheelUIOpened_Implementation();
};
