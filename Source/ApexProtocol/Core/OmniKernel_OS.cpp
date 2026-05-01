#include "OmniKernel_OS.h"
#include "../Entities/Characters/A_BioAsset.h"
#include "../Entities/Environment/A_InteractiveNode.h"
#include "../Entities/Environment/A_SectorVolume.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

// ============================================================
// AOmniKernel_OS — Unified Facility Director AI Implementation
// ============================================================

AOmniKernel_OS::AOmniKernel_OS()
{
	PrimaryActorTick.bCanEverTick = true;

	bFinalPurgeActive = false;
	bOmniKernelShutdown = false;
	bThreatEventThisFrame = false;

	CurrentGlobalThreatLevel = 0.0f;

	// Tuning defaults — fully overridable in Blueprint subclass
	AcousticDispatchThreshold = 80.0f;
	ThreatDecayRate = 1.5f;      // Units per second of natural threat decay
	MaxGlobalThreatLevel = 10.0f;
	BioAssetPatrolSpeedMin = 180.0f;
	BioAssetPatrolSpeedMax = 450.0f;
}

void AOmniKernel_OS::BeginPlay()
{
	Super::BeginPlay();

	// Build the world cache — avoids GetAllActorsOfClass in Tick (expensive)
	RebuildWorldCache();
}

void AOmniKernel_OS::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bOmniKernelShutdown)
	{
		return;
	}

	// Decay global threat toward zero when the facility is quiet
	TickThreatDecay(DeltaTime);

	// Modulate BioAsset patrol speeds based on current threat level
	ModulatePatrolStates();

	// Reset the per-frame acoustic event flag
	bThreatEventThisFrame = false;
}

// ─────────────────────────────────────────────────────────────
// EvaluateFacilityState — Primary sensor fusion
// ─────────────────────────────────────────────────────────────
void AOmniKernel_OS::EvaluateFacilityState(float PlayerAcousticLevel, float SectorTemperature)
{
	if (bOmniKernelShutdown)
	{
		return;
	}

	bThreatEventThisFrame = true;

	// Thermal weighting: hot sectors indicate hazardous activity — raises sensitivity
	const float ThermalBias = (SectorTemperature >= 30.0f) ? 1.25f : 1.0f;
	const float EffectiveAcousticLevel = PlayerAcousticLevel * ThermalBias;

	// Dynamic threshold — lower during Final Purge for maximum aggression
	const float ActiveThreshold = bFinalPurgeActive
		? (AcousticDispatchThreshold * 0.65f)
		: AcousticDispatchThreshold;

	if (EffectiveAcousticLevel > ActiveThreshold)
	{
		if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
		{
			const FVector PlayerCoordinates = PlayerPawn->GetActorLocation();

			// Base threat level derived from acoustic intensity and temperature
			int32 ComputedThreatLevel = (SectorTemperature >= 30.0f) ? 4 : 3;

			if (bFinalPurgeActive)
			{
				ComputedThreatLevel += 2;
			}

			// Accumulate global threat — capped at max
			const float ThreatAccumulation = FMath::GetMappedRangeValueClamped(
				FVector2D(ActiveThreshold, 100.0f),
				FVector2D(0.5f, 2.5f),
				EffectiveAcousticLevel
			);
			CurrentGlobalThreatLevel = FMath::Clamp(
				CurrentGlobalThreatLevel + ThreatAccumulation,
				0.0f,
				MaxGlobalThreatLevel
			);

			DispatchBiologicalAssets(PlayerCoordinates, ComputedThreatLevel);

			// Broadcast threat escalation event to Blueprint/EndgameDirector
			const FName CurrentSectorID = NAME_None; // resolved from SectorVolume if needed
			OnThreatLevelEscalated.Broadcast(CurrentGlobalThreatLevel, CurrentSectorID);
			OnThreatEscalated(CurrentGlobalThreatLevel, CurrentSectorID);
		}
	}
}

