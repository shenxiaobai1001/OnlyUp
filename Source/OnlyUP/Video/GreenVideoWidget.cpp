// Copyright Epic Games, Inc. All Rights Reserved.

#include "GreenVideoWidget.h"

#include "Components/Image.h"
#include "VideoViewportUtil.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "MediaPlayer.h"
#include "MediaTexture.h"
#include "MediaSoundComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/GameViewportSubsystem.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Styling/SlateBrush.h"
#include "Widgets/Layout/Anchors.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Camera/CameraTypes.h"

void UGreenVideoWidget::EnsureVideoImage()
{
	if (VideoImage)
	{
		return;
	}

	if (!WidgetTree)
	{
		return;
	}

	if (!WidgetTree->RootWidget)
	{
		VideoImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("VideoImage"));
		WidgetTree->RootWidget = VideoImage;
	}
	else
	{
		VideoImage = WidgetTree->FindWidget<UImage>(TEXT("VideoImage"));
		if (!VideoImage)
		{
			VideoImage = Cast<UImage>(WidgetTree->RootWidget);
		}
	}
}

void UGreenVideoWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	EnsureVideoImage();
	EnsureMediaObjects();
}

TSharedRef<SWidget> UGreenVideoWidget::RebuildWidget()
{
	EnsureVideoImage();
	return Super::RebuildWidget();
}

void UGreenVideoWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureVideoImage();
	EnsureMediaObjects();
	EnsureMediaSound();
	BindMediaEvents();

	if (!bHasRevealed)
	{
		HideUntilFirstFrame();
	}
}

void UGreenVideoWidget::NativeDestruct()
{
	StopWaitForFirstFrame();
	UnbindMediaEvents();
	if (MediaPlayer)
	{
		MediaPlayer->Close();
	}
	if (MediaSound)
	{
		MediaSound->SetMediaPlayer(nullptr);
		if (MediaSound->IsRegistered())
		{
			MediaSound->UnregisterComponent();
		}
		MediaSound = nullptr;
	}
	Super::NativeDestruct();
}

bool UGreenVideoWidget::PlayRequest(const FVideoPlayRequest& Request)
{
	EnsureVideoImage();
	EnsureMediaObjects();
	EnsureMediaSound();
	BindMediaEvents();
	ActiveRequest = Request;
	bHasFinished = false;
	bHasRevealed = false;
	ConsecutiveValidFrameTicks = 0;
	PendingViewportZOrder = Request.ZOrder;

	HideUntilFirstFrame();
	if (Request.bDisableChromaKey)
	{
		// 非绿幕：强制走 Fallback，避免抠像把画面抠成全透明（只剩声音）
		DefaultChromaMaterial.Reset();
		bUsingFallbackDisplay = true;
		ChromaMID = nullptr;
		EnsureFallbackDisplayMaterial();
		RefreshMediaMaterial();
	}
	else
	{
		ApplyChromaMaterial(Request.ChromaMaterial);
	}

	if (!VideoImage)
	{
		VideoImage = BindVideoImage();
	}

	ApplyViewportLayout();

	if (MediaSound)
	{
		MediaSound->SetVolumeMultiplier(Request.Volume);
	}

	if (Request.MediaSource)
	{
		return OpenAndPlay(Request.MediaSource, Request.bLoop);
	}

	OnFailed.Broadcast(this, TEXT("PlayRequest missing MediaSource"));
	return false;
}

bool UGreenVideoWidget::PlayMediaSource(UMediaSource* Source, bool bLoop)
{
	EnsureMediaObjects();
	return OpenAndPlay(Source, bLoop);
}

void UGreenVideoWidget::StopPlayback()
{
	StopWaitForFirstFrame();
	UnbindMediaEvents();
	if (MediaPlayer)
	{
		MediaPlayer->Close();
	}
	if (MediaSound)
	{
		MediaSound->SetMediaPlayer(nullptr);
	}
	RemoveFromParent();
}

bool UGreenVideoWidget::IsPlaybackActive() const
{
	return MediaPlayer && MediaPlayer->IsPlaying();
}

