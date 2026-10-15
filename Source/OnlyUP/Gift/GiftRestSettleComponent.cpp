#include "GiftRestSettleComponent.h"

#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GiftRestSettleComponent)

UGiftRestSettleComponent::UGiftRestSettleComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

UGiftRestSettleComponent* UGiftRestSettleComponent::AttachAndWatch(
	AActor* Owner,
	UPrimitiveComponent* Prim,
	float InMaxRollSeconds,
	float InRestSpeed)
{
	if (!Owner || !Prim)
	{
		return nullptr;
	}

	UGiftRestSettleComponent* Comp = NewObject<UGiftRestSettleComponent>(Owner, TEXT("GiftRestSettle"));
	Comp->RegisterComponent();
	Comp->StartWatching(Prim, InMaxRollSeconds, InRestSpeed);
	return Comp;
}

void UGiftRestSettleComponent::StartWatching(UPrimitiveComponent* Prim, float InMaxRollSeconds, float InRestSpeed)
{
	WatchedPrim = Prim;
	MaxRollSeconds = FMath::Max(0.5f, InMaxRollSeconds);
	RestSpeedThreshold = FMath::Max(1.0f, InRestSpeed);
	Elapsed = 0.0f;
	RestAccum = 0.0f;
	bSettled = false;
	SetComponentTickEnabled(true);
}

void UGiftRestSettleComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bSettled)
	{
		return;
	}

	UPrimitiveComponent* Prim = WatchedPrim.Get();
	if (!IsValid(Prim))
	{
		SetComponentTickEnabled(false);
		return;
	}

	Elapsed += DeltaTime;

	const FVector LinVel = Prim->IsSimulatingPhysics() ? Prim->GetPhysicsLinearVelocity() : FVector::ZeroVector;
	const float Speed = LinVel.Size();
	const bool bNearlyResting = Speed <= RestSpeedThreshold && FMath::Abs(LinVel.Z) <= RestSpeedThreshold;

	if (Elapsed >= MinAirSeconds && bNearlyResting)
	{
		RestAccum += DeltaTime;
	}
	else
	{
		RestAccum = 0.0f;
	}

	if (RestAccum >= RestHoldSeconds || Elapsed >= MaxRollSeconds)
	{
		FreezeAsFixedObstacle();
	}
}

void UGiftRestSettleComponent::FreezeAsFixedObstacle()
{
	if (bSettled)
	{
		return;
	}
	bSettled = true;
	SetComponentTickEnabled(false);

	UPrimitiveComponent* Prim = WatchedPrim.Get();
	if (!IsValid(Prim))
	{
		return;
	}

	// Kill residual motion, then leave physics entirely.
	if (Prim->IsSimulatingPhysics())
	{
		Prim->SetPhysicsLinearVelocity(FVector::ZeroVector);
		Prim->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	}
	Prim->SetSimulatePhysics(false);
	Prim->SetEnableGravity(false);
	Prim->PutRigidBodyToSleep();

	Prim->SetNotifyRigidBodyCollision(false);
	Prim->SetGenerateOverlapEvents(false);
	Prim->SetCollisionObjectType(ECC_WorldDynamic);
	Prim->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Prim->SetCollisionResponseToAllChannels(ECR_Block);
	Prim->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Prim->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Prim->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);

	if (AActor* Owner = GetOwner())
	{
		Owner->SetLifeSpan(0.0f);
		Owner->SetActorEnableCollision(true);

		// Ensure sibling primitives on the same actor also become solid (visual meshes etc.).
		TArray<UPrimitiveComponent*> Prims;
		Owner->GetComponents<UPrimitiveComponent>(Prims);
		for (UPrimitiveComponent* Other : Prims)
		{
			if (!Other || Other == Prim)
			{
				continue;
			}
			Other->SetSimulatePhysics(false);
			Other->SetEnableGravity(false);
			if (Other->GetCollisionEnabled() == ECollisionEnabled::NoCollision)
			{
				Other->SetCollisionObjectType(ECC_WorldDynamic);
				Other->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				Other->SetCollisionResponseToAllChannels(ECR_Block);
				Other->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
			}
		}

		UE_LOG(LogTemp, Log, TEXT("[GiftSettle] Frozen %s as fixed obstacle"), *GetNameSafe(Owner));
	}
}
