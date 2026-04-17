#include "ApexGameMode.h"
#include "../Entities/Characters/A_PlayerCharacter.h"
#include "ApexPlayerController.h"

AApexGameMode::AApexGameMode()
{
	DefaultPawnClass = AA_PlayerCharacter::StaticClass();
	PlayerControllerClass = AApexPlayerController::StaticClass();
	LastSafeRoomSector = NAME_None;
	LastCheckpointTransform = FTransform::Identity;
	LastCheckpointToxicity = 0.0f;
	bHasCheckpointData = false;
	RequiredSubroutineShutdowns = 4;
	bCoreBreachSequenceStarted = false;
	bApexProtocolCompleted = false;
	bEscapeVectorActivated = false;
}

void AApexGameMode::RegisterSafeRoomEntry(FName SectorID)
{
	LastSafeRoomSector = SectorID;
}

void AApexGameMode::RegisterCheckpoint(const FTransform& CheckpointTransform, FName SectorID, float ToxicityLevel)
{
	LastSafeRoomSector = SectorID;
	LastCheckpointTransform = CheckpointTransform;
	LastCheckpointToxicity = ToxicityLevel;
	bHasCheckpointData = true;
}

FName AApexGameMode::GetLastSafeRoomSector() const
{
	return LastSafeRoomSector;
}

bool AApexGameMode::HasCheckpointData() const
{
	return bHasCheckpointData;
}

FTransform AApexGameMode::GetLastCheckpointTransform() const
{
	return LastCheckpointTransform;
}

float AApexGameMode::GetLastCheckpointToxicity() const
{
	return LastCheckpointToxicity;
}

void AApexGameMode::RegisterSubroutineShutdown(FName SectorID)
{
	if (!SectorID.IsNone())
	{
		ShutdownSectors.AddUnique(SectorID);
	}
}

int32 AApexGameMode::GetShutdownProgress() const
{
	return ShutdownSectors.Num();
}

int32 AApexGameMode::GetRequiredShutdownCount() const
{
	return RequiredSubroutineShutdowns;
}

bool AApexGameMode::HasCoreBreachAccess() const
{
	return ShutdownSectors.Num() >= RequiredSubroutineShutdowns;
}

int32 AApexGameMode::GetRemainingShutdownCount() const
{
	return FMath::Max(RequiredSubroutineShutdowns - ShutdownSectors.Num(), 0);
}

FString AApexGameMode::GetObjectiveReadout() const
{
	if (bApexProtocolCompleted)
	{
		return TEXT("PRIMARY OBJECTIVE: ESCAPE VECTOR CONFIRMED");
	}

	if (bEscapeVectorActivated)
	{
		return TEXT("FINAL OBJECTIVE: REACH THE EXTRACTION VECTOR");
	}

	if (bCoreBreachSequenceStarted)
	{
		return TEXT("FINAL OBJECTIVE: PURGE THE MAINFRAME CORE");
	}

	if (HasCoreBreachAccess())
	{
		return TEXT("PRIMARY OBJECTIVE: CORE BREACH AUTHORIZED");
	}

	return FString::Printf(
		TEXT("PRIMARY OBJECTIVE: SEVER %d MORE SUBROUTINE LINKS"),
		GetRemainingShutdownCount());
}

void AApexGameMode::BeginCoreBreachSequence()
{
	if (HasCoreBreachAccess() && !bCoreBreachSequenceStarted)
	{
		bCoreBreachSequenceStarted = true;
		OnCoreBreachSequenceStartedEvent.Broadcast();
		OnCoreBreachSequenceStarted();
	}
}

bool AApexGameMode::IsCoreBreachSequenceStarted() const
{
	return bCoreBreachSequenceStarted;
}

void AApexGameMode::CompleteApexProtocol()
{
	if (!bApexProtocolCompleted)
	{
		bApexProtocolCompleted = true;
		OnApexProtocolCompletedEvent.Broadcast();
		OnApexProtocolCompleted();
	}
}

bool AApexGameMode::IsApexProtocolCompleted() const
{
	return bApexProtocolCompleted;
}

void AApexGameMode::ActivateEscapeVector()
{
	if (!bEscapeVectorActivated)
	{
		bEscapeVectorActivated = true;
		OnEscapeVectorActivatedEvent.Broadcast();
		OnEscapeVectorActivated();
	}
}

bool AApexGameMode::IsEscapeVectorActivated() const
{
	return bEscapeVectorActivated;
}
