#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GiftPanelWidget.generated.h"

DECLARE_DELEGATE_TwoParams(FOnGiftPanelTriggered, FName /*GiftId*/, int32 /*Count*/);
DECLARE_DELEGATE(FOnGiftPanelClosed);

USTRUCT()
struct FGiftPanelItemDesc
{
	GENERATED_BODY()

	UPROPERTY()
	FName GiftId;

	UPROPERTY()
	FText DisplayName;
};

/**
 * Dark translucent two-column gift panel (Alt+R).
 * Each row: function button + quantity editable text.
 */
UCLASS()
class ONLYUP_API UGiftPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetItems(const TArray<FGiftPanelItemDesc>& InItems);
	void SetPanelVisible(bool bVisible);

	FOnGiftPanelTriggered OnGiftTriggered;
	FOnGiftPanelClosed OnClosed;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	void HandleTriggerClicked(FName GiftId);
	void HandleCloseClicked();
	int32 GetQuantityFor(FName GiftId) const;

	TArray<FGiftPanelItemDesc> Items;
	TMap<FName, TSharedPtr<SEditableTextBox>> QuantityBoxes;
};
