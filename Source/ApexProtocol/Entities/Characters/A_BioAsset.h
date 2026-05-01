#pragma once

#include "CoreMinimal.h"
#include "../Base/A_ApexCharacterEntity.h"
#include "Perception/PawnSensingComponent.h"
#include "A_BioAsset.generated.h"

class UToxicityManager;
class AOmniKernel_OS;

// ============================================================
// EBioAssetState — Inline C++ AI state machine.
// Transitions are deterministic and tick-driven; no Behavior Tree assets required.
// ============================================================
UENUM(BlueprintType)
enum class EBioAssetState : uint8
{
	Dormant			UMETA(DisplayName = "Dormant"),
	Patrolling		UMETA(DisplayName = "Patrolling"),
	Investigating	UMETA(DisplayName = "Investigating"),
	Hunting			UMETA(DisplayName = "Hunting"),
	Suppressing		UMETA(DisplayName = "Suppressing")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBioAssetStateChangedEvent, EBioAssetState, NewState);

UCLASS()
class APEXPROTOCOL_API AA_BioAsset : public AApexCharacterEntity
{
	GENERATED_BODY()

public:
	AA_BioAsset();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// ─── OmniKernel Interface ──────────────────────────────────────────
	/** Called by OmniKernel_OS to direct the asset to an acoustic disturbance location. */
	UFUNCTION(BlueprintCallable, Category = "BioAsset|AI")
	void InvestigateAcousticDisturbance(FVector Location);

	/** Force-transition to hunt state, targeting a known pawn directly. */
	UFUNCTION(BlueprintCallable, Category = "BioAsset|AI")
	void InitiateHuntSequence(APawn* TargetPawn);

	/** Deactivate this asset (used by EndgameDirector after protocol completion). */
	UFUNCTION(BlueprintCallable, Category = "BioAsset|AI")
	void Deactivate();

	// ─── State Queries ─────────────────────────────────────────────────
	UFUNCTION(BlueprintPure, Category = "BioAsset|AI")
	FORCEINLINE EBioAssetState GetCurrentState() const { return CurrentState; }

	UFUNCTION(BlueprintPure, Category = "BioAsset|AI")
	FORCEINLINE bool IsHunting() const { return CurrentState == EBioAssetState::Hunting; }

	// ─── Events ────────────────────────────────────────────────────────
	UPROPERTY(BlueprintAssignable, Category = "BioAsset|AI")
	FBioAssetStateChangedEvent OnStateChanged;

	UFUNCTION(BlueprintImplementableEvent, Category = "BioAsset|AI")
	void OnBioAssetStateChanged(EBioAssetState NewState);

protected:
	// ─── Components ────────────────────────────────────────────────────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BioAsset|Sensing")
	TObjectPtr<UPawnSensingComponent> PawnSensor;

	// ─── Sensing Properties ────────────────────────────────────────────
	/** Hearing sensitivity radius in Unreal units. OmniKernel modulates via ThreatLevel. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BioAsset|Sensing")
	float AcousticSensitivityRadius;

	/** Line-of-sight range for visual acquisition. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BioAsset|Sensing")
	float ThermalDetectionRange;

	/** Radius within which this asset radiates passive toxin exposure to the player. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BioAsset|ToxicAura")
	float ToxicAuraRadius;

	/** Exposure rate per second applied to the player when within ToxicAuraRadius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BioAsset|ToxicAura")
	float ToxicAuraStrength;

	// ─── Movement Speeds ───────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BioAsset|Movement")
	float PatrolSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BioAsset|Movement")
	float InvestigateSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BioAsset|Movement")
	float HuntSpeed;

	// ─── State Machine ─────────────────────────────────────────────────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BioAsset|AI")
	EBioAssetState CurrentState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BioAsset|AI")
	FVector InvestigationTarget;

	UPROPERTY(Transient)
	TObjectPtr<APawn> HuntTarget;

	UPROPERTY(Transient)
	TObjectPtr<AOmniKernel_OS> CachedOmniKernel;

	// ─── State Timers ──────────────────────────────────────────────────
	/** How long the asset lingers in Investigating before returning to Patrol. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BioAsset|AI")
	float InvestigateTimeout;

	/** Elapsed time since entering the current state. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BioAsset|AI")
	float TimeInCurrentState;

	/** How long the asset suppresses (after losing sight) before returning to patrol. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BioAsset|AI")
	float SuppressTimeout;

private:
	// ─── State Machine Internals ───────────────────────────────────────
	void TransitionToState(EBioAssetState NewState);
	void TickDormant(float DeltaTime);
	void TickPatrolling(float DeltaTime);
	void TickInvestigating(float DeltaTime);
	void TickHunting(float DeltaTime);
	void TickSuppressing(float DeltaTime);

	/** Scan all overlapping actors for the player and apply ToxicAura exposure. */
	void TickToxicAura(float DeltaTime);

	/** UPawnSensingComponent callbacks. */
	UFUNCTION()
	void OnHearNoise(APawn* InInstigator, const FVector& Location, float Volume);

	UFUNCTION()
	void OnSeePawn(APawn* InPawn);

	bool bDeactivated;
	float PatrolDirectionTimer;
};
