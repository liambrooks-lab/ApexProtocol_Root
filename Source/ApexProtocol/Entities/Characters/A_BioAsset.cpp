#include "A_BioAsset.h"
#include "../../Core/OmniKernel_OS.h"
#include "../../Systems/ToxicityManager.h"
#include "A_PlayerCharacter.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Perception/PawnSensingComponent.h"

// ============================================================
// AA_BioAsset — Inline C++ AI State Machine Implementation
//
// State Transition Graph:
//   Dormant  ──activate──►  Patrolling
//   Patrolling  ──hear────►  Investigating
//   Patrolling  ──see─────►  Hunting
//   Investigating  ──see──►  Hunting
//   Investigating  ──timeout►  Patrolling
//   Hunting  ──lose target►  Suppressing
//   Suppressing  ──timeout►  Patrolling
//   Suppressing  ──see─────►  Hunting
// ============================================================

AA_BioAsset::AA_BioAsset()
{
	PrimaryActorTick.bCanEverTick = true;

	// Sensing component — core link to the acoustic/thermal detection loop
	PawnSensor = CreateDefaultSubobject<UPawnSensingComponent>(TEXT("PawnSensor"));
	PawnSensor->HearingThreshold = 600.0f;
	PawnSensor->LOSHearingThreshold = 1200.0f;
	PawnSensor->SightRadius = 1400.0f;
	PawnSensor->SetPeripheralVisionAngle(55.0f);
	PawnSensor->bHearNoises = true;
	PawnSensor->bSeePawns = true;

	// Movement configuration
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	ThermalSignature = 45.0f;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 360.0f, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = 220.0f;

	// Sensing parameters
	AcousticSensitivityRadius = 600.0f;
	ThermalDetectionRange = 1400.0f;

	// ToxicAura — passive proximity hazard
	ToxicAuraRadius = 220.0f;
	ToxicAuraStrength = 4.0f; // units/sec applied to player toxicity

	// Speed tiers
	PatrolSpeed = 180.0f;
	InvestigateSpeed = 280.0f;
	HuntSpeed = 450.0f;

	// State init
	CurrentState = EBioAssetState::Dormant;
	InvestigationTarget = FVector::ZeroVector;
	HuntTarget = nullptr;
	TimeInCurrentState = 0.0f;
	InvestigateTimeout = 8.0f;
	SuppressTimeout = 5.0f;

	bDeactivated = false;
	PatrolDirectionTimer = 0.0f;
}

void AA_BioAsset::BeginPlay()
{
	Super::BeginPlay();

	// Bind sensing delegates
	if (PawnSensor != nullptr)
	{
		PawnSensor->OnHearNoise.AddDynamic(this, &AA_BioAsset::OnHearNoise);
		PawnSensor->OnSeePawn.AddDynamic(this, &AA_BioAsset::OnSeePawn);
	}

	// Cache OmniKernel reference — avoids GetAllActorsOfClass in Tick
	CachedOmniKernel = Cast<AOmniKernel_OS>(UGameplayStatics::GetActorOfClass(this, AOmniKernel_OS::StaticClass()));

	// Entities begin in Patrolling unless explicitly deactivated
	TransitionToState(EBioAssetState::Patrolling);
}

void AA_BioAsset::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bDeactivated || !bIsEntityActive)
	{
		return;
	}

	TimeInCurrentState += DeltaTime;

	// Run the active state logic
	switch (CurrentState)
	{
	case EBioAssetState::Dormant:		  TickDormant(DeltaTime);		  break;
	case EBioAssetState::Patrolling:	  TickPatrolling(DeltaTime);	  break;
	case EBioAssetState::Investigating:   TickInvestigating(DeltaTime);   break;
	case EBioAssetState::Hunting:		  TickHunting(DeltaTime);		  break;
	case EBioAssetState::Suppressing:	  TickSuppressing(DeltaTime);	  break;
	default: break;
	}

	// ToxicAura runs regardless of state — proximity is always dangerous
	TickToxicAura(DeltaTime);
}

