#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "JumpRopeComponent.generated.h"

class UAnimMontage;
class UAnimSequence;
class USkeletalMeshComponent;
class UUserWidget;
class UTextBlock;
class UInputComponent;
class USoundBase;

/**
 * 跳绳弹幕会话：在玩家处生成 BP_TSheng + 计数 UI，
 * 空格同步跳跃与绳子 CheckJump；剩余次数归零结束。
 * 玩家 jump/down 用 SingleNode 播完整段（避免 ALS Montage 被掐断）。
 */
UCLASS(ClassGroup = (Gift), meta = (BlueprintSpawnableComponent))
class ONLYUP_API UJumpRopeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UJumpRopeComponent();

	/** 增加待跳次数并启动/续上会话。默认每次礼物 +5。 */
	void AddJumpCharges(int32 Count = 5);

protected:
	virtual void BeginDestroy() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void EnsureSession();
	void EndSession();

	void SpawnRopeActor();
	void CacheRopeMesh();
	void EnsureRopeLooping();
	void SyncRopeForJump();

	void EnsureCountWidget();
	void CloseCountWidget();
	void RefreshCountUI(bool bPulseAll, bool bPulseSucc, bool bPulseFail);
	void PulseText(UTextBlock* TextBlock);
	void TickTextPulses(float DeltaTime);

	void EnsureSpaceBinding();
	void ClearSpaceBinding();
	void OnSpacePressed();

	void LockPlayerMovement();
	void UnlockPlayerMovement();
	void SnapPlayerToRope();

	void BeginPlayerPose(UAnimSequence* Sequence, bool bIsDown);
	void RestorePlayerAnimBlueprint();
	void TickPlayerPose(float DeltaTime);
	bool IsPlayerJumpPlaying() const;
	bool IsPlayerDownPlaying() const;

	void HandleCheckJump();
	void OnSuccess();
	void OnFail();
	void RequestEndAfterRopeCycle();
	void ClearCycleBusy();
	void TryUnlockCycle();

	float FindCheckJumpTime(const UAnimSequence* Sequence) const;
	void PlaySfx(USoundBase* Sound) const;
	void LoadAssets();
	FVector GetPlayerFootLocation(const ACharacter* Character) const;

	USkeletalMeshComponent* FindNamedMesh(AActor* Actor, FName Name) const;

	static constexpr float TextPulseScale = 1.2f;
	static constexpr float TextPulseDuration = 0.18f;
	static constexpr float RopeYawOffsetDeg = -90.0f;
	static constexpr float RopeExtraDownCm = 20.0f;

	int32 RemainingJumps = 0;
	int32 SuccessCount = 0;
	int32 FailCount = 0;

	bool bSessionActive = false;
	bool bMovementLocked = false;
	bool bSpaceBound = false;
	bool bArmedForSuccess = false;
	bool bCheckJumpHandledThisCycle = false;
	bool bRopeSoundPlayedThisCycle = false;
	bool bPlayingJump = false;
	bool bPlayingDown = false;
	bool bPlayerPoseActive = false;
	/** 本轮空格起跳未结束：禁止再按打断。 */
	bool bCycleBusy = false;
	/** 本轮需等绳子回绕后再解锁。 */
	bool bCycleAwaitingWrap = false;
	bool bCycleWrapped = false;
	bool bPendingEndSession = false;

	float CheckJumpTime = -1.0f;
	float RopeAnimLength = 0.0f;
	float LastRopeTime = 0.0f;
	float EndSessionDelay = 0.0f;
	float PlayerPoseElapsed = 0.0f;
	float PlayerPoseDuration = 0.0f;
	FRotator SavedPlayerRotation = FRotator::ZeroRotator;

	float PulseAllTime = 0.0f;
	float PulseSuccTime = 0.0f;
	float PulseFailTime = 0.0f;

	float SavedGravityScale = 1.0f;
	float SavedRootMotionScale = 1.0f;
	uint8 SavedJumpMaxCount = 1;
	EVisibilityBasedAnimTickOption SavedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	UPROPERTY()
	TObjectPtr<AActor> RopeActor = nullptr;

	TWeakObjectPtr<USkeletalMeshComponent> RopeMesh;
	TWeakObjectPtr<USkeletalMeshComponent> PlayerMesh;

	UPROPERTY()
	TObjectPtr<UAnimSequence> RopeSequence = nullptr;

	UPROPERTY()
	TObjectPtr<UAnimSequence> JumpSequence = nullptr;

	UPROPERTY()
	TObjectPtr<UAnimSequence> DownSequence = nullptr;

	UPROPERTY()
	TObjectPtr<USoundBase> SfxLoop = nullptr;

	UPROPERTY()
	TObjectPtr<USoundBase> SfxSuccess = nullptr;

	UPROPERTY()
	TObjectPtr<USoundBase> SfxFail = nullptr;

	UPROPERTY()
	TObjectPtr<UUserWidget> CountWidget = nullptr;

	UPROPERTY()
	TObjectPtr<UTextBlock> TxAll = nullptr;

	UPROPERTY()
	TObjectPtr<UTextBlock> TxSucc = nullptr;

	UPROPERTY()
	TObjectPtr<UTextBlock> TxFail = nullptr;

	UPROPERTY()
	TObjectPtr<UInputComponent> SessionInputComponent = nullptr;
};
