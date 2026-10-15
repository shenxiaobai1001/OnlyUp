// Copyright Epic Games, Inc. All Rights Reserved.

#include "VideoViewportUtil.h"

#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

bool UVideoViewportUtil::ComputeConstrainedViewPixelRect(
	float WindowW,
	float WindowH,
	float ConstrainedAspect,
	float VerticalBias,
	int32 OffsetY,
	FVector2D& OutPos,
	FVector2D& OutSize)
{
	if (WindowW <= KINDA_SMALL_NUMBER || WindowH <= KINDA_SMALL_NUMBER || ConstrainedAspect <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const float WindowAspect = WindowW / WindowH;
	float ViewW = WindowW;
	float ViewH = WindowH;
	if (ConstrainedAspect > WindowAspect)
	{
		ViewW = WindowW;
		ViewH = WindowW / ConstrainedAspect;
	}
	else
	{
		ViewH = WindowH;
		ViewW = WindowH * ConstrainedAspect;
	}

	OutSize = FVector2D(ViewW, ViewH);

	const float FreeX = WindowW - ViewW;
	const float FreeY = WindowH - ViewH;
	const float ClampedBias = FMath::Clamp(VerticalBias, -1.f, 1.f);

	float PosX = FreeX * (0.5f + 0.5f * ClampedBias);
	float PosY = FreeY * (0.5f + 0.5f * ClampedBias);
	PosY += static_cast<float>(OffsetY);

	PosX = FMath::Clamp(PosX, 0.f, FMath::Max(0.f, FreeX));
	PosY = FMath::Clamp(PosY, 0.f, FMath::Max(0.f, FreeY));
	OutPos = FVector2D(PosX, PosY);
	return true;
}

bool UVideoViewportUtil::TryGetGameViewPixelRect(
	const UObject* WorldContextObject,
	FVector2D& OutPos,
	FVector2D& OutSize)
{
	OutPos = FVector2D::ZeroVector;
	OutSize = FVector2D::ZeroVector;

	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World)
	{
		return false;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	if (!PC)
	{
		return false;
	}

	int32 ViewportX = 0;
	int32 ViewportY = 0;
	PC->GetViewportSize(ViewportX, ViewportY);
	if (ViewportX <= 0 || ViewportY <= 0)
	{
		return false;
	}

	float ConstrainedAspect = 0.f;
	bool bHasConstrainedAspect = false;
	if (PC->PlayerCameraManager)
	{
		const FMinimalViewInfo& POV = PC->PlayerCameraManager->GetCameraCacheView();
		if (POV.bConstrainAspectRatio && POV.AspectRatio > KINDA_SMALL_NUMBER)
		{
			ConstrainedAspect = POV.AspectRatio;
			bHasConstrainedAspect = true;
		}
	}
	if (!bHasConstrainedAspect)
	{
		if (const AActor* ViewTarget = PC->GetViewTarget())
		{
			TArray<UCameraComponent*> Cameras;
			ViewTarget->GetComponents<UCameraComponent>(Cameras);
			for (const UCameraComponent* Cam : Cameras)
			{
				if (Cam && Cam->bConstrainAspectRatio && Cam->AspectRatio > KINDA_SMALL_NUMBER)
				{
					ConstrainedAspect = Cam->AspectRatio;
					bHasConstrainedAspect = true;
					break;
				}
			}
		}
	}
	if (!bHasConstrainedAspect)
	{
		return false;
	}

	return ComputeConstrainedViewPixelRect(
		static_cast<float>(ViewportX),
		static_cast<float>(ViewportY),
		ConstrainedAspect,
		0.f,
		0,
		OutPos,
		OutSize);
}
