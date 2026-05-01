#include "AudioDirector.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundMix.h"
#include "Sound/SoundClass.h"

// ============================================================
// AAudioDirector — Implementation
// ============================================================

AAudioDirector::AAudioDirector()
{
	PrimaryActorTick.bCanEverTick = false; // Driven externally by PlayerCharacter

	CurrentHallucinationIntensity = 0.0f;
	bHallucinationLayerActive = false;
	bFinalPurgeMixActive = false;
	LastBroadcastIntensity = -1.0f; // Force first update

	HallucinationSoundMix = nullptr;
	FinalPurgeSoundMix = nullptr;
	MasterSoundClass = nullptr;
}

void AAudioDirector::BeginPlay()
{
	Super::BeginPlay();
}

void AAudioDirector::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// ─────────────────────────────────────────────────────────────
// ApplyPsychoacousticFilters
// Called every frame from PlayerCharacter::RefreshAudioState.
// Maps raw toxicity [0–100] to a normalized intensity scalar
// and delegates to SetHallucinationIntensity.
// ─────────────────────────────────────────────────────────────
void AAudioDirector::ApplyPsychoacousticFilters(float ToxicityLevel)
{
	// Map toxicity [40, 100] → intensity [0.0, 1.0]
	// Below 40: intensity is 0 (no hallucination effect)
	constexpr float OnsetThreshold = 40.0f;

	const float NormalizedIntensity = (ToxicityLevel < OnsetThreshold)
		? 0.0f
		: FMath::GetMappedRangeValueClamped(
			FVector2D(OnsetThreshold, 100.0f),
			FVector2D(0.0f, 1.0f),
			ToxicityLevel
		);

	SetHallucinationIntensity(NormalizedIntensity);
}

// ─────────────────────────────────────────────────────────────
// SetHallucinationIntensity
// The central intensity setter. Toleranced to avoid redundant
// SoundMix calls, which are engine-level and not free.
// ─────────────────────────────────────────────────────────────
void AAudioDirector::SetHallucinationIntensity(float NormalizedIntensity)
{
	const float ClampedIntensity = FMath::Clamp(NormalizedIntensity, 0.0f, 1.0f);

	// Recovery path — hallucination was active, now cleared
	if (bHallucinationLayerActive && ClampedIntensity <= 0.0f)
	{
		CurrentHallucinationIntensity = 0.0f;
		bHallucinationLayerActive = false;
		PopHallucinationMix();
		LastBroadcastIntensity = 0.0f;
		OnHallucinationRecovered();
		OnHallucinationIntensityChanged(0.0f);
		return;
	}

	if (ClampedIntensity <= 0.0f)
	{
		return;
	}

	// Only push/update the mix if intensity has changed meaningfully
	if (FMath::Abs(ClampedIntensity - LastBroadcastIntensity) >= IntensityChangeTolerance)
	{
		CurrentHallucinationIntensity = ClampedIntensity;
		bHallucinationLayerActive = true;
		PushHallucinationMix(ClampedIntensity);
		LastBroadcastIntensity = ClampedIntensity;
		OnHallucinationIntensityChanged(ClampedIntensity);
	}
}

// ─────────────────────────────────────────────────────────────
// ApplyFinalPurgeMix
// Binary toggle — does not blend, cuts in/out on state change.
// ─────────────────────────────────────────────────────────────
void AAudioDirector::ApplyFinalPurgeMix(bool bPurgeActive)
{
	if (bPurgeActive && !bFinalPurgeMixActive)
	{
		bFinalPurgeMixActive = true;

		// Push the Purge SoundMix via the engine's global mix stack
		if (FinalPurgeSoundMix != nullptr)
		{
			UGameplayStatics::PushSoundMixModifier(this, FinalPurgeSoundMix);
		}

		OnFinalPurgeMixActivated();
	}
	else if (!bPurgeActive && bFinalPurgeMixActive)
	{
		bFinalPurgeMixActive = false;

		if (FinalPurgeSoundMix != nullptr)
		{
			UGameplayStatics::PopSoundMixModifier(this, FinalPurgeSoundMix);
		}

		OnFinalPurgeMixDeactivated();
	}
}

// ─────────────────────────────────────────────────────────────
// PushHallucinationMix (Internal)
// Applies SoundMix with intensity-scaled overrides:
//   Volume: slight reduction (disorientation)
//   Pitch:  low-frequency warping (intensity-scaled)
//   Both values are clamped to safe audio-engine ranges.
// ─────────────────────────────────────────────────────────────
void AAudioDirector::PushHallucinationMix(float NormalizedIntensity)
{
	if (HallucinationSoundMix == nullptr || MasterSoundClass == nullptr || GetWorld() == nullptr)
	{
		// No mix asset assigned — Blueprint subclass must assign these in Details panel.
		// Log once to avoid spam.
		UE_LOG(LogTemp, Warning, TEXT("AAudioDirector: HallucinationSoundMix or MasterSoundClass not assigned. "
			"Assign these in the Blueprint subclass Details panel."));
		return;
	}

	// Map intensity to audio parameters
	// At intensity 0.0: no change
	// At intensity 1.0: pitch −0.25 semitones (warp), volume −0.15 (muffling)
	const float PitchModifier = FMath::Lerp(1.0f, 0.75f, NormalizedIntensity);   // [1.0 → 0.75]
	const float VolumeModifier = FMath::Lerp(1.0f, 0.85f, NormalizedIntensity);  // [1.0 → 0.85]

	// SetSoundMixClassOverride: pushes per-SoundClass volume/pitch overrides
	// FadeInTime of 0.3s smooths rapid toxicity fluctuations
	UGameplayStatics::SetSoundMixClassOverride(
		this,
		HallucinationSoundMix,
		MasterSoundClass,
		VolumeModifier,
		PitchModifier,
		0.3f,    // FadeInTime
		true     // bApplyToChildren
	);

	UGameplayStatics::PushSoundMixModifier(this, HallucinationSoundMix);
}

// ─────────────────────────────────────────────────────────────
// PopHallucinationMix (Internal)
// Cleanly restores the audio mix stack.
// ─────────────────────────────────────────────────────────────
void AAudioDirector::PopHallucinationMix()
{
	if (HallucinationSoundMix == nullptr || GetWorld() == nullptr)
	{
		return;
	}

	// Restore to defaults (volume 1.0, pitch 1.0) with a fade-out
	if (MasterSoundClass != nullptr)
	{
		UGameplayStatics::SetSoundMixClassOverride(
			this,
			HallucinationSoundMix,
			MasterSoundClass,
			1.0f,  // Volume: full
			1.0f,  // Pitch: normal
			0.5f,  // FadeInTime (fade back to normal)
			true
		);
	}

	UGameplayStatics::PopSoundMixModifier(this, HallucinationSoundMix);
}
