#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "ApexEndgameDirector.generated.h"

class AApexGameMode;
class AOmniKernel_OS;
class AA_BioAsset;
class AAudioDirector;

// ============================================================
// AApexEndgameDirector — Post-Protocol Cinematic Coordinator
//
// Listens to GameMode and OmniKernel progression events.
// On ApexProtocol completion:
//   1. Shuts down OmniKernel (if not already)
//   2. Deactivates all BioAssets
//   3. Fires Blueprint events for cinematic cutscene triggers
// ============================================================
UCLASS()
class APEXPROTOCOL_API AApexEndgameDirector : public AInfo
{
	GENERATED_BODY()

public:
	AApexEndgameDirector();

protected:
	virtual void BeginPlay() override;

	// ─── Delegate Handlers ─────────────────────────────────────────────
	UFUNCTION()
	void HandleCoreBreachStarted();

	UFUNCTION()
	void HandleEscapeVectorActivated();

	UFUNCTION()
	void HandleApexProtocolCompleted();

	UFUNCTION()
	void HandleFinalPurgeChanged(bool bPurgeActive);

	UFUNCTION()
	void HandleCoreBreachPathUnlocked();

	UFUNCTION()
	void HandleOmniKernelShutdownChanged(bool bIsShutdown);

	// ─── Blueprint Events ──────────────────────────────────────────────
	UFUNCTION(BlueprintImplementableEvent, Category = "Endgame")
	void OnCoreBreachSequenceStarted_BP();

	UFUNCTION(BlueprintImplementableEvent, Category = "Endgame")
	void OnEscapeVectorActivated_BP();

	UFUNCTION(BlueprintImplementableEvent, Category = "Endgame")
	void OnApexProtocolCompleted_BP();

	UFUNCTION(BlueprintImplementableEvent, Category = "Endgame")
	void OnFinalPurgeStateChanged_BP(bool bPurgeActive);

	UFUNCTION(BlueprintImplementableEvent, Category = "Endgame")
	void OnCoreBreachPathUnlocked_BP();

	UFUNCTION(BlueprintImplementableEvent, Category = "Endgame")
	void OnOmniKernelShutdown_BP(bool bIsShutdown);

	/** Blueprint event for endgame cinematic sequence start. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Endgame")
	void OnEndgameCinematicTriggered();

	UPROPERTY(Transient)
	TObjectPtr<AApexGameMode> CachedGameMode;

	UPROPERTY(Transient)
	TObjectPtr<AOmniKernel_OS> CachedOmniKernel;

private:
	/** Deactivates all BioAssets in the world — called on protocol completion. */
	void DeactivateAllBioAssets();
};
