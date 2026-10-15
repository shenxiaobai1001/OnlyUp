// Copyright Epic Games, Inc. All Rights Reserved.

#include "GreenVideoSurfaceWidget.h"

#include "Components/Image.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/Texture.h"
#include "Materials/MaterialInterface.h"
#include "Styling/SlateBrush.h"

void UGreenVideoSurfaceWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureImage();
}

void UGreenVideoSurfaceWidget::EnsureImage()
{
	if (VideoImage)
	{
		return;
	}

	if (WidgetTree && !WidgetTree->RootWidget)
	{
		VideoImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("VideoImage"));
		WidgetTree->RootWidget = VideoImage;
	}
	else if (WidgetTree)
	{
		VideoImage = WidgetTree->FindWidget<UImage>(TEXT("VideoImage"));
	}
}

void UGreenVideoSurfaceWidget::SetVideoTexture(UTexture* Texture)
{
	EnsureImage();
	if (!VideoImage || !Texture)
	{
		return;
	}

	FSlateBrush Brush;
	Brush.SetResourceObject(Texture);
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.TintColor = FSlateColor(FLinearColor::White);
	Brush.ImageSize = FVector2D(1920.f, 1080.f);
	VideoImage->SetBrush(Brush);
	VideoImage->SetVisibility(ESlateVisibility::Visible);
}

void UGreenVideoSurfaceWidget::SetVideoMaterial(UMaterialInterface* Material)
{
	EnsureImage();
	if (!VideoImage || !Material)
	{
		return;
	}

	VideoImage->SetBrushFromMaterial(Material);

	FSlateBrush Brush = VideoImage->GetBrush();
	Brush.ImageSize = FVector2D(1920.f, 1080.f);
	Brush.TintColor = FSlateColor(FLinearColor::White);
	VideoImage->SetBrush(Brush);
	VideoImage->SetDesiredSizeOverride(FVector2D(1920.f, 1080.f));
	VideoImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UGreenVideoSurfaceWidget::SetSurfaceSize(FVector2D Size)
{
	EnsureImage();
	if (!VideoImage)
	{
		return;
	}

	if (Size.X > 0.f && Size.Y > 0.f)
	{
		VideoImage->SetDesiredSizeOverride(Size);
	}
}
