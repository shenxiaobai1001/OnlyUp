#include "JumpRopeComponent.h"

#include "AlsCharacter.h"
#include "AlsCharacterMovementComponent.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Blueprint/UserWidget.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/TextBlock.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Utility/AlsGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JumpRopeComponent)

namespace JumpRopePrivate
{
	static const FName CheckJumpNotifyName(TEXT("CheckJump"));
	static const FName RopeMeshName(TEXT("ts"));

	static const TCHAR* RopeActorPath = TEXT("/Game/ModAsset/tiaosheng/BP_TSheng.BP_TSheng_C");
	static const TCHAR* CountWidgetPath = TEXT("/Game/ModAsset/tiaosheng/BP_TiaoshengCount.BP_TiaoshengCount_C");
	static const TCHAR* RopeSequencePath = TEXT("/Game/ModAsset/tiaosheng/tiaoshengScene2.tiaoshengScene2");

	static const TCHAR* JumpSequencePaths[] = {
		TEXT("/Game/ModAsset/TSAnimation/Export/jump_Anim.jump_Anim"),
		TEXT("/Game/ModAsset/TSAnimation/jump_Anim.jump_Anim"),
	};
	static const TCHAR* DownSequencePaths[] = {
		TEXT("/Game/ModAsset/TSAnimation/Export/down_Anim.down_Anim"),
		TEXT("/Game/ModAsset/TSAnimation/down_Anim.down_Anim"),
	};

	static const TCHAR* SfxLoopPath = TEXT("/Game/ModAsset/TSAnimation/\u8df3\u7ef3.\u8df3\u7ef3");
	static const TCHAR* SfxSuccessPath = TEXT("/Game/ModAsset/TSAnimation/\u8df3\u7ef3\u6210\u529f.\u8df3\u7ef3\u6210\u529f");
	static const TCHAR* SfxFailPath = TEXT("/Game/ModAsset/TSAnimation/\u8df3\u7ef3\u5931\u8d25.\u8df3\u7ef3\u5931\u8d25");

	static UAnimSequence* LoadFirstSequence(const TCHAR* const* Paths, int32 Num)
	{
		for (int32 i = 0; i < Num; ++i)
		{
			if (UAnimSequence* Seq = LoadObject<UAnimSequence>(nullptr, Paths[i]))
			{
				return Seq;
			}
		}
		return nullptr;
	}
}

UJumpRopeComponent::UJumpRopeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UJumpRopeComponent::BeginDestroy()
{
	EndSession();
	Super::BeginDestroy();
}

void UJumpRopeComponent::AddJumpCharges(int32 Count)
{
	const int32 Add = FMath::Clamp(Count, 1, 100);
	RemainingJumps += Add;

	EnsureSession();
	RefreshCountUI(true, false, false);

	UE_LOG(LogTemp, Log, TEXT("[JumpRope] +%d remaining=%d succ=%d fail=%d"),
		Add, RemainingJumps, SuccessCount, FailCount);
}

void UJumpRopeComponent::LoadAssets()
{
	if (!JumpSequence)
	{
		JumpSequence = JumpRopePrivate::LoadFirstSequence(
			JumpRopePrivate::JumpSequencePaths, UE_ARRAY_COUNT(JumpRopePrivate::JumpSequencePaths));
	}
	if (!DownSequence)
	{
		DownSequence = JumpRopePrivate::LoadFirstSequence(
			JumpRopePrivate::DownSequencePaths, UE_ARRAY_COUNT(JumpRopePrivate::DownSequencePaths));
	}
	if (!RopeSequence)
	{
		RopeSequence = LoadObject<UAnimSequence>(nullptr, JumpRopePrivate::RopeSequencePath);
	}
	if (!SfxLoop)
	{
		SfxLoop = LoadObject<USoundBase>(nullptr, JumpRopePrivate::SfxLoopPath);
	}
	if (!SfxSuccess)
	{
		SfxSuccess = LoadObject<USoundBase>(nullptr, JumpRopePrivate::SfxSuccessPath);
	}
	if (!SfxFail)
	{
		SfxFail = LoadObject<USoundBase>(nullptr, JumpRopePrivate::SfxFailPath);
	}

	if (RopeSequence)
	{
		RopeAnimLength = RopeSequence->GetPlayLength();
		CheckJumpTime = FindCheckJumpTime(RopeSequence);
		if (CheckJumpTime < 0.0f && RopeAnimLength > KINDA_SMALL_NUMBER)
		{
			CheckJumpTime = RopeAnimLength * 0.75f;
		}
	}

	if (!JumpSequence)
	{
		UE_LOG(LogTemp, Error, TEXT("[JumpRope] jump_Anim sequence missing."));
	}
	if (!DownSequence)
	{
		UE_LOG(LogTemp, Error, TEXT("[JumpRope] down_Anim sequence missing."));
	}
}

