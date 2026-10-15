#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FryingPanActor.generated.h"

class USphereComponent;
class USoundBase;

UCLASS()
class ONLYUP_API AFryingPanActor : public AActor
{
	GENERATED_BODY()

public:
	AFryingPanActor();

	void LaunchAtTarget(AActor* InTarget);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void EnsureVisual();
	void ApplyGuidedMotion(float DeltaSeconds);
	void SwitchToPhysicsBounce();
	void PlayHitSound();
	FVector GetAimPoint() const;
	bool IsCloseEnoughToHit() const;
	UPrimitiveComponent* FindVisualPrimitive() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FryingPan")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FryingPan")
	TObjectPtr<USphereComponent> HitProbe;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FryingPan")
	TObjectPtr<UStaticMeshComponent> FallbackMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FryingPan|Motion")
	float GuidedSpeed = 960.0f;

	/** Must reach this distance to the aim point (chest) before bouncing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FryingPan|Motion")
	float HitAcceptDistance = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FryingPan|Physics")
	float BounceSpeed = 233.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FryingPan|Physics")
	float BounceUpSpeed = 127.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FryingPan|Physics")
	float SpinAngularSpeed = 360.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FryingPan|Visual")
	float VisualScale = 1.0f;

private:
	TWeakObjectPtr<AActor> TargetActor;
	TWeakObjectPtr<AActor> VisualActor;
	TObjectPtr<USoundBase> HitSound = nullptr;

	bool bGuidedPhase = true;
	bool bHasBounced = false;
};