void UGreenVideoWidget::ApplyChromaMaterial_Implementation(const TSoftObjectPtr<UMaterialInterface>& Material)
{
	bUsingFallbackDisplay = false;
	ChromaMID = nullptr;

	UMaterialInterface* Parent = ResolveChromaMaterial(Material);
	if (Parent)
	{
		ChromaMID = UMaterialInstanceDynamic::Create(Parent, this);
		if (ChromaMID && MediaTexture)
		{
			MediaTexture->SetMediaPlayer(MediaPlayer);
			// ???????????????????
			ChromaMID->SetTextureParameterValue(TextureParamName, MediaTexture);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("UGreenVideoWidget: chroma material missing — will fallback. Assign DefaultChromaMaterial (/Game/Videos/M_UI_GreenChroma) or Request.ChromaMaterial. Missing this often causes sound-without-picture."));
		EnsureFallbackDisplayMaterial();
		bUsingFallbackDisplay = true;
	}

	RefreshMediaMaterial();
}

void UGreenVideoWidget::EnsureFallbackDisplayMaterial()
{
	UMaterialInterface* Parent = FallbackDisplayMaterial.LoadSynchronous();
	if (!Parent)
	{
		Parent = LoadObject<UMaterialInterface>(
			nullptr, TEXT("/Engine/EngineMaterials/Widget3DPassThrough_Translucent.Widget3DPassThrough_Translucent"));
		if (!Parent)
		{
			Parent = LoadObject<UMaterialInterface>(
				nullptr, TEXT("/Engine/EngineMaterials/Widget3DPassThrough_Opaque.Widget3DPassThrough_Opaque"));
		}
	}

	if (!Parent)
	{
		UE_LOG(LogTemp, Error, TEXT("UGreenVideoWidget: failed to load fallback display material."));
		return;
	}

	ChromaMID = UMaterialInstanceDynamic::Create(Parent, this);
	if (ChromaMID && MediaTexture)
	{
		MediaTexture->SetMediaPlayer(MediaPlayer);
		ChromaMID->SetTextureParameterValue(TEXT("SlateUI"), MediaTexture);
		ChromaMID->SetTextureParameterValue(TEXT("Texture"), MediaTexture);
		ChromaMID->SetTextureParameterValue(TEXT("MediaTexture"), MediaTexture);
	}
}

UImage* UGreenVideoWidget::BindVideoImage_Implementation()
{
	if (VideoImage)
	{
		return VideoImage;
	}

	if (!WidgetTree)
	{
		return nullptr;
	}

	return WidgetTree->FindWidget<UImage>(TEXT("VideoImage"));
}

bool UGreenVideoWidget::OpenAndPlay(UMediaSource* Source, bool bLoop)
{
	if (!MediaPlayer || !Source)
	{
		OnFailed.Broadcast(this, TEXT("MediaPlayer or Source is null"));
		return false;
	}

	EnsureMediaSound();
	bHasFinished = false;
	bHasRevealed = false;
	HideUntilFirstFrame();

	if (MediaTexture)
	{
		MediaTexture->SetMediaPlayer(MediaPlayer);
		MediaTexture->AutoClear = true;
		MediaTexture->ClearColor = FLinearColor(0.f, 1.f, 0.f, 1.f);
		MediaTexture->UpdateResource();
	}

	if (MediaSound)
	{
		MediaSound->SetMediaPlayer(MediaPlayer);
	}

	MediaPlayer->SetLooping(bLoop);
	if (!MediaPlayer->OpenSource(Source))
	{
		OnFailed.Broadcast(this, TEXT("OpenSource failed"));
		return false;
	}

	RefreshMediaMaterial();
	return true;
}

void UGreenVideoWidget::EnsureMediaObjects()
{
	if (!MediaPlayer)
	{
		MediaPlayer = NewObject<UMediaPlayer>(this, TEXT("MediaPlayer"));
		MediaPlayer->PlayOnOpen = true;
	}

	if (!MediaTexture)
	{
		MediaTexture = NewObject<UMediaTexture>(this, TEXT("MediaTexture"));
		MediaTexture->AutoClear = true;
		MediaTexture->ClearColor = FLinearColor(0.f, 1.f, 0.f, 1.f);
	}

	MediaTexture->SetMediaPlayer(MediaPlayer);
	MediaTexture->UpdateResource();
}

void UGreenVideoWidget::EnsureMediaSound()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (!MediaSound)
	{
		MediaSound = NewObject<UMediaSoundComponent>(this, TEXT("MediaSound"));
		MediaSound->bAutoActivate = true;
		MediaSound->bAllowSpatialization = false;
		MediaSound->RegisterComponentWithWorld(World);
	}

	if (MediaPlayer)
	{
		MediaSound->SetMediaPlayer(MediaPlayer);
	}
}

