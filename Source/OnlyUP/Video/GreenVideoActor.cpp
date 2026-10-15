// Copyright Epic Games, Inc. All Rights Reserved.

#include "GreenVideoActor.h"

#include "GreenVideoSurfaceWidget.h"
#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "MediaPlayer.h"
#include "MediaSoundComponent.h"
#include "MediaTexture.h"

AGreenVideoActor::AGreenVideoActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	VideoScreen = CreateDefaultSubobject<UWidgetComponent>(TEXT("VideoScreen"));
	VideoScreen->SetupAttachment(RootScene);
	VideoScreen->SetWidgetSpace(EWidgetSpace::World);
	VideoScreen->SetDrawAtDesiredSize(false);
	VideoScreen->SetDrawSize(DefaultDrawSize);
	VideoScreen->SetPivot(FVector2D(0.5f, 0.5f));
	VideoScreen->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VideoScreen->SetTwoSided(true);
	VideoScreen->SetBlendMode(EWidgetBlendMode::Transparent);
	VideoScreen->SetTickWhenOffscreen(true);

	MediaSound = CreateDefaultSubobject<UMediaSoundComponent>(TEXT("MediaSound"));
	MediaSound->SetupAttachment(RootScene);
	MediaSound->bAllowSpatialization = false;

	MediaPlayer = CreateDefaultSubobject<UMediaPlayer>(TEXT("MediaPlayer"));
	MediaPlayer->PlayOnOpen = true;
	MediaPlayer->SetLooping(false);

	MediaTexture = CreateDefaultSubobject<UMediaTexture>(TEXT("MediaTexture"));
	MediaTexture->SetMediaPlayer(MediaPlayer);
	MediaTexture->AutoClear = true;
	MediaTexture->UpdateResource();

	MediaSound->SetMediaPlayer(MediaPlayer);
}

void AGreenVideoActor::BeginPlay()
{
	Super::BeginPlay();
	EnsureSurfaceWidget();
	BindMediaEvents();
}

void AGreenVideoActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindMediaEvents();
	if (MediaPlayer)
	{
		MediaPlayer->Close();
	}
	Super::EndPlay(EndPlayReason);
}

