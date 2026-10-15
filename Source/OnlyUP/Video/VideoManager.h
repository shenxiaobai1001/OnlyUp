// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "VideoTypes.h"
#include "VideoManager.generated.h"

class AGreenVideoActor;
class UGreenVideoWidget;
class UMaterialInterface;
class UGameInstance;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVideoManagerStarted, FVideoHandle, Handle, const FVideoPlayRequest&, Request);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVideoManagerFinished, FVideoHandle, Handle, const FVideoPlayRequest&, Request);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnVideoManagerFailed, FVideoHandle, Handle, const FVideoPlayRequest&, Request, const FString&, Reason);

/** 按句柄绑定：回调里不再带 Handle，无需手动比较 */
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnVideoHandleStarted, const FVideoPlayRequest&, Request);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnVideoHandleFinished, const FVideoPlayRequest&, Request);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnVideoHandleFailed, const FVideoPlayRequest&, Request, const FString&, Reason);

/**
 * 绿幕视频唯一入口（普通 UObject，不是场景子系统）。
 *
 * 多路 MediaPlayer 易卡顿/泄漏：由本管理器集中创建、Stop→Close、清理会话。
 *
 * C++：UVideoManager::Get()->PlayVideo(Request)
 * 蓝图：Get Video Manager → Play Video → Bind On Video Finished (Handle, Event)
 */
UCLASS(BlueprintType, Blueprintable, Config = Game)
class ONLYUP_API UVideoManager : public UObject
{
	GENERATED_BODY()

public:
	virtual UWorld* GetWorld() const override;

	/** 仅 C++ 使用 */
	static UVideoManager* Get();

	/** 蓝图入口（WorldContext 由引擎自动注入） */
	UFUNCTION(BlueprintPure, Category = "Video",
		meta = (WorldContext = "WorldContextObject", DisplayName = "Get Video Manager", Keywords = "VideoManager Get Video Manager"))
	static UVideoManager* GetVideoManager(const UObject* WorldContextObject);

	/**
	 * 唯一播放入口。参数全部写在 FVideoPlayRequest 里（MediaSource、抠像、通道策略等）。
	 * 可多路并行；MaxConcurrentVideos>0 且超限时按 QueuePolicy 处理或拒绝（默认 0 不限制）。
	 */
	UFUNCTION(BlueprintCallable, Category = "Video", meta = (AutoCreateRefTerm = "Request", DisplayName = "Play Video"))
	FVideoHandle PlayVideo(const FVideoPlayRequest& Request);

	/** 按句柄安全停止：先解绑事件，再 Stop→Close 媒体，最后清会话 */
	UFUNCTION(BlueprintCallable, Category = "Video")
	bool Stop(FVideoHandle Handle);

	/** 强制关闭全部会话（关卡切换 / Exclusive 策略会走这里） */
	UFUNCTION(BlueprintCallable, Category = "Video")
	void StopAll();

	/** 当前正在播放（含刚 Start、尚未 Finished）的句柄 */
	UFUNCTION(BlueprintPure, Category = "Video")
	TArray<FVideoHandle> GetActiveHandles() const;

	UFUNCTION(BlueprintPure, Category = "Video")
	bool IsHandleActive(FVideoHandle Handle) const;

	/** 蓝图推荐：只监听指定 Handle 的开始（回调无 Handle，不用手动比较） */
	UFUNCTION(BlueprintCallable, Category = "Video|Events", meta = (DisplayName = "Bind On Video Started"))
	void BindOnVideoStarted(FVideoHandle Handle, FOnVideoHandleStarted Callback);

	/** 蓝图推荐：只监听指定 Handle 的结束 */
	UFUNCTION(BlueprintCallable, Category = "Video|Events", meta = (DisplayName = "Bind On Video Finished"))
	void BindOnVideoFinished(FVideoHandle Handle, FOnVideoHandleFinished Callback);

	/** 蓝图推荐：只监听指定 Handle 的失败 */
	UFUNCTION(BlueprintCallable, Category = "Video|Events", meta = (DisplayName = "Bind On Video Failed"))
	void BindOnVideoFailed(FVideoHandle Handle, FOnVideoHandleFailed Callback);

	/** 全局多播（C++ / 需要听全部会话时用）；蓝图请优先用上面的 Bind On Video * */
	UPROPERTY(BlueprintAssignable, Category = "Video|Events|Advanced")
	FOnVideoManagerStarted OnVideoStarted;

	UPROPERTY(BlueprintAssignable, Category = "Video|Events|Advanced")
	FOnVideoManagerFinished OnVideoFinished;

