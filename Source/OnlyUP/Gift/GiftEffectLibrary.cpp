#include "GiftEffectLibrary.h"

#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "BilliardBallActor.h"
#include "ExplosionFxPoolSubsystem.h"
#include "FileMediaSource.h"
#include "FryingPanActor.h"
#include "PoopEjectComponent.h"
#include "PoopProjectileActor.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HomingArrowMotorComponent.h"
#include "JumpRopeComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Misc/Paths.h"
#include "StreamMediaSource.h"
#include "VideoManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GiftEffectLibrary)

namespace GiftEffectPrivate
{
	static AActor* ResolveTarget(const UObject* WorldContextObject, AActor* Target)
	{
		if (IsValid(Target))
		{
			return Target;
		}

		UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
		if (!World)
		{
			return nullptr;
		}

		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (APawn* Pawn = PC->GetPawn())
			{
				return Pawn;
			}
		}
		return nullptr;
	}

	static void SpawnHomingArrows(const UObject* WorldContextObject, AActor* Target, int32 Count, bool bIce)
	{
		UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
		AActor* ResolvedTarget = ResolveTarget(WorldContextObject, Target);
		if (!World || !ResolvedTarget || Count <= 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Gift] SpawnHomingArrows failed. ice=%d"), bIce ? 1 : 0);
			return;
		}

		Count = FMath::Clamp(Count, 1, 100);

		const TCHAR* ClassPath = bIce
			? TEXT("/Game/ModAsset/arrow/BPIceArrow.BPIceArrow_C")
			: TEXT("/Game/ModAsset/arrow/BPFireArrow.BPFireArrow_C");

		UClass* ArrowClass = StaticLoadClass(AActor::StaticClass(), nullptr, ClassPath);
		if (!ArrowClass)
		{
			UE_LOG(LogTemp, Error, TEXT("[Gift] Failed to load arrow class: %s"), ClassPath);
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red,
					FString::Printf(TEXT("加载箭蓝图失败: %s"), ClassPath));
			}
			return;
		}

		float CapsuleHalfHeight = 90.0f;
		if (const ACharacter* Character = Cast<ACharacter>(ResolvedTarget))
		{
			if (const UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
			{
				CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
			}
		}

		const FVector Center = ResolvedTarget->GetActorLocation() + FVector(0.0f, 0.0f, CapsuleHalfHeight * 0.35f);
		constexpr float OrbitRadiusCm = 1500.0f; // 15 meters

		int32 SpawnedCount = 0;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const float Yaw = FMath::FRandRange(0.0f, 360.0f);
			const float Pitch = FMath::FRandRange(-35.0f, 35.0f);
			const FVector Offset = FRotator(Pitch, Yaw, 0.0f).RotateVector(FVector(OrbitRadiusCm, 0.0f, 0.0f));
			const FVector SpawnLocation = Center + Offset;
			const FRotator SpawnRotation = (Center - SpawnLocation).Rotation();

			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Params.Instigator = Cast<APawn>(ResolvedTarget);

			AActor* Arrow = World->SpawnActor<AActor>(ArrowClass, SpawnLocation, SpawnRotation, Params);
			if (!Arrow)
			{
				continue;
			}

			UHomingArrowMotorComponent* Motor = NewObject<UHomingArrowMotorComponent>(Arrow, TEXT("HomingArrowMotor"));
			Motor->RegisterComponent();
			Motor->StartHoming(ResolvedTarget, bIce);
			++SpawnedCount;
		}

		UE_LOG(LogTemp, Log, TEXT("[Gift] Spawned %d %s arrow(s)"), SpawnedCount, bIce ? TEXT("ice") : TEXT("fire"));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, bIce ? FColor::Cyan : FColor::Orange,
				FString::Printf(TEXT("已生成%s x%d"), bIce ? TEXT("冰箭") : TEXT("火箭"), SpawnedCount));
		}
	}
}

