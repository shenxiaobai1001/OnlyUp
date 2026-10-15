#include "IceArrowSlowComponent.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(IceArrowSlowComponent)

UIceArrowSlowComponent::UIceArrowSlowComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	// After CMC has set MaxWalkSpeed / Velocity for this frame — clamp horizontal speed.
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UIceArrowSlowComponent::BeginPlay()
{
	Super::BeginPlay();
	SetComponentTickEnabled(true);
}

bool UIceArrowSlowComponent::IsSlowActive() const
{
	const UWorld* World = GetWorld();
	return World && World->GetTimeSeconds() < SlowEndTime;
}

void UIceArrowSlowComponent::RefreshSlow(float DurationSeconds)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	SlowEndTime = World->GetTimeSeconds() + FMath::Max(DurationSeconds, 0.0f);
	SetComponentTickEnabled(true);
}

void UIceArrowSlowComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!IsSlowActive())
	{
		return;
	}

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character)
	{
		return;
	}

	UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	if (!Movement)
	{
		return;
	}

	// ALS refreshes MaxWalkSpeed every move update; clamp resulting velocity to half of current max.
	const float Cap = Movement->GetMaxSpeed() * SpeedMultiplier;
	const FVector Velocity = Movement->Velocity;
	const float Speed2D = Velocity.Size2D();
	if (Speed2D > Cap && Speed2D > KINDA_SMALL_NUMBER)
	{
		const FVector Dir = Velocity.GetSafeNormal2D();
		Movement->Velocity = FVector(Dir.X * Cap, Dir.Y * Cap, Velocity.Z);
	}
}
