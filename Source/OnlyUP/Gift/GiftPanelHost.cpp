#include "GiftPanelHost.h"

#include "Blueprint/UserWidget.h"
#include "Components/InputComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "GiftEffectLibrary.h"
#include "GiftPanelWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GiftPanelHost)

AGiftPanelHost::AGiftPanelHost()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	bReplicates = false;
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}

void AGiftPanelHost::BeginPlay()
{
	Super::BeginPlay();
	BindInputIfNeeded();
	EnsureWidget();
}

void AGiftPanelHost::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		if (InputComponent)
		{
			PC->PopInputComponent(InputComponent);
		}
	}

	if (PanelWidget)
	{
		PanelWidget->RemoveFromParent();
		PanelWidget = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void AGiftPanelHost::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bInputBound)
	{
		BindInputIfNeeded();
	}
	if (!PanelWidget)
	{
		EnsureWidget();
	}
}

void AGiftPanelHost::BindInputIfNeeded()
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

	InputComponent->BindKey(FInputChord(EKeys::R, EModifierKey::Alt), IE_Pressed, this, &AGiftPanelHost::TogglePanel);
	PC->PushInputComponent(InputComponent);
	bInputBound = true;
	UE_LOG(LogTemp, Log, TEXT("[GiftPanel] Alt+R bound."));
}

void AGiftPanelHost::EnsureWidget()
{
	if (PanelWidget)
	{
		return;
	}

	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PC)
	{
		return;
	}

	PanelWidget = CreateWidget<UGiftPanelWidget>(PC, UGiftPanelWidget::StaticClass());
	if (!PanelWidget)
	{
		return;
	}

	TArray<FGiftPanelItemDesc> Items;

	auto AddItem = [&Items](const TCHAR* Id, const TCHAR* Label)
	{
		FGiftPanelItemDesc Desc;
		Desc.GiftId = Id;
		Desc.DisplayName = FText::FromString(Label);
		Items.Add(Desc);
	};

	AddItem(TEXT("FryingPan"), TEXT("扔平底锅"));
	AddItem(TEXT("BilliardBall"), TEXT("扔台球"));
	AddItem(TEXT("ThrowPoop"), TEXT("扔大便"));
	AddItem(TEXT("PoopEject"), TEXT("拉大便"));
	AddItem(TEXT("FireArrow"), TEXT("射火箭"));
	AddItem(TEXT("IceArrow"), TEXT("射冰箭"));
	AddItem(TEXT("JumpRope"), TEXT("跳绳"));

	PanelWidget->SetItems(Items);
	PanelWidget->OnGiftTriggered.BindUObject(this, &AGiftPanelHost::HandleGiftTriggered);
	PanelWidget->AddToViewport(1000);
	PanelWidget->SetVisibility(ESlateVisibility::Collapsed);

	UE_LOG(LogTemp, Log, TEXT("[GiftPanel] Widget created."));
}

void AGiftPanelHost::TogglePanel()
{
	EnsureWidget();
	if (!PanelWidget)
	{
		return;
	}

	const bool bShow = PanelWidget->GetVisibility() == ESlateVisibility::Collapsed;
	PanelWidget->SetPanelVisible(bShow);

	if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		if (bShow)
		{
			FInputModeGameAndUI Mode;
			Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			Mode.SetHideCursorDuringCapture(false);
			Mode.SetWidgetToFocus(PanelWidget->TakeWidget());
			PC->SetInputMode(Mode);
			PC->bShowMouseCursor = true;
		}
		else
		{
			const FInputModeGameOnly Mode;
			PC->SetInputMode(Mode);
			PC->bShowMouseCursor = false;
		}
	}
}

void AGiftPanelHost::HandleGiftTriggered(FName GiftId, int32 Count)
{
	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	AActor* Target = PC ? static_cast<AActor*>(PC->GetPawn()) : nullptr;

	UE_LOG(LogTemp, Log, TEXT("[GiftPanel] Trigger %s x%d target=%s"),
		*GiftId.ToString(), Count, *GetNameSafe(Target));

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green,
			FString::Printf(TEXT("触发 %s x%d"), *GiftId.ToString(), Count));
	}

	UGiftEffectLibrary::ExecuteGiftById(this, Target, GiftId, Count);
}