void UJumpRopeComponent::EnsureSession()
{
	LoadAssets();

	if (!bSessionActive)
	{
		bSessionActive = true;
		SuccessCount = 0;
		FailCount = 0;
		bCycleBusy = false;
		bCycleAwaitingWrap = false;
		bCycleWrapped = false;
		bPendingEndSession = false;
		SpawnRopeActor();
		EnsureCountWidget();
		EnsureSpaceBinding();
		LockPlayerMovement();
		SnapPlayerToRope();
		EnsureRopeLooping();
		SetComponentTickEnabled(true);
	}
	else
	{
		if (!RopeActor)
		{
			SpawnRopeActor();
			EnsureRopeLooping();
		}
		EnsureCountWidget();
		EnsureSpaceBinding();
		LockPlayerMovement();
	}
}

void UJumpRopeComponent::EndSession()
{
	ClearSpaceBinding();
	CloseCountWidget();
	RestorePlayerAnimBlueprint();
	UnlockPlayerMovement();

	if (RopeActor)
	{
		RopeActor->Destroy();
		RopeActor = nullptr;
	}
	RopeMesh.Reset();
	PlayerMesh.Reset();

	bSessionActive = false;
	bArmedForSuccess = false;
	bCheckJumpHandledThisCycle = false;
	bPlayingJump = false;
	bPlayingDown = false;
	bPlayerPoseActive = false;
	bCycleBusy = false;
	bCycleAwaitingWrap = false;
	bCycleWrapped = false;
	bPendingEndSession = false;
	EndSessionDelay = 0.0f;
	RemainingJumps = 0;
	SetComponentTickEnabled(false);

	UE_LOG(LogTemp, Log, TEXT("[JumpRope] Session end. succ=%d fail=%d"), SuccessCount, FailCount);
}

