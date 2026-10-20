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
		SetLookIgnored(false);
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

	InputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &ADanceWheelHost::OpenFromTab);
	InputComponent->BindKey(EKeys::Tab, IE_Released, this, &ADanceWheelHost::CloseFromTab);
	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ADanceWheelHost::ConfirmFromClick);
	for (FInputKeyBinding& Binding : InputComponent->KeyBindings)
	{
		if (Binding.Chord.Key == EKeys::Tab)
		{
			Binding.bConsumeInput = true;
		}
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

void ADanceWheelHost::OpenFromTab()
{
	if (bClosedByClickWhileHeld)
	{
		return;
	}

	EnsureWidget();
	if (!WheelWidget || bWheelOpen)
	{
		return;
	}

	bWheelOpen = true;
	WheelWidget->OpenWheel();
	ApplyInputMode(true);
	SetLookIgnored(true);
	UpdateClickConsume();
}

void ADanceWheelHost::CloseFromTab()
{
	bClosedByClickWhileHeld = false;
	if (!bWheelOpen || !WheelWidget)
	{
		SetLookIgnored(false);
		return;
	}

	bCloseRequestedByTab = true;
	WheelWidget->CloseWheel();
	bCloseRequestedByTab = false;
}

void ADanceWheelHost::ConfirmFromClick()
{
	if (bWheelOpen && WheelWidget)
	{
		WheelWidget->ConfirmSelection();
	}
}

void ADanceWheelHost::HandleWheelClosedFromWidget()
{
	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!bCloseRequestedByTab && PC && PC->IsInputKeyDown(EKeys::Tab))
	{
		bClosedByClickWhileHeld = true;
	}

	bWheelOpen = false;
	ApplyInputMode(false);
	SetLookIgnored(false);
	UpdateClickConsume();
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
		PC->SetInputMode(Mode);
		PC->bShowMouseCursor = WheelWidget->ShouldShowCursorWhileOpen();
	}
	else
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->bShowMouseCursor = false;
	}
}

void ADanceWheelHost::SetLookIgnored(bool bIgnore)
{
	if (bLookIgnored == bIgnore)
	{
		return;
	}

	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PC)
	{
		return;
	}

	PC->SetIgnoreLookInput(bIgnore);
	bLookIgnored = bIgnore;
}

void ADanceWheelHost::UpdateClickConsume()
{
	if (!InputComponent)
	{
		return;
	}

	for (FInputKeyBinding& Binding : InputComponent->KeyBindings)
	{
		if (Binding.Chord.Key == EKeys::LeftMouseButton)
		{
			Binding.bConsumeInput = bWheelOpen;
		}
	}
}
