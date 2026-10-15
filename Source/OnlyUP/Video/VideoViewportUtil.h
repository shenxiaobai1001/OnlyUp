// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "VideoViewportUtil.generated.h"

/**
 * 绿幕视频布局用：计算相机 Constrain Aspect Ratio 锁定画面的像素矩形。
 * （从 PickLeaf LocalPlayer 抽出的轻量工具，不替换项目 LocalPlayer。）
 */
UCLASS()
class ONLYUP_API UVideoViewportUtil : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static bool ComputeConstrainedViewPixelRect(
		float WindowW,
		float WindowH,
		float ConstrainedAspect,
		float VerticalBias,
		int32 OffsetY,
		FVector2D& OutPos,
		FVector2D& OutSize);

	UFUNCTION(BlueprintPure, Category = "Video|Viewport", meta = (WorldContext = "WorldContextObject"))
	static bool TryGetGameViewPixelRect(
		const UObject* WorldContextObject,
		FVector2D& OutPos,
		FVector2D& OutSize);
};