void UGreenVideoWidget::BindMediaEvents()
{
	EnsureMediaObjects();
	if (!MediaPlayer || bMediaEventsBound)
	{
		return;
	}

	MediaPlayer->OnMediaOpened.AddDynamic(this, &UGreenVideoWidget::HandleMediaOpened);
	MediaPlayer->OnMediaOpenFailed.AddDynamic(this, &UGreenVideoWidget::HandleMediaOpenFailed);
	MediaPlayer->OnEndReached.AddDynamic(this, &UGreenVideoWidget::HandleEndReached);
	bMediaEventsBound = true;
}

void UGreenVideoWidget::UnbindMediaEvents()
{
	if (!MediaPlayer || !bMediaEventsBound)
	{
		return;
	}

	MediaPlayer->OnMediaOpened.RemoveDynamic(this, &UGreenVideoWidget::HandleMediaOpened);
	MediaPlayer->OnMediaOpenFailed.RemoveDynamic(this, &UGreenVideoWidget::HandleMediaOpenFailed);
	MediaPlayer->OnEndReached.RemoveDynamic(this, &UGreenVideoWidget::HandleEndReached);
	bMediaEventsBound = false;
}

void UGreenVideoWidget::RefreshMediaMaterial()
{
	if (!MediaTexture)
	{
		return;
	}

	if (!VideoImage)
	{
		VideoImage = BindVideoImage();
	}
	if (!VideoImage)
	{
		return;
	}

	MediaTexture->SetMediaPlayer(MediaPlayer);

	if (!ChromaMID && bUsingFallbackDisplay)
	{
		EnsureFallbackDisplayMaterial();
	}

	if (!ChromaMID)
	{
		return;
	}

	if (bUsingFallbackDisplay)
	{
		ChromaMID->SetTextureParameterValue(TEXT("SlateUI"), MediaTexture);
		ChromaMID->SetTextureParameterValue(TEXT("Texture"), MediaTexture);
		ChromaMID->SetTextureParameterValue(TEXT("MediaTexture"), MediaTexture);
	}
	else
	{
		ChromaMID->SetTextureParameterValue(TextureParamName, MediaTexture);
	}

	VideoImage->SetBrushFromMaterial(ChromaMID);

	FSlateBrush Brush = VideoImage->GetBrush();
	Brush.TintColor = FSlateColor(FLinearColor::White);
	Brush.DrawAs = ESlateBrushDrawType::Image;

	// ???????????? ImageSize ????????????????????? DPI ????????????
	if (ActiveRequest.ScreenSize.X > 0.f && ActiveRequest.ScreenSize.Y > 0.f)
	{
		Brush.ImageSize = ActiveRequest.ScreenSize;
		VideoImage->SetBrush(Brush);
		VideoImage->SetDesiredSizeOverride(ActiveRequest.ScreenSize);
	}
	else
	{
		Brush.ImageSize = FVector2D(32.f, 32.f);
		VideoImage->SetBrush(Brush);
	}
	VideoImage->SetColorAndOpacity(FLinearColor::White);

	if (!bHasRevealed)
	{
		VideoImage->SetVisibility(ESlateVisibility::Collapsed);
		SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		VideoImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}

bool UGreenVideoWidget::TryGetConstrainedViewPixelRect(FVector2D& OutPos, FVector2D& OutSize) const
{
	return UVideoViewportUtil::TryGetGameViewPixelRect(this, OutPos, OutSize);
}

void UGreenVideoWidget::ApplyViewportLayout()
{
	UGameViewportSubsystem* Subsystem = UGameViewportSubsystem::Get();
	if (!Subsystem)
	{
		return;
	}

	FGameViewportWidgetSlot ViewportSlot;
	if (Subsystem->IsWidgetAdded(this))
	{
		ViewportSlot = Subsystem->GetWidgetSlot(this);
	}
	ViewportSlot.ZOrder = PendingViewportZOrder;

	const bool bExplicitSize = ActiveRequest.ScreenSize.X > 0.f && ActiveRequest.ScreenSize.Y > 0.f;
	const float DpiScale = FMath::Max(UWidgetLayoutLibrary::GetViewportScale(this), KINDA_SMALL_NUMBER);
	FVector2D LayoutSize = FVector2D::ZeroVector;
	FVector2D LayoutPos = FVector2D::ZeroVector;
	FVector2D LayoutOffset = FVector2D::ZeroVector;
	bool bUseAbsolutePixelRect = false;
	bool bUseFixedCenteredSize = false;

	if (bExplicitSize)
	{
		LayoutSize = ActiveRequest.ScreenSize;
		LayoutOffset = ActiveRequest.ScreenOffset;
		bUseFixedCenteredSize = true;
	}
	else if (ActiveRequest.bMatchCameraAspect)
	{
		FVector2D ConstrainedPos;
		FVector2D ConstrainedSize;
		if (TryGetConstrainedViewPixelRect(ConstrainedPos, ConstrainedSize))
		{
			// OutPos/OutSize 须与主画面锁定矩形一致（含项目自定义 Bias）
			LayoutPos = ConstrainedPos / DpiScale;
			LayoutSize = ConstrainedSize / DpiScale;
			bUseAbsolutePixelRect = true;
		}
	}

	if (bUseAbsolutePixelRect)
	{
		ViewportSlot.Anchors = FAnchors(0.f, 0.f);
		ViewportSlot.Alignment = FVector2D(0.f, 0.f);
		ViewportSlot.Offsets = FMargin(LayoutPos.X, LayoutPos.Y, LayoutSize.X, LayoutSize.Y);
	}
	else if (bUseFixedCenteredSize)
	{
		ViewportSlot.Anchors = FAnchors(0.5f, 0.5f);
		ViewportSlot.Alignment = FVector2D(0.5f, 0.5f);
		ViewportSlot.Offsets = FMargin(LayoutOffset.X, LayoutOffset.Y, LayoutSize.X, LayoutSize.Y);
	}
	else
	{
		ViewportSlot.Anchors = FAnchors(0.f, 0.f, 1.f, 1.f);
		ViewportSlot.Offsets = FMargin(0.f);
		ViewportSlot.Alignment = FVector2D(0.f, 0.f);
	}

	Subsystem->SetWidgetSlot(this, ViewportSlot);

	if (VideoImage && (bUseAbsolutePixelRect || bUseFixedCenteredSize))
	{
		VideoImage->SetDesiredSizeOverride(LayoutSize);
	}
}

void UGreenVideoWidget::HideUntilFirstFrame()
{
	bHasRevealed = false;
	SetRenderOpacity(0.f);
	if (VideoImage)
	{
		VideoImage->SetVisibility(ESlateVisibility::Collapsed);
		VideoImage->SetRenderOpacity(0.f);
	}
	SetVisibility(ESlateVisibility::Collapsed);
}

void UGreenVideoWidget::StartWaitForFirstFrame()
{
	StopWaitForFirstFrame();
	bWaitingForFirstFrame = true;
	ConsecutiveValidFrameTicks = 0;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			FirstFrameTimerHandle,
			this,
			&UGreenVideoWidget::HandleFirstFrameTick,
			0.016f,
			true);

		World->GetTimerManager().SetTimer(
			FirstFrameTimeoutHandle,
			this,
			&UGreenVideoWidget::HandleFirstFrameTimeout,
			3.0f,
			false);
	}

	HandleFirstFrameTick();
}

