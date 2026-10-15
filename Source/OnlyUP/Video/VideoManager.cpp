// Copyright Epic Games, Inc. All Rights Reserved.

#include "VideoManager.h"

#include "GreenVideoActor.h"
#include "GreenVideoWidget.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Blueprint/UserWidget.h"

TWeakObjectPtr<UVideoManager> UVideoManager::Instance;

UWorld* UVideoManager::GetWorld() const
{
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return nullptr;
	}

	if (const UGameInstance* GI = Cast<UGameInstance>(GetOuter()))
	{
		if (UWorld* World = GI->GetWorld())
		{
			return World;
		}
	}

	return ResolveGameWorld();
}

UWorld* UVideoManager::ResolveGameWorld()
{
	if (!GEngine)
	{
		return nullptr;
	}

	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		if (Context.World()
			&& (Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Game))
		{
			return Context.World();
		}
	}

	return nullptr;
}

void UVideoManager::InitializeManager()
{
	if (bInitialized)
	{
		return;
	}

	if (DefaultVideoActorClass.IsNull())
	{
		DefaultVideoActorClass = AGreenVideoActor::StaticClass();
	}
	if (DefaultVideoWidgetClass.IsNull())
	{
		DefaultVideoWidgetClass = UGreenVideoWidget::StaticClass();
	}

	// 未在 DefaultGame.ini / 蓝图里配时，用项目 UI 绿幕材质（缺这个会只有声音没画面）
	if (DefaultChromaMaterial.IsNull())
	{
		DefaultChromaMaterial = TSoftObjectPtr<UMaterialInterface>(
			FSoftObjectPath(TEXT("/Game/Videos/M_UI_GreenChroma.M_UI_GreenChroma")));
	}

	FWorldDelegates::OnWorldCleanup.AddUObject(this, &UVideoManager::HandleWorldCleanup);
	bInitialized = true;
}

void UVideoManager::ShutdownManager()
{
	if (!bInitialized)
	{
		return;
	}

	FWorldDelegates::OnWorldCleanup.RemoveAll(this);
	StopAll();
	bInitialized = false;
}

void UVideoManager::HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
	// ???????? Stop?Close??? MediaTexture / MediaPlayer ????????
	StopAll();

	if (bSessionEnded && Instance.Get() == this)
	{
		ShutdownManager();
		if (UGameInstance* GI = Cast<UGameInstance>(GetOuter()))
		{
			GI->UnregisterReferencedObject(this);
		}
		if (IsRooted())
		{
			RemoveFromRoot();
		}
		Instance.Reset();
	}
}

UVideoManager* UVideoManager::Get()
{
	if (UVideoManager* Existing = Instance.Get())
	{
		return Existing;
	}

	UWorld* World = ResolveGameWorld();
	UGameInstance* GI = World ? World->GetGameInstance() : nullptr;

	UVideoManager* Created = nullptr;
	if (GI)
	{
		Created = FindObject<UVideoManager>(GI, TEXT("VideoManager"));
		if (!Created)
		{
			Created = NewObject<UVideoManager>(GI, UVideoManager::StaticClass(), TEXT("VideoManager"));
			GI->RegisterReferencedObject(Created);
		}
	}
	else
	{
		Created = NewObject<UVideoManager>(GetTransientPackage(), UVideoManager::StaticClass(), TEXT("VideoManager"));
		Created->AddToRoot();
	}

	Created->InitializeManager();
	Instance = Created;
	return Created;
}

UVideoManager* UVideoManager::GetVideoManager(const UObject* WorldContextObject)
{
	(void)WorldContextObject;
	return Get();
}

FVideoHandle UVideoManager::PlayVideo(const FVideoPlayRequest& Request)
{
	FString Error;
	if (!ValidateRequest(Request, Error))
	{
		UE_LOG(LogTemp, Warning, TEXT("UVideoManager::PlayVideo rejected: %s"), *Error);
		FVideoHandle Invalid;
		OnVideoFailed.Broadcast(Invalid, Request, Error);
		return Invalid;
	}

	return EnqueueOrStart(Request);
}

