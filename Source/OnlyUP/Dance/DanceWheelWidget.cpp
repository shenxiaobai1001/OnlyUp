#include "DanceWheelWidget.h"

#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DanceWheelWidget)

void UDanceWheelWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindButtons();
	SetVisibility(ESlateVisibility::Collapsed);
}

void UDanceWheelWidget::OpenWheel()
{
	SetVisibility(ESlateVisibility::Visible);
}

void UDanceWheelWidget::CloseWheel()
{
	const bool bWasOpen = IsWheelOpen();
	SetVisibility(ESlateVisibility::Collapsed);
	if (bWasOpen)
	{
		OnWheelClosed.Broadcast();
	}
}

bool UDanceWheelWidget::IsWheelOpen() const
{
	return GetVisibility() == ESlateVisibility::Visible;
}

void UDanceWheelWidget::PlayDance_Implementation(int32 DanceIndex)
{
	UE_LOG(LogTemp, Log, TEXT("[DanceWheel] PlayDance index=%d (empty)"), DanceIndex);
}

void UDanceWheelWidget::BindButtons()
{
	if (bButtonsBound)
	{
		return;
	}
	bButtonsBound = true;

	if (Btn_Dismiss)
	{
		Btn_Dismiss->OnClicked.AddDynamic(this, &UDanceWheelWidget::HandleDismissClicked);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[DanceWheel] Missing button Btn_Dismiss."));
	}

	UButton* DanceButtons[8] = {
		Btn_Dance_0, Btn_Dance_1, Btn_Dance_2, Btn_Dance_3,
		Btn_Dance_4, Btn_Dance_5, Btn_Dance_6, Btn_Dance_7
	};

	for (int32 Index = 0; Index < 8; ++Index)
	{
		if (!DanceButtons[Index])
		{
			UE_LOG(LogTemp, Warning, TEXT("[DanceWheel] Missing button Btn_Dance_%d."), Index);
		}
	}

	if (Btn_Dance_0)
	{
		Btn_Dance_0->OnClicked.AddDynamic(this, &UDanceWheelWidget::HandleDanceClicked0);
		Btn_Dance_0->OnHovered.AddDynamic(this, &UDanceWheelWidget::HandleDanceHovered0);
	}
	if (Btn_Dance_1)
	{
		Btn_Dance_1->OnClicked.AddDynamic(this, &UDanceWheelWidget::HandleDanceClicked1);
		Btn_Dance_1->OnHovered.AddDynamic(this, &UDanceWheelWidget::HandleDanceHovered1);
	}
	if (Btn_Dance_2)
	{
		Btn_Dance_2->OnClicked.AddDynamic(this, &UDanceWheelWidget::HandleDanceClicked2);
		Btn_Dance_2->OnHovered.AddDynamic(this, &UDanceWheelWidget::HandleDanceHovered2);
	}
	if (Btn_Dance_3)
	{
		Btn_Dance_3->OnClicked.AddDynamic(this, &UDanceWheelWidget::HandleDanceClicked3);
		Btn_Dance_3->OnHovered.AddDynamic(this, &UDanceWheelWidget::HandleDanceHovered3);
	}
	if (Btn_Dance_4)
	{
		Btn_Dance_4->OnClicked.AddDynamic(this, &UDanceWheelWidget::HandleDanceClicked4);
		Btn_Dance_4->OnHovered.AddDynamic(this, &UDanceWheelWidget::HandleDanceHovered4);
	}
	if (Btn_Dance_5)
	{
		Btn_Dance_5->OnClicked.AddDynamic(this, &UDanceWheelWidget::HandleDanceClicked5);
		Btn_Dance_5->OnHovered.AddDynamic(this, &UDanceWheelWidget::HandleDanceHovered5);
	}
	if (Btn_Dance_6)
	{
		Btn_Dance_6->OnClicked.AddDynamic(this, &UDanceWheelWidget::HandleDanceClicked6);
		Btn_Dance_6->OnHovered.AddDynamic(this, &UDanceWheelWidget::HandleDanceHovered6);
	}
	if (Btn_Dance_7)
	{
		Btn_Dance_7->OnClicked.AddDynamic(this, &UDanceWheelWidget::HandleDanceClicked7);
		Btn_Dance_7->OnHovered.AddDynamic(this, &UDanceWheelWidget::HandleDanceHovered7);
	}
}

void UDanceWheelWidget::HandleDismissClicked()
{
	CloseWheel();
}

void UDanceWheelWidget::HandleDanceClicked(int32 DanceIndex)
{
	PlayUiSound(ClickSound, ClickSoundStartTime);
	CloseWheel();
	PlayDance(DanceIndex);
}

void UDanceWheelWidget::HandleDanceHovered(int32 DanceIndex)
{
	(void)DanceIndex;
	PlayUiSound(HoverSound, HoverSoundStartTime);
}

void UDanceWheelWidget::PlayUiSound(USoundBase* Sound, float StartTime) const
{
	if (Sound)
	{
		UGameplayStatics::PlaySound2D(this, Sound, 1.f, 1.f, StartTime);
	}
}

void UDanceWheelWidget::HandleDanceClicked0() { HandleDanceClicked(0); }
void UDanceWheelWidget::HandleDanceClicked1() { HandleDanceClicked(1); }
void UDanceWheelWidget::HandleDanceClicked2() { HandleDanceClicked(2); }
void UDanceWheelWidget::HandleDanceClicked3() { HandleDanceClicked(3); }
void UDanceWheelWidget::HandleDanceClicked4() { HandleDanceClicked(4); }
void UDanceWheelWidget::HandleDanceClicked5() { HandleDanceClicked(5); }
void UDanceWheelWidget::HandleDanceClicked6() { HandleDanceClicked(6); }
void UDanceWheelWidget::HandleDanceClicked7() { HandleDanceClicked(7); }

void UDanceWheelWidget::HandleDanceHovered0() { HandleDanceHovered(0); }
void UDanceWheelWidget::HandleDanceHovered1() { HandleDanceHovered(1); }
void UDanceWheelWidget::HandleDanceHovered2() { HandleDanceHovered(2); }
void UDanceWheelWidget::HandleDanceHovered3() { HandleDanceHovered(3); }
void UDanceWheelWidget::HandleDanceHovered4() { HandleDanceHovered(4); }
void UDanceWheelWidget::HandleDanceHovered5() { HandleDanceHovered(5); }
void UDanceWheelWidget::HandleDanceHovered6() { HandleDanceHovered(6); }
void UDanceWheelWidget::HandleDanceHovered7() { HandleDanceHovered(7); }