FVector UJumpRopeComponent::GetPlayerFootLocation(const ACharacter* Character) const
{
	if (!Character)
	{
		return FVector::ZeroVector;
	}

	float HalfHeight = 90.0f;
	if (const UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
	{
		HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	}
	return Character->GetActorLocation() - FVector(0.0f, 0.0f, HalfHeight + RopeExtraDownCm);
}

void UJumpRopeComponent::SpawnRopeActor()
{
	if (RopeActor)
	{
		return;
	}

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	UWorld* World = GetWorld();
	if (!Character || !World)
	{
		return;
	}

	UClass* RopeClass = StaticLoadClass(AActor::StaticClass(), nullptr, JumpRopePrivate::RopeActorPath);
	if (!RopeClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[JumpRope] Failed to load BP_TSheng."));
		return;
	}

	SavedPlayerRotation = Character->GetActorRotation();

	FRotator SpawnRot = SavedPlayerRotation;
	SpawnRot.Yaw += RopeYawOffsetDeg;
	const FVector SpawnLoc = GetPlayerFootLocation(Character);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.Owner = Character;
	Params.Instigator = Character;

	RopeActor = World->SpawnActor<AActor>(RopeClass, SpawnLoc, SpawnRot, Params);
	if (!RopeActor)
	{
		UE_LOG(LogTemp, Error, TEXT("[JumpRope] Spawn BP_TSheng failed."));
		return;
	}

	CacheRopeMesh();
}

USkeletalMeshComponent* UJumpRopeComponent::FindNamedMesh(AActor* Actor, FName Name) const
{
	if (!Actor)
	{
		return nullptr;
	}

	TArray<USkeletalMeshComponent*> Meshes;
	Actor->GetComponents<USkeletalMeshComponent>(Meshes);
	for (USkeletalMeshComponent* Mesh : Meshes)
	{
		if (Mesh && Mesh->GetFName() == Name)
		{
			return Mesh;
		}
	}
	for (USkeletalMeshComponent* Mesh : Meshes)
	{
		if (Mesh && Mesh->GetName().Equals(Name.ToString(), ESearchCase::IgnoreCase))
		{
			return Mesh;
		}
	}
	return nullptr;
}

void UJumpRopeComponent::CacheRopeMesh()
{
	RopeMesh = FindNamedMesh(RopeActor, JumpRopePrivate::RopeMeshName);
	if (!RopeMesh.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[JumpRope] ts mesh not found on BP_TSheng."));
	}
}

void UJumpRopeComponent::EnsureRopeLooping()
{
	USkeletalMeshComponent* Mesh = RopeMesh.Get();
	if (!Mesh || !RopeSequence)
	{
		return;
	}

	Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	Mesh->SetAnimation(RopeSequence);
	Mesh->Play(true);
	Mesh->SetPosition(0.0f, false);

	LastRopeTime = 0.0f;
	bCheckJumpHandledThisCycle = false;
	bRopeSoundPlayedThisCycle = false;
	PlaySfx(SfxLoop);
	bRopeSoundPlayedThisCycle = true;
}

void UJumpRopeComponent::SyncRopeForJump()
{
	USkeletalMeshComponent* Mesh = RopeMesh.Get();
	if (!Mesh || !RopeSequence)
	{
		return;
	}

	if (UAnimSingleNodeInstance* Single = Mesh->GetSingleNodeInstance())
	{
		Single->SetPosition(0.0f, false);
		Single->SetPlaying(true);
		Single->SetLooping(true);
	}
	else
	{
		Mesh->SetPosition(0.0f, false);
		Mesh->Play(true);
	}

	LastRopeTime = 0.0f;
	bCheckJumpHandledThisCycle = false;
	bRopeSoundPlayedThisCycle = false;
	PlaySfx(SfxLoop);
	bRopeSoundPlayedThisCycle = true;
}

float UJumpRopeComponent::FindCheckJumpTime(const UAnimSequence* Sequence) const
{
	if (!Sequence)
	{
		return -1.0f;
	}

	for (const FAnimNotifyEvent& Event : Sequence->Notifies)
	{
		if (Event.NotifyName == JumpRopePrivate::CheckJumpNotifyName)
		{
			return Event.GetTriggerTime();
		}
		if (Event.Notify && Event.Notify->GetNotifyName().Equals(TEXT("CheckJump"), ESearchCase::IgnoreCase))
		{
			return Event.GetTriggerTime();
		}
	}
	return -1.0f;
}

void UJumpRopeComponent::EnsureCountWidget()
{
	if (CountWidget)
	{
		if (!CountWidget->IsInViewport())
		{
			CountWidget->AddToViewport(1300);
		}
		return;
	}

	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	if (!PC)
	{
		return;
	}

	UClass* WidgetClass = StaticLoadClass(UUserWidget::StaticClass(), nullptr, JumpRopePrivate::CountWidgetPath);
	if (!WidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[JumpRope] Failed to load BP_TiaoshengCount."));
		return;
	}

	CountWidget = CreateWidget<UUserWidget>(PC, WidgetClass);
	if (!CountWidget)
	{
		return;
	}

	CountWidget->AddToViewport(1300);
	TxAll = Cast<UTextBlock>(CountWidget->GetWidgetFromName(TEXT("tx_all")));
	TxSucc = Cast<UTextBlock>(CountWidget->GetWidgetFromName(TEXT("tx_Succ")));
	TxFail = Cast<UTextBlock>(CountWidget->GetWidgetFromName(TEXT("tx_Fail")));

	if (!TxAll || !TxSucc || !TxFail)
	{
		UE_LOG(LogTemp, Warning, TEXT("[JumpRope] UI bind tx_all=%d tx_Succ=%d tx_Fail=%d"),
			TxAll != nullptr, TxSucc != nullptr, TxFail != nullptr);
	}
}

void UJumpRopeComponent::CloseCountWidget()
{
	if (CountWidget)
	{
		CountWidget->RemoveFromParent();
		CountWidget = nullptr;
	}
	TxAll = nullptr;
	TxSucc = nullptr;
	TxFail = nullptr;
	PulseAllTime = PulseSuccTime = PulseFailTime = 0.0f;
}

void UJumpRopeComponent::RefreshCountUI(bool bPulseAll, bool bPulseSucc, bool bPulseFail)
{
	if (TxAll)
	{
		TxAll->SetText(FText::AsNumber(RemainingJumps));
		if (bPulseAll)
		{
			PulseText(TxAll);
			PulseAllTime = TextPulseDuration;
		}
	}
	if (TxSucc)
	{
		TxSucc->SetText(FText::AsNumber(SuccessCount));
		if (bPulseSucc)
		{
			PulseText(TxSucc);
			PulseSuccTime = TextPulseDuration;
		}
	}
	if (TxFail)
	{
		TxFail->SetText(FText::AsNumber(FailCount));
		if (bPulseFail)
		{
			PulseText(TxFail);
			PulseFailTime = TextPulseDuration;
		}
	}
}

void UJumpRopeComponent::PulseText(UTextBlock* TextBlock)
{
	if (TextBlock)
	{
		TextBlock->SetRenderScale(FVector2D(TextPulseScale, TextPulseScale));
	}
}

void UJumpRopeComponent::TickTextPulses(float DeltaTime)
{
	auto TickOne = [DeltaTime](UTextBlock* Text, float& TimeLeft, float PeakScale, float Duration)
	{
		if (!Text || TimeLeft <= 0.0f)
		{
			return;
		}
		TimeLeft = FMath::Max(0.0f, TimeLeft - DeltaTime);
		const float Alpha = Duration > KINDA_SMALL_NUMBER ? (1.0f - (TimeLeft / Duration)) : 1.0f;
		const float Scale = FMath::Lerp(PeakScale, 1.0f, Alpha);
		Text->SetRenderScale(FVector2D(Scale, Scale));
		if (TimeLeft <= 0.0f)
		{
			Text->SetRenderScale(FVector2D(1.0f, 1.0f));
		}
	};

	TickOne(TxAll, PulseAllTime, TextPulseScale, TextPulseDuration);
	TickOne(TxSucc, PulseSuccTime, TextPulseScale, TextPulseDuration);
	TickOne(TxFail, PulseFailTime, TextPulseScale, TextPulseDuration);
}

void UJumpRopeComponent::EnsureSpaceBinding()
{
	if (bSpaceBound)
	{
		return;
	}

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	APlayerController* PC = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
	if (!PC)
	{
		PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	}
	if (!PC)
	{
		return;
	}

	SessionInputComponent = NewObject<UInputComponent>(this, TEXT("JumpRopeInput"));
	SessionInputComponent->RegisterComponent();
	SessionInputComponent->bBlockInput = false;
	SessionInputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &UJumpRopeComponent::OnSpacePressed);
	if (SessionInputComponent->KeyBindings.Num() > 0)
	{
		SessionInputComponent->KeyBindings.Last().bConsumeInput = true;
	}

	PC->PushInputComponent(SessionInputComponent);
	bSpaceBound = true;
}

