#include "ToxicityManager.h"
#include "Math/UnrealMathUtility.h"

// ============================================================
// UToxicityManager — Blood Toxicity ODE Solver
//
// Governing Equation:
//   dC/dt = R_in - k * C
//
//   C   = CurrentToxicity (concentration, 0–100)
//   R_in = EnvironmentalExposureRate (input rate from sector gas/BioAssets)
//   k   = MetabolicClearanceRate (natural body clearance constant)
//
// Equilibrium: C* = R_in / k  (reached asymptotically)
// At k = 0.05 and R_in = 5.0 → C* = 100 (lethal ceiling)
// At k = 0.05 and R_in = 0.0 → C decays exponentially to 0
// ============================================================

UToxicityManager::UToxicityManager()
{
	PrimaryComponentTick.bCanEverTick = true;

	CurrentToxicity = 0.0f;
	MetabolicClearanceRate = 0.05f;
	EnvironmentalExposureRate = 0.0f;

	// Threshold values
	HallucinationThreshold = 40.0f;
	CriticalThreshold = 75.0f;
	HysteresisWindow = 10.0f;

	// Hysteresis state flags
	bHallucinationThresholdActive = false;
	bCriticalThresholdActive = false;
	bCollapseTriggered = false;
}

void UToxicityManager::BeginPlay()
{
	Super::BeginPlay();
}

void UToxicityManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bCollapseTriggered)
	{
		// Post-collapse: ODE halted, awaiting checkpoint recovery reset.
		return;
	}

	if (CurrentToxicity <= 0.0f && EnvironmentalExposureRate <= 0.0f)
	{
		// Short-circuit: nothing to integrate, no exposure active.
		return;
	}

	const float PreviousToxicity = CurrentToxicity;

	// Euler integration of dC/dt = R_in - k*C
	// A more accurate RK4 could be used but Euler with small DeltaTime (≤1/30s) is sufficient.
	const float dCdt = EnvironmentalExposureRate - (MetabolicClearanceRate * CurrentToxicity);
	CurrentToxicity = FMath::Clamp(CurrentToxicity + (dCdt * DeltaTime), 0.0f, 100.0f);

	// Evaluate all threshold crossings after update
	EvaluateThresholds(PreviousToxicity);
}

// ─────────────────────────────────────────────────────────────
// EvaluateThresholds — Hysteresis-gated event system
// Each threshold has two zones:
//   Rising:  fires when C crosses threshold from below
//   Recovery: fires and resets when C drops below (threshold - window)
// ─────────────────────────────────────────────────────────────
void UToxicityManager::EvaluateThresholds(float PreviousToxicity)
{
	// ── Hallucination Threshold (40) ──────────────────────────
	if (!bHallucinationThresholdActive && CurrentToxicity >= HallucinationThreshold)
	{
		bHallucinationThresholdActive = true;
		OnHallucinationThresholdReached.Broadcast(CurrentToxicity);
	}
	else if (bHallucinationThresholdActive && CurrentToxicity < (HallucinationThreshold - HysteresisWindow))
	{
		bHallucinationThresholdActive = false;
		OnHallucinationRecovered.Broadcast(CurrentToxicity);
	}

	// ── Critical Threshold (75) ───────────────────────────────
	if (!bCriticalThresholdActive && CurrentToxicity >= CriticalThreshold)
	{
		bCriticalThresholdActive = true;
		OnCriticalToxicityReached.Broadcast(CurrentToxicity);
	}
	else if (bCriticalThresholdActive && CurrentToxicity < (CriticalThreshold - HysteresisWindow))
	{
		bCriticalThresholdActive = false;
	}

	// ── Biological Collapse (100) ─────────────────────────────
	if (!bCollapseTriggered && CurrentToxicity >= 100.0f)
	{
		bCollapseTriggered = true;
		OnBiologicalCollapseEvent.Broadcast(CurrentToxicity);
	}
}

// ─────────────────────────────────────────────────────────────
// GetInputLatencyScalar
// Maps toxicity [HallucinationThreshold, 100] → scalar [0.0, 1.0]
// Used by PlayerCharacter to delay input forwarding (ring buffer).
// ─────────────────────────────────────────────────────────────
float UToxicityManager::GetInputLatencyScalar() const
{
	if (CurrentToxicity < HallucinationThreshold)
	{
		return 0.0f;
	}

	return FMath::GetMappedRangeValueClamped(
		FVector2D(HallucinationThreshold, 100.0f),
		FVector2D(0.0f, 1.0f),
		CurrentToxicity
	);
}

// ─────────────────────────────────────────────────────────────
// Mutation Methods
// ─────────────────────────────────────────────────────────────
void UToxicityManager::ApplyEnvironmentalExposure(float ExposureDose)
{
	if (bCollapseTriggered)
	{
		return;
	}

	const float Previous = CurrentToxicity;
	CurrentToxicity = FMath::Clamp(CurrentToxicity + ExposureDose, 0.0f, 100.0f);
	EvaluateThresholds(Previous);
}

void UToxicityManager::AdministerAntidote(float NeutralizationFactor)
{
	const float Previous = CurrentToxicity;
	CurrentToxicity = FMath::Clamp(CurrentToxicity - NeutralizationFactor, 0.0f, 100.0f);

	// Antidote recovery can de-escalate a critical state — re-evaluate
	EvaluateThresholds(Previous);
}

void UToxicityManager::SetEnvironmentalExposureRate(float ExposureRate)
{
	EnvironmentalExposureRate = FMath::Max(0.0f, ExposureRate);
}

void UToxicityManager::SetCurrentToxicity(float NewToxicity)
{
	const float Previous = CurrentToxicity;
	CurrentToxicity = FMath::Clamp(NewToxicity, 0.0f, 100.0f);

	// Checkpoint restoration may fully reset collapse state
	if (NewToxicity < 100.0f)
	{
		bCollapseTriggered = false;
	}

	if (NewToxicity < (HallucinationThreshold - HysteresisWindow))
	{
		bHallucinationThresholdActive = false;
		bCriticalThresholdActive = false;
	}

	EvaluateThresholds(Previous);
}