// ─────────────────────────────────────────────────────────────
// STATE: Dormant
// ─────────────────────────────────────────────────────────────
void AA_BioAsset::TickDormant(float DeltaTime)
{
	// Dormant assets do not respond to sensing; they are manually activated
	// by OmniKernel_OS via InvestigateAcousticDisturbance or InitiateHuntSequence.
	(void)DeltaTime;
}

// ─────────────────────────────────────────────────────────────
// STATE: Patrolling
// ─────────────────────────────────────────────────────────────
void AA_BioAsset::TickPatrolling(float DeltaTime)
{
	// Drift the patrol direction every 4-7 seconds to create organic wandering
	PatrolDirectionTimer -= DeltaTime;

	if (PatrolDirectionTimer <= 0.0f)
	{
		PatrolDirectionTimer = FMath::RandRange(4.0f, 7.0f);

		const FVector RandomOffset = FMath::VRand() * FMath::RandRange(300.0f, 800.0f);
		const FVector PatrolTarget = GetActorLocation() + FVector(RandomOffset.X, RandomOffset.Y, 0.0f);

		if (AAIController* AIControl = Cast<AAIController>(GetController()))
		{
			AIControl->MoveToLocation(PatrolTarget, 50.0f, false);
		}
	}
}

// ─────────────────────────────────────────────────────────────
// STATE: Investigating
// ─────────────────────────────────────────────────────────────
void AA_BioAsset::TickInvestigating(float DeltaTime)
{
	(void)DeltaTime;

	// Timeout guard — return to patrol if investigation yields nothing
	if (TimeInCurrentState >= InvestigateTimeout)
	{
		TransitionToState(EBioAssetState::Patrolling);
	}
}

// ─────────────────────────────────────────────────────────────
// STATE: Hunting
// ─────────────────────────────────────────────────────────────
void AA_BioAsset::TickHunting(float DeltaTime)
{
	(void)DeltaTime;

	if (HuntTarget == nullptr || !IsValid(HuntTarget))
	{
		TransitionToState(EBioAssetState::Suppressing);
		return;
	}

	// Continuously update movement target to the hunt target's live position
	if (AAIController* AIControl = Cast<AAIController>(GetController()))
	{
		AIControl->MoveToActor(HuntTarget, 80.0f, false);
	}
}

// ─────────────────────────────────────────────────────────────
// STATE: Suppressing
// Lingering after losing the target — sweeps the last known area.
// ─────────────────────────────────────────────────────────────
void AA_BioAsset::TickSuppressing(float DeltaTime)
{
	(void)DeltaTime;

	if (TimeInCurrentState >= SuppressTimeout)
	{
		HuntTarget = nullptr;
		TransitionToState(EBioAssetState::Patrolling);
	}
}

// ─────────────────────────────────────────────────────────────
// ToxicAura — Proximity-based passive toxin exposure
// ─────────────────────────────────────────────────────────────
void AA_BioAsset::TickToxicAura(float DeltaTime)
{
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (PlayerPawn == nullptr)
	{
		return;
	}

	const float DistanceToPlayer = FVector::Dist(GetActorLocation(), PlayerPawn->GetActorLocation());

	if (DistanceToPlayer <= ToxicAuraRadius)
	{
		if (AA_PlayerCharacter* PlayerCharacter = Cast<AA_PlayerCharacter>(PlayerPawn))
		{
			if (UToxicityManager* ToxManager = PlayerCharacter->GetBiologicalSystems())
			{
				// Exposure scales inversely with distance — full dose at zero range
				const float ProximityFactor = 1.0f - (DistanceToPlayer / ToxicAuraRadius);
				const float ExposureDose = ToxicAuraStrength * ProximityFactor * DeltaTime;
				ToxManager->ApplyEnvironmentalExposure(ExposureDose);
			}
		}
	}
}

