#include "PoopEjectComponent.h"

#include "AlsCharacter.h"
#include "AlsCharacterMovementComponent.h"
#include "Animation/AnimMontage.h"
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
#include "PoopProjectileActor.h"
#include "Utility/AlsGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PoopEjectComponent)

namespace PoopEjectPrivate
{
	static const FName ShitNotifyName(TEXT("shit"));

	// 用 \u 转义，避免源文件编码导致中文乱码（铁链 UI 则只写数字、中文留在蓝图）。
	static FText MakeAllText(int32 Count)
	{
		return FText::FromString(FString::Printf(TEXT("\u6240\u6709\u5927\u4fbf\uff1a%d"), Count));
	}

	static FText MakeDoneText(int32 Count)
	{
		return FText::FromString(FString::Printf(TEXT("\u5df2\u62c9\u5927\u4fbf\uff1a%d"), Count));
	}
}

UPoopEjectComponent::UPoopEjectComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UPoopEjectComponent::BeginDestroy()
{
	ClearSpaceBinding();
	CloseCountWidget();

	if (bSwitchedToSingleNode)
	{
		if (USkeletalMeshComponent* Mesh = BoundMesh.Get())
		{
			RestoreAnimBlueprint(Mesh);
		}
	}

	Super::BeginDestroy();
}

void UPoopEjectComponent::AddGiftCharges(int32 GiftCount)
{
	const int32 Add = ClicksPerGift * FMath::Clamp(GiftCount, 1, 50);
	PendingClicks += Add;

	EnsureCountWidget();
	EnsureSpaceBinding();
	RefreshCountUI();
	SetComponentTickEnabled(true);

	UE_LOG(LogTemp, Log, TEXT("[PoopEject] +%d pending, now pending=%d done=%d"),
		Add, PendingClicks, CompletedClicks);
}

void UPoopEjectComponent::EnsureCountWidget()
{
	if (CountWidget)
	{
		return;
	}

	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	if (!PC)
	{
		return;
	}

	UClass* WidgetClass = StaticLoadClass(
		UUserWidget::StaticClass(),
		nullptr,
		TEXT("/Game/ModAsset/dabian/BP_DabianCount.BP_DabianCount_C"));
	if (!WidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[PoopEject] Failed to load BP_DabianCount."));
		return;
	}

	CountWidget = CreateWidget<UUserWidget>(PC, WidgetClass);
	if (!CountWidget)
	{
		return;
	}

	CountWidget->AddToViewport(1200);
	// 字体/颜色用蓝图里配好的，C++ 只改文字内容。
	TxAll = Cast<UTextBlock>(CountWidget->GetWidgetFromName(TEXT("tx_all")));
	TxCount = Cast<UTextBlock>(CountWidget->GetWidgetFromName(TEXT("tx_Count")));

	if (!TxAll || !TxCount)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PoopEject] tx_all=%d tx_Count=%d"),
			TxAll != nullptr, TxCount != nullptr);
	}
}

void UPoopEjectComponent::CloseCountWidget()
{
	if (CountWidget)
	{
		CountWidget->RemoveFromParent();
		CountWidget = nullptr;
	}
	TxAll = nullptr;
	TxCount = nullptr;
}

void UPoopEjectComponent::RefreshCountUI()
{
	// 字体用蓝图绑定的 textHeppt_Font；这里只改内容，不改 Font/Color。
	if (TxAll)
	{
		TxAll->SetText(PoopEjectPrivate::MakeAllText(PendingClicks));
	}
	if (TxCount)
	{
		TxCount->SetText(PoopEjectPrivate::MakeDoneText(CompletedClicks));
	}
}

void UPoopEjectComponent::EnsureSpaceBinding()
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

	SessionInputComponent = NewObject<UInputComponent>(this, TEXT("PoopEjectInput"));
	SessionInputComponent->RegisterComponent();
	SessionInputComponent->bBlockInput = false;
	SessionInputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &UPoopEjectComponent::OnSpacePressed);
	if (SessionInputComponent->KeyBindings.Num() > 0)
	{
		SessionInputComponent->KeyBindings.Last().bConsumeInput = true;
	}

	PC->PushInputComponent(SessionInputComponent);
	bSpaceBound = true;
}

void UPoopEjectComponent::ClearSpaceBinding()
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

void UPoopEjectComponent::OnSpacePressed()
{
	if (PendingClicks <= 0)
	{
		return;
	}

	--PendingClicks;
	++CompletedClicks;
	RefreshCountUI();

	// 连按：上一段未到关键帧则先补喷，再立刻重播。
	if (bPlayingOne && !bEjected)
	{
		bEjected = true;
		SpawnEjectedPoop();
	}

	PlayOneEject();
}

