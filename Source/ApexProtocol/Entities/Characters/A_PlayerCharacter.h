#pragma once

#include "CoreMinimal.h"
#include "../Base/A_ApexCharacterEntity.h"
#include "A_PlayerCharacter.generated.h"

class AAudioDirector;
class UCameraComponent;
class USpringArmComponent;
class UTerminalWidget_Native;
class UToxicityManager;
class AOmniKernel_OS;
class AA_SectorVolume;
class AA_InteractiveNode;

// ============================================================
// Input Latency Ring Buffer
//
// Stores deferred movement input samples. At high toxicity,
// inputs are written then replayed N ticks later rather than
// processed immediately — simulating neurological impairment.
// ============================================================
struct FDeferredInputSample
{
	float ForwardValue;
	float RightValue;
	float DelayRemaining; // Seconds until this sample should be consumed

	FDeferredInputSample()
		: ForwardValue(0.0f), RightValue(0.0f), DelayRemaining(0.0f)
	{}

	FDeferredInputSample(float Fwd, float Rt, float Delay)
		: ForwardValue(Fwd), RightValue(Rt), DelayRemaining(Delay)
	{}
};

UCLASS()
class APEXPROTOCOL_API AA_PlayerCharacter : public AApexCharacterEntity
{
	GENERATED_BODY()

public:
	AA_PlayerCharacter();

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaTime) override;

	// ─── Public Interface ──────────────────────────────────────────────
	/** Restore player state from the last registered checkpoint. */
	UFUNCTION(BlueprintCallable, Category = "Recovery")
	void RestoreFromCheckpoint();

	/** Exposes BiologicalSystems to BioAsset ToxicAura system. */
	UFUNCTION(BlueprintPure, Category = "Survival Systems")
	FORCEINLINE UToxicityManager* GetBiologicalSystems() const { return BiologicalSystems; }

	/** Reports an acoustic event to OmniKernel_OS. Loudness is in [0, 100] units. */
	void ReportAcousticEvent(float Loudness);

protected:
	// ─── Components ────────────────────────────────────────────────────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Systems")
	UToxicityManager* BiologicalSystems;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* FollowCamera;

	// ─── Terminal State ─────────────────────────────────────────────────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Terminal")
	bool bTerminalVisible;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Terminal")
	TSubclassOf<UTerminalWidget_Native> TerminalWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Terminal")
	TObjectPtr<UTerminalWidget_Native> ActiveTerminalWidget;

	// ─── Interaction ────────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	float InteractionRange;

	// ─── Senses / Survival State ────────────────────────────────────────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Senses")
	float AcousticOutputLevel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Senses")
	float CurrentSectorTemperature;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Survival")
	bool bInSafeRoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Survival")
	FName CurrentSectorID;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Survival")
	FName LastCheckpointSectorID;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Survival")
	bool bBiologicalCollapseTriggered;

	// ─── Movement Modifiers ─────────────────────────────────────────────
	/** True when player is crouching. Crouching reduces AcousticOutputLevel by CrouchAcousticMultiplier. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	bool bIsCrouching;

	/** True when player is sprinting. Sprinting raises AcousticOutputLevel by SprintAcousticMultiplier. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	bool bIsSprinting;

	/** Acoustic multiplier when crouching [default: 0.25 — nearly silent]. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float CrouchAcousticMultiplier;

	/** Acoustic multiplier when sprinting [default: 2.0 — double noise]. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float SprintAcousticMultiplier;

	/** Walk speed when crouching. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float CrouchWalkSpeed;

	/** Walk speed when sprinting. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float SprintSpeed;

	/** Normal walk speed. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float NormalWalkSpeed;

	// ─── Input Latency Ring Buffer ──────────────────────────────────────
	/** Maximum input delay at toxicity = 100 [seconds, default: 0.35]. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Survival")
	float MaxInputLatencySeconds;

	// ─── Transient Cached References ───────────────────────────────────
	UPROPERTY(Transient)
	TObjectPtr<AA_InteractiveNode> FocusedNode;

	UPROPERTY(Transient)
	TObjectPtr<AA_SectorVolume> ActiveSectorVolume;

	UPROPERTY(Transient)
	TObjectPtr<AOmniKernel_OS> CachedOmniKernel;

	UPROPERTY(Transient)
	TObjectPtr<AAudioDirector> CachedAudioDirector;

	// ─── Blueprint Events — Implement for Visual Effects ───────────────
	/** Called when toxicity crosses the Hallucination threshold (≥40).
	 *  Trigger post-process material parameters here (chromatic aberration, vignette). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Survival|Effects")
	void OnHallucinationOnset(float ToxicityLevel);

	/** Called when toxicity recovers below the hallucination threshold. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Survival|Effects")
	void OnHallucinationCleared();

	/** Called when toxicity crosses the Critical threshold (≥75).
	 *  Trigger heavy camera distortion, involuntary audio. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Survival|Effects")
	void OnCriticalToxicityOnset(float ToxicityLevel);

	virtual void BeginPlay() override;

private:
	// ─── Private Helpers ───────────────────────────────────────────────
	void HandleBiologicalCollapse();
	void UpdateTerminalContextReadout() const;
	void RegisterSafeRoomCheckpoint();
	void UpdateSectorState(float DeltaTime);
	void TickInputLatencyBuffer(float DeltaTime);
	void ApplyDeferredMovement(float ForwardValue, float RightValue);
	AA_SectorVolume* ResolveCurrentSectorVolume() const;
	AA_InteractiveNode* TraceInteractiveNode() const;
	void AcquireFocusedNode();
	void RefreshAudioState() const;

	// ─── Input Callbacks ───────────────────────────────────────────────
	void Interact();
	void VoiceOverride();
	void MoveForward(float Value);
	void MoveRight(float Value);
	void TurnAtRate(float Value);
	void LookUpAtRate(float Value);
	void ToggleTerminal();
	void ToggleCrouch();
	void StartSprint();
	void StopSprint();

	// ─── ToxicityManager Delegate Handlers ────────────────────────────
	UFUNCTION()
	void HandleHallucinationThreshold(float ToxicityLevel);

	UFUNCTION()
	void HandleHallucinationRecovered(float ToxicityLevel);

	UFUNCTION()
	void HandleCriticalToxicity(float ToxicityLevel);

	UFUNCTION()
	void HandleBiologicalCollapseDelegate(float ToxicityLevel);

	// ─── Input Latency Ring Buffer ─────────────────────────────────────
	TArray<FDeferredInputSample> InputLatencyBuffer;

	/** Raw input captured this frame — written to buffer or applied directly. */
	float RawForwardInput;
	float RawRightInput;
};