void UGreenVideoWidget::HandleFirstFrameTimeout()
{
	if (bHasRevealed)
	{
		return;
	}

	// ?????????????????????????????????????????? 0 ????????
	if (MediaPlayer && MediaPlayer->IsPlaying())
	{
		bHasRevealed = true;
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(FirstFrameTimerHandle);
			World->GetTimerManager().ClearTimer(FirstFrameTimeoutHandle);
		}
		FirstFrameTimerHandle.Invalidate();
		FirstFrameTimeoutHandle.Invalidate();
		bWaitingForFirstFrame = false;
		ConsecutiveValidFrameTicks = 0;

		RefreshMediaMaterial();
		SetRenderOpacity(0.f);
		if (VideoImage)
		{
			VideoImage->SetRenderOpacity(0.f);
			VideoImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		if (!IsInViewport())
		{
			AddToViewport(PendingViewportZOrder);
			ApplyViewportLayout();
		}
		HandleRevealFadeTick();
		return;
	}

	StopWaitForFirstFrame();
	UE_LOG(LogTemp, Warning, TEXT("UGreenVideoWidget: first frame timeout and not playing."));
}

void UGreenVideoWidget::StopWaitForFirstFrame()
{
	bWaitingForFirstFrame = false;
	ConsecutiveValidFrameTicks = 0;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FirstFrameTimerHandle);
		World->GetTimerManager().ClearTimer(FirstFrameTimeoutHandle);
		World->GetTimerManager().ClearTimer(RevealFadeTimerHandle);
	}
	FirstFrameTimerHandle.Invalidate();
	FirstFrameTimeoutHandle.Invalidate();
	RevealFadeTimerHandle.Invalidate();
}