void UJumpRopeComponent::ClearSpaceBinding()
{
	if (!bSpaceBound)
	{
		return;
	}

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	APlayerController* PC = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
	if (!PC)
	{
		PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	}

	if (PC && SessionInputComponent)
	{
		PC->PopInputComponent(SessionInputComponent);
	}
	if (SessionInputComponent)
	{
		SessionInputComponent->DestroyComponent();
		SessionInputComponent = nullptr;
	}
	bSpaceBound = false;
}

void UJumpRopeComponent::OnSpacePressed()
{
	if (!bSessionActive || RemainingJumps <= 0 || bPendingEndSession)
	{
		return;
	}

	// 频率限制：上一圈绳子+起跳/滑倒未结束，忽略连按。
	if (bCycleBusy || bPlayingDown || bPlayingJump)
	{
		return;
	}

	bCycleBusy = true;
	bCycleAwaitingWrap = true;
	bCycleWrapped = false;
	bArmedForSuccess = true;
	BeginPlayerPose(JumpSequence, false);
	SyncRopeForJump();
}

void UJumpRopeComponent::ClearCycleBusy()
{
	bCycleBusy = false;
	bCycleAwaitingWrap = false;
	bCycleWrapped = false;
	bArmedForSuccess = false;
}

void UJumpRopeComponent::TryUnlockCycle()
{
	if (!bCycleBusy || bPendingEndSession)
	{
		return;
	}
	// 成功路径：绳子整圈回绕 + 跳跃姿态结束。
	if (bCycleAwaitingWrap && bCycleWrapped && !bPlayingJump && !bPlayingDown)
	{
		ClearCycleBusy();
	}
}