bool UVideoManager::Stop(FVideoHandle Handle)
{
	FActiveSession* Session = FindSessionByHandleMutable(Handle);
	if (!Session)
	{
		return false;
	}

	const FName Channel = Session->Request.Channel;
	EndSession(Handle.Id, /*bStopPlayback*/ true, /*bBroadcastFinished*/ false, /*bBroadcastFailed*/ false, FString());
	ProcessChannelQueue(Channel);
	return true;
}

TArray<FVideoHandle> UVideoManager::GetActiveHandles() const
{
	TArray<FVideoHandle> Out;
	Out.Reserve(ActiveSessions.Num());
	for (const TPair<FGuid, FActiveSession>& Pair : ActiveSessions)
	{
		Out.Add(Pair.Value.Handle);
	}
	return Out;
}

bool UVideoManager::IsHandleActive(FVideoHandle Handle) const
{
	return Handle.IsValid() && ActiveSessions.Contains(Handle.Id);
}

void UVideoManager::StopAll()
{
	TArray<FGuid> AllIds;
	ActiveSessions.GetKeys(AllIds);

	for (const FGuid& Id : AllIds)
	{
		EndSession(Id, /*bStopPlayback*/ true, /*bBroadcastFinished*/ false, /*bBroadcastFailed*/ false, FString());
	}

	ActiveSessions.Reset();
	ChannelQueues.Reset();
	StartedBindings.Reset();
	FinishedBindings.Reset();
	FailedBindings.Reset();
}

void UVideoManager::BindOnVideoStarted(FVideoHandle Handle, FOnVideoHandleStarted Callback)
{
	if (!Handle.IsValid() || !Callback.IsBound())
	{
		return;
	}

	FStartedBinding Binding;
	Binding.HandleId = Handle.Id;
	Binding.Callback = Callback;
	StartedBindings.Add(MoveTemp(Binding));
}

void UVideoManager::BindOnVideoFinished(FVideoHandle Handle, FOnVideoHandleFinished Callback)
{
	if (!Handle.IsValid() || !Callback.IsBound())
	{
		return;
	}

	FFinishedBinding Binding;
	Binding.HandleId = Handle.Id;
	Binding.Callback = Callback;
	FinishedBindings.Add(MoveTemp(Binding));
}

void UVideoManager::BindOnVideoFailed(FVideoHandle Handle, FOnVideoHandleFailed Callback)
{
	if (!Handle.IsValid() || !Callback.IsBound())
	{
		return;
	}

	FFailedBinding Binding;
	Binding.HandleId = Handle.Id;
	Binding.Callback = Callback;
	FailedBindings.Add(MoveTemp(Binding));
}

void UVideoManager::DispatchStartedBindings(FGuid Id, const FVideoPlayRequest& Request)
{
	TArray<FOnVideoHandleStarted> ToCall;
	for (int32 Index = StartedBindings.Num() - 1; Index >= 0; --Index)
	{
		if (StartedBindings[Index].HandleId == Id)
		{
			ToCall.Add(StartedBindings[Index].Callback);
			StartedBindings.RemoveAtSwap(Index);
		}
	}

	for (FOnVideoHandleStarted& Callback : ToCall)
	{
		Callback.ExecuteIfBound(Request);
	}
}

void UVideoManager::DispatchFinishedBindings(FGuid Id, const FVideoPlayRequest& Request)
{
	TArray<FOnVideoHandleFinished> ToCall;
	for (int32 Index = FinishedBindings.Num() - 1; Index >= 0; --Index)
	{
		if (FinishedBindings[Index].HandleId == Id)
		{
			ToCall.Add(FinishedBindings[Index].Callback);
			FinishedBindings.RemoveAtSwap(Index);
		}
	}

	for (FOnVideoHandleFinished& Callback : ToCall)
	{
		Callback.ExecuteIfBound(Request);
	}
}

