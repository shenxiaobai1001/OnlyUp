#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PoopProjectileActor.generated.h"

class USphereComponent;
class USoundBase;

UCLASS()
class ONLYUP_API APoopProjectileActor : public AActor
{
	GENERATED_BODY()

public:
	APoopProjectileActor();

	/** Homing throw toward target (same pattern as frying pan). */
	void LaunchAtTarget(AActor* InTarget);

	/** Launch with an initial world-space velocity (拉大便 eject). */
	void LaunchWithVelocity(const FVector& WorldVelocity);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void EnsureVisual();
	void ApplyGuidedMotion(float DeltaSeconds);
	void SwitchToPhysicsBounce();
	void BeginPhysicsFlight(const FVector& LinearVelocity, const FVector& AngularVelocity);
	void PlayHitSound();
	FVector GetAimPoint() const;
	bool IsCloseEnoughToHit() const;
	UPrimitiveComponent* FindVisualPrimitive() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Poop")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Poop")
	TObjectPtr<USphereComponent> HitProbe;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Poop")
	TObjectPtr<UStaticMeshComponent> FallbackMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Poop|Motion")
	float GuidedSpeed = 960.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Poop|Motion")
	float HitAcceptDistance = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Poop|Physics")
	float BounceSpeed = 233.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Poop|Physics")
	float BounceUpSpeed = 127.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Poop|Physics")
	float SpinAngularSpeed = 360.0f;

	/** Max seconds of rolling before forced settle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Poop|Physics")
	float MaxRollSeconds = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Poop|Visual")
	float VisualScale = 1.0f;

private:
	TWeakObjectPtr<AActor> TargetActor;
	TWeakObjectPtr<AActor> VisualActor;
	TObjectPtr<USoundBase> HitSound = nullptr;

	bool bGuidedPhase = false;
	bool bHasBounced = false;
	bool bEjectMode = false;
};
