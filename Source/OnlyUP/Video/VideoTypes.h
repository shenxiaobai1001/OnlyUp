// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MediaSource.h"
#include "Materials/MaterialInterface.h"
#include "VideoTypes.generated.h"

/** 视频呈现方式，可按需扩展自定义 Actor / Widget 子类。 */
UENUM(BlueprintType)
enum class EVideoDisplayMode : uint8
{
	/** 世界空间平面（绿幕抠像） */
	WorldActor UMETA(DisplayName = "World Actor"),

	/** 屏幕 Overlay Widget（绿幕抠像） */
	ScreenWidget UMETA(DisplayName = "Screen Widget"),
};

/** 同一通道内的排队 / 抢占策略。 */
UENUM(BlueprintType)
enum class EVideoQueuePolicy : uint8
{
	/** 立即播放，可与其他实例并存 */
	PlayImmediately UMETA(DisplayName = "Play Immediately"),

	/** 加入通道队列，等当前通道播完再播 */
	Enqueue UMETA(DisplayName = "Enqueue"),

	/** 停止同通道后立即播放 */
	ReplaceChannel UMETA(DisplayName = "Replace Channel"),

	/** 停止全部后立即播放 */
	Exclusive UMETA(DisplayName = "Exclusive"),
};

/** 一次播放请求。必须指定 MediaSource。 */
USTRUCT(BlueprintType)
struct FVideoPlayRequest
{
	GENERATED_BODY()

	/** 硬引用：蓝图直接拖 MediaSource 资源即可，无需 Load Soft */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video")
	TObjectPtr<UMediaSource> MediaSource = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video")
	EVideoDisplayMode DisplayMode = EVideoDisplayMode::ScreenWidget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video")
	EVideoQueuePolicy QueuePolicy = EVideoQueuePolicy::PlayImmediately;

	/** 逻辑通道名，用于队列隔离（如 Lottery / Reward / FX） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video")
	FName Channel = TEXT("Default");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video")
	int32 Priority = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video")
	bool bLoop = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video")
	bool bAutoDestroy = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Volume = 1.f;

	/**
	 * 可选抠像材质；空则用 Actor/Widget/Manager 默认。
	 * 扣绿容差/软边在材质内调，代码只注入 MediaTexture。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video")
	TSoftObjectPtr<UMaterialInterface> ChromaMaterial;

	/**
	 * true：不走绿幕抠像，用普通 UI 材质显示 MediaTexture。
	 * 非绿幕片源（有声音却看不见画面）时请打开。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video")
	bool bDisableChromaKey = false;

	/** WorldActor：生成位置 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video|World")
	FTransform WorldTransform = FTransform::Identity;

	/** WorldActor：平面相对缩放 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video|World")
	FVector MeshScale = FVector(1.f, 1.f, 1.f);

	/** ScreenWidget：视口尺寸（0 表示自动：优先贴合相机锁定比例，否则铺满） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video|Screen")
	FVector2D ScreenSize = FVector2D::ZeroVector;

	/**
	 * ScreenWidget：相对屏幕中心的像素偏移（仅 ScreenSize 非 0 时生效）。
	 * +X 右、+Y 下；自动贴合/铺满模式忽略。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video|Screen")
	FVector2D ScreenOffset = FVector2D::ZeroVector;

	/**
	 * ScreenSize 为 0 时：若相机 Constrain Aspect Ratio，绿幕贴合中间游戏画面。
	 * 需要盖住整窗（含黑边）时设 false。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video|Screen")
	bool bMatchCameraAspect = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video|Screen")
	int32 ZOrder = 1000;
};

/** 不透明播放句柄，用于 Stop / 查询。 */
USTRUCT(BlueprintType)
struct FVideoHandle
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Video")
	FGuid Id;

	bool IsValid() const { return Id.IsValid(); }

	void Invalidate() { Id.Invalidate(); }

	bool operator==(const FVideoHandle& Other) const { return Id == Other.Id; }

	friend uint32 GetTypeHash(const FVideoHandle& Handle)
	{
		return GetTypeHash(Handle.Id);
	}
};