bool AGreenVideoActor::PlayRequest(const FVideoPlayRequest& Request)
{
	EnsureSurfaceWidget();
	BindMediaEvents();

	ActiveRequest = Request;
	bHasFinished = false;

	SetActorTransform(Request.WorldTransform);
	SetActorScale3D(Request.MeshScale);
	SetActorHiddenInGame(false);

	const FVector2D DrawSize = (Request.ScreenSize.X > 0.f && Request.ScreenSize.Y > 0.f)
		? Request.ScreenSize
		: DefaultDrawSize;
	VideoScreen->SetDrawSize(DrawSize);
	VideoScreen->SetVisibility(true);
	VideoScreen->SetHiddenInGame(false);

	if (SurfaceWidget)
	{
		SurfaceWidget->SetSurfaceSize(DrawSize);
	}

	ApplyChromaMaterial(Request.ChromaMaterial);

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

bool AGreenVideoActor::PlayMediaSource(UMediaSource* Source, bool bLoop)
{
	EnsureSurfaceWidget();
	BindMediaEvents();
	return OpenAndPlay(Source, bLoop);
}

void AGreenVideoActor::StopPlayback(bool bDestroyActor)
{
	UnbindMediaEvents();
	if (MediaPlayer)
	{
		MediaPlayer->Close();
	}

	if (bDestroyActor && !IsActorBeingDestroyed())
	{
		Destroy();
	}
}

bool AGreenVideoActor::IsPlaybackActive() const
{
	return MediaPlayer && MediaPlayer->IsPlaying();
}

void AGreenVideoActor::ApplyChromaMaterial_Implementation(const TSoftObjectPtr<UMaterialInterface>& Material)
{
	EnsureSurfaceWidget();
	bUsingChromaMaterial = false;
	ChromaMID = nullptr;

	UMaterialInterface* Parent = ResolveChromaMaterial(Material);
	if (Parent)
	{
		ChromaMID = UMaterialInstanceDynamic::Create(Parent, this);
		if (ChromaMID && MediaTexture)
		{
			MediaTexture->SetMediaPlayer(MediaPlayer);
			// 扣绿参数在材质内调，这里只注入影片纹理
			ChromaMID->SetTextureParameterValue(TextureParamName, MediaTexture);
			bUsingChromaMaterial = true;
		}
	}
	else
	{
		UMaterialInterface* Fallback = LoadObject<UMaterialInterface>(
			nullptr, TEXT("/Engine/EngineMaterials/Widget3DPassThrough_Translucent.Widget3DPassThrough_Translucent"));
		if (!Fallback)
		{
			Fallback = LoadObject<UMaterialInterface>(
				nullptr, TEXT("/Engine/EngineMaterials/Widget3DPassThrough_Opaque.Widget3DPassThrough_Opaque"));
		}
		if (Fallback)
		{
			ChromaMID = UMaterialInstanceDynamic::Create(Fallback, this);
			if (ChromaMID && MediaTexture)
			{
				ChromaMID->SetTextureParameterValue(TEXT("SlateUI"), MediaTexture);
				ChromaMID->SetTextureParameterValue(TEXT("Texture"), MediaTexture);
				ChromaMID->SetTextureParameterValue(TEXT("MediaTexture"), MediaTexture);
			}
		}
	}

	RefreshDisplay();
}

void AGreenVideoActor::OnPlaybackStarted_Implementation()
{
	RefreshDisplay();
}

void AGreenVideoActor::OnPlaybackFinished_Implementation()
{
}

bool AGreenVideoActor::OpenAndPlay(UMediaSource* Source, bool bLoop)
{
	if (!MediaPlayer || !Source)
	{
		OnFailed.Broadcast(this, TEXT("MediaPlayer or Source is null"));
		return false;
	}

	bHasFinished = false;

	if (MediaTexture)
	{
		MediaTexture->SetMediaPlayer(MediaPlayer);
		MediaTexture->UpdateResource();
	}

	MediaPlayer->SetLooping(bLoop);
	if (!MediaPlayer->OpenSource(Source))
	{
		OnFailed.Broadcast(this, TEXT("OpenSource failed"));
		return false;
	}

	RefreshDisplay();
	return true;
}

void AGreenVideoActor::EnsureSurfaceWidget()
{
	if (!VideoScreen)
	{
		return;
	}

	if (!SurfaceWidget)
	{
		SurfaceWidget = Cast<UGreenVideoSurfaceWidget>(VideoScreen->GetWidget());
	}

	if (!SurfaceWidget)
	{
		UWorld* World = GetWorld();
		if (!World)
		{
			return;
		}

		SurfaceWidget = CreateWidget<UGreenVideoSurfaceWidget>(World, UGreenVideoSurfaceWidget::StaticClass());
		VideoScreen->SetWidget(SurfaceWidget);
	}
}

void AGreenVideoActor::BindMediaEvents()
{
	if (!MediaPlayer || bMediaEventsBound)
	{
		return;
	}

	MediaPlayer->OnMediaOpened.AddDynamic(this, &AGreenVideoActor::HandleMediaOpened);
	MediaPlayer->OnMediaOpenFailed.AddDynamic(this, &AGreenVideoActor::HandleMediaOpenFailed);
	MediaPlayer->OnEndReached.AddDynamic(this, &AGreenVideoActor::HandleEndReached);
	bMediaEventsBound = true;
}

void AGreenVideoActor::UnbindMediaEvents()
{
	if (!MediaPlayer || !bMediaEventsBound)
	{
		return;
	}

	MediaPlayer->OnMediaOpened.RemoveDynamic(this, &AGreenVideoActor::HandleMediaOpened);
	MediaPlayer->OnMediaOpenFailed.RemoveDynamic(this, &AGreenVideoActor::HandleMediaOpenFailed);
	MediaPlayer->OnEndReached.RemoveDynamic(this, &AGreenVideoActor::HandleEndReached);
	bMediaEventsBound = false;
}

void AGreenVideoActor::RefreshDisplay()
{
	EnsureSurfaceWidget();
	if (!SurfaceWidget || !MediaTexture)
	{
		return;
	}

	MediaTexture->SetMediaPlayer(MediaPlayer);

	if (ChromaMID)
	{
		if (bUsingChromaMaterial)
		{
			ChromaMID->SetTextureParameterValue(TextureParamName, MediaTexture);
		}
		else
		{
			ChromaMID->SetTextureParameterValue(TEXT("SlateUI"), MediaTexture);
			ChromaMID->SetTextureParameterValue(TEXT("Texture"), MediaTexture);
			ChromaMID->SetTextureParameterValue(TEXT("MediaTexture"), MediaTexture);
		}
		SurfaceWidget->SetVideoMaterial(ChromaMID);
	}

	VideoScreen->RequestRedraw();
	VideoScreen->MarkRenderStateDirty();
}

UMaterialInterface* AGreenVideoActor::ResolveChromaMaterial(const TSoftObjectPtr<UMaterialInterface>& Material) const
{
	if (!Material.IsNull())
	{
		return Material.LoadSynchronous();
	}
	return DefaultChromaMaterial.LoadSynchronous();
}

void AGreenVideoActor::HandleMediaOpened(FString OpenedUrl)
{
	if (MediaPlayer && !MediaPlayer->IsPlaying())
	{
		MediaPlayer->Play();
	}

	RefreshDisplay();
	OnPlaybackStarted();
	OnStarted.Broadcast(this);
}

void AGreenVideoActor::HandleMediaOpenFailed(FString FailedUrl)
{
	OnFailed.Broadcast(this, FString::Printf(TEXT("Open failed: %s"), *FailedUrl));
}

void AGreenVideoActor::HandleEndReached()
{
	if (bHasFinished || (MediaPlayer && MediaPlayer->IsLooping()))
	{
		return;
	}

	bHasFinished = true;
	OnPlaybackFinished();
	OnFinished.Broadcast(this);

	if (ActiveRequest.bAutoDestroy)
	{
		Destroy();
	}
}