// ─────────────────────────────────────────────────────────────
// TickThreatDecay — Natural threat decay toward 0
// Suppressed for one tick after an acoustic event fires.
// ─────────────────────────────────────────────────────────────
void AOmniKernel_OS::TickThreatDecay(float DeltaTime)
{
	if (bThreatEventThisFrame || CurrentGlobalThreatLevel <= 0.0f)
	{
		return;
	}

	CurrentGlobalThreatLevel = FMath::Max(
		0.0f,
		CurrentGlobalThreatLevel - (ThreatDecayRate * DeltaTime)
	);
}

// ─────────────────────────────────────────────────────────────
// ModulatePatrolStates — Speed-scales ALL patrolling BioAssets
// proportionally to the current global threat level.
// Only adjusts assets in Patrolling state to avoid interrupting hunts.
// ─────────────────────────────────────────────────────────────
void AOmniKernel_OS::ModulatePatrolStates()
{
	if (CachedBioAssets.Num() == 0)
	{
		return;
	}

	// Normalized threat in [0, 1]
	const float ThreatAlpha = FMath::Clamp(CurrentGlobalThreatLevel / MaxGlobalThreatLevel, 0.0f, 1.0f);
	const float TargetPatrolSpeed = FMath::Lerp(BioAssetPatrolSpeedMin, BioAssetPatrolSpeedMax, ThreatAlpha);

	for (TWeakObjectPtr<AA_BioAsset>& AssetRef : CachedBioAssets)
	{
		if (!AssetRef.IsValid())
		{
			continue;
		}

		AA_BioAsset* BioAsset = AssetRef.Get();

		// Only modulate patrolling assets — hunting/investigating have their own speeds
		if (BioAsset->GetCurrentState() == EBioAssetState::Patrolling)
		{
			BioAsset->GetCharacterMovement()->MaxWalkSpeed = TargetPatrolSpeed;
		}
	}
}

// ─────────────────────────────────────────────────────────────
// ExecuteSectorLockdown — Immediate lockdown
// ─────────────────────────────────────────────────────────────
void AOmniKernel_OS::ExecuteSectorLockdown(FName SectorID)
{
	SectorPowerGrid.Add(SectorID, 0.0f);

	TArray<AActor*> InteractiveNodes;
	UGameplayStatics::GetAllActorsOfClass(this, AA_InteractiveNode::StaticClass(), InteractiveNodes);

	for (AActor* NodeActor : InteractiveNodes)
	{
		if (AA_InteractiveNode* InteractiveNode = Cast<AA_InteractiveNode>(NodeActor))
		{
			if (InteractiveNode->GetSectorIdentifier() == SectorID)
			{
				InteractiveNode->SetNodeOperationalState(false);
			}
		}
	}
}

// ─────────────────────────────────────────────────────────────
// EscalateToLockdown — Deferred lockdown with timer
// ─────────────────────────────────────────────────────────────
void AOmniKernel_OS::EscalateToLockdown(FName SectorID, float Delay)
{
	if (bOmniKernelShutdown || SectorID.IsNone() || GetWorld() == nullptr)
	{
		return;
	}

	// Cancel any existing pending lockdown for this sector
	if (FTimerHandle* ExistingTimer = PendingLockdownTimers.Find(SectorID))
	{
		GetWorld()->GetTimerManager().ClearTimer(*ExistingTimer);
	}

	FTimerHandle NewTimer;
	FTimerDelegate LockdownDelegate;
	LockdownDelegate.BindUFunction(this, FName("ExecuteSectorLockdown"), SectorID);

	GetWorld()->GetTimerManager().SetTimer(NewTimer, LockdownDelegate, Delay, false);
	PendingLockdownTimers.Add(SectorID, NewTimer);
}

// ─────────────────────────────────────────────────────────────
// ReportHackingDisturbance
// ─────────────────────────────────────────────────────────────
void AOmniKernel_OS::ReportHackingDisturbance(FVector TargetCoordinates, int32 ThreatLevel)
{
	if (bOmniKernelShutdown)
	{
		return;
	}

	bThreatEventThisFrame = true;

	// Hacking is a significant disturbance — accumulate threat
	CurrentGlobalThreatLevel = FMath::Clamp(
		CurrentGlobalThreatLevel + (float)ThreatLevel,
		0.0f,
		MaxGlobalThreatLevel
	);

	DispatchBiologicalAssets(TargetCoordinates, ThreatLevel);
}

