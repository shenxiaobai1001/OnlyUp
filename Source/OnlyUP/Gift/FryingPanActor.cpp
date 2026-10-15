#include "FryingPanActor.h"

#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GiftRestSettleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FryingPanActor)

AFryingPanActor::AFryingPanActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	// Probe kept only as a tiny helper; hit is decided by distance to the capsule, not loose overlaps.
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

	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshFinder(TEXT("/Game/ModAsset/pdg-output/pdg.pdg"));
	if (MeshFinder.Succeeded())
	{
		FallbackMesh->SetStaticMesh(MeshFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> SoundFinder(
		TEXT("/Game/ModAsset/pdg-output/\u5e73\u5e95\u9505.\u5e73\u5e95\u9505"));
	if (SoundFinder.Succeeded())
	{
		HitSound = SoundFinder.Object;
	}
}

void AFryingPanActor::BeginPlay()
{
	Super::BeginPlay();
	EnsureVisual();
}

void AFryingPanActor::EnsureVisual()
{
	if (VisualActor.IsValid())
	{
		return;
	}

	UClass* PingdiguoClass = StaticLoadClass(
		AActor::StaticClass(),
		nullptr,
		TEXT("/Game/ModAsset/pdg-output/Pingdiguo.Pingdiguo_C"));

	if (PingdiguoClass)
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		if (AActor* Spawned = GetWorld()->SpawnActor<AActor>(PingdiguoClass, GetActorTransform(), Params))
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

void AFryingPanActor::LaunchAtTarget(AActor* InTarget)
{
	TargetActor = InTarget;
	bGuidedPhase = true;
	bHasBounced = false;
	EnsureVisual();
	SetActorTickEnabled(true);
}

FVector AFryingPanActor::GetAimPoint() const
{
	AActor* Target = TargetActor.Get();
	if (!Target)
	{
		return GetActorLocation();
	}

	// Use capsule / actor location — NOT GetActorBounds (ALS bounds can be huge and cause early "hits").
	if (const ACharacter* Character = Cast<ACharacter>(Target))
	{
		if (const UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
		{
			const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
			// Aim roughly at upper chest / neck.
			return Character->GetActorLocation() + FVector(0.0f, 0.0f, HalfHeight * 0.35f);
		}
	}

	return Target->GetActorLocation() + FVector(0.0f, 0.0f, 70.0f);
}

bool AFryingPanActor::IsCloseEnoughToHit() const
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

	// Extra safety: must also be near the pawn itself (not some wrong aim outlier).
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

UPrimitiveComponent* AFryingPanActor::FindVisualPrimitive() const
{
	if (AActor* Visual = VisualActor.Get())
	{
		TArray<UStaticMeshComponent*> Meshes;
		Visual->GetComponents<UStaticMeshComponent>(Meshes);
		for (UStaticMeshComponent* MeshComp : Meshes)
		{
			if (MeshComp && MeshComp->GetStaticMesh())
			{
				return MeshComp;
			}
		}

		TArray<UPrimitiveComponent*> Prims;
		Visual->GetComponents<UPrimitiveComponent>(Prims);
		for (UPrimitiveComponent* Prim : Prims)
		{
			if (Prim)
			{
				return Prim;
			}
		}
	}
	return FallbackMesh;
}

void AFryingPanActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bGuidedPhase)
	{
		ApplyGuidedMotion(DeltaSeconds);
	}
}

void AFryingPanActor::ApplyGuidedMotion(float DeltaSeconds)
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
	const float Step = FMath::Min(GuidedSpeed * DeltaSeconds, Distance);
	SetActorLocation(Current + ToTarget * Step, false, nullptr, ETeleportType::None);

	const FRotator FaceRot = ToTarget.Rotation();
	SetActorRotation(FRotator(FaceRot.Pitch - 90.0f, FaceRot.Yaw, GetWorld()->GetTimeSeconds() * 180.0f));
}

void AFryingPanActor::PlayHitSound()
{
	USoundBase* Sound = HitSound;
	if (!Sound)
	{
		Sound = LoadObject<USoundBase>(nullptr, TEXT("/Game/ModAsset/pdg-output/\u5e73\u5e95\u9505.\u5e73\u5e95\u9505"));
	}
	if (Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation());
	}
}

void AFryingPanActor::SwitchToPhysicsBounce()
{
	if (bHasBounced)
	{
		return;
	}

	bHasBounced = true;
	bGuidedPhase = false;
	PlayHitSound();

	const FVector Aim = GetAimPoint();
	FVector Away = GetActorLocation() - Aim;
	Away.Z = 0.0f;
	if (!Away.Normalize())
	{
		Away = GetActorForwardVector().GetSafeNormal2D();
		if (Away.IsNearlyZero())
		{
			Away = FVector(1.0f, 0.0f, 0.0f);
		}
	}

	AActor* Visual = VisualActor.Get();
	UPrimitiveComponent* Prim = FindVisualPrimitive();

	if (Visual)
	{
		Visual->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		Visual->SetOwner(nullptr);
		Visual->SetActorEnableCollision(true);
		Visual->SetLifeSpan(0.0f);
	}

	const FVector BounceVelocity = Away * BounceSpeed + FVector(0.0f, 0.0f, BounceUpSpeed);
	const FVector Angular(
		FMath::FRandRange(-1.f, 1.f),
		FMath::FRandRange(-1.f, 1.f),
		FMath::FRandRange(-1.f, 1.f));

	if (Prim)
	{
		Prim->SetNotifyRigidBodyCollision(true);
		Prim->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Prim->SetCollisionObjectType(ECC_PhysicsBody);
		Prim->SetCollisionResponseToAllChannels(ECR_Block);
		Prim->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		Prim->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		Prim->SetSimulatePhysics(true);
		Prim->SetEnableGravity(true);
		Prim->WakeRigidBody();
		Prim->SetPhysicsLinearVelocity(BounceVelocity);
		Prim->SetPhysicsAngularVelocityInDegrees(Angular * SpinAngularSpeed);

		AActor* SettleOwner = Visual ? Visual : this;
		UGiftRestSettleComponent::AttachAndWatch(SettleOwner, Prim, 3.0f, 55.0f);
	}
	else if (FallbackMesh && FallbackMesh->IsVisible())
	{
		FallbackMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		FallbackMesh->SetCollisionResponseToAllChannels(ECR_Block);
		FallbackMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		FallbackMesh->SetSimulatePhysics(true);
		FallbackMesh->SetEnableGravity(true);
		FallbackMesh->SetPhysicsLinearVelocity(BounceVelocity);
		UGiftRestSettleComponent::AttachAndWatch(this, FallbackMesh, 3.0f, 55.0f);
	}

	SetActorTickEnabled(false);
	// Motor host can go away; settled visual persists as a fixed obstacle.
	SetLifeSpan(Visual ? 0.5f : 0.0f);
}