void UJumpRopeComponent::LockPlayerMovement()
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character || bMovementLocked)
	{
		return;
	}

	SavedJumpMaxCount = Character->JumpMaxCount;
	SavedRootMotionScale = Character->GetAnimRootMotionTranslationScale();
	Character->JumpMaxCount = 0;
	Character->StopJumping();
	Character->SetAnimRootMotionTranslationScale(0.0f);

	if (UCharacterMovementComponent* Move = Character->GetCharacterMovement())
	{
		SavedGravityScale = Move->GravityScale > KINDA_SMALL_NUMBER ? Move->GravityScale : 1.0f;
		Move->StopMovementImmediately();
		Move->Velocity = FVector::ZeroVector;
		Move->DisableMovement();
		if (UAlsCharacterMovementComponent* AlsMove = Cast<UAlsCharacterMovementComponent>(Move))
		{
			AlsMove->SetInputBlocked(true);
			AlsMove->SetMovementModeLocked(true);
		}
	}

	if (AAlsCharacter* Als = Cast<AAlsCharacter>(Character))
	{
		Als->SetLocomotionAction(AlsLocomotionActionTags::GettingUp);
	}

	if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
	{
		PC->SetIgnoreMoveInput(true);
	}

	bMovementLocked = true;
}

void UJumpRopeComponent::UnlockPlayerMovement()
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character || !bMovementLocked)
	{
		bMovementLocked = false;
		return;
	}

	Character->JumpMaxCount = SavedJumpMaxCount > 0 ? SavedJumpMaxCount : 1;
	Character->SetAnimRootMotionTranslationScale(SavedRootMotionScale);

	if (AAlsCharacter* Als = Cast<AAlsCharacter>(Character))
	{
		if (Als->GetLocomotionAction() == AlsLocomotionActionTags::GettingUp)
		{
			Als->SetLocomotionAction(FGameplayTag::EmptyTag);
		}
	}

	if (UCharacterMovementComponent* Move = Character->GetCharacterMovement())
	{
		Move->GravityScale = SavedGravityScale;
		if (UAlsCharacterMovementComponent* AlsMove = Cast<UAlsCharacterMovementComponent>(Move))
		{
			AlsMove->SetMovementModeLocked(false);
			AlsMove->SetInputBlocked(false);
		}
		Move->SetMovementMode(MOVE_Walking);
	}

	if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
	{
		PC->ResetIgnoreMoveInput();
	}

	bMovementLocked = false;
}