// ─────────────────────────────────────────────────────────────
// TriggerSectorGasFlood
// ─────────────────────────────────────────────────────────────
void AOmniKernel_OS::TriggerSectorGasFlood(FName SectorID, bool bEnabled)
{
	SectorToxicityGrid.Add(SectorID, bEnabled ? 1.0f : 0.0f);

	for (TWeakObjectPtr<AA_SectorVolume>& SectorRef : CachedSectorVolumes)
	{
		if (!SectorRef.IsValid())
		{
			continue;
		}

		AA_SectorVolume* SectorVolume = SectorRef.Get();
		if (SectorVolume->GetSectorIdentifier() == SectorID)
		{
			SectorVolume->SetGasFlooded(bEnabled);
			break; // SectorIDs are unique
		}
	}
}

// ─────────────────────────────────────────────────────────────
// AddThreat — Direct threat accumulation (called by BioAssets on confirm)
// ─────────────────────────────────────────────────────────────
void AOmniKernel_OS::AddThreat(float Amount)
{
	if (bOmniKernelShutdown)
	{
		return;
	}

	CurrentGlobalThreatLevel = FMath::Clamp(
		CurrentGlobalThreatLevel + Amount,
		0.0f,
		MaxGlobalThreatLevel
	);

	bThreatEventThisFrame = true;
}

// ─────────────────────────────────────────────────────────────
// UnlockCoreBreachPath
// ─────────────────────────────────────────────────────────────
void AOmniKernel_OS::UnlockCoreBreachPath()
{
	TArray<AActor*> InteractiveNodes;
	UGameplayStatics::GetAllActorsOfClass(this, AA_InteractiveNode::StaticClass(), InteractiveNodes);

	for (AActor* NodeActor : InteractiveNodes)
	{
		if (AA_InteractiveNode* InteractiveNode = Cast<AA_InteractiveNode>(NodeActor))
		{
			if (InteractiveNode->RequiresCoreBreachAccess())
			{
				InteractiveNode->SetCoreBreachAuthorized(true);
				InteractiveNode->SetNodeOperationalState(true);
			}
		}
	}

	OnCoreBreachPathUnlockedEvent.Broadcast();
	OnCoreBreachPathUnlocked();
}

// ─────────────────────────────────────────────────────────────
// BeginFinalPurgeSequence
// ─────────────────────────────────────────────────────────────
void AOmniKernel_OS::BeginFinalPurgeSequence()
{
	if (bFinalPurgeActive || bOmniKernelShutdown)
	{
		return;
	}

	bFinalPurgeActive = true;
	CurrentGlobalThreatLevel = MaxGlobalThreatLevel; // Instant max threat

	for (TWeakObjectPtr<AA_SectorVolume>& SectorRef : CachedSectorVolumes)
	{
		if (!SectorRef.IsValid())
		{
			continue;
		}

		AA_SectorVolume* SectorVolume = SectorRef.Get();
		const FName SectorID = SectorVolume->GetSectorIdentifier();

		if (!SectorID.IsNone() && !SectorVolume->IsSafeRoom())
		{
			ExecuteSectorLockdown(SectorID);
			TriggerSectorGasFlood(SectorID, true);
		}
	}

	OnFinalPurgeStateChangedEvent.Broadcast(true);
	OnFinalPurgeStateChanged(true);
}