bool UGreenVideoWidget::HasValidVideoFrame() const
{
	if (!MediaTexture || !MediaPlayer)
	{
		return false;
	}

	if (!MediaPlayer->IsPlaying())
	{
		return false;
	}

	const double Seconds = MediaPlayer->GetTime().GetTotalSeconds();
	if (Seconds < 0.08)
	{
		return false;
	}

	// ?????????????????? 0??????????????????????????????? Mask??
	const int32 W = MediaTexture->GetWidth();
	const int32 H = MediaTexture->GetHeight();
	return (W > 2 && H > 2) || Seconds >= 0.12;
}

void UGreenVideoWidget::RevealWhenReady()
{
	if (bHasRevealed)
	{
		return;
	}

	if (!HasValidVideoFrame())
	{
		return;
	}

	bHasRevealed = true;

	// ??? StopWaitForFirstFrame???????????????????????????????
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FirstFrameTimerHandle);
		World->GetTimerManager().ClearTimer(FirstFrameTimeoutHandle);
	}
	FirstFrameTimerHandle.Invalidate();
	FirstFrameTimeoutHandle.Invalidate();
	bWaitingForFirstFrame = false;
	ConsecutiveValidFrameTicks = 0;

	RefreshMediaMaterial();

	SetRenderOpacity(0.f);
	if (VideoImage)
	{
		VideoImage->SetRenderOpacity(0.f);
		VideoImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (!IsInViewport())
	{
		AddToViewport(PendingViewportZOrder);
		ApplyViewportLayout();
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			RevealFadeTimerHandle,
			this,
			&UGreenVideoWidget::HandleRevealFadeTick,
			0.05f,
			false);
	}
	else
	{
		HandleRevealFadeTick();
	}
}

void UGreenVideoWidget::HandleRevealFadeTick()
{
	SetRenderOpacity(1.f);
	if (VideoImage)
	{
		VideoImage->SetRenderOpacity(1.f);
	}
	OnStarted.Broadcast(this);
}

void UGreenVideoWidget::HandleFirstFrameTick()
{
	if (!bWaitingForFirstFrame || bHasRevealed)
	{
		return;
	}

	if (HasValidVideoFrame())
	{
		++ConsecutiveValidFrameTicks;
		if (ConsecutiveValidFrameTicks >= 3)
		{
			RevealWhenReady();
		}
	}
	else
	{
		ConsecutiveValidFrameTicks = 0;
	}
}

UMaterialInterface* UGreenVideoWidget::ResolveChromaMaterial(const TSoftObjectPtr<UMaterialInterface>& Material) const
{
	if (!Material.IsNull())
	{
		return Material.LoadSynchronous();
	}
	return DefaultChromaMaterial.LoadSynchronous();
}

void UGreenVideoWidget::HandleMediaOpened(FString OpenedUrl)
{
	if (MediaPlayer && !MediaPlayer->IsPlaying())
	{
		MediaPlayer->Play();
	}

	RefreshMediaMaterial();
	StartWaitForFirstFrame();
}

void UGreenVideoWidget::HandleMediaOpenFailed(FString FailedUrl)
{
	StopWaitForFirstFrame();
	OnFailed.Broadcast(this, FString::Printf(TEXT("Open failed: %s"), *FailedUrl));
}

void UGreenVideoWidget::HandleEndReached()
{
	if (bHasFinished || (MediaPlayer && MediaPlayer->IsLooping()))
	{
		return;
	}

	bHasFinished = true;
	StopWaitForFirstFrame();
	OnFinished.Broadcast(this);

	if (ActiveRequest.bAutoDestroy)
	{
		RemoveFromParent();
	}
}
