#include "BilliardBallActor.h"

#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "ExplosionFxPoolSubsystem.h"
#include "GameFramework/Character.h"
#include "GiftRestSettleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BilliardBallActor)

namespace BilliardBallPrivate
{
	static const TCHAR* TextureAssetNames[] = {
		TEXT("1_yellow"),
		TEXT("2_blue"),
		TEXT("3_red"),
		TEXT("4_purple"),
		TEXT("5_orange"),
		TEXT("6_green"),
		TEXT("7_bordo"),
		TEXT("8_black"),
		TEXT("9_yellow_stripe"),
		TEXT("10_blue_stripe"),
		TEXT("11_red_stripe"),
		TEXT("12_purple_stripe"),
		TEXT("13_orange"),
		TEXT("14_green_white"),
		TEXT("15_bordo_stripe"),
		TEXT("White"),
	};

	static const FName BallTextureParam(TEXT("BallTexture"));
	static const TCHAR* RuntimeMaterialPath = TEXT("/Game/ModAsset/TaiQiu/caizhi/M_TaiQiuBall.M_TaiQiuBall");
}

ABilliardBallActor::ABilliardBallActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	HitProbe = CreateDefaultSubobject<USphereComponent>(TEXT("HitProbe"));
	HitProbe->SetupAttachment(SceneRoot);
	HitProbe->InitSphereRadius(12.0f);
	HitProbe->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitProbe->SetGenerateOverlapEvents(false);
	HitProbe->SetHiddenInGame(true);

	FallbackMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FallbackMesh"));
	FallbackMesh->SetupAttachment(SceneRoot);
	FallbackMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FallbackMesh->SetGenerateOverlapEvents(false);
	FallbackMesh->SetSimulatePhysics(false);
	FallbackMesh->SetMobility(EComponentMobility::Movable);
	FallbackMesh->SetVisibility(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshFinder(TEXT("/Game/ModAsset/TaiQiu/ball_model.ball_model"));
	if (MeshFinder.Succeeded())
	{
		FallbackMesh->SetStaticMesh(MeshFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> SoundFinder(
		TEXT("/Game/ModAsset/\u53f0\u7403.\u53f0\u7403"));
	if (SoundFinder.Succeeded())
	{
		HitSound = SoundFinder.Object;
	}
}

void ABilliardBallActor::BeginPlay()
{
	Super::BeginPlay();
	EnsureVisual();
	ApplyRandomTexture();
}

void ABilliardBallActor::EnsureVisual()
{
	if (VisualActor.IsValid())
	{
		return;
	}

	UClass* BallClass = StaticLoadClass(
		AActor::StaticClass(),
		nullptr,
		TEXT("/Game/ModAsset/TaiQiu/BP_TAIQIU.BP_TAIQIU_C"));

	if (BallClass)
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		if (AActor* Spawned = GetWorld()->SpawnActor<AActor>(BallClass, GetActorTransform(), Params))
		{
			VisualActor = Spawned;
			Spawned->AttachToComponent(SceneRoot, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			Spawned->SetActorScale3D(FVector(VisualScale));
			Spawned->SetActorEnableCollision(false);

			TArray<UPrimitiveComponent*> Prims;
			Spawned->GetComponents<UPrimitiveComponent>(Prims);
			for (UPrimitiveComponent* Prim : Prims)
			{
				Prim->SetSimulatePhysics(false);
				Prim->SetEnableGravity(false);
				Prim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			}

			FallbackMesh->SetVisibility(false);
			return;
		}
	}

	FallbackMesh->SetVisibility(true);
	FallbackMesh->SetRelativeScale3D(FVector(VisualScale));
}

void ABilliardBallActor::ApplyRandomTexture()
{
	// QIU uses a hardcoded TextureSample (not a parameter), so MID overrides never showed.
	// Use M_TaiQiuBall which exposes BallTexture as TextureSampleParameter2D.
	const int32 NumTextures = UE_ARRAY_COUNT(BilliardBallPrivate::TextureAssetNames);
	const int32 Pick = FMath::RandRange(0, NumTextures - 1);
	const FString AssetName = BilliardBallPrivate::TextureAssetNames[Pick];
	const FString Path = FString::Printf(
		TEXT("/Game/ModAsset/TaiQiu/Tuxture/%s.%s"), *AssetName, *AssetName);

	ChosenTexture = LoadObject<UTexture2D>(nullptr, *Path);
	ChosenTextureName = ChosenTexture ? FName(*ChosenTexture->GetName()) : FName(*AssetName);
	// Strict: only the 8_black texture asset may trigger knockback / explosion.
	bIsEightBall = ChosenTextureName.IsEqual(TEXT("8_black"));

	if (!ChosenTexture)
	{
		bIsEightBall = false;
		UE_LOG(LogTemp, Warning, TEXT("[TaiQiu] Failed to load texture: %s"), *Path);
		return;
	}

	UMaterialInterface* RuntimeMat = LoadObject<UMaterialInterface>(
		nullptr, BilliardBallPrivate::RuntimeMaterialPath);
	if (!RuntimeMat)
	{
		UE_LOG(LogTemp, Error, TEXT("[TaiQiu] Missing runtime material: %s"),
			BilliardBallPrivate::RuntimeMaterialPath);
		return;
	}

	TArray<UStaticMeshComponent*> TargetMeshes;
	if (UStaticMeshComponent* BallMesh = FindBallModelMesh())
	{
		TargetMeshes.Add(BallMesh);
	}
	if (FallbackMesh)
	{
		TargetMeshes.AddUnique(FallbackMesh);
	}

	for (UStaticMeshComponent* MeshComp : TargetMeshes)
	{
		if (!MeshComp)
		{
			continue;
		}

		const int32 NumMaterials = FMath::Max(MeshComp->GetNumMaterials(), 1);
		for (int32 Slot = 0; Slot < NumMaterials; ++Slot)
		{
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(RuntimeMat, this);
			if (!MID)
			{
				continue;
			}
			MID->SetTextureParameterValue(BilliardBallPrivate::BallTextureParam, ChosenTexture);
			MeshComp->SetMaterial(Slot, MID);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[TaiQiu] Texture=%s EightBall=%d"), *AssetName, bIsEightBall ? 1 : 0);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1, 1.5f, bIsEightBall ? FColor::Black : FColor::Yellow,
			FString::Printf(TEXT("台球贴图: %s"), *AssetName));
	}
}

UStaticMeshComponent* ABilliardBallActor::FindBallModelMesh() const
{
	if (AActor* Visual = VisualActor.Get())
	{
		TArray<UStaticMeshComponent*> Meshes;
		Visual->GetComponents<UStaticMeshComponent>(Meshes);
		for (UStaticMeshComponent* MeshComp : Meshes)
		{
			if (!MeshComp)
			{
				continue;
			}
			const FString CompName = MeshComp->GetName();
			if (CompName.Contains(TEXT("ball_model"), ESearchCase::IgnoreCase))
			{
				return MeshComp;
			}
		}
		for (UStaticMeshComponent* MeshComp : Meshes)
		{
			if (MeshComp && MeshComp->GetStaticMesh())
			{
				return MeshComp;
			}
		}
	}
	return nullptr;
}

void ABilliardBallActor::LaunchAtTarget(AActor* InTarget)
{
	TargetActor = InTarget;
	bGuidedPhase = true;
	bHasBounced = false;
	EnsureVisual();
	if (!ChosenTexture)
	{
		ApplyRandomTexture();
	}
	SetActorTickEnabled(true);
}

FVector ABilliardBallActor::GetAimPoint() const
{
	AActor* Target = TargetActor.Get();
	if (!Target)
	{
		return GetActorLocation();
	}

	if (const ACharacter* Character = Cast<ACharacter>(Target))
	{
		if (const UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
		{
			const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
			return Character->GetActorLocation() + FVector(0.0f, 0.0f, HalfHeight * 0.35f);
		}
	}

	return Target->GetActorLocation() + FVector(0.0f, 0.0f, 70.0f);
}

bool ABilliardBallActor::IsCloseEnoughToHit() const
{
	AActor* Target = TargetActor.Get();
	if (!Target)
	{
		return false;
	}

	const FVector Aim = GetAimPoint();
	const float DistToAim = FVector::Dist(GetActorLocation(), Aim);
	if (DistToAim > HitAcceptDistance)
	{
		return false;
	}

	float BodyRadius = 50.0f;
	if (const ACharacter* Character = Cast<ACharacter>(Target))
	{
		if (const UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
		{
			BodyRadius = Capsule->GetScaledCapsuleRadius() + 25.0f;
		}
	}

	const float DistToBody = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
	return DistToBody <= (BodyRadius + HitAcceptDistance);
}

UPrimitiveComponent* ABilliardBallActor::FindVisualPrimitive() const
{
	if (UStaticMeshComponent* BallMesh = FindBallModelMesh())
	{
		return BallMesh;
	}
	return FallbackMesh;
}

void ABilliardBallActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bGuidedPhase)
	{
		ApplyGuidedMotion(DeltaSeconds);
	}
}

void ABilliardBallActor::ApplyGuidedMotion(float DeltaSeconds)
{
	if (!TargetActor.IsValid())
	{
		return;
	}

	if (IsCloseEnoughToHit())
	{
		SwitchToPhysicsBounce();
		return;
	}

	const FVector Aim = GetAimPoint();
	const FVector Current = GetActorLocation();
	FVector ToTarget = Aim - Current;
	const float Distance = ToTarget.Size();
	if (Distance < KINDA_SMALL_NUMBER)
	{
		return;
	}

	ToTarget /= Distance;
	FlightDirection = ToTarget;
	const float Step = FMath::Min(GuidedSpeed * DeltaSeconds, Distance);
	SetActorLocation(Current + ToTarget * Step, false, nullptr, ETeleportType::None);

	const FRotator FaceRot = ToTarget.Rotation();
	SetActorRotation(FRotator(FaceRot.Pitch, FaceRot.Yaw, GetWorld()->GetTimeSeconds() * 360.0f));
}

void ABilliardBallActor::PlayHitSound()
{
	USoundBase* Sound = HitSound;
	if (!Sound)
	{
		Sound = LoadObject<USoundBase>(nullptr, TEXT("/Game/ModAsset/\u53f0\u7403.\u53f0\u7403"));
	}
	if (Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation());
	}
}

bool ABilliardBallActor::IsEightBallTexture() const
{
	return bIsEightBall && ChosenTextureName.IsEqual(TEXT("8_black"));
}

void ABilliardBallActor::ApplyEightBallHitEffects(const FVector& ContactLocation, const FVector& FlightDir)
{
	// Hard gate — never knock / explode unless texture asset is exactly 8_black.
	if (!IsEightBallTexture())
	{
		return;
	}

	AActor* Target = TargetActor.Get();
	ACharacter* Character = Cast<ACharacter>(Target);

	FVector FxLocation = ContactLocation;
	if (Character)
	{
		// Play slightly in front of the body so the FX is not buried inside the mesh.
		FxLocation = Character->GetActorLocation()
			+ FlightDir.GetSafeNormal() * 40.0f
			+ FVector(0.0f, 0.0f, 50.0f);
	}

	if (UWorld* World = GetWorld())
	{
		if (UExplosionFxPoolSubsystem* Pool = World->GetSubsystem<UExplosionFxPoolSubsystem>())
		{
			Pool->PlayExplosion(FxLocation, 2.0f);
		}
	}

	if (!Character)
	{
		return;
	}

	// Knock along the ball's flight direction (same as incoming travel).
	FVector KnockDir = FlightDir.GetSafeNormal();
	if (KnockDir.IsNearlyZero())
	{
		KnockDir = (Character->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	}
	if (KnockDir.IsNearlyZero())
	{
		KnockDir = Character->GetActorForwardVector();
	}

	const FVector LaunchVelocity = KnockDir * EightBallKnockSpeed + FVector(0.0f, 0.0f, EightBallKnockUpSpeed);
	Character->LaunchCharacter(LaunchVelocity, true, true);

	UE_LOG(LogTemp, Log, TEXT("[TaiQiu] 8-ball knock dir=%s tex=%s"),
		*KnockDir.ToCompactString(), *ChosenTextureName.ToString());
}

void ABilliardBallActor::SwitchToPhysicsBounce()
{
	if (bHasBounced)
	{
		return;
	}

	bHasBounced = true;
	bGuidedPhase = false;
	PlayHitSound();

	const FVector Aim = GetAimPoint();
	const FVector ContactLocation = GetActorLocation();
	const FVector FlightDir = FlightDirection.GetSafeNormal();

	// Only exact 8_black may knock / explode. Non-8 must never LaunchCharacter.
	if (IsEightBallTexture())
	{
		ApplyEightBallHitEffects(ContactLocation, FlightDir);
	}

	FVector Away = GetActorLocation() - Aim;
	Away.Z = 0.0f;
	if (!Away.Normalize())
	{
		Away = (-FlightDir).GetSafeNormal2D();
		if (Away.IsNearlyZero())
		{
			Away = GetActorForwardVector().GetSafeNormal2D();
		}
		if (Away.IsNearlyZero())
		{
			Away = FVector(1.0f, 0.0f, 0.0f);
		}
	}

	// Leave the pawn volume BEFORE enabling physics — overlapping Block+Pawn causes Chaos hitch.
	const FVector BounceOrigin = GetActorLocation() + Away * 120.0f + FVector(0.0f, 0.0f, 50.0f);
	SetActorLocation(BounceOrigin, false, nullptr, ETeleportType::TeleportPhysics);

	// Keep BP visual for rendering only — never simulate its (often complex) mesh collision.
	if (AActor* Visual = VisualActor.Get())
	{
		Visual->SetActorEnableCollision(false);
		TArray<UPrimitiveComponent*> Prims;
		Visual->GetComponents<UPrimitiveComponent>(Prims);
		for (UPrimitiveComponent* Prim : Prims)
		{
			if (!Prim)
			{
				continue;
			}
			Prim->SetSimulatePhysics(false);
			Prim->SetEnableGravity(false);
			Prim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Prim->SetGenerateOverlapEvents(false);
		}
	}

	FallbackMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FallbackMesh->SetSimulatePhysics(false);

	// Bounce body: never collide with the player (prevents false "knock" on non-8 balls).
	HitProbe->SetHiddenInGame(true);
	HitProbe->SetSphereRadius(28.0f);
	HitProbe->SetCollisionObjectType(ECC_PhysicsBody);
	HitProbe->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	HitProbe->SetCollisionResponseToAllChannels(ECR_Ignore);
	HitProbe->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	HitProbe->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	HitProbe->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	HitProbe->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Ignore);
	HitProbe->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Ignore);
	HitProbe->SetGenerateOverlapEvents(false);
	HitProbe->SetNotifyRigidBodyCollision(false);

	if (AActor* Target = TargetActor.Get())
	{
		HitProbe->IgnoreActorWhenMoving(Target, true);
		if (ACharacter* Character = Cast<ACharacter>(Target))
		{
			if (UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
			{
				Capsule->IgnoreActorWhenMoving(this, true);
			}
		}
	}

	// Physics must run on the root so the whole actor (and attached visual) moves.
	if (GetRootComponent() != HitProbe)
	{
		SceneRoot->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		HitProbe->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		SetRootComponent(HitProbe);
		SceneRoot->AttachToComponent(HitProbe, FAttachmentTransformRules::KeepWorldTransform);
		if (AActor* Visual = VisualActor.Get())
		{
			Visual->AttachToComponent(HitProbe, FAttachmentTransformRules::KeepWorldTransform);
		}
		else if (FallbackMesh)
		{
			FallbackMesh->AttachToComponent(HitProbe, FAttachmentTransformRules::KeepWorldTransform);
		}
	}

	HitProbe->SetSimulatePhysics(true);
	HitProbe->SetEnableGravity(true);
	HitProbe->WakeRigidBody();

	const FVector BounceVelocity = Away * BounceSpeed + FVector(0.0f, 0.0f, BounceUpSpeed);
	HitProbe->SetPhysicsLinearVelocity(BounceVelocity);
	HitProbe->SetPhysicsAngularVelocityInDegrees(
		FVector(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f)) * SpinAngularSpeed);

	UGiftRestSettleComponent::AttachAndWatch(this, HitProbe, 3.0f, 55.0f);

	SetActorTickEnabled(false);
	SetLifeSpan(0.0f);
}
