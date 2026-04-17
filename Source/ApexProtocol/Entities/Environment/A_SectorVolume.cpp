#include "A_SectorVolume.h"
#include "Components/BoxComponent.h"

AA_SectorVolume::AA_SectorVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	SectorBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("SectorBounds"));
	RootComponent = SectorBounds;
	SectorBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SectorBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
	SectorBounds->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SectorBounds->SetGenerateOverlapEvents(true);

	SectorIdentifier = NAME_None;
	BaseExposureRate = 0.0f;
	GasFloodExposureRate = 15.0f;
	SectorTemperature = 21.0f;
	bIsSafeRoom = false;
	bIsGasFlooded = false;
}

FName AA_SectorVolume::GetSectorIdentifier() const
{
	return SectorIdentifier;
}

float AA_SectorVolume::GetEnvironmentalExposureRate() const
{
	return bIsGasFlooded ? BaseExposureRate + GasFloodExposureRate : BaseExposureRate;
}

float AA_SectorVolume::GetSectorTemperature() const
{
	return SectorTemperature;
}

bool AA_SectorVolume::IsSafeRoom() const
{
	return bIsSafeRoom;
}

void AA_SectorVolume::SetGasFlooded(bool bInGasFlooded)
{
	bIsGasFlooded = bInGasFlooded;
}

void AA_SectorVolume::SetSafeRoomState(bool bInSafeRoom)
{
	bIsSafeRoom = bInSafeRoom;
}
