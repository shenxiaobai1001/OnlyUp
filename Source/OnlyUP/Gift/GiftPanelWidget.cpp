#include "GiftPanelWidget.h"

#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GiftPanelWidget)

namespace GiftPanelStyle
{
	static const FLinearColor ButtonBg(0.28f, 0.28f, 0.30f, 0.92f);
	static const FLinearColor CloseBg(0.92f, 0.92f, 0.94f, 1.0f);
}

void UGiftPanelWidget::SetItems(const TArray<FGiftPanelItemDesc>& InItems)
{
	Items = InItems;
	QuantityBoxes.Reset();
}

void UGiftPanelWidget::SetPanelVisible(bool bVisible)
{
	SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}

void UGiftPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Collapsed);
}

int32 UGiftPanelWidget::GetQuantityFor(FName GiftId) const
{
	if (const TSharedPtr<SEditableTextBox>* BoxPtr = QuantityBoxes.Find(GiftId))
	{
		if (BoxPtr->IsValid())
		{
			const FString Text = (*BoxPtr)->GetText().ToString().TrimStartAndEnd();
			const int32 Value = FCString::Atoi(*Text);
			return FMath::Clamp(Value <= 0 ? 1 : Value, 1, 50);
		}
	}
	return 1;
}

void UGiftPanelWidget::HandleTriggerClicked(FName GiftId)
{
	const int32 Count = GetQuantityFor(GiftId);
	UE_LOG(LogTemp, Log, TEXT("[GiftPanel] Clicked %s x%d"), *GiftId.ToString(), Count);
	OnGiftTriggered.ExecuteIfBound(GiftId, Count);
}

void UGiftPanelWidget::HandleCloseClicked()
{
	SetVisibility(ESlateVisibility::Collapsed);
	OnClosed.ExecuteIfBound();
}

TSharedRef<SWidget> UGiftPanelWidget::RebuildWidget()
{
	QuantityBoxes.Reset();

	TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);

	for (const FGiftPanelItemDesc& Item : Items)
	{
		TSharedPtr<SEditableTextBox> QtyBox;

		Rows->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 4.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(180.0f)
				.HeightOverride(42.0f)
				[
					SNew(SButton)
					.ContentPadding(FMargin(12.0f, 8.0f))
					.ButtonColorAndOpacity(GiftPanelStyle::ButtonBg)
					.OnClicked_Lambda([this, GiftId = Item.GiftId]()
					{
						HandleTriggerClicked(GiftId);
						return FReply::Handled();
					})
					[
						SNew(STextBlock)
						.Text(Item.DisplayName)
						.ColorAndOpacity(FLinearColor::White)
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 16))
						.Justification(ETextJustify::Center)
					]
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(8.0f, 0.0f, 0.0f, 0.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(48.0f)
				.HeightOverride(40.0f)
				[
					SAssignNew(QtyBox, SEditableTextBox)
					.Text(FText::AsNumber(1))
					.Justification(ETextJustify::Center)
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 15))
					.BackgroundColor(FLinearColor::White)
				]
			]
		];

		QuantityBoxes.Add(Item.GiftId, QtyBox);
	}

	// Compact floating panel — no full-screen gray backdrop.
	return SNew(SBox)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		.Padding(FMargin(36.0f, 28.0f))
		[
			SNew(SBorder)
			.Padding(FMargin(16.0f, 14.0f))
			.BorderBackgroundColor(FLinearColor(0.07f, 0.07f, 0.08f, 0.78f))
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Top)
			[
				SNew(SBox)
				.WidthOverride(280.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.VAlign(VAlign_Center)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("补礼物面板")))
								.ColorAndOpacity(FLinearColor::White)
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 22))
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0f, 4.0f, 0.0f, 0.0f)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("Alt+R 开关 · 点功能名触发")))
								.ColorAndOpacity(FLinearColor(0.85f, 0.85f, 0.85f, 0.95f))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
							]
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Top)
						[
							SNew(SBox)
							.WidthOverride(56.0f)
							.HeightOverride(28.0f)
							[
								SNew(SButton)
								.ContentPadding(FMargin(6.0f, 2.0f))
								.ButtonColorAndOpacity(GiftPanelStyle::CloseBg)
								.OnClicked_Lambda([this]()
								{
									HandleCloseClicked();
									return FReply::Handled();
								})
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("关闭")))
									.ColorAndOpacity(FLinearColor(0.1f, 0.1f, 0.1f))
									.Justification(ETextJustify::Center)
									.Font(FCoreStyle::GetDefaultFontStyle("Regular", 13))
								]
							]
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 12.0f, 0.0f, 0.0f)
					[
						Rows
					]
				]
			]
		];
}