void UVideoManager::DispatchFailedBindings(FGuid Id, const FVideoPlayRequest& Request, const FString& Reason)
{
	TArray<FOnVideoHandleFailed> ToCall;
	for (int32 Index = FailedBindings.Num() - 1; Index >= 0; --Index)
	{
		if (FailedBindings[Index].HandleId == Id)
		{
			ToCall.Add(FailedBindings[Index].Callback);
			FailedBindings.RemoveAtSwap(Index);
		}
	}

	for (FOnVideoHandleFailed& Callback : ToCall)
	{
		Callback.ExecuteIfBound(Request, Reason);
	}
}

void UVideoManager::ClearHandleBindings(FGuid Id)
{
	StartedBindings.RemoveAll([&Id](const FStartedBinding& Binding) { return Binding.HandleId == Id; });
	FinishedBindings.RemoveAll([&Id](const FFinishedBinding& Binding) { return Binding.HandleId == Id; });
	FailedBindings.RemoveAll([&Id](const FFailedBinding& Binding) { return Binding.HandleId == Id; });
}

void UVideoManager::StopChannelInternal(FName Channel)
{
	TArray<FGuid> ToStop;
	for (const TPair<FGuid, FActiveSession>& Pair : ActiveSessions)
	{
		if (Pair.Value.Request.Channel == Channel)
		{
			ToStop.Add(Pair.Key);
		}
	}

	for (const FGuid& Id : ToStop)
	{
		EndSession(Id, true, false, false, FString());
	}

	ChannelQueues.Remove(Channel);
}

FVideoHandle UVideoManager::EnqueueOrStart(const FVideoPlayRequest& Request)
{
	// ?????????????? CPU/?????? MediaPlayer ??????
	if (MaxConcurrentVideos > 0
		&& ActiveSessions.Num() >= MaxConcurrentVideos
		&& Request.QueuePolicy != EVideoQueuePolicy::Exclusive
		&& Request.QueuePolicy != EVideoQueuePolicy::ReplaceChannel)
	{
		if (Request.QueuePolicy == EVideoQueuePolicy::Enqueue)
		{
			ChannelQueues.FindOrAdd(Request.Channel).Add(Request);
			return FVideoHandle();
		}

		const FString Error = FString::Printf(
			TEXT("MaxConcurrentVideos (%d) reached"), MaxConcurrentVideos);
		UE_LOG(LogTemp, Warning, TEXT("UVideoManager::PlayVideo rejected: %s"), *Error);
		FVideoHandle Invalid;
		OnVideoFailed.Broadcast(Invalid, Request, Error);
		return Invalid;
	}

	switch (Request.QueuePolicy)
	{
	case EVideoQueuePolicy::Exclusive:
		StopAll();
		return StartSession(Request);

	case EVideoQueuePolicy::ReplaceChannel:
		StopChannelInternal(Request.Channel);
		return StartSession(Request);

	case EVideoQueuePolicy::Enqueue:
		if (IsChannelBusy(Request.Channel))
		{
			ChannelQueues.FindOrAdd(Request.Channel).Add(Request);
			return FVideoHandle();
		}
		return StartSession(Request);

	case EVideoQueuePolicy::PlayImmediately:
	default:
		return StartSession(Request);
	}
}

FVideoHandle UVideoManager::StartSession(const FVideoPlayRequest& Request)
{
	FVideoHandle Handle;
	Handle.Id = FGuid::NewGuid();

	switch (Request.DisplayMode)
	{
	case EVideoDisplayMode::ScreenWidget:
		return StartScreenWidget(Request, Handle);

	case EVideoDisplayMode::WorldActor:
	default:
		return StartWorldActor(Request, Handle);
	}
}