void UGiftEffectLibrary::SpawnFryingPans(const UObject* WorldContextObject, AActor* Target, int32 Count)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	AActor* ResolvedTarget = GiftEffectPrivate::ResolveTarget(WorldContextObject, Target);
	if (!World || !ResolvedTarget || Count <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Gift] SpawnFryingPans failed. World=%d Target=%s Count=%d"),
			World != nullptr, *GetNameSafe(ResolvedTarget), Count);
		return;
	}

	Count = FMath::Clamp(Count, 1, 50);

	float CapsuleHalfHeight = 90.0f;
	if (const ACharacter* Character = Cast<ACharacter>(ResolvedTarget))
	{
		if (const UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
		{
			CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
		}
	}

	const FVector HeadLocation = ResolvedTarget->GetActorLocation() + FVector(0.0f, 0.0f, CapsuleHalfHeight);

	constexpr float HeightAboveHeadCm = 100.0f;
	constexpr float OrbitRadiusMin = 180.0f;
	constexpr float OrbitRadiusMax = 320.0f;

	int32 SpawnedCount = 0;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const float Yaw = FMath::FRandRange(0.0f, 360.0f);
		const float Radius = FMath::FRandRange(OrbitRadiusMin, OrbitRadiusMax);
		const FVector Offset = FRotator(0.0f, Yaw, 0.0f).RotateVector(FVector(Radius, 0.0f, 0.0f));
		const FVector SpawnLocation = HeadLocation + FVector(0.0f, 0.0f, HeightAboveHeadCm) + Offset;

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.Instigator = Cast<APawn>(ResolvedTarget);

		if (AFryingPanActor* Pan = World->SpawnActor<AFryingPanActor>(AFryingPanActor::StaticClass(), SpawnLocation, FRotator::ZeroRotator, Params))
		{
			Pan->LaunchAtTarget(ResolvedTarget);
			++SpawnedCount;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[Gift] Spawned %d frying pan(s) at %s"), SpawnedCount, *HeadLocation.ToCompactString());
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan,
			FString::Printf(TEXT("已生成平底锅 x%d"), SpawnedCount));
	}
}

void UGiftEffectLibrary::SpawnBilliardBalls(const UObject* WorldContextObject, AActor* Target, int32 Count)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	AActor* ResolvedTarget = GiftEffectPrivate::ResolveTarget(WorldContextObject, Target);
	if (!World || !ResolvedTarget || Count <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Gift] SpawnBilliardBalls failed. World=%d Target=%s Count=%d"),
			World != nullptr, *GetNameSafe(ResolvedTarget), Count);
		return;
	}

	Count = FMath::Clamp(Count, 1, 50);

	if (UExplosionFxPoolSubsystem* FxPool = World->GetSubsystem<UExplosionFxPoolSubsystem>())
	{
		FxPool->Prewarm();
	}

	float CapsuleHalfHeight = 90.0f;
	if (const ACharacter* Character = Cast<ACharacter>(ResolvedTarget))
	{
		if (const UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
		{
			CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
		}
	}

	const FVector HeadLocation = ResolvedTarget->GetActorLocation() + FVector(0.0f, 0.0f, CapsuleHalfHeight);

	constexpr float HeightAboveHeadCm = 100.0f;
	constexpr float OrbitRadiusMin = 180.0f;
	constexpr float OrbitRadiusMax = 320.0f;

	int32 SpawnedCount = 0;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const float Yaw = FMath::FRandRange(0.0f, 360.0f);
		const float Radius = FMath::FRandRange(OrbitRadiusMin, OrbitRadiusMax);
		const FVector Offset = FRotator(0.0f, Yaw, 0.0f).RotateVector(FVector(Radius, 0.0f, 0.0f));
		const FVector SpawnLocation = HeadLocation + FVector(0.0f, 0.0f, HeightAboveHeadCm) + Offset;

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.Instigator = Cast<APawn>(ResolvedTarget);

		if (ABilliardBallActor* Ball = World->SpawnActor<ABilliardBallActor>(
				ABilliardBallActor::StaticClass(), SpawnLocation, FRotator::ZeroRotator, Params))
		{
			Ball->LaunchAtTarget(ResolvedTarget);
			++SpawnedCount;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[Gift] Spawned %d billiard ball(s)"), SpawnedCount);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow,
			FString::Printf(TEXT("已生成台球 x%d"), SpawnedCount));
	}
}

