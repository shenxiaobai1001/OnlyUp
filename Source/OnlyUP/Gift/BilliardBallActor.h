#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BilliardBallActor.generated.h"

class USphereComponent;
class USoundBase;
class UTexture2D;

UCLASS()
class ONLYUP_API ABilliardBallActor : public AActor
{
	GENERATED_BODY()

public:
	ABilliardBallActor();

	void LaunchAtTarget(AActor* InTarget);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void EnsureVisual();
	void ApplyRandomTexture();
	void ApplyGuidedMotion(float DeltaSeconds);
	void SwitchToPhysicsBounce();
	void PlayHitSound();
	void ApplyEightBallHitEffects(const FVector& ContactLocation, const FVector& FlightDir);
	bool IsEightBallTexture() const;
	FVector GetAimPoint() const;
	bool IsCloseEnoughToHit() const;
	UPrimitiveComponent* FindVisualPrimitive() const;
	UStaticMeshComponent* FindBallModelMesh() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TaiQiu")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TaiQiu")
	TObjectPtr<USphereComponent> HitProbe;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TaiQiu")
	TObjectPtr<UStaticMeshComponent> FallbackMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaiQiu|Motion")
	float GuidedSpeed = 960.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaiQiu|Motion")
	float HitAcceptDistance = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaiQiu|Physics")
	float BounceSpeed = 233.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaiQiu|Physics")
	float BounceUpSpeed = 127.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaiQiu|Physics")
	float SpinAngularSpeed = 360.0f;

	/** Horizontal knockback speed when 8-ball hits (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaiQiu|EightBall")
	float EightBallKnockSpeed = 1400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaiQiu|EightBall")
	float EightBallKnockUpSpeed = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaiQiu|Visual")
	float VisualScale = 1.0f;

private:
	TWeakObjectPtr<AActor> TargetActor;
	TWeakObjectPtr<AActor> VisualActor;
	TObjectPtr<USoundBase> HitSound = nullptr;
	TObjectPtr<UTexture2D> ChosenTexture = nullptr;
	FName ChosenTextureName = NAME_None;

	FVector FlightDirection = FVector::ForwardVector;
	bool bIsEightBall = false;
	bool bGuidedPhase = true;
	bool bHasBounced = false;
};
