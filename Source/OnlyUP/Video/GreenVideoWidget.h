// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "VideoTypes.h"
#include "GreenVideoWidget.generated.h"

class UMediaPlayer;
class UMediaTexture;
class UMediaSource;
class UImage;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UMediaSoundComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGreenVideoWidgetEvent, UGreenVideoWidget*, VideoWidget);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGreenVideoWidgetFailed, UGreenVideoWidget*, VideoWidget, const FString&, Reason);

/**
 * 屏幕 Overlay 绿幕视频。
 * 子类可在 UMG 中绑定名为 VideoImage 的 Image，或重写 BindVideoImage。
 */
UCLASS(Blueprintable, BlueprintType)
class ONLYUP_API UGreenVideoWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UFUNCTION(BlueprintCallable, Category = "Video")
	bool PlayRequest(const FVideoPlayRequest& Request);

	UFUNCTION(BlueprintCallable, Category = "Video")
	bool PlayMediaSource(UMediaSource* Source, bool bLoop = false);

	UFUNCTION(BlueprintCallable, Category = "Video")
	void StopPlayback();

	UFUNCTION(BlueprintPure, Category = "Video")
	bool IsPlaybackActive() const;

	UFUNCTION(BlueprintPure, Category = "Video")
	UMediaPlayer* GetMediaPlayer() const { return MediaPlayer; }

	/** 未在 Request 中覆盖时使用的默认抠像材质 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video|Chroma")
	TSoftObjectPtr<UMaterialInterface> DefaultChromaMaterial;

	/**
	 * 无抠像材质时的 UI 显示材质。
	 * MediaTexture 不能直接塞给 Image Brush，必须走材质采样，否则只有声音没画面。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video|Display")
	TSoftObjectPtr<UMaterialInterface> FallbackDisplayMaterial;

	UPROPERTY(BlueprintAssignable, Category = "Video|Events")
	FOnGreenVideoWidgetEvent OnStarted;

	UPROPERTY(BlueprintAssignable, Category = "Video|Events")
	FOnGreenVideoWidgetEvent OnFinished;

	UPROPERTY(BlueprintAssignable, Category = "Video|Events")
	FOnGreenVideoWidgetFailed OnFailed;

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "Video")
	void ApplyChromaMaterial(const TSoftObjectPtr<UMaterialInterface>& Material);
	virtual void ApplyChromaMaterial_Implementation(const TSoftObjectPtr<UMaterialInterface>& Material);

	UFUNCTION(BlueprintNativeEvent, Category = "Video")
	UImage* BindVideoImage();
	virtual UImage* BindVideoImage_Implementation();

	UPROPERTY(BlueprintReadOnly, Category = "Video", meta = (BindWidgetOptional))
	TObjectPtr<UImage> VideoImage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Video")
	TObjectPtr<UMediaPlayer> MediaPlayer;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Video")
	TObjectPtr<UMediaTexture> MediaTexture;

	/** 屏幕播放也需要声音组件（注册到 World） */
	UPROPERTY(Transient)
	TObjectPtr<UMediaSoundComponent> MediaSound;

	/** 抠像材质里采样影片的 Texture 参数名 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video|Chroma")
	FName TextureParamName = TEXT("MediaTexture");

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ChromaMID;

	UPROPERTY(Transient)
	FVideoPlayRequest ActiveRequest;

	bool bHasFinished = false;

private:
	bool OpenAndPlay(UMediaSource* Source, bool bLoop);
	void EnsureMediaObjects();
	void EnsureMediaSound();
	void EnsureFallbackDisplayMaterial();
	void EnsureVideoImage();
	void BindMediaEvents();
	void UnbindMediaEvents();
	void RefreshMediaMaterial();
	void ApplyViewportLayout();
	bool TryGetConstrainedViewPixelRect(FVector2D& OutPos, FVector2D& OutSize) const;
	void HideUntilFirstFrame();
	void StartWaitForFirstFrame();
	void StopWaitForFirstFrame();
	void RevealWhenReady();
	bool HasValidVideoFrame() const;
	UMaterialInterface* ResolveChromaMaterial(const TSoftObjectPtr<UMaterialInterface>& Material) const;

	UFUNCTION()
	void HandleMediaOpened(FString OpenedUrl);

	UFUNCTION()
	void HandleMediaOpenFailed(FString FailedUrl);

	UFUNCTION()
	void HandleEndReached();

	UFUNCTION()
	void HandleFirstFrameTick();

	UFUNCTION()
	void HandleFirstFrameTimeout();

	UFUNCTION()
	void HandleRevealFadeTick();

	bool bMediaEventsBound = false;
	bool bUsingFallbackDisplay = false;
	bool bWaitingForFirstFrame = false;
	bool bHasRevealed = false;
	int32 PendingViewportZOrder = 1000;
	int32 ConsecutiveValidFrameTicks = 0;

	FTimerHandle FirstFrameTimerHandle;
	FTimerHandle FirstFrameTimeoutHandle;
	FTimerHandle RevealFadeTimerHandle;
};