	UPROPERTY(BlueprintAssignable, Category = "Video|Events|Advanced")
	FOnVideoManagerFailed OnVideoFailed;

protected:
	void InitializeManager();
	void ShutdownManager();

	UPROPERTY(EditAnywhere, Config, Category = "Video|Defaults")
	TSoftClassPtr<AGreenVideoActor> DefaultVideoActorClass;

	UPROPERTY(EditAnywhere, Config, Category = "Video|Defaults")
	TSoftClassPtr<UGreenVideoWidget> DefaultVideoWidgetClass;

	UPROPERTY(EditAnywhere, Config, Category = "Video|Defaults")
	TSoftObjectPtr<UMaterialInterface> DefaultChromaMaterial;

	/**
	 * 同时存在的最大会话数（含 Widget / Actor）。
	 * 0 = 不限制（默认）。多路高清解码会吃 CPU；需要限流时再设正数。
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Video|Performance", meta = (ClampMin = "0"))
	int32 MaxConcurrentVideos = 0;

private:
	struct FActiveSession
	{
		FVideoHandle Handle;
		FVideoPlayRequest Request;
		TWeakObjectPtr<AGreenVideoActor> Actor;
		TWeakObjectPtr<UGreenVideoWidget> Widget;
	};

	struct FStartedBinding
	{
		FGuid HandleId;
		FOnVideoHandleStarted Callback;
	};

	struct FFinishedBinding
	{
		FGuid HandleId;
		FOnVideoHandleFinished Callback;
	};

	struct FFailedBinding
	{
		FGuid HandleId;
		FOnVideoHandleFailed Callback;
	};

	FVideoHandle EnqueueOrStart(const FVideoPlayRequest& Request);
	FVideoHandle StartSession(const FVideoPlayRequest& Request);
	FVideoHandle StartWorldActor(const FVideoPlayRequest& Request, const FVideoHandle& Handle);
	FVideoHandle StartScreenWidget(const FVideoPlayRequest& Request, const FVideoHandle& Handle);

	void StopChannelInternal(FName Channel);
	void EndSession(FGuid Id, bool bStopPlayback, bool bBroadcastFinished, bool bBroadcastFailed, const FString& FailReason);
	void ProcessChannelQueue(FName Channel);

	void DispatchStartedBindings(FGuid Id, const FVideoPlayRequest& Request);
	void DispatchFinishedBindings(FGuid Id, const FVideoPlayRequest& Request);
	void DispatchFailedBindings(FGuid Id, const FVideoPlayRequest& Request, const FString& Reason);
	void ClearHandleBindings(FGuid Id);

	TSubclassOf<AGreenVideoActor> ResolveActorClass(const FVideoPlayRequest& Request) const;
	TSubclassOf<UGreenVideoWidget> ResolveWidgetClass(const FVideoPlayRequest& Request) const;
	void CustomizeSpawnTransform(const FVideoPlayRequest& Request, FTransform& InOutTransform) const;

	bool ValidateRequest(const FVideoPlayRequest& Request, FString& OutError) const;
	bool IsChannelBusy(FName Channel) const;
	APlayerController* FindLocalPlayerController() const;

	void UnbindSessionEvents(FActiveSession& Session);
	void StopSessionPlayback(FActiveSession& Session);

	UFUNCTION()
	void HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);

	UFUNCTION()
	void HandleActorStarted(AGreenVideoActor* VideoActor);

	UFUNCTION()
	void HandleActorFinished(AGreenVideoActor* VideoActor);

	UFUNCTION()
	void HandleActorFailed(AGreenVideoActor* VideoActor, const FString& Reason);

	UFUNCTION()
	void HandleWidgetStarted(UGreenVideoWidget* VideoWidget);

	UFUNCTION()
	void HandleWidgetFinished(UGreenVideoWidget* VideoWidget);

	UFUNCTION()
	void HandleWidgetFailed(UGreenVideoWidget* VideoWidget, const FString& Reason);

	FActiveSession* FindSessionByActor(AGreenVideoActor* Actor);
	FActiveSession* FindSessionByWidget(UGreenVideoWidget* Widget);
	FActiveSession* FindSessionByHandleMutable(const FVideoHandle& Handle);

	TMap<FGuid, FActiveSession> ActiveSessions;
	TMap<FName, TArray<FVideoPlayRequest>> ChannelQueues;

	TArray<FStartedBinding> StartedBindings;
	TArray<FFinishedBinding> FinishedBindings;
	TArray<FFailedBinding> FailedBindings;

	bool bInitialized = false;

	static UWorld* ResolveGameWorld();
	static TWeakObjectPtr<UVideoManager> Instance;
};
