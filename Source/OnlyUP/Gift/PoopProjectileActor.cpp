#include "PoopProjectileActor.h"

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

#include UE_INLINE_GENERATED_CPP_BY_NAME(PoopProjectileActor)

APoopProjectileActor::APoopProjectileActor()
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

	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshFinder(TEXT("/Game/ModAsset/dabian/bainbain.bainbain"));
	if (MeshFinder.Succeeded())
	{
		FallbackMesh->SetStaticMesh(MeshFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> SoundFinder(
		TEXT("/Game/ModAsset/dabian/\u5927\u4fbf\u58f0\u97f3.\u5927\u4fbf\u58f0\u97f3"));
	if (SoundFinder.Succeeded())
	{
		HitSound = SoundFinder.Object;
	}
}

void APoopProjectileActor::BeginPlay()
{
	Super::BeginPlay();
	EnsureVisual();
}

void APoopProjectileActor::EnsureVisual()
{
	if (VisualActor.IsValid())
	{
		return;
	}

	UClass* PoopClass = StaticLoadClass(
		AActor::StaticClass(),
		nullptr,
		TEXT("/Game/ModAsset/dabian/BP_BBian.BP_BBian_C"));

	if (PoopClass)
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		if (AActor* Spawned = GetWorld()->SpawnActor<AActor>(PoopClass, GetActorTransform(), Params))
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

void APoopProjectileActor::LaunchAtTarget(AActor* InTarget)
{
	TargetActor = InTarget;
	bEjectMode = false;
	bGuidedPhase = true;
	bHasBounced = false;
	EnsureVisual();
	SetActorTickEnabled(true);
}

void APoopProjectileActor::LaunchWithVelocity(const FVector& WorldVelocity)
{
	bEjectMode = true;
	bGuidedPhase = false;
	bHasBounced = false;
	EnsureVisual();
	SetActorTickEnabled(false);

	const FVector Angular(
		FMath::FRandRange(-1.f, 1.f),
		FMath::FRandRange(-1.f, 1.f),
		FMath::FRandRange(-1.f, 1.f));
	BeginPhysicsFlight(WorldVelocity, Angular * SpinAngularSpeed);
}

FVector APoopProjectileActor::GetAimPoint() const
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

bool APoopProjectileActor::IsCloseEnoughToHit() const
{
	AActor* Target = TargetActor.Get();
	if (!Target)
	{
		return false;
	}

	const FVector Aim = GetAimPoint();
	if (FVector::Dist(GetActorLocation(), Aim) > HitAcceptDistance)
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

	return FVector::Dist(GetActorLocation(), Target->GetActorLocation()) <= (BodyRadius + HitAcceptDistance);
}

UPrimitiveComponent* APoopProjectileActor::FindVisualPrimitive() const
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

void APoopProjectileActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bGuidedPhase)
	{
		ApplyGuidedMotion(DeltaSeconds);
	}
}

void APoopProjectileActor::ApplyGuidedMotion(float DeltaSeconds)
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

void APoopProjectileActor::PlayHitSound()
{
	USoundBase* Sound = HitSound;
	if (!Sound)
	{
		Sound = LoadObject<USoundBase>(nullptr, TEXT("/Game/ModAsset/dabian/\u5927\u4fbf\u58f0\u97f3.\u5927\u4fbf\u58f0\u97f3"));
	}
	if (Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation());
	}
}

void APoopProjectileActor::BeginPhysicsFlight(const FVector& LinearVelocity, const FVector& AngularVelocity)
{
	PlayHitSound();

	AActor* Visual = VisualActor.Get();
	UPrimitiveComponent* Prim = FindVisualPrimitive();

	if (Visual)
	{
		Visual->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		Visual->SetOwner(nullptr);
		Visual->SetActorEnableCollision(true);
		Visual->SetLifeSpan(0.0f);
	}

	auto ConfigurePhysicsBody = [&](UPrimitiveComponent* Body)
	{
		if (!Body)
		{
			return;
		}
		Body->SetNotifyRigidBodyCollision(true);
		Body->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Body->SetCollisionObjectType(ECC_PhysicsBody);
		Body->SetCollisionResponseToAllChannels(ECR_Block);
		Body->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		Body->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		Body->SetSimulatePhysics(true);
		Body->SetEnableGravity(true);
		Body->WakeRigidBody();
		Body->SetPhysicsLinearVelocity(LinearVelocity);
		Body->SetPhysicsAngularVelocityInDegrees(AngularVelocity);
	};

	if (Prim)
	{
		ConfigurePhysicsBody(Prim);
		AActor* SettleOwner = Visual ? Visual : this;
		UGiftRestSettleComponent::AttachAndWatch(SettleOwner, Prim, MaxRollSeconds, 55.0f);
	}
	else if (FallbackMesh)
	{
		FallbackMesh->SetVisibility(true);
		ConfigurePhysicsBody(FallbackMesh);
		UGiftRestSettleComponent::AttachAndWatch(this, FallbackMesh, MaxRollSeconds, 55.0f);
	}

	// Host motor can die; the visual / settle owner persists.
	if (Visual)
	{
		SetLifeSpan(0.5f);
	}
	else
	{
		SetLifeSpan(0.0f);
	}

	bHasBounced = true;
	bGuidedPhase = false;
	SetActorTickEnabled(false);
}

void APoopProjectileActor::SwitchToPhysicsBounce()
{
	if (bHasBounced)
	{
		return;
	}

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

	const FVector BounceVelocity = Away * BounceSpeed + FVector(0.0f, 0.0f, BounceUpSpeed);
	const FVector Angular(
		FMath::FRandRange(-1.f, 1.f),
		FMath::FRandRange(-1.f, 1.f),
		FMath::FRandRange(-1.f, 1.f));
	BeginPhysicsFlight(BounceVelocity, Angular * SpinAngularSpeed);
}
