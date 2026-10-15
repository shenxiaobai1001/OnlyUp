#include "HomingArrowMotorComponent.h"

#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "IceArrowSlowComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(HomingArrowMotorComponent)

UHomingArrowMotorComponent::UHomingArrowMotorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UHomingArrowMotorComponent::StartHoming(AActor* InTarget, bool bInIceArrow)
{
	TargetActor = InTarget;
	bIceArrow = bInIceArrow;
	bGuided = true;
	bStuck = false;

	if (AActor* Owner = GetOwner())
	{
		Owner->SetActorEnableCollision(false);
		TArray<UPrimitiveComponent*> Prims;
		Owner->GetComponents<UPrimitiveComponent>(Prims);
		for (UPrimitiveComponent* Prim : Prims)
		{
			Prim->SetSimulatePhysics(false);
			Prim->SetEnableGravity(false);
			Prim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}

	if (bIceArrow && !IceBuffAreaSystem)
	{
		IceBuffAreaSystem = LoadObject<UNiagaraSystem>(
			nullptr, TEXT("/Game/ModAsset/bingboom/PS/FX_BuffArea_Ice.FX_BuffArea_Ice"));
	}

	SetComponentTickEnabled(true);
}

FVector UHomingArrowMotorComponent::GetAimPoint() const
{
	AActor* Target = TargetActor.Get();
	if (!Target)
	{
		return GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
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

bool UHomingArrowMotorComponent::IsCloseEnoughToHit() const
{
	AActor* Owner = GetOwner();
	AActor* Target = TargetActor.Get();
	if (!Owner || !Target)
	{
		return false;
	}

	const float DistToAim = FVector::Dist(Owner->GetActorLocation(), GetAimPoint());
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

	return FVector::Dist(Owner->GetActorLocation(), Target->GetActorLocation()) <= (BodyRadius + HitAcceptDistance);
}

void UHomingArrowMotorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bGuided && !bStuck)
	{
		ApplyGuidedMotion(DeltaTime);
	}
}

void UHomingArrowMotorComponent::ApplyGuidedMotion(float DeltaSeconds)
{
	AActor* Owner = GetOwner();
	if (!Owner || !TargetActor.IsValid())
	{
		return;
	}

	if (IsCloseEnoughToHit())
	{
		StickToTarget();
		return;
	}

	const FVector Aim = GetAimPoint();
	const FVector Current = Owner->GetActorLocation();
	FVector ToTarget = Aim - Current;
	const float Distance = ToTarget.Size();
	if (Distance < KINDA_SMALL_NUMBER)
	{
		StickToTarget();
		return;
	}

	ToTarget /= Distance;
	const float Step = FMath::Min(GuidedSpeed * DeltaSeconds, Distance);
	Owner->SetActorLocation(Current + ToTarget * Step, false, nullptr, ETeleportType::None);

	// Point arrow tip toward travel direction (mesh forward assumed +X).
	Owner->SetActorRotation(ToTarget.Rotation());
}

void UHomingArrowMotorComponent::PlayIcePathBurst()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	TArray<UNiagaraComponent*> NiagaraComps;
	Owner->GetComponents<UNiagaraComponent>(NiagaraComps);
	for (UNiagaraComponent* Niagara : NiagaraComps)
	{
		if (!Niagara)
		{
			continue;
		}

		const FString Name = Niagara->GetName();
		if (Name.Contains(TEXT("FX_IcePathBurst"), ESearchCase::IgnoreCase))
		{
			Niagara->SetVisibility(true);
			Niagara->Activate(true);
			Niagara->ReinitializeSystem();
			Niagara->Activate(true);
		}
	}
}

void UHomingArrowMotorComponent::SpawnIceBuffArea()
{
	AActor* Target = TargetActor.Get();
	if (!Target || !IceBuffAreaSystem)
	{
		IceBuffAreaSystem = LoadObject<UNiagaraSystem>(
			nullptr, TEXT("/Game/ModAsset/bingboom/PS/FX_BuffArea_Ice.FX_BuffArea_Ice"));
	}
	if (!Target || !IceBuffAreaSystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Arrow] FX_BuffArea_Ice missing."));
		return;
	}

	USceneComponent* AttachParent = Target->GetRootComponent();
	FVector RelativeFeet = FVector::ZeroVector;
	if (const ACharacter* Character = Cast<ACharacter>(Target))
	{
		if (UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
		{
			AttachParent = Capsule;
			RelativeFeet = FVector(0.0f, 0.0f, -Capsule->GetScaledCapsuleHalfHeight());
		}
	}

	UNiagaraComponent* SpawnedFx = UNiagaraFunctionLibrary::SpawnSystemAttached(
		IceBuffAreaSystem,
		AttachParent,
		NAME_None,
		RelativeFeet,
		FRotator::ZeroRotator,
		EAttachLocation::KeepRelativeOffset,
		true,
		true,
		ENCPoolMethod::None,
		true);

	if (SpawnedFx && Target->GetWorld())
	{
		TWeakObjectPtr<UNiagaraComponent> WeakFx(SpawnedFx);
		FTimerHandle TimerHandle;
		Target->GetWorld()->GetTimerManager().SetTimer(
			TimerHandle,
			FTimerDelegate::CreateLambda([WeakFx]()
			{
				if (UNiagaraComponent* Fx = WeakFx.Get())
				{
					Fx->DeactivateImmediate();
					Fx->DestroyComponent();
				}
			}),
			IceSlowDuration,
			false);
	}
}

void UHomingArrowMotorComponent::HandleIceImpact()
{
	PlayIcePathBurst();
	SpawnIceBuffArea();

	AActor* Target = TargetActor.Get();
	if (!Target)
	{
		return;
	}

	UIceArrowSlowComponent* Slow = Target->FindComponentByClass<UIceArrowSlowComponent>();
	if (!Slow)
	{
		Slow = NewObject<UIceArrowSlowComponent>(Target, TEXT("IceArrowSlow"));
		Slow->RegisterComponent();
	}
	Slow->RefreshSlow(IceSlowDuration);
}

void UHomingArrowMotorComponent::StickToTarget()
{
	if (bStuck)
	{
		return;
	}

	bStuck = true;
	bGuided = false;
	SetComponentTickEnabled(false);

	AActor* Owner = GetOwner();
	AActor* Target = TargetActor.Get();
	if (!Owner || !Target)
	{
		return;
	}

	// Must attach to a torso BONE/socket so arrows follow spine animation (bend/crouch).
	// Attaching only to the mesh component root leaves them floating when the body bends.
	USceneComponent* AttachParent = Target->GetRootComponent();
	FName AttachSocket = NAME_None;

	if (ACharacter* Character = Cast<ACharacter>(Target))
	{
		if (USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			AttachParent = Mesh;

			static const FName TorsoBoneCandidates[] = {
				TEXT("spine_03"),
				TEXT("spine_02"),
				TEXT("spine_01"),
				TEXT("pelvis"),
			};

			for (const FName BoneName : TorsoBoneCandidates)
			{
				if (Mesh->DoesSocketExist(BoneName) || Mesh->GetBoneIndex(BoneName) != INDEX_NONE)
				{
					AttachSocket = BoneName;
					break;
				}
			}
		}
	}

	Owner->AttachToComponent(AttachParent, FAttachmentTransformRules::KeepWorldTransform, AttachSocket);
	Owner->SetActorEnableCollision(false);

	if (bIceArrow)
	{
		HandleIceImpact();
	}
}
