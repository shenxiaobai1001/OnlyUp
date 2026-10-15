#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "PoopEjectComponent.generated.h"

class UAnimMontage;
class UAnimSequence;
class USkeletalMeshComponent;
class UUserWidget;
class UTextBlock;
class UInputComponent;

/**
 * 拉大便会话：待点击空格计数 UI + 空格触发蹲姿/生成，
 * 在 shit 关键帧给玩家一次向上冲量。
 */
UCLASS(ClassGroup = (Gift), meta = (BlueprintSpawnableComponent))
class ONLYUP_API UPoopEjectComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPoopEjectComponent();

	void AddGiftCharges(int32 GiftCount = 1);

protected:
	virtual void BeginDestroy() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void OnSpacePressed();
	void EnsureCountWidget();
	void CloseCountWidget();
	void EnsureSpaceBinding();
	void ClearSpaceBinding();
	void RefreshCountUI();

	void PlayOneEject();
	void FinishOneEject();
	float FindShitNotifyTime(const UAnimMontage* Montage) const;
	void SpawnEjectedPoop();
	void ApplyUpBoost(ACharacter* Character);
	void LockAlsForAnim(ACharacter* Character);
	void UnlockAls(ACharacter* Character);
	void BeginSingleNodePose(USkeletalMeshComponent* Mesh);
	void RestoreAnimBlueprint(USkeletalMeshComponent* Mesh);
	void EndSession();

	static constexpr int32 ClicksPerGift = 10;

	int32 PendingClicks = 0;
	int32 CompletedClicks = 0;

	bool bPlayingOne = false;
	bool bEjected = false;
	bool bAlsLocked = false;
	bool bSwitchedToSingleNode = false;
	bool bSpaceBound = false;

	float Elapsed = 0.0f;
	float AnimDuration = 0.0f;
	float ShitNotifyTime = -1.0f;
	float SavedGravityScale = 1.0f;
	float SavedRootMotionScale = 1.0f;

	/** shit 关键帧向上冲量 (cm/s)。 */
	float UpBoostSpeed = 1100.0f;

	EVisibilityBasedAnimTickOption SavedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	TObjectPtr<UAnimMontage> DanceMontage = nullptr;
	TObjectPtr<UAnimSequence> DanceSequence = nullptr;
	TWeakObjectPtr<USkeletalMeshComponent> BoundMesh;

	UPROPERTY()
	TObjectPtr<UUserWidget> CountWidget = nullptr;

	UPROPERTY()
	TObjectPtr<UTextBlock> TxAll = nullptr;

	UPROPERTY()
	TObjectPtr<UTextBlock> TxCount = nullptr;

	UPROPERTY()
	TObjectPtr<UInputComponent> SessionInputComponent = nullptr;
};
