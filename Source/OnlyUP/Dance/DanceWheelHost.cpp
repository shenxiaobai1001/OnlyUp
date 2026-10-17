#include "DanceWheelHost.h"

#include "Blueprint/UserWidget.h"
#include "Components/InputComponent.h"
#include "DanceWheelWidget.h"
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DanceWheelHost)

namespace DanceWheelPrivate
{
	const TCHAR* WidgetPath = TEXT("/Game/UI/DanceWheelUI/WBP_DanceWheel.WBP_DanceWheel_C");
}

ADanceWheelHost::ADanceWheelHost()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	bReplicates = false;
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}

void ADanceWheelHost::BeginPlay()
{
	Super::BeginPlay();
	BindInputIfNeeded();
	EnsureWidget();
}

void ADanceWheelHost::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		if (InputComponent)
		{
			PC->PopInputComponent(InputComponent);
		}
		if (bWheelOpen)
		{
			ApplyInputMode(false);
		}
	}

	if (WheelWidget)
	{
		WheelWidget->RemoveFromParent();
		WheelWidget = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void ADanceWheelHost::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bInputBound)
	{
		BindInputIfNeeded();
	}
	if (!WheelWidget)
	{
		EnsureWidget();
	}
}

void ADanceWheelHost::BindInputIfNeeded()
{
	if (bInputBound)
	{
		return;
	}

	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PC)
	{
		return;
	}

	EnableInput(PC);
	if (!InputComponent)
	{
		return;
	}

	InputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &ADanceWheelHost::ToggleWheel);
	if (InputComponent->KeyBindings.Num() > 0)
	{
		InputComponent->KeyBindings.Last().bConsumeInput = true;
	}
	PC->PushInputComponent(InputComponent);
	bInputBound = true;
	UE_LOG(LogTemp, Log, TEXT("[DanceWheel] Tab bound."));
}

void ADanceWheelHost::EnsureWidget()
{
	if (WheelWidget)
	{
		return;
	}

	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PC)
	{
		return;
	}

	UClass* WidgetClass = StaticLoadClass(UDanceWheelWidget::StaticClass(), nullptr, DanceWheelPrivate::WidgetPath);
	if (!WidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DanceWheel] Widget not found at %s. Create a UDanceWheelWidget blueprint there."), DanceWheelPrivate::WidgetPath);
		return;
	}

	WheelWidget = CreateWidget<UDanceWheelWidget>(PC, WidgetClass);
	if (!WheelWidget)
	{
		return;
	}

	WheelWidget->AddToViewport(1000);
	WheelWidget->OnWheelClosed.AddUObject(this, &ADanceWheelHost::HandleWheelClosedFromWidget);
	WheelWidget->CloseWheel();
	UE_LOG(LogTemp, Log, TEXT("[DanceWheel] Widget created."));
}

void ADanceWheelHost::ToggleWheel()
{
	EnsureWidget();
	if (!WheelWidget)
	{
		return;
	}

	if (!bWheelOpen)
	{
		bWheelOpen = true;
		WheelWidget->OpenWheel();
		ApplyInputMode(true);
		return;
	}

	WheelWidget->CloseWheel();
}

void ADanceWheelHost::HandleWheelClosedFromWidget()
{
	bWheelOpen = false;
	ApplyInputMode(false);
}

void ADanceWheelHost::ApplyInputMode(bool bOpen)
{
	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PC)
	{
		return;
	}

	if (bOpen && WheelWidget)
	{
		FInputModeGameAndUI Mode;
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Mode.SetHideCursorDuringCapture(false);
		Mode.SetWidgetToFocus(WheelWidget->TakeWidget());
		PC->SetInputMode(Mode);
		PC->bShowMouseCursor = true;
	}
	else
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->bShowMouseCursor = false;
	}
}