// ─────────────────────────────────────────────────────────────
// ShutdownOmniKernel
// ─────────────────────────────────────────────────────────────
void AOmniKernel_OS::ShutdownOmniKernel()
{
	if (bOmniKernelShutdown)
	{
		return;
	}

	bOmniKernelShutdown = true;
	bFinalPurgeActive = false;
	CurrentGlobalThreatLevel = 0.0f;

	// Clear all pending deferred lockdowns
	if (GetWorld() != nullptr)
	{
		for (auto& TimerEntry : PendingLockdownTimers)
		{
			GetWorld()->GetTimerManager().ClearTimer(TimerEntry.Value);
		}
	}
	PendingLockdownTimers.Empty();

	// Revert all gas floods
	for (TWeakObjectPtr<AA_SectorVolume>& SectorRef : CachedSectorVolumes)
	{
		if (!SectorRef.IsValid())
		{
			continue;
		}

		AA_SectorVolume* SectorVolume = SectorRef.Get();
		const FName SectorID = SectorVolume->GetSectorIdentifier();

		if (!SectorID.IsNone())
		{
			TriggerSectorGasFlood(SectorID, false);
		}
	}

	OnFinalPurgeStateChangedEvent.Broadcast(false);
	OnOmniKernelShutdownStateChangedEvent.Broadcast(true);
	OnFinalPurgeStateChanged(false);
	OnOmniKernelShutdownStateChanged(true);
}

bool AOmniKernel_OS::IsFinalPurgeActive() const
{
	return bFinalPurgeActive;
}

bool AOmniKernel_OS::IsOmniKernelShutdown() const
{
	return bOmniKernelShutdown;
}

// ─────────────────────────────────────────────────────────────
// DispatchBiologicalAssets — Proximity-sorted dispatch
// ─────────────────────────────────────────────────────────────
void AOmniKernel_OS::DispatchBiologicalAssets(FVector TargetCoordinates, int32 ThreatLevel)
{
	// Filter out invalid weak pointers from the cache
	TArray<AA_BioAsset*> ValidAssets;
	for (TWeakObjectPtr<AA_BioAsset>& AssetRef : CachedBioAssets)
	{
		if (AssetRef.IsValid() && AssetRef->IsEntityActive() && !AssetRef->IsHunting())
		{
			ValidAssets.Add(AssetRef.Get());
		}
	}

	if (ValidAssets.Num() == 0)
	{
		return;
	}

	// Sort by distance to the target — closest assets respond first
	ValidAssets.Sort([&TargetCoordinates](const AA_BioAsset& Left, const AA_BioAsset& Right)
	{
		const float LeftDist = FVector::DistSquared(Left.GetActorLocation(), TargetCoordinates);
		const float RightDist = FVector::DistSquared(Right.GetActorLocation(), TargetCoordinates);
		return LeftDist < RightDist;
	});

	const int32 AssetsToDispatch = FMath::Clamp(ThreatLevel, 1, ValidAssets.Num());

	for (int32 Index = 0; Index < AssetsToDispatch; ++Index)
	{
		ValidAssets[Index]->InvestigateAcousticDisturbance(TargetCoordinates);
	}
}

// ─────────────────────────────────────────────────────────────
// RebuildWorldCache — Called once in BeginPlay
// Caches typed weak pointers to avoid per-tick class queries.
// ─────────────────────────────────────────────────────────────
void AOmniKernel_OS::RebuildWorldCache()
{
	CachedBioAssets.Empty();
	CachedSectorVolumes.Empty();

	TArray<AActor*> BioAssetActors;
	UGameplayStatics::GetAllActorsOfClass(this, AA_BioAsset::StaticClass(), BioAssetActors);
	for (AActor* Actor : BioAssetActors)
	{
		if (AA_BioAsset* BioAsset = Cast<AA_BioAsset>(Actor))
		{
			CachedBioAssets.Add(TWeakObjectPtr<AA_BioAsset>(BioAsset));
		}
	}

	TArray<AActor*> SectorActors;
	UGameplayStatics::GetAllActorsOfClass(this, AA_SectorVolume::StaticClass(), SectorActors);
	for (AActor* Actor : SectorActors)
	{
		if (AA_SectorVolume* SectorVol = Cast<AA_SectorVolume>(Actor))
		{
			CachedSectorVolumes.Add(TWeakObjectPtr<AA_SectorVolume>(SectorVol));

			// Initialize the power grid with full power for each sector
			if (!SectorVol->GetSectorIdentifier().IsNone())
			{
				SectorPowerGrid.FindOrAdd(SectorVol->GetSectorIdentifier()) = 1.0f;
			}
		}
	}
}
