#include "A_BioAsset.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"

AA_BioAsset::AA_BioAsset()
{
	PrimaryActorTick.bCanEverTick = true;
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	ThermalSignature = 45.0f;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->MaxWalkSpeed = 220.0f;
}

void AA_BioAsset::InvestigateAcousticDisturbance(FVector Location)
{
	if (AAIController* AIControl = Cast<AAIController>(GetController()))
	{
		AIControl->MoveToLocation(Location, 50.0f);
	}
}
