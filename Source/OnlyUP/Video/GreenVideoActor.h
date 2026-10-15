// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VideoTypes.h"
#include "GreenVideoActor.generated.h"

class UMediaPlayer;
class UMediaTexture;
class UMediaSource;
class UMediaSoundComponent;
class USceneComponent;
class UWidgetComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UGreenVideoSurfaceWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGreenVideoActorEvent, AGreenVideoActor*, VideoActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGreenVideoActorFailed, AGreenVideoActor*, VideoActor, const FString&, Reason);

/**
 * 世界空间绿幕视频播放器。
 * 画面通过 WidgetComponent + MediaTexture 显示（可靠可见）；抠像材质可选。
 */
UCLASS(Blueprintable, BlueprintType)
class ONLYUP_API AGreenVideoActor : public AActor
{
	GENERATED_BODY()

public:
	AGreenVideoActor();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "Video")
	bool PlayRequest(const FVideoPlayRequest& Request);

	UFUNCTION(BlueprintCallable, Category = "Video")
	bool PlayMediaSource(UMediaSource* Source, bool bLoop = false);

	UFUNCTION(BlueprintCallable, Category = "Video")
	void StopPlayback(bool bDestroyActor = true);

	UFUNCTION(BlueprintPure, Category = "Video")
	bool IsPlaybackActive() const;

	UFUNCTION(BlueprintPure, Category = "Video")
	UMediaPlayer* GetMediaPlayer() const { return MediaPlayer; }

	UFUNCTION(BlueprintPure, Category = "Video")
	UMediaTexture* GetMediaTexture() const { return MediaTexture; }

	UFUNCTION(BlueprintPure, Category = "Video")
	const FVideoPlayRequest& GetActiveRequest() const { return ActiveRequest; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video|Chroma")
	TSoftObjectPtr<UMaterialInterface> DefaultChromaMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video|Display")
	FVector2D DefaultDrawSize = FVector2D(960.f, 540.f);

	UPROPERTY(BlueprintAssignable, Category = "Video|Events")
	FOnGreenVideoActorEvent OnStarted;

	UPROPERTY(BlueprintAssignable, Category = "Video|Events")
	FOnGreenVideoActorEvent OnFinished;

	UPROPERTY(BlueprintAssignable, Category = "Video|Events")
	FOnGreenVideoActorFailed OnFailed;

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "Video")
	void ApplyChromaMaterial(const TSoftObjectPtr<UMaterialInterface>& Material);
	virtual void ApplyChromaMaterial_Implementation(const TSoftObjectPtr<UMaterialInterface>& Material);

	UFUNCTION(BlueprintNativeEvent, Category = "Video")
	void OnPlaybackStarted();
	virtual void OnPlaybackStarted_Implementation();

	UFUNCTION(BlueprintNativeEvent, Category = "Video")
	void OnPlaybackFinished();
	virtual void OnPlaybackFinished_Implementation();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> RootScene;

	/** 世界空间画面（比 StaticMesh+材质更稳定） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> VideoScreen;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UMediaSoundComponent> MediaSound;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Video")
	TObjectPtr<UMediaPlayer> MediaPlayer;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Video")
	TObjectPtr<UMediaTexture> MediaTexture;

	/** 抠像材质里采样影片的 Texture 参数名 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video|Chroma")
	FName TextureParamName = TEXT("MediaTexture");

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ChromaMID;

	UPROPERTY(Transient)
	TObjectPtr<UGreenVideoSurfaceWidget> SurfaceWidget;

	UPROPERTY(Transient)
	FVideoPlayRequest ActiveRequest;

	bool bHasFinished = false;

private:
	bool OpenAndPlay(UMediaSource* Source, bool bLoop);
	void EnsureSurfaceWidget();
	void BindMediaEvents();
	void UnbindMediaEvents();
	void RefreshDisplay();
	UMaterialInterface* ResolveChromaMaterial(const TSoftObjectPtr<UMaterialInterface>& Material) const;

	UFUNCTION()
	void HandleMediaOpened(FString OpenedUrl);

	UFUNCTION()
	void HandleMediaOpenFailed(FString FailedUrl);

	UFUNCTION()
	void HandleEndReached();

	bool bMediaEventsBound = false;
	bool bUsingChromaMaterial = false;
};
