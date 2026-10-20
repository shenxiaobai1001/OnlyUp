#include "DanceWheelWidget.h"

#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ContentWidget.h"
#include "Components/PanelWidget.h"
#include "DanceSectorWidget.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DanceWheelWidget)

UDanceWheelWidget::UDanceWheelWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bHasScriptImplementedTick = true;
}

void UDanceWheelWidget::NativeConstruct()
{
	Super::NativeConstruct();
	StopNavigation(this);
	if (WidgetTree)
	{
		StopNavigation(WidgetTree->RootWidget);
	}
	GenerateSectors();
	SetVisibility(ESlateVisibility::Collapsed);
}

void UDanceWheelWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (IsWheelOpen())
	{
		UpdateSelection();
	}
}

void UDanceWheelWidget::OpenWheel()
{
	GenerateSectors();
	for (UDanceSectorWidget* Sector : SectorWidgets)
	{
		if (Sector)
		{
			Sector->ResetToDefault();
			Sector->OnWheelUIOpened();
		}
	}
	SetSelectedIndex(INDEX_NONE);
	SetVisibility(ESlateVisibility::Visible);
}

void UDanceWheelWidget::CloseWheel()
{
	const bool bWasOpen = IsWheelOpen();
	SetSelectedIndex(INDEX_NONE);
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

void UDanceWheelWidget::ConfirmSelection()
{
	if (!IsWheelOpen() || SelectedIndex == INDEX_NONE)
	{
		return;
	}

	const int32 Index = SelectedIndex;
	PlayUiSound(ClickSound, ClickSoundStartTime);
	CloseWheel();
	PlayDance(Index);
}

void UDanceWheelWidget::PlayDance_Implementation(int32 DanceIndex)
{
	UE_LOG(LogTemp, Log, TEXT("[DanceWheel] PlayDance index=%d (empty)"), DanceIndex);
}

void UDanceWheelWidget::GenerateSectors()
{
	if (bSectorsGenerated)
	{
		return;
	}

	if (!Panel_Sectors)
	{
		if (!bLoggedMissingPanel)
		{
			bLoggedMissingPanel = true;
			UE_LOG(LogTemp, Warning, TEXT("[DanceWheel] Panel_Sectors must be a Canvas Panel."));
		}
		return;
	}

	if (SectorCount < 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DanceWheel] SectorCount must be at least 1."));
		return;
	}

	UClass* SectorClass = StaticLoadClass(
		UDanceSectorWidget::StaticClass(),
		nullptr,
		TEXT("/Game/UI/DanceWheelUI/WBP_DanceSector.WBP_DanceSector_C"));
	if (!SectorClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DanceWheel] WBP_DanceSector was not found."));
		return;
	}

	Panel_Sectors->ClearChildren();
	SectorWidgets.Reset();
	bSectorsGenerated = true;

	const float StepDegrees = 360.f / static_cast<float>(SectorCount);
	for (int32 Index = 0; Index < SectorCount; ++Index)
	{
		UDanceSectorWidget* Sector = CreateWidget<UDanceSectorWidget>(this, SectorClass);
		if (!Sector)
		{
			continue;
		}

		Sector->SetSectorIndex(Index);
		UCanvasPanelSlot* CanvasSlot = Panel_Sectors->AddChildToCanvas(Sector);
		if (!CanvasSlot)
		{
			continue;
		}

		Sector->SetButtonScale(SectorScale);

		const float Degrees = FirstSectorAngle + StepDegrees * static_cast<float>(Index);
		const float Radians = FMath::DegreesToRadians(Degrees);
		CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CanvasSlot->SetAutoSize(true);
		CanvasSlot->SetPosition(FVector2D(FMath::Sin(Radians) * SectorRadius, -FMath::Cos(Radians) * SectorRadius));
		StopNavigation(Sector);
		SectorWidgets.Add(Sector);
	}
}

