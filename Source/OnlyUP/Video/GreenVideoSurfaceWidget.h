// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GreenVideoSurfaceWidget.generated.h"

class UImage;
class UTexture;
class UMaterialInterface;

/** 仅负责把 MediaTexture / 抠像材质画到 Image 上（不做播放）。 */
UCLASS(Blueprintable)
class ONLYUP_API UGreenVideoSurfaceWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "Video")
	void SetVideoTexture(UTexture* Texture);

	UFUNCTION(BlueprintCallable, Category = "Video")
	void SetVideoMaterial(UMaterialInterface* Material);

	UFUNCTION(BlueprintCallable, Category = "Video")
	void SetSurfaceSize(FVector2D Size);

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> VideoImage;

	void EnsureImage();
};