void UGiftEffectLibrary::SpawnThrowPoops(const UObject* WorldContextObject, AActor* Target, int32 Count)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	AActor* ResolvedTarget = GiftEffectPrivate::ResolveTarget(WorldContextObject, Target);
	if (!World || !ResolvedTarget || Count <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Gift] SpawnThrowPoops failed."));
		return;
	}

	Count = FMath::Clamp(Count, 1, 50);

	float CapsuleHalfHeight = 90.0f;
	if (const ACharacter* Character = Cast<ACharacter>(ResolvedTarget))
	{
		if (const UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
		{
			CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
		}
	}

	const FVector HeadLocation = ResolvedTarget->GetActorLocation() + FVector(0.0f, 0.0f, CapsuleHalfHeight);
	constexpr float HeightAboveHeadCm = 100.0f;
	constexpr float OrbitRadiusMin = 180.0f;
	constexpr float OrbitRadiusMax = 320.0f;

	int32 SpawnedCount = 0;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const float Yaw = FMath::FRandRange(0.0f, 360.0f);
		const float Radius = FMath::FRandRange(OrbitRadiusMin, OrbitRadiusMax);
		const FVector Offset = FRotator(0.0f, Yaw, 0.0f).RotateVector(FVector(Radius, 0.0f, 0.0f));
		const FVector SpawnLocation = HeadLocation + FVector(0.0f, 0.0f, HeightAboveHeadCm) + Offset;

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.Instigator = Cast<APawn>(ResolvedTarget);

		if (APoopProjectileActor* Poop = World->SpawnActor<APoopProjectileActor>(
				APoopProjectileActor::StaticClass(), SpawnLocation, FRotator::ZeroRotator, Params))
		{
			Poop->LaunchAtTarget(ResolvedTarget);
			++SpawnedCount;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[Gift] Spawned %d throw-poop(s)"), SpawnedCount);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Orange,
			FString::Printf(TEXT("已生成扔大便 x%d"), SpawnedCount));
	}
}

void UGiftEffectLibrary::SpawnPoopEject(const UObject* WorldContextObject, AActor* Target, int32 Count)
{
	AActor* ResolvedTarget = GiftEffectPrivate::ResolveTarget(WorldContextObject, Target);
	ACharacter* Character = Cast<ACharacter>(ResolvedTarget);
	if (!Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Gift] SpawnPoopEject needs a Character target."));
		return;
	}

	Count = FMath::Clamp(Count, 1, 50);

	UPoopEjectComponent* Eject = Character->FindComponentByClass<UPoopEjectComponent>();
	if (!Eject)
	{
		Eject = NewObject<UPoopEjectComponent>(Character, TEXT("PoopEject"));
		Eject->RegisterComponent();
	}
	// 每次触发 +10×数量 待点击空格；UI 同步出现，按空格逐次拉。
	Eject->AddGiftCharges(Count);
}

void UGiftEffectLibrary::SpawnFireArrows(const UObject* WorldContextObject, AActor* Target, int32 Count)
{
	GiftEffectPrivate::SpawnHomingArrows(WorldContextObject, Target, Count, false);
}

void UGiftEffectLibrary::SpawnIceArrows(const UObject* WorldContextObject, AActor* Target, int32 Count)
{
	GiftEffectPrivate::SpawnHomingArrows(WorldContextObject, Target, Count, true);
}

void UGiftEffectLibrary::SpawnJumpRope(const UObject* WorldContextObject, AActor* Target, int32 Count)
{
	AActor* ResolvedTarget = GiftEffectPrivate::ResolveTarget(WorldContextObject, Target);
	ACharacter* Character = Cast<ACharacter>(ResolvedTarget);
	if (!Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Gift] SpawnJumpRope needs a Character target."));
		return;
	}

	Count = FMath::Clamp(Count <= 0 ? 5 : Count, 1, 100);

	UJumpRopeComponent* Rope = Character->FindComponentByClass<UJumpRopeComponent>();
	if (!Rope)
	{
		Rope = NewObject<UJumpRopeComponent>(Character, TEXT("JumpRope"));
		Rope->RegisterComponent();
	}
	Rope->AddJumpCharges(Count);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green,
			FString::Printf(TEXT("跳绳 +%d"), Count));
	}
}

namespace GiftVideoPrivate
{
	static FVideoPlayRequest MakeScreenRequest(UMediaSource* Media, FName Channel, bool bChromaKey)
	{
		FVideoPlayRequest Request;
		Request.MediaSource = Media;
		Request.DisplayMode = EVideoDisplayMode::ScreenWidget;
		Request.QueuePolicy = EVideoQueuePolicy::PlayImmediately;
		Request.Channel = Channel.IsNone() ? FName(TEXT("Default")) : Channel;
		Request.Priority = 50;
		Request.bLoop = false;
		Request.bAutoDestroy = true;
		Request.Volume = 1.f;
		Request.ZOrder = 100;
		Request.bDisableChromaKey = !bChromaKey;
		if (bChromaKey)
		{
			Request.ChromaMaterial = TSoftObjectPtr<UMaterialInterface>(
				FSoftObjectPath(TEXT("/Game/Videos/M_UI_GreenChroma.M_UI_GreenChroma")));
		}
		return Request;
	}
}