FVideoHandle UVideoManager::StartWorldActor(const FVideoPlayRequest& Request, const FVideoHandle& Handle)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		OnVideoFailed.Broadcast(Handle, Request, TEXT("No World"));
		return FVideoHandle();
	}

	TSubclassOf<AGreenVideoActor> ActorClass = ResolveActorClass(Request);
	if (!ActorClass)
	{
		OnVideoFailed.Broadcast(Handle, Request, TEXT("ResolveActorClass returned null"));
		return FVideoHandle();
	}

	FTransform SpawnTransform = Request.WorldTransform;
	CustomizeSpawnTransform(Request, SpawnTransform);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = FindLocalPlayerController();

	AGreenVideoActor* Actor = World->SpawnActor<AGreenVideoActor>(ActorClass, SpawnTransform, SpawnParams);
	if (!Actor)
	{
		OnVideoFailed.Broadcast(Handle, Request, TEXT("SpawnActor failed"));
		return FVideoHandle();
	}

	if (Request.ChromaMaterial.IsNull()
		&& !DefaultChromaMaterial.IsNull()
		&& Actor->DefaultChromaMaterial.IsNull())
	{
		Actor->DefaultChromaMaterial = DefaultChromaMaterial;
	}

	Actor->OnStarted.AddDynamic(this, &UVideoManager::HandleActorStarted);
	Actor->OnFinished.AddDynamic(this, &UVideoManager::HandleActorFinished);
	Actor->OnFailed.AddDynamic(this, &UVideoManager::HandleActorFailed);

	FActiveSession Session;
	Session.Handle = Handle;
	Session.Request = Request;
	Session.Actor = Actor;
	ActiveSessions.Add(Handle.Id, Session);

	if (!Actor->PlayRequest(Request))
	{
		EndSession(Handle.Id, true, false, false, FString());
		return FVideoHandle();
	}

	return Handle;
}

FVideoHandle UVideoManager::StartScreenWidget(const FVideoPlayRequest& Request, const FVideoHandle& Handle)
{
	APlayerController* PC = FindLocalPlayerController();
	if (!PC)
	{
		OnVideoFailed.Broadcast(Handle, Request, TEXT("No local PlayerController"));
		return FVideoHandle();
	}

	TSubclassOf<UGreenVideoWidget> WidgetClass = ResolveWidgetClass(Request);
	if (!WidgetClass)
	{
		OnVideoFailed.Broadcast(Handle, Request, TEXT("ResolveWidgetClass returned null"));
		return FVideoHandle();
	}

	UGreenVideoWidget* Widget = CreateWidget<UGreenVideoWidget>(PC, WidgetClass);
	if (!Widget)
	{
		OnVideoFailed.Broadcast(Handle, Request, TEXT("CreateWidget failed"));
		return FVideoHandle();
	}

	if (!Request.bDisableChromaKey
		&& !DefaultChromaMaterial.IsNull()
		&& Widget->DefaultChromaMaterial.IsNull()
		&& Request.ChromaMaterial.IsNull())
	{
		Widget->DefaultChromaMaterial = DefaultChromaMaterial;
	}

	Widget->OnStarted.AddDynamic(this, &UVideoManager::HandleWidgetStarted);
	Widget->OnFinished.AddDynamic(this, &UVideoManager::HandleWidgetFinished);
	Widget->OnFailed.AddDynamic(this, &UVideoManager::HandleWidgetFailed);

	// ?????????????????????????
	FVideoPlayRequest AdjustedRequest = Request;
	AdjustedRequest.ZOrder = Request.ZOrder + ActiveSessions.Num();

	FActiveSession Session;
	Session.Handle = Handle;
	Session.Request = AdjustedRequest;
	Session.Widget = Widget;
	ActiveSessions.Add(Handle.Id, Session);

	if (!Widget->PlayRequest(AdjustedRequest))
	{
		EndSession(Handle.Id, true, false, false, FString());
		return FVideoHandle();
	}

	return Handle;
}

void UVideoManager::UnbindSessionEvents(FActiveSession& Session)
{
	if (AGreenVideoActor* Actor = Session.Actor.Get())
	{
		Actor->OnStarted.RemoveDynamic(this, &UVideoManager::HandleActorStarted);
		Actor->OnFinished.RemoveDynamic(this, &UVideoManager::HandleActorFinished);
		Actor->OnFailed.RemoveDynamic(this, &UVideoManager::HandleActorFailed);
	}

	if (UGreenVideoWidget* Widget = Session.Widget.Get())
	{
		Widget->OnStarted.RemoveDynamic(this, &UVideoManager::HandleWidgetStarted);
		Widget->OnFinished.RemoveDynamic(this, &UVideoManager::HandleWidgetFinished);
		Widget->OnFailed.RemoveDynamic(this, &UVideoManager::HandleWidgetFailed);
	}
}

