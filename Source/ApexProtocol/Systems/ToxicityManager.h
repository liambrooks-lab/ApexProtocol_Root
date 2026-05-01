#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ToxicityManager.generated.h"

// ============================================================
// Toxicity Threshold Events — Hysteresis-gated delegates
// Fires on crossing, will not re-fire until toxicity drops
// below (Threshold - HysteresisWindow) before rising again.
// ============================================================
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FToxicityThresholdEvent, float, CurrentToxicity);

UCLASS(ClassGroup=(Custom), BlueprintSpawnableComponent)
class APEXPROTOCOL_API UToxicityManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UToxicityManager();

	// ─── Core ODE Accessors ────────────────────────────────────────────
	FORCEINLINE float GetCurrentToxicity() const { return CurrentToxicity; }
	FORCEINLINE float GetEnvironmentalExposureRate() const { return EnvironmentalExposureRate; }

	/**
	 * Returns a normalized [0,1] scalar for input latency injection.
	 * Maps toxicity range [HallucinationThreshold, 100] → [0, 1].
	 * Below HallucinationThreshold: returns 0 (no latency).
	 */
	UFUNCTION(BlueprintPure, Category = "Survival Systems")
	float GetInputLatencyScalar() const;

	// ─── Mutation Interface ────────────────────────────────────────────
	/** Applies an instantaneous one-shot exposure dose (e.g., BioAsset sting). */
	UFUNCTION(BlueprintCallable, Category = "Survival Systems")
	void ApplyEnvironmentalExposure(float ExposureDose);

	/** Decrements toxicity by NeutralizationFactor — safe room antidote drip. */
	UFUNCTION(BlueprintCallable, Category = "Survival Systems")
	void AdministerAntidote(float NeutralizationFactor);

	/** Sets the continuous exposure rate used in the ODE Tick: dC/dt = R_in - k*C. */
	UFUNCTION(BlueprintCallable, Category = "Survival Systems")
	void SetEnvironmentalExposureRate(float ExposureRate);

	/** Direct override for checkpoint restoration. */
	UFUNCTION(BlueprintCallable, Category = "Survival Systems")
	void SetCurrentToxicity(float NewToxicity);

	// ─── Threshold Configuration ───────────────────────────────────────
	/** Toxicity value at which hallucinations begin [default: 40]. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Biological Math")
	float HallucinationThreshold;

	/** Toxicity value at which critical stress symptoms manifest [default: 75]. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Biological Math")
	float CriticalThreshold;

	/** Hysteresis window — toxicity must drop this far below a threshold before
	 *  the same threshold can re-fire. Prevents rapid event spam [default: 10]. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Biological Math")
	float HysteresisWindow;

	// ─── Threshold Events ──────────────────────────────────────────────
	/** Fired when toxicity crosses HallucinationThreshold (≥40) for the first time. */
	UPROPERTY(BlueprintAssignable, Category = "Survival Systems|Events")
	FToxicityThresholdEvent OnHallucinationThresholdReached;

	/** Fired when toxicity crosses CriticalThreshold (≥75). */
	UPROPERTY(BlueprintAssignable, Category = "Survival Systems|Events")
	FToxicityThresholdEvent OnCriticalToxicityReached;

	/** Fired when toxicity reaches 100 — triggers biological collapse. */
	UPROPERTY(BlueprintAssignable, Category = "Survival Systems|Events")
	FToxicityThresholdEvent OnBiologicalCollapseEvent;

	/** Fired when toxicity drops back below HallucinationThreshold (recovery). */
	UPROPERTY(BlueprintAssignable, Category = "Survival Systems|Events")
	FToxicityThresholdEvent OnHallucinationRecovered;

protected:
	// ─── ODE State ─────────────────────────────────────────────────────
	/** C — current blood toxicity concentration [0, 100]. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Biological Math")
	float CurrentToxicity;

	/** k — metabolic clearance rate constant. dC/dt = R_in - k*C. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Biological Math")
	float MetabolicClearanceRate;

	/** R_in — continuous environmental exposure rate (units/sec). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Biological Math")
	float EnvironmentalExposureRate;

	// ─── Hysteresis Flags ──────────────────────────────────────────────
	bool bHallucinationThresholdActive;
	bool bCriticalThresholdActive;
	bool bCollapseTriggered;

	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void EvaluateThresholds(float PreviousToxicity);
};
