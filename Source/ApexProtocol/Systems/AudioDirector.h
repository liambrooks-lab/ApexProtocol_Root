#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "Sound/SoundMix.h"
#include "Sound/SoundClass.h"
#include "AudioDirector.generated.h"

// ============================================================
// AAudioDirector — Diegetic Psychoacoustic Orchestrator
//
// Bridges the ToxicityManager ODE output and OmniKernel threat state
// to the audio engine. Two independent mix layers:
//   1. Hallucination Layer  — toxicity-driven (40–100%)
//   2. Final Purge Mix      — OmniKernel event-driven (binary)
//
// Blueprint Implementable Events allow a BP subclass to bind
// SoundMix assets and trigger visual post-process effects
// (chromatic aberration, vignette, etc.) in sync with audio.
// ============================================================

UCLASS()
class APEXPROTOCOL_API AAudioDirector : public AInfo
{
	GENERATED_BODY()

public:
	AAudioDirector();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// ─── Core Audio Interface ──────────────────────────────────────────
	/**
	 * Called every frame by A_PlayerCharacter with the current toxicity value.
	 * Drives hallucination mix intensity continuously.
	 * @param ToxicityLevel   Current C value from ToxicityManager [0–100].
	 */
	UFUNCTION(BlueprintCallable, Category = "Audio Architecture")
	void ApplyPsychoacousticFilters(float ToxicityLevel);

	/**
	 * Toggles the Final Purge audio mix (sirens, alarm drones, tension bed).
	 * @param bPurgeActive   True when OmniKernel Final Purge sequence is active.
	 */
	UFUNCTION(BlueprintCallable, Category = "Audio Architecture")
	void ApplyFinalPurgeMix(bool bPurgeActive);

	/**
	 * Sets hallucination intensity directly from a normalized [0,1] scalar.
	 * Used by PlayerCharacter when toxicity threshold delegates fire.
	 */
	UFUNCTION(BlueprintCallable, Category = "Audio Architecture")
	void SetHallucinationIntensity(float NormalizedIntensity);

	// ─── State Queries ─────────────────────────────────────────────────
	UFUNCTION(BlueprintPure, Category = "Audio Architecture")
	FORCEINLINE float GetHallucinationIntensity() const { return CurrentHallucinationIntensity; }

	UFUNCTION(BlueprintPure, Category = "Audio Architecture")
	FORCEINLINE bool IsFinalPurgeMixActive() const { return bFinalPurgeMixActive; }

	// ─── SoundMix Asset References ─────────────────────────────────────
	/** The SoundMix to push when hallucination layer is active.
	 *  Assign in Blueprint subclass or via Details panel.
	 *  Expected modifiers: Master pitch −0.1 to −0.25, Reverb wet +0.4. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Audio Architecture|Assets")
	TObjectPtr<USoundMix> HallucinationSoundMix;

	/** The SoundMix for the Final Purge event (high-tension alarm bed).
	 *  Expected modifiers: Master volume +0.3, low-freq emphasis. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Audio Architecture|Assets")
	TObjectPtr<USoundMix> FinalPurgeSoundMix;

	/** The SoundClass controlled by the hallucination mix
	 *  (typically "Master" or a dedicated "Ambient" class). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Audio Architecture|Assets")
	TObjectPtr<USoundClass> MasterSoundClass;

	// ─── Blueprint Events — Implement in BP subclass ───────────────────
	/**
	 * Fires whenever hallucination intensity changes by >= IntensityChangeTolerance.
	 * Drive post-process material parameters here (chromatic aberration, vignette, etc.).
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Audio Architecture")
	void OnHallucinationIntensityChanged(float NewIntensity);

	/** Fires when the Final Purge mix is activated. Use for alarm light flicker. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Audio Architecture")
	void OnFinalPurgeMixActivated();

	/** Fires when the Final Purge mix is deactivated (post-shutdown). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Audio Architecture")
	void OnFinalPurgeMixDeactivated();

	/** Fires on hallucination recovery (toxicity dropped below hysteresis). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Audio Architecture")
	void OnHallucinationRecovered();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio Architecture")
	float CurrentHallucinationIntensity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio Architecture")
	bool bHallucinationLayerActive;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio Architecture")
	bool bFinalPurgeMixActive;

private:
	/** Minimum change in intensity before re-applying SoundMix parameters.
	 *  Prevents redundant SetSoundMixClassOverride calls every frame. */
	static constexpr float IntensityChangeTolerance = 0.02f;

	float LastBroadcastIntensity;

	/** Internal: push or update the hallucination SoundMix with current intensity. */
	void PushHallucinationMix(float NormalizedIntensity);

	/** Internal: pop the hallucination SoundMix cleanly. */
	void PopHallucinationMix();
};