void UVideoManager::StopSessionPlayback(FActiveSession& Session)
{
	// ????????? ? Stop/Close ? ?????????????????????
	if (AGreenVideoActor* Actor = Session.Actor.Get())
	{
		Actor->StopPlayback(true);
		Session.Actor.Reset();
	}

	if (UGreenVideoWidget* Widget = Session.Widget.Get())
	{
		Widget->StopPlayback();
		Session.Widget.Reset();
	}
}

void UVideoManager::EndSession(
	FGuid Id,
	bool bStopPlayback,
	bool bBroadcastFinished,
	bool bBroadcastFailed,
	const FString& FailReason)
{
	FActiveSession* Session = ActiveSessions.Find(Id);
	if (!Session)
	{
		return;
	}

	const FVideoHandle Handle = Session->Handle;
	const FVideoPlayRequest Request = Session->Request;

	UnbindSessionEvents(*Session);

	if (bStopPlayback)
	{
		StopSessionPlayback(*Session);
	}
	else
	{
		// ?????Actor/Widget ??? Close??????
		Session->Actor.Reset();
		Session->Widget.Reset();
	}

	ActiveSessions.Remove(Id);

	if (bBroadcastFinished)
	{
		OnVideoFinished.Broadcast(Handle, Request);
		DispatchFinishedBindings(Id, Request);
		ClearHandleBindings(Id);
	}
	else if (bBroadcastFailed)
	{
		OnVideoFailed.Broadcast(Handle, Request, FailReason);
		DispatchFailedBindings(Id, Request, FailReason);
		ClearHandleBindings(Id);
	}
	else
	{
		// Stop / ?????? Finished?????
		ClearHandleBindings(Id);
	}
}

void UVideoManager::ProcessChannelQueue(FName Channel)
{
	TArray<FVideoPlayRequest>* Queue = ChannelQueues.Find(Channel);
	if (!Queue || Queue->Num() == 0)
	{
		return;
	}

	if (IsChannelBusy(Channel))
	{
		return;
	}

	if (MaxConcurrentVideos > 0 && ActiveSessions.Num() >= MaxConcurrentVideos)
	{
		return;
	}

	FVideoPlayRequest Next = (*Queue)[0];
	Queue->RemoveAt(0);
	if (Queue->Num() == 0)
	{
		ChannelQueues.Remove(Channel);
	}

	StartSession(Next);
}

TSubclassOf<AGreenVideoActor> UVideoManager::ResolveActorClass(const FVideoPlayRequest& Request) const
{
	(void)Request;
	if (UClass* Loaded = DefaultVideoActorClass.LoadSynchronous())
	{
		return Loaded;
	}
	return AGreenVideoActor::StaticClass();
}

TSubclassOf<UGreenVideoWidget> UVideoManager::ResolveWidgetClass(const FVideoPlayRequest& Request) const
{
	(void)Request;
	if (UClass* Loaded = DefaultVideoWidgetClass.LoadSynchronous())
	{
		return Loaded;
	}
	return UGreenVideoWidget::StaticClass();
}

void UVideoManager::CustomizeSpawnTransform(const FVideoPlayRequest& Request, FTransform& InOutTransform) const
{
	if (!InOutTransform.Equals(FTransform::Identity))
	{
		return;
	}

	APlayerController* PC = FindLocalPlayerController();
	if (!PC || !PC->PlayerCameraManager)
	{
		return;
	}

	const FVector CamLoc = PC->PlayerCameraManager->GetCameraLocation();
	const FRotator CamRot = PC->PlayerCameraManager->GetCameraRotation();
	const FVector SpawnLoc = CamLoc + CamRot.Vector() * 250.f;
	InOutTransform = FTransform(CamRot, SpawnLoc);
}

bool UVideoManager::ValidateRequest(const FVideoPlayRequest& Request, FString& OutError) const
{
	if (!Request.MediaSource)
	{
		OutError = TEXT("MediaSource is empty");
		return false;
	}
	return true;
}