void UDanceWheelWidget::StopNavigation(UWidget* Widget)
{
	if (!Widget)
	{
		return;
	}

	Widget->SetAllNavigationRules(EUINavigationRule::Stop, NAME_None);

	if (UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
	{
		for (int32 ChildIndex = 0; ChildIndex < Panel->GetChildrenCount(); ++ChildIndex)
		{
			StopNavigation(Panel->GetChildAt(ChildIndex));
		}
	}
	else if (UContentWidget* Content = Cast<UContentWidget>(Widget))
	{
		StopNavigation(Content->GetContent());
	}
}

void UDanceWheelWidget::UpdateSelection()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC || !Panel_Sectors || SectorWidgets.Num() == 0)
	{
		return;
	}

	float MouseX = 0.f;
	float MouseY = 0.f;
	if (!PC->GetMousePosition(MouseX, MouseY))
	{
		return;
	}

	FVector2D CenterAbsolute;
	if (!TryGetAbsoluteCenter(Panel_Sectors, CenterAbsolute))
	{
		return;
	}

	FVector2D Center;
	FVector2D UnusedViewport;
	USlateBlueprintLibrary::AbsoluteToViewport(this, CenterAbsolute, Center, UnusedViewport);

	FVector2D Mouse(MouseX, MouseY);
	FVector2D FromCenter = Mouse - Center;
	float Distance = FromCenter.Size();
	if (Distance > KINDA_SMALL_NUMBER)
	{
		LastRingDirection = FromCenter / Distance;
	}
	else
	{
		FromCenter = LastRingDirection;
		Distance = 0.f;
	}

	const float InnerRadius = FMath::Max(0.f, CursorInnerRadius);
	const float OuterRadius = FMath::Max(InnerRadius, CursorOuterRadius);
	if (Distance < InnerRadius || Distance > OuterRadius)
	{
		const float ClampedDistance = FMath::Clamp(Distance, InnerRadius, OuterRadius);
		Mouse = Center + LastRingDirection * ClampedDistance;
		PC->SetMouseLocation(FMath::RoundToInt(Mouse.X), FMath::RoundToInt(Mouse.Y));
	}

	if (FVector2D::Distance(Mouse, Center) <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	// 0 is straight up and increases clockwise. Shift by FirstSectorAngle so sector 0 is centered there.
	float FromUp = FMath::Atan2(Mouse.Y - Center.Y, Mouse.X - Center.X) + HALF_PI;
	FromUp -= FMath::DegreesToRadians(FirstSectorAngle);
	FromUp = FMath::UnwindRadians(FromUp);
	if (FromUp < 0.f)
	{
		FromUp += TWO_PI;
	}

	const int32 Count = SectorWidgets.Num();
	const float Sector = TWO_PI / static_cast<float>(Count);
	const int32 Index = FMath::FloorToInt((FromUp + Sector * 0.5f) / Sector) % Count;
	SetSelectedIndex(Index);
}

void UDanceWheelWidget::SetSelectedIndex(int32 NewIndex)
{
	if (SelectedIndex == NewIndex)
	{
		return;
	}

	const int32 Previous = SelectedIndex;
	SelectedIndex = NewIndex;

	const bool bSwitchSector = Previous != INDEX_NONE && NewIndex != INDEX_NONE;
	if (bSwitchSector)
	{
		if (SectorWidgets.IsValidIndex(Previous))
		{
			SectorWidgets[Previous]->OnSectorUnhovered();
		}
		OnSectorDeselected(Previous);
	}
	if (SelectedIndex != INDEX_NONE)
	{
		if (SectorWidgets.IsValidIndex(SelectedIndex))
		{
			SectorWidgets[SelectedIndex]->OnSectorHovered();
		}
		OnSectorSelected(SelectedIndex);
		PlayUiSound(HoverSound, HoverSoundStartTime);
	}
}

void UDanceWheelWidget::PlayUiSound(USoundBase* Sound, float StartTime) const
{
	if (Sound)
	{
		UGameplayStatics::PlaySound2D(this, Sound, 1.f, 1.f, StartTime);
	}
}

bool UDanceWheelWidget::TryGetAbsoluteCenter(UWidget* Widget, FVector2D& OutCenter) const
{
	if (!Widget)
	{
		return false;
	}

	const FGeometry& Geometry = Widget->GetCachedGeometry();
	const FVector2D Size = Geometry.GetAbsoluteSize();
	if (Size.X <= KINDA_SMALL_NUMBER || Size.Y <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	OutCenter = Geometry.GetAbsolutePosition() + Size * 0.5f;
	return true;
}
