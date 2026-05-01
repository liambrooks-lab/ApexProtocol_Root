#include "ApexPlayerController.h"
#include "../Entities/Characters/A_PlayerCharacter.h"
#include "Camera/PlayerCameraManager.h"
#include "TimerManager.h"

AApexPlayerController::AApexPlayerController()
{
	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;

	CollapseFadeDuration = 1.2f;
	CollapseHoldDuration = 1.5f;
}

void AApexPlayerController::BeginPlay()
{
	Super::BeginPlay();

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
}

// ============================================================
// TriggerCheckpointRecovery
//
// Sequence:
//   1. Fade camera to black (CollapseFadeDuration)
//   2. Hold at black (CollapseHoldDuration) — simulates unconsciousness
//   3. Execute checkpoint restore (teleport, reset toxicity)
//   4. Fade camera back in (CollapseFadeDuration)
// ============================================================
void AApexPlayerController::TriggerCheckpointRecovery()
{
	// Disable player input during recovery to prevent movement during fade
	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);

	// Fade camera to black
	if (PlayerCameraManager != nullptr)
	{
		PlayerCameraManager->StartCameraFade(0.0f, 1.0f, CollapseFadeDuration, FLinearColor::Black, false, true);
	}

	OnCheckpointRecoveryStarted();

	// Schedule the actual restore after fade + hold duration
	const float TotalDelay = CollapseFadeDuration + CollapseHoldDuration;

	if (GetWorld() != nullptr)
	{
		GetWorld()->GetTimerManager().SetTimer(
			RecoveryTimerHandle,
			this,
			&AApexPlayerController::ExecuteCheckpointRestore,
			TotalDelay,
			false
		);
	}
}

void AApexPlayerController::ExecuteCheckpointRestore()
{
	// Perform the actual checkpoint restoration
	if (AA_PlayerCharacter* ApexCharacter = Cast<AA_PlayerCharacter>(GetPawn()))
	{
		ApexCharacter->RestoreFromCheckpoint();
	}

	// Fade camera back in
	if (PlayerCameraManager != nullptr)
	{
		PlayerCameraManager->StartCameraFade(1.0f, 0.0f, CollapseFadeDuration, FLinearColor::Black, false, false);
	}

	// Re-enable input
	SetIgnoreMoveInput(false);
	SetIgnoreLookInput(false);

	OnCheckpointRecoveryCompleted();
}