bool UVideoManager::IsChannelBusy(FName Channel) const
{
	for (const TPair<FGuid, FActiveSession>& Pair : ActiveSessions)
	{
		if (Pair.Value.Request.Channel == Channel)
		{
			return true;
		}
	}
	return false;
}

APlayerController* UVideoManager::FindLocalPlayerController() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	return UGameplayStatics::GetPlayerController(World, 0);
}

void UVideoManager::HandleActorStarted(AGreenVideoActor* VideoActor)
{
	if (FActiveSession* Session = FindSessionByActor(VideoActor))
	{
		OnVideoStarted.Broadcast(Session->Handle, Session->Request);
		DispatchStartedBindings(Session->Handle.Id, Session->Request);
	}
}

void UVideoManager::HandleActorFinished(AGreenVideoActor* VideoActor)
{
	FActiveSession* Session = FindSessionByActor(VideoActor);
	if (!Session)
	{
		return;
	}

	const FGuid Id = Session->Handle.Id;
	const FName Channel = Session->Request.Channel;
	// ?????? Close??? MediaPlayer/MediaTexture ??
	EndSession(Id, /*bStopPlayback*/ true, /*bBroadcastFinished*/ true, false, FString());
	ProcessChannelQueue(Channel);
}

void UVideoManager::HandleActorFailed(AGreenVideoActor* VideoActor, const FString& Reason)
{
	FActiveSession* Session = FindSessionByActor(VideoActor);
	if (!Session)
	{
		return;
	}

	const FGuid Id = Session->Handle.Id;
	const FName Channel = Session->Request.Channel;
	EndSession(Id, /*bStopPlayback*/ true, false, /*bBroadcastFailed*/ true, Reason);
	ProcessChannelQueue(Channel);
}

void UVideoManager::HandleWidgetStarted(UGreenVideoWidget* VideoWidget)
{
	if (FActiveSession* Session = FindSessionByWidget(VideoWidget))
	{
		OnVideoStarted.Broadcast(Session->Handle, Session->Request);
		DispatchStartedBindings(Session->Handle.Id, Session->Request);
	}
}

void UVideoManager::HandleWidgetFinished(UGreenVideoWidget* VideoWidget)
{
	FActiveSession* Session = FindSessionByWidget(VideoWidget);
	if (!Session)
	{
		return;
	}

	const FGuid Id = Session->Handle.Id;
	const FName Channel = Session->Request.Channel;
	EndSession(Id, /*bStopPlayback*/ true, /*bBroadcastFinished*/ true, false, FString());
	ProcessChannelQueue(Channel);
}

void UVideoManager::HandleWidgetFailed(UGreenVideoWidget* VideoWidget, const FString& Reason)
{
	FActiveSession* Session = FindSessionByWidget(VideoWidget);
	if (!Session)
	{
		return;
	}

	const FGuid Id = Session->Handle.Id;
	const FName Channel = Session->Request.Channel;
	EndSession(Id, /*bStopPlayback*/ true, false, /*bBroadcastFailed*/ true, Reason);
	ProcessChannelQueue(Channel);
}

UVideoManager::FActiveSession* UVideoManager::FindSessionByActor(AGreenVideoActor* Actor)
{
	if (!Actor)
	{
		return nullptr;
	}
	for (TPair<FGuid, FActiveSession>& Pair : ActiveSessions)
	{
		if (Pair.Value.Actor.Get() == Actor)
		{
			return &Pair.Value;
		}
	}
	return nullptr;
}

UVideoManager::FActiveSession* UVideoManager::FindSessionByWidget(UGreenVideoWidget* Widget)
{
	if (!Widget)
	{
		return nullptr;
	}
	for (TPair<FGuid, FActiveSession>& Pair : ActiveSessions)
	{
		if (Pair.Value.Widget.Get() == Widget)
		{
			return &Pair.Value;
		}
	}
	return nullptr;
}

UVideoManager::FActiveSession* UVideoManager::FindSessionByHandleMutable(const FVideoHandle& Handle)
{
	return ActiveSessions.Find(Handle.Id);
}
