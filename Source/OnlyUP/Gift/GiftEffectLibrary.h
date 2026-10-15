#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Video/VideoTypes.h"
#include "GiftEffectLibrary.generated.h"

class UMediaSource;

UCLASS()
class ONLYUP_API UGiftEffectLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Gift", meta = (WorldContext = "WorldContextObject"))
	static void SpawnFryingPans(const UObject* WorldContextObject, AActor* Target, int32 Count = 1);

	UFUNCTION(BlueprintCallable, Category = "Gift", meta = (WorldContext = "WorldContextObject"))
	static void SpawnBilliardBalls(const UObject* WorldContextObject, AActor* Target, int32 Count = 1);

	UFUNCTION(BlueprintCallable, Category = "Gift", meta = (WorldContext = "WorldContextObject"))
	static void SpawnThrowPoops(const UObject* WorldContextObject, AActor* Target, int32 Count = 1);

	UFUNCTION(BlueprintCallable, Category = "Gift", meta = (WorldContext = "WorldContextObject"))
	static void SpawnPoopEject(const UObject* WorldContextObject, AActor* Target, int32 Count = 1);

	UFUNCTION(BlueprintCallable, Category = "Gift", meta = (WorldContext = "WorldContextObject"))
	static void SpawnFireArrows(const UObject* WorldContextObject, AActor* Target, int32 Count = 1);

	UFUNCTION(BlueprintCallable, Category = "Gift", meta = (WorldContext = "WorldContextObject"))
	static void SpawnIceArrows(const UObject* WorldContextObject, AActor* Target, int32 Count = 1);

	/**
	 * 跳绳弹幕：在玩家处生成 BP_TSheng + 计数 UI。
	 * Count = 本次增加的待跳次数（默认 5）。重复调用只加次数，不重复生成。
	 */
	UFUNCTION(BlueprintCallable, Category = "Gift", meta = (WorldContext = "WorldContextObject"))
	static void SpawnJumpRope(const UObject* WorldContextObject, AActor* Target, int32 Count = 5);

	/** 播放绿幕/屏幕视频（完整请求）。返回句柄，可交给 VideoManager 停止。 */
	UFUNCTION(BlueprintCallable, Category = "Gift|Video", meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "Request"))
	static FVideoHandle PlayGreenVideo(const UObject* WorldContextObject, const FVideoPlayRequest& Request);

	/** 用 MediaSource 资源快速播屏幕绿幕视频。 */
	UFUNCTION(BlueprintCallable, Category = "Gift|Video", meta = (WorldContext = "WorldContextObject"))
	static FVideoHandle PlayGreenVideoFromSource(
		const UObject* WorldContextObject,
		UMediaSource* MediaSource,
		bool bChromaKey = true,
		FName Channel = TEXT("Default"));

	/** 本地文件路径（绝对或相对 Content）→ FileMediaSource 后播放。 */
	UFUNCTION(BlueprintCallable, Category = "Gift|Video", meta = (WorldContext = "WorldContextObject"))
	static FVideoHandle PlayGreenVideoFromFile(
		const UObject* WorldContextObject,
		const FString& FilePath,
		bool bChromaKey = true,
		FName Channel = TEXT("Default"));

	/** http(s) / 流地址 → StreamMediaSource 后播放。 */
	UFUNCTION(BlueprintCallable, Category = "Gift|Video", meta = (WorldContext = "WorldContextObject"))
	static FVideoHandle PlayGreenVideoFromUrl(
		const UObject* WorldContextObject,
		const FString& Url,
		bool bChromaKey = true,
		FName Channel = TEXT("Default"));

	UFUNCTION(BlueprintCallable, Category = "Gift", meta = (WorldContext = "WorldContextObject"))
	static void ExecuteGiftById(const UObject* WorldContextObject, AActor* Target, FName GiftId, int32 Count = 1);
};
