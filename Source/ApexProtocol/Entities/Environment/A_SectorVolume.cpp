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

	// Bind overlap delegates
	SectorBounds->OnComponentBeginOverlap.AddDynamic(this, &AA_SectorVolume::HandleBeginOverlap);
	SectorBounds->OnComponentEndOverlap.AddDynamic(this, &AA_SectorVolume::HandleEndOverlap);
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

bool AA_SectorVolume::IsGasFlooded() const
{
	return bIsGasFlooded;
}

void AA_SectorVolume::SetGasFlooded(bool bInGasFlooded)
{
	if (bIsGasFlooded != bInGasFlooded)
	{
		bIsGasFlooded = bInGasFlooded;

		// Increase sector temperature when gas floods — thermal byproduct
		if (bIsGasFlooded)
		{
			SectorTemperature += 8.0f;
		}
		else
		{
			SectorTemperature = FMath::Max(SectorTemperature - 8.0f, 21.0f);
		}

		OnGasFloodStateChanged(bIsGasFlooded);
	}
}

void AA_SectorVolume::SetSafeRoomState(bool bInSafeRoom)
{
	bIsSafeRoom = bInSafeRoom;
}

void AA_SectorVolume::SetSectorTemperature(float NewTemperature)
{
	SectorTemperature = FMath::Clamp(NewTemperature, -40.0f, 200.0f);
}

// ─────────────────────────────────────────────────────────────
// Overlap Handlers — Broadcast to any listeners (OmniKernel, etc.)
// ─────────────────────────────────────────────────────────────
void AA_SectorVolume::HandleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor != nullptr && OtherActor->IsA(APawn::StaticClass()))
	{
		APawn* OverlappingPawn = Cast<APawn>(OtherActor);
		if (OverlappingPawn != nullptr && OverlappingPawn->IsPlayerControlled())
		{
			OnPlayerEnteredSector.Broadcast(this, OtherActor);
		}
	}
}

void AA_SectorVolume::HandleEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor != nullptr && OtherActor->IsA(APawn::StaticClass()))
	{
		APawn* OverlappingPawn = Cast<APawn>(OtherActor);
		if (OverlappingPawn != nullptr && OverlappingPawn->IsPlayerControlled())
		{
			OnPlayerExitedSector.Broadcast(this, OtherActor);
		}
	}
}
