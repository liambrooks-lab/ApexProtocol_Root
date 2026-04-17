#include "ToxicityManager.h"
#include "Math/UnrealMathUtility.h"

UToxicityManager::UToxicityManager()
{
	PrimaryComponentTick.bCanEverTick = true;

	CurrentToxicity = 0.0f;
	MetabolicClearanceRate = 0.05f;
	EnvironmentalExposureRate = 0.0f;
}

void UToxicityManager::BeginPlay()
{
	Super::BeginPlay();
}

void UToxicityManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (CurrentToxicity > 0.0f || EnvironmentalExposureRate > 0.0f)
	{
		const float ToxicityDelta = (EnvironmentalExposureRate - (MetabolicClearanceRate * CurrentToxicity)) * DeltaTime;
		CurrentToxicity = FMath::Clamp(CurrentToxicity + ToxicityDelta, 0.0f, 100.0f);
	}
}

void UToxicityManager::ApplyEnvironmentalExposure(float ExposureRate)
{
	CurrentToxicity = FMath::Clamp(CurrentToxicity + ExposureRate, 0.0f, 100.0f);
}

void UToxicityManager::AdministerAntidote(float NeutralizationFactor)
{
	CurrentToxicity = FMath::Clamp(CurrentToxicity - NeutralizationFactor, 0.0f, 100.0f);
}

void UToxicityManager::SetEnvironmentalExposureRate(float ExposureRate)
{
	EnvironmentalExposureRate = FMath::Max(0.0f, ExposureRate);
}

void UToxicityManager::SetCurrentToxicity(float NewToxicity)
{
	CurrentToxicity = FMath::Clamp(NewToxicity, 0.0f, 100.0f);
}
