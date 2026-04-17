#include "A_ApexCharacterEntity.h"

AApexCharacterEntity::AApexCharacterEntity()
{
	PrimaryActorTick.bCanEverTick = true;

	bIsEntityActive = true;
	ThermalSignature = 37.0f;
}

void AApexCharacterEntity::BeginPlay()
{
	Super::BeginPlay();
}

void AApexCharacterEntity::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