void UPoopEjectComponent::PlayOneEject()
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character)
	{
		return;
	}

	USkeletalMeshComponent* Mesh = Character->GetMesh();
	if (!Mesh)
	{
		return;
	}

	if (!DanceMontage)
	{
		DanceMontage = LoadObject<UAnimMontage>(
			nullptr, TEXT("/Game/ModAsset/dabian/dance2_Anim_Montage.dance2_Anim_Montage"));
	}
	if (!DanceSequence)
	{
		DanceSequence = LoadObject<UAnimSequence>(
			nullptr, TEXT("/Game/ModAsset/dabian/dance2_Anim.dance2_Anim"));
		if (!DanceSequence)
		{
			DanceSequence = LoadObject<UAnimSequence>(
				nullptr, TEXT("/Game/ModAsset/dabian/animation/dance2_Anim.dance2_Anim"));
		}
	}
	if (!DanceSequence)
	{
		UE_LOG(LogTemp, Error, TEXT("[PoopEject] dance2_Anim sequence missing."));
		return;
	}

	bPlayingOne = true;
	bEjected = false;
	Elapsed = 0.0f;
	AnimDuration = 0.0f;

	ShitNotifyTime = FindShitNotifyTime(DanceMontage);
	if (ShitNotifyTime < 0.0f)
	{
		ShitNotifyTime = DanceSequence->GetPlayLength() * 0.55f;
	}

	LockAlsForAnim(Character);

	if (bSwitchedToSingleNode)
	{
		RestoreAnimBlueprint(Mesh);
	}

	BeginSingleNodePose(Mesh);
	SetComponentTickEnabled(true);
}

float UPoopEjectComponent::FindShitNotifyTime(const UAnimMontage* Montage) const
{
	if (!Montage)
	{
		return -1.0f;
	}

	for (const FAnimNotifyEvent& Event : Montage->Notifies)
	{
		if (Event.NotifyName == PoopEjectPrivate::ShitNotifyName)
		{
			return Event.GetTriggerTime();
		}
		if (Event.Notify && Event.Notify->GetNotifyName().Equals(TEXT("shit"), ESearchCase::IgnoreCase))
		{
			return Event.GetTriggerTime();
		}
	}
	return -1.0f;
}

void UPoopEjectComponent::LockAlsForAnim(ACharacter* Character)
{
	if (!Character)
	{
		return;
	}

	UCharacterMovementComponent* Move = Character->GetCharacterMovement();
	if (!Move)
	{
		return;
	}

	if (!bAlsLocked)
	{
		SavedGravityScale = Move->GravityScale;
		SavedRootMotionScale = Character->GetAnimRootMotionTranslationScale();
	}

	Character->SetAnimRootMotionTranslationScale(0.0f);

	// 保留重力，关键帧 LaunchCharacter 才能往上飞。
	Move->GravityScale = SavedGravityScale > KINDA_SMALL_NUMBER ? SavedGravityScale : 1.0f;

	if (UAlsCharacterMovementComponent* AlsMove = Cast<UAlsCharacterMovementComponent>(Move))
	{
		AlsMove->SetInputBlocked(true);
		// 不锁 MovementMode，否则无法进入 Falling，向上冲量会被吃掉。
		AlsMove->SetMovementModeLocked(false);
	}

	if (AAlsCharacter* Als = Cast<AAlsCharacter>(Character))
	{
		Als->SetLocomotionAction(AlsLocomotionActionTags::GettingUp);
	}

	bAlsLocked = true;
}

void UPoopEjectComponent::UnlockAls(ACharacter* Character)
{
	if (!Character || !bAlsLocked)
	{
		return;
	}

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
	}

	bAlsLocked = false;
}

void UPoopEjectComponent::BeginSingleNodePose(USkeletalMeshComponent* Mesh)
{
	if (!Mesh || !DanceSequence)
	{
		return;
	}

	SavedAnimTickOption = Mesh->VisibilityBasedAnimTickOption;
	Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	Mesh->PlayAnimation(DanceSequence, false);
	bSwitchedToSingleNode = true;
	BoundMesh = Mesh;
	AnimDuration = DanceSequence->GetPlayLength();
}

void UPoopEjectComponent::RestoreAnimBlueprint(USkeletalMeshComponent* Mesh)
{
	if (!Mesh || !bSwitchedToSingleNode)
	{
		return;
	}

	Mesh->Stop();
	Mesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	Mesh->VisibilityBasedAnimTickOption = SavedAnimTickOption;
	bSwitchedToSingleNode = false;
}

void UPoopEjectComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bPlayingOne)
	{
		return;
	}

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	USkeletalMeshComponent* Mesh = BoundMesh.Get();
	if (!Character || !Mesh)
	{
		FinishOneEject();
		return;
	}

	Elapsed += DeltaTime;

	float AnimTime = Elapsed;
	if (UAnimSingleNodeInstance* Single = Mesh->GetSingleNodeInstance())
	{
		AnimTime = Single->GetCurrentTime();
	}

	if (!bEjected && ShitNotifyTime >= 0.0f && AnimTime + KINDA_SMALL_NUMBER >= ShitNotifyTime)
	{
		bEjected = true;
		SpawnEjectedPoop();
	}

	const bool bAnimFinished = (AnimDuration > KINDA_SMALL_NUMBER && AnimTime >= AnimDuration - 0.02f)
		|| (Elapsed >= AnimDuration + 0.15f);
	if (bAnimFinished)
	{
		if (!bEjected)
		{
			bEjected = true;
			SpawnEjectedPoop();
		}
		FinishOneEject();
	}
}

void UPoopEjectComponent::ApplyUpBoost(ACharacter* Character)
{
	if (!Character)
	{
		return;
	}

	if (UCharacterMovementComponent* Move = Character->GetCharacterMovement())
	{
		Move->GravityScale = SavedGravityScale > KINDA_SMALL_NUMBER ? SavedGravityScale : 1.0f;
		if (UAlsCharacterMovementComponent* AlsMove = Cast<UAlsCharacterMovementComponent>(Move))
		{
			AlsMove->SetMovementModeLocked(false);
		}
		Move->SetMovementMode(MOVE_Falling);
	}

	// 只覆盖 Z，保留水平速度。
	Character->LaunchCharacter(FVector(0.0f, 0.0f, UpBoostSpeed), false, true);
}

void UPoopEjectComponent::SpawnEjectedPoop()
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	UWorld* World = GetWorld();
	if (!Character || !World)
	{
		return;
	}

	// 拉大便关键帧：给玩家一次向上的力。
	ApplyUpBoost(Character);

	const FVector Back = -Character->GetActorForwardVector().GetSafeNormal2D();
	const FVector Right = Character->GetActorRightVector();
	const float BehindYaw = Back.IsNearlyZero()
		? Character->GetActorRotation().Yaw + 180.0f
		: Back.Rotation().Yaw;
	const FVector LaunchDir = FRotator(-80.0f, BehindYaw, 0.0f).Vector().GetSafeNormal();

	FVector ButtLocation = Character->GetActorLocation();
	if (USkeletalMeshComponent* Mesh = Character->GetMesh())
	{
		static const FName ButtBones[] = { TEXT("pelvis"), TEXT("spine_01"), TEXT("root") };
		bool bFound = false;
		for (const FName BoneName : ButtBones)
		{
			if (Mesh->DoesSocketExist(BoneName) || Mesh->GetBoneIndex(BoneName) != INDEX_NONE)
			{
				ButtLocation = Mesh->GetSocketLocation(BoneName);
				bFound = true;
				break;
			}
		}
		if (!bFound)
		{
			float HalfHeight = 90.0f;
			if (UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
			{
				HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
			}
			ButtLocation = Character->GetActorLocation() - FVector(0.0f, 0.0f, HalfHeight * 0.55f);
		}
	}

	ButtLocation += Back * 20.0f - FVector(0.0f, 0.0f, 15.0f);

	constexpr float EjectSpeed = 650.0f;
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.Instigator = Character;
	Params.Owner = Character;

	if (APoopProjectileActor* Poop = World->SpawnActor<APoopProjectileActor>(
			APoopProjectileActor::StaticClass(), ButtLocation, LaunchDir.Rotation(), Params))
	{
		Poop->LaunchWithVelocity(LaunchDir * EjectSpeed + Right * FMath::FRandRange(-40.0f, 40.0f));
	}
}

void UPoopEjectComponent::FinishOneEject()
{
	if (!bPlayingOne)
	{
		return;
	}

	bPlayingOne = false;

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (USkeletalMeshComponent* Mesh = BoundMesh.Get())
	{
		RestoreAnimBlueprint(Mesh);
	}
	else if (Character && Character->GetMesh())
	{
		RestoreAnimBlueprint(Character->GetMesh());
	}

	BoundMesh = nullptr;
	UnlockAls(Character);

	if (PendingClicks <= 0)
	{
		EndSession();
	}
	else
	{
		// 会话未结束也保持 tick 关着即可，等下次空格再开。
		SetComponentTickEnabled(false);
	}
}

void UPoopEjectComponent::EndSession()
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	UnlockAls(Character);
	ClearSpaceBinding();
	CloseCountWidget();
	PendingClicks = 0;
	CompletedClicks = 0;
	SetComponentTickEnabled(false);
	UE_LOG(LogTemp, Log, TEXT("[PoopEject] Session complete."));
}
