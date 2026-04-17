#include "A_ApexEntity.h"

AApexEntity::AApexEntity()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f;

	bIsActiveNode = true;
	ThermalSignature = 37.0f;
}

void AApexEntity::BeginPlay()
{
	Super::BeginPlay();
}

void AApexEntity::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