FVideoHandle UGiftEffectLibrary::PlayGreenVideo(const UObject* WorldContextObject, const FVideoPlayRequest& Request)
{
	(void)WorldContextObject;
	UVideoManager* Manager = UVideoManager::Get();
	if (!Manager)
	{
		UE_LOG(LogTemp, Error, TEXT("[Gift|Video] VideoManager unavailable."));
		return FVideoHandle();
	}
	return Manager->PlayVideo(Request);
}

FVideoHandle UGiftEffectLibrary::PlayGreenVideoFromSource(
	const UObject* WorldContextObject,
	UMediaSource* MediaSource,
	bool bChromaKey,
	FName Channel)
{
	if (!MediaSource)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Gift|Video] MediaSource is null."));
		return FVideoHandle();
	}
	return PlayGreenVideo(WorldContextObject, GiftVideoPrivate::MakeScreenRequest(MediaSource, Channel, bChromaKey));
}

FVideoHandle UGiftEffectLibrary::PlayGreenVideoFromFile(
	const UObject* WorldContextObject,
	const FString& FilePath,
	bool bChromaKey,
	FName Channel)
{
	if (FilePath.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Gift|Video] FilePath is empty."));
		return FVideoHandle();
	}

	FString Resolved = FilePath;
	if (FPaths::IsRelative(Resolved))
	{
		Resolved = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir(), Resolved);
	}

	UObject* Outer = UVideoManager::Get();
	if (!Outer)
	{
		Outer = GetTransientPackage();
	}

	UFileMediaSource* FileMedia = NewObject<UFileMediaSource>(Outer);
	FileMedia->SetFilePath(Resolved);
	return PlayGreenVideoFromSource(WorldContextObject, FileMedia, bChromaKey, Channel);
}

FVideoHandle UGiftEffectLibrary::PlayGreenVideoFromUrl(
	const UObject* WorldContextObject,
	const FString& Url,
	bool bChromaKey,
	FName Channel)
{
	if (Url.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Gift|Video] Url is empty."));
		return FVideoHandle();
	}

	UObject* Outer = UVideoManager::Get();
	if (!Outer)
	{
		Outer = GetTransientPackage();
	}

	UStreamMediaSource* StreamMedia = NewObject<UStreamMediaSource>(Outer);
	StreamMedia->StreamUrl = Url;
	return PlayGreenVideoFromSource(WorldContextObject, StreamMedia, bChromaKey, Channel);
}

void UGiftEffectLibrary::ExecuteGiftById(const UObject* WorldContextObject, AActor* Target, FName GiftId, int32 Count)
{
	if (GiftId == TEXT("FryingPan") || GiftId == TEXT("扔平底锅"))
	{
		SpawnFryingPans(WorldContextObject, Target, Count);
		return;
	}
	if (GiftId == TEXT("BilliardBall") || GiftId == TEXT("扔台球") || GiftId == TEXT("台球"))
	{
		SpawnBilliardBalls(WorldContextObject, Target, Count);
		return;
	}
	if (GiftId == TEXT("ThrowPoop") || GiftId == TEXT("扔大便"))
	{
		SpawnThrowPoops(WorldContextObject, Target, Count);
		return;
	}
	if (GiftId == TEXT("PoopEject") || GiftId == TEXT("拉大便"))
	{
		SpawnPoopEject(WorldContextObject, Target, Count);
		return;
	}
	if (GiftId == TEXT("FireArrow") || GiftId == TEXT("射火箭"))
	{
		SpawnFireArrows(WorldContextObject, Target, Count);
		return;
	}
	if (GiftId == TEXT("IceArrow") || GiftId == TEXT("射冰箭"))
	{
		SpawnIceArrows(WorldContextObject, Target, Count);
		return;
	}
	if (GiftId == TEXT("JumpRope") || GiftId == TEXT("跳绳"))
	{
		SpawnJumpRope(WorldContextObject, Target, Count > 0 ? Count : 5);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Gift '%s' is not implemented yet."), *GiftId.ToString());
}
