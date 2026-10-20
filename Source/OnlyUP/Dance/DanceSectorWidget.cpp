#include "DanceSectorWidget.h"

#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Components/Button.h"
#include "Components/ContentWidget.h"
#include "Components/PanelWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DanceSectorWidget)

namespace DanceSectorPrivate
{
	void PushChildren(UWidget* Widget, TArray<UWidget*>& Pending)
	{
		if (UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
		{
			for (int32 ChildIndex = Panel->GetChildrenCount() - 1; ChildIndex >= 0; --ChildIndex)
			{
				Pending.Add(Panel->GetChildAt(ChildIndex));
			}
		}
		else if (UContentWidget* Content = Cast<UContentWidget>(Widget))
		{
			if (UWidget* Child = Content->GetContent())
			{
				Pending.Add(Child);
			}
		}
	}

	int32 FindIndexUnderPanel(UDanceSectorWidget* Sector, UWidget* Panel)
	{
		TArray<UWidget*> Pending;
		PushChildren(Panel, Pending);
		int32 NextIndex = 0;
		while (Pending.Num() > 0)
		{
			UWidget* Widget = Pending.Pop();
			if (!Widget)
			{
				continue;
			}

			if (UDanceSectorWidget* Other = Cast<UDanceSectorWidget>(Widget))
			{
				if (Other == Sector)
				{
					return NextIndex;
				}
				++NextIndex;
			}
			PushChildren(Widget, Pending);
		}
		return 0;
	}
}

void UDanceSectorWidget::NativeConstruct()
{
	for (UWidget* Parent = GetParent(); Parent; Parent = Parent->GetParent())
	{
		if (Parent->GetFName() == TEXT("Panel_Sectors"))
		{
			Index = DanceSectorPrivate::FindIndexUnderPanel(this, Parent);
			break;
		}
	}

	Super::NativeConstruct();
}

void UDanceSectorWidget::OnSectorHovered_Implementation()
{
}

void UDanceSectorWidget::OnSectorUnhovered_Implementation()
{
}

void UDanceSectorWidget::OnWheelUIOpened_Implementation()
{
}

void UDanceSectorWidget::SetSectorIndex(int32 InIndex)
{
	Index = InIndex;
}

void UDanceSectorWidget::SetButtonScale(float Scale)
{
	if (DanceSectorButton)
	{
		DanceSectorButton->SetRenderScale(FVector2D(Scale));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[DanceWheel] %s is missing button DanceSectorButton."), *GetName());
	}
}

void UDanceSectorWidget::ResetToDefault()
{
	UWidgetBlueprintGeneratedClass* GeneratedClass = Cast<UWidgetBlueprintGeneratedClass>(GetClass());
	if (!GeneratedClass)
	{
		return;
	}

	for (UWidgetAnimation* Animation : GeneratedClass->Animations)
	{
		if (!Animation)
		{
			continue;
		}

		PlayAnimation(Animation, 0.f, 1, EUMGSequencePlayMode::Forward, 1.f, false);
		SetAnimationCurrentTime(Animation, 0.f);
		StopAnimation(Animation);
	}
}