void UJumpRopeComponent::SnapPlayerToRope()
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character || !RopeActor)
	{
		return;
	}

	const FVector RopeLoc = RopeActor->GetActorLocation();
	const FVector PlayerLoc = Character->GetActorLocation();
	Character->SetActorLocation(
		FVector(RopeLoc.X, RopeLoc.Y, PlayerLoc.Z),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	Character->SetActorRotation(SavedPlayerRotation);
}

void UJumpRopeComponent::BeginPlayerPose(UAnimSequence* Sequence, bool bIsDown)
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character || !Sequence)
	{
		return;
	}

	USkeletalMeshComponent* Mesh = Character->GetMesh();
	if (!Mesh)
	{
		return;
	}

	if (bPlayerPoseActive)
	{
		RestorePlayerAnimBlueprint();
	}

	SavedAnimTickOption = Mesh->VisibilityBasedAnimTickOption;
	Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	Mesh->PlayAnimation(Sequence, false);

	PlayerMesh = Mesh;
	bPlayerPoseActive = true;
	bPlayingJump = !bIsDown;
	bPlayingDown = bIsDown;
	PlayerPoseElapsed = 0.0f;
	PlayerPoseDuration = Sequence->GetPlayLength();
}

void UJumpRopeComponent::RestorePlayerAnimBlueprint()
{
	USkeletalMeshComponent* Mesh = PlayerMesh.Get();
	if (!Mesh)
	{
		if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
		{
			Mesh = Character->GetMesh();
		}
	}
	if (!Mesh || !bPlayerPoseActive)
	{
		bPlayerPoseActive = false;
		bPlayingJump = false;
		bPlayingDown = false;
		return;
	}

	Mesh->Stop();
	Mesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	Mesh->VisibilityBasedAnimTickOption = SavedAnimTickOption;
	bPlayerPoseActive = false;
	bPlayingJump = false;
	bPlayingDown = false;
	PlayerPoseElapsed = 0.0f;
	PlayerPoseDuration = 0.0f;
}

void UJumpRopeComponent::TickPlayerPose(float DeltaTime)
{
	if (!bPlayerPoseActive)
	{
		return;
	}

	PlayerPoseElapsed += DeltaTime;

	float AnimTime = PlayerPoseElapsed;
	if (USkeletalMeshComponent* Mesh = PlayerMesh.Get())
	{
		if (UAnimSingleNodeInstance* Single = Mesh->GetSingleNodeInstance())
		{
			AnimTime = Single->GetCurrentTime();
		}
	}

	const bool bFinished = (PlayerPoseDuration > KINDA_SMALL_NUMBER && AnimTime >= PlayerPoseDuration - 0.02f)
		|| (PlayerPoseElapsed >= PlayerPoseDuration + 0.1f);

	if (!bFinished)
	{
		return;
	}

	const bool bWasDown = bPlayingDown;
	RestorePlayerAnimBlueprint();

	if (bWasDown)
	{
		// 滑倒播完：解除忙锁。
		ClearCycleBusy();
	}
	else
	{
		TryUnlockCycle();
	}
}

bool UJumpRopeComponent::IsPlayerJumpPlaying() const
{
	return bPlayingJump && bPlayerPoseActive;
}

bool UJumpRopeComponent::IsPlayerDownPlaying() const
{
	return bPlayingDown && bPlayerPoseActive;
}

void UJumpRopeComponent::PlaySfx(USoundBase* Sound) const
{
	if (!Sound)
	{
		return;
	}
	const AActor* Owner = GetOwner();
	UGameplayStatics::PlaySoundAtLocation(
		GetWorld(),
		Sound,
		Owner ? Owner->GetActorLocation() : FVector::ZeroVector);
}

