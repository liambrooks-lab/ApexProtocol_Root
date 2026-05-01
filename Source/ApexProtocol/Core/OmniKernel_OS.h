#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "OmniKernel_OS.generated.h"

class AA_BioAsset;
class AA_SectorVolume;
class AA_InteractiveNode;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOmniKernelBoolEvent, bool, bStateActive);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOmniKernelEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOmniKernelThreatEvent, float, ThreatLevel, FName, SectorID);

// ============================================================
// AOmniKernel_OS — Unified Facility Director AI
//
// Acts as the OS-level "mind" of the facility:
//   • Tracks player acoustic/thermal footprint
//   • Manages facility power grid and toxicity grid per sector
//   • Dispatches BioAssets proportionally to threat level
//   • Modulates BioAsset patrol speed via ThreatLevel
//   • Executes deferred sector lockdowns via timer handles
//   • Decays global threat over time when no acoustic events occur
// ============================================================
UCLASS()
class APEXPROTOCOL_API AOmniKernel_OS : public AInfo
{
	GENERATED_BODY()

public:
	AOmniKernel_OS();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// ─── Primary Evaluation Interface ─────────────────────────────────
	/**
	 * Main sensor fusion entry point. Called by PlayerCharacter on every
	 * acoustic event (movement, hacking, voice override).
	 * Evaluates threat level from acoustic + thermal inputs.
	 */
	UFUNCTION(BlueprintCallable, Category = "Unified Architecture")
	void EvaluateFacilityState(float PlayerAcousticLevel, float SectorTemperature);

	/** Immediately locks all InteractiveNodes in the given sector and cuts power. */
	UFUNCTION(BlueprintCallable, Category = "Unified Architecture")
	void ExecuteSectorLockdown(FName SectorID);

	/**
	 * Schedules a sector lockdown with a Delay (seconds).
	 * Used for escalating response — gives the player a narrow reaction window.
	 */
	UFUNCTION(BlueprintCallable, Category = "Unified Architecture")
	void EscalateToLockdown(FName SectorID, float Delay);

	/** Routes a hacking disturbance event directly to DispatchBiologicalAssets. */
	UFUNCTION(BlueprintCallable, Category = "Unified Architecture")
	void ReportHackingDisturbance(FVector TargetCoordinates, int32 ThreatLevel);

	/** Activates or clears gas flood for the specified sector. */
	UFUNCTION(BlueprintCallable, Category = "Unified Architecture")
	void TriggerSectorGasFlood(FName SectorID, bool bEnabled);

	/** Grants all CoreBreachAccess nodes authorization. */
	UFUNCTION(BlueprintCallable, Category = "Unified Architecture")
	void UnlockCoreBreachPath();

	/** Triggers the facility-wide Final Purge: lockdown all non-safe sectors + gas flood. */
	UFUNCTION(BlueprintCallable, Category = "Unified Architecture")
	void BeginFinalPurgeSequence();

	/** Shuts down OmniKernel: reverts all gas floods, ceases threat evaluation. */
	UFUNCTION(BlueprintCallable, Category = "Unified Architecture")
	void ShutdownOmniKernel();

	// ─── State Queries ─────────────────────────────────────────────────
	UFUNCTION(BlueprintPure, Category = "Unified Architecture")
	bool IsFinalPurgeActive() const;

	UFUNCTION(BlueprintPure, Category = "Unified Architecture")
	bool IsOmniKernelShutdown() const;

	UFUNCTION(BlueprintPure, Category = "Unified Architecture")
	FORCEINLINE float GetCurrentGlobalThreatLevel() const { return CurrentGlobalThreatLevel; }

	// ─── Threat Modulation (called by EndgameDirector / BioAssets) ─────
	/** Directly add threat (used by BioAssets when they confirm a player sighting). */
	UFUNCTION(BlueprintCallable, Category = "Unified Architecture")
	void AddThreat(float Amount);

	// ─── Multicast Delegates ───────────────────────────────────────────
	UPROPERTY(BlueprintAssignable, Category = "Unified Architecture")
	FOmniKernelBoolEvent OnFinalPurgeStateChangedEvent;

	UPROPERTY(BlueprintAssignable, Category = "Unified Architecture")
	FOmniKernelEvent OnCoreBreachPathUnlockedEvent;

	UPROPERTY(BlueprintAssignable, Category = "Unified Architecture")
	FOmniKernelBoolEvent OnOmniKernelShutdownStateChangedEvent;

	UPROPERTY(BlueprintAssignable, Category = "Unified Architecture")
	FOmniKernelThreatEvent OnThreatLevelEscalated;

	// ─── Blueprint Implementable Events ───────────────────────────────
	UFUNCTION(BlueprintImplementableEvent, Category = "Unified Architecture")
	void OnFinalPurgeStateChanged(bool bNewFinalPurgeState);

	UFUNCTION(BlueprintImplementableEvent, Category = "Unified Architecture")
	void OnCoreBreachPathUnlocked();

	UFUNCTION(BlueprintImplementableEvent, Category = "Unified Architecture")
	void OnOmniKernelShutdownStateChanged(bool bIsShutdown);

	UFUNCTION(BlueprintImplementableEvent, Category = "Unified Architecture")
	void OnThreatEscalated(float NewThreatLevel, FName SectorID);

	// ─── Tuning Parameters ─────────────────────────────────────────────
	/** Acoustic level above which OmniKernel dispatches BioAssets. [default: 80] */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unified Architecture|Tuning")
	float AcousticDispatchThreshold;

	/** Rate at which global threat decays per second when no acoustic events occur. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unified Architecture|Tuning")
	float ThreatDecayRate;

	/** Maximum global threat level ceiling. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unified Architecture|Tuning")
	float MaxGlobalThreatLevel;

	/** Minimum BioAsset walk speed during low-threat patrol. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unified Architecture|Tuning")
	float BioAssetPatrolSpeedMin;

	/** Maximum BioAsset walk speed during max-threat hunting. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unified Architecture|Tuning")
	float BioAssetPatrolSpeedMax;

protected:
	// ─── Internal State ────────────────────────────────────────────────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unified Architecture")
	float CurrentGlobalThreatLevel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unified Architecture")
	bool bFinalPurgeActive;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unified Architecture")
	bool bOmniKernelShutdown;

	// Sector state grids — FName = SectorID
	TMap<FName, float> SectorPowerGrid;
	TMap<FName, float> SectorToxicityGrid;

	// Cached world references — built in BeginPlay to avoid Tick-time queries
	TArray<TWeakObjectPtr<AA_BioAsset>> CachedBioAssets;
	TArray<TWeakObjectPtr<AA_SectorVolume>> CachedSectorVolumes;

	// Timer handles for deferred lockdowns
	TMap<FName, FTimerHandle> PendingLockdownTimers;

private:
	void DispatchBiologicalAssets(FVector TargetCoordinates, int32 ThreatLevel);
	void TickThreatDecay(float DeltaTime);
	void ModulatePatrolStates();
	void RebuildWorldCache();

	// Whether a threat event was registered this frame (suppresses decay for one tick)
	bool bThreatEventThisFrame;
};