// ─────────────────────────────────────────────────────────────
// Sensing Callbacks
// ─────────────────────────────────────────────────────────────
void AA_BioAsset::OnHearNoise(APawn* InInstigator, const FVector& Location, float Volume)
{
	if (bDeactivated || !bIsEntityActive)
	{
		return;
	}

	// If already hunting, don't downgrade to investigating
	if (CurrentState == EBioAssetState::Hunting)
	{
		return;
	}

	// Loud noises (volume >= 0.7 normalized) escalate directly to hunt
	if (Volume >= 0.7f && InInstigator != nullptr)
	{
		HuntTarget = InInstigator;
		TransitionToState(EBioAssetState::Hunting);

		// Report to OmniKernel — this triggers facility-wide threat escalation
		if (CachedOmniKernel != nullptr)
		{
			CachedOmniKernel->EvaluateFacilityState(Volume * 100.0f, ThermalSignature);
		}
	}
	else
	{
		InvestigationTarget = Location;
		TransitionToState(EBioAssetState::Investigating);

		if (AAIController* AIControl = Cast<AAIController>(GetController()))
		{
			AIControl->MoveToLocation(InvestigationTarget, 60.0f, false);
		}
	}
}

void AA_BioAsset::OnSeePawn(APawn* InPawn)
{
	if (bDeactivated || !bIsEntityActive || InPawn == nullptr)
	{
		return;
	}

	// Only target player characters — BioAssets ignore each other
	if (!InPawn->IsPlayerControlled())
	{
		return;
	}

	HuntTarget = InPawn;
	TransitionToState(EBioAssetState::Hunting);

	if (CachedOmniKernel != nullptr)
	{
		CachedOmniKernel->EvaluateFacilityState(90.0f, ThermalSignature);
	}
}

// ─────────────────────────────────────────────────────────────
// OmniKernel Interface
// ─────────────────────────────────────────────────────────────
void AA_BioAsset::InvestigateAcousticDisturbance(FVector Location)
{
	if (bDeactivated)
	{
		return;
	}

	if (CurrentState == EBioAssetState::Hunting)
	{
		// Already on a higher-priority task; don't interrupt
		return;
	}

	InvestigationTarget = Location;
	TransitionToState(EBioAssetState::Investigating);

	if (AAIController* AIControl = Cast<AAIController>(GetController()))
	{
		AIControl->MoveToLocation(InvestigationTarget, 50.0f, false);
	}
}

void AA_BioAsset::InitiateHuntSequence(APawn* TargetPawn)
{
	if (bDeactivated || TargetPawn == nullptr)
	{
		return;
	}

	HuntTarget = TargetPawn;
	TransitionToState(EBioAssetState::Hunting);
}

void AA_BioAsset::Deactivate()
{
	bDeactivated = true;
	bIsEntityActive = false;

	if (AAIController* AIControl = Cast<AAIController>(GetController()))
	{
		AIControl->StopMovement();
	}

	GetCharacterMovement()->DisableMovement();
	TransitionToState(EBioAssetState::Dormant);
}

// ─────────────────────────────────────────────────────────────
// State Transition — Central hub: sets speed, resets timer, broadcasts events
// ─────────────────────────────────────────────────────────────
void AA_BioAsset::TransitionToState(EBioAssetState NewState)
{
	if (CurrentState == NewState && NewState != EBioAssetState::Patrolling)
	{
		return;
	}

	CurrentState = NewState;
	TimeInCurrentState = 0.0f;

	float TargetSpeed = PatrolSpeed;

	switch (NewState)
	{
	case EBioAssetState::Dormant:
		TargetSpeed = 0.0f;
		break;
	case EBioAssetState::Patrolling:
		TargetSpeed = PatrolSpeed;
		PatrolDirectionTimer = 0.0f; // Force immediate new patrol target
		break;
	case EBioAssetState::Investigating:
		TargetSpeed = InvestigateSpeed;
		break;
	case EBioAssetState::Hunting:
		TargetSpeed = HuntSpeed;
		break;
	case EBioAssetState::Suppressing:
		TargetSpeed = InvestigateSpeed;
		// Sweep last known position on transition
		if (AAIController* AIControl = Cast<AAIController>(GetController()))
		{
			const FVector SweepTarget = HuntTarget != nullptr
				? HuntTarget->GetActorLocation()
				: InvestigationTarget;
			AIControl->MoveToLocation(SweepTarget, 80.0f, false);
		}
		break;
	}

	GetCharacterMovement()->MaxWalkSpeed = TargetSpeed;

	// Broadcast state change to Blueprint layer and any listeners
	OnStateChanged.Broadcast(NewState);
	OnBioAssetStateChanged(NewState);
}