void UJumpRopeComponent::OnSuccess()
{
	if (RemainingJumps <= 0)
	{
		return;
	}

	--RemainingJumps;
	++SuccessCount;
	bArmedForSuccess = false;
	PlaySfx(SfxSuccess);
	RefreshCountUI(true, true, false);

	if (RemainingJumps <= 0)
	{
		RequestEndAfterRopeCycle();
	}
	// 成功后仍保持 bCycleBusy，等本圈绳子回绕再解锁（保证整圈播完）。
}

void UJumpRopeComponent::OnFail()
{
	if (bPlayingDown)
	{
		return;
	}

	++FailCount;
	bArmedForSuccess = false;
	bCycleBusy = true;
	BeginPlayerPose(DownSequence, true);
	PlaySfx(SfxFail);
	RefreshCountUI(false, false, true);
}

void UJumpRopeComponent::RequestEndAfterRopeCycle()
{
	bPendingEndSession = true;
	const float Remain = (RopeAnimLength > KINDA_SMALL_NUMBER && CheckJumpTime >= 0.0f)
		? FMath::Max(0.15f, RopeAnimLength - CheckJumpTime + 0.05f)
		: 0.35f;
	EndSessionDelay = Remain;
	ClearSpaceBinding();
}

void UJumpRopeComponent::HandleCheckJump()
{
	if (RemainingJumps <= 0 || bPendingEndSession)
	{
		return;
	}

	if (bPlayingDown)
	{
		return;
	}

	const bool bSuccess = bArmedForSuccess || IsPlayerJumpPlaying();
	if (bSuccess)
	{
		OnSuccess();
	}
	else
	{
		OnFail();
	}
}

void UJumpRopeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bSessionActive)
	{
		return;
	}

	SnapPlayerToRope();
	TickTextPulses(DeltaTime);
	TickPlayerPose(DeltaTime);

	if (bPendingEndSession)
	{
		EndSessionDelay -= DeltaTime;
		if (EndSessionDelay <= 0.0f)
		{
			EndSession();
			return;
		}
	}

	USkeletalMeshComponent* Mesh = RopeMesh.Get();
	if (!Mesh)
	{
		CacheRopeMesh();
		Mesh = RopeMesh.Get();
		if (!Mesh)
		{
			return;
		}
		EnsureRopeLooping();
	}

	float AnimTime = LastRopeTime;
	if (UAnimSingleNodeInstance* Single = Mesh->GetSingleNodeInstance())
	{
		AnimTime = Single->GetCurrentTime();
		if (!Single->IsPlaying())
		{
			Single->SetPlaying(true);
		}
	}
	else
	{
		AnimTime = Mesh->GetPosition();
	}

	const bool bWrapped =
		RopeAnimLength > KINDA_SMALL_NUMBER
		&& AnimTime + KINDA_SMALL_NUMBER < LastRopeTime - RopeAnimLength * 0.25f;

	if (bPendingEndSession && bWrapped)
	{
		EndSession();
		return;
	}

	if (bWrapped)
	{
		bCheckJumpHandledThisCycle = false;
		bRopeSoundPlayedThisCycle = false;

		if (bCycleBusy && bCycleAwaitingWrap)
		{
			bCycleWrapped = true;
			TryUnlockCycle();
		}
	}

	if (!bRopeSoundPlayedThisCycle && AnimTime < 0.08f)
	{
		PlaySfx(SfxLoop);
		bRopeSoundPlayedThisCycle = true;
	}

	if (!bPendingEndSession
		&& !bPlayingDown
		&& !bCheckJumpHandledThisCycle
		&& CheckJumpTime >= 0.0f
		&& LastRopeTime < CheckJumpTime
		&& AnimTime + KINDA_SMALL_NUMBER >= CheckJumpTime)
	{
		bCheckJumpHandledThisCycle = true;
		HandleCheckJump();
		if (!bSessionActive)
		{
			return;
		}
	}

	LastRopeTime = AnimTime;
}
