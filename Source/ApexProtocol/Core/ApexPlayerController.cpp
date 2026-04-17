#include "ApexPlayerController.h"
#include "../Entities/Characters/A_PlayerCharacter.h"

AApexPlayerController::AApexPlayerController()
{
	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;
}

void AApexPlayerController::BeginPlay()
{
	Super::BeginPlay();

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
}

void AApexPlayerController::TriggerCheckpointRecovery()
{
	if (AA_PlayerCharacter* ApexCharacter = Cast<AA_PlayerCharacter>(GetPawn()))
	{
		ApexCharacter->RestoreFromCheckpoint();
	}
}
