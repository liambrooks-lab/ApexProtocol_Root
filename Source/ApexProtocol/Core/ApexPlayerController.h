#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ApexPlayerController.generated.h"

// ============================================================
// AApexPlayerController — Manages input modes and checkpoint recovery
//
// On biological collapse, initiates a camera fade → checkpoint
// restore → camera fade-in sequence for seamless diegetic recovery.
// ============================================================
UCLASS()
class APEXPROTOCOL_API AApexPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AApexPlayerController();

	/** Triggers the full checkpoint recovery sequence with camera fade. */
	UFUNCTION(BlueprintCallable, Category = "Recovery")
	void TriggerCheckpointRecovery();

	UFUNCTION(BlueprintImplementableEvent, Category = "Recovery")
	void OnCheckpointRecoveryStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Recovery")
	void OnCheckpointRecoveryCompleted();

protected:
	virtual void BeginPlay() override;

	/** Duration of the fade-to-black on biological collapse [seconds]. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recovery")
	float CollapseFadeDuration;

	/** Duration held at black before restoring [seconds]. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recovery")
	float CollapseHoldDuration;

private:
	/** Called after the fade+hold completes — performs the actual restore. */
	void ExecuteCheckpointRestore();

	FTimerHandle RecoveryTimerHandle;
};
