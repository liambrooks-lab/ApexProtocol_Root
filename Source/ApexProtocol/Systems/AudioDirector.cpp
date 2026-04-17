#include "AudioDirector.h"

AAudioDirector::AAudioDirector()
{
	bHallucinationLayerActive = false;
	bFinalPurgeMixActive = false;
}

void AAudioDirector::ApplyPsychoacousticFilters(float ToxicityLevel)
{
	if (ToxicityLevel > 80.0f)
	{
		if (!bHallucinationLayerActive)
		{
			bHallucinationLayerActive = true;
			UE_LOG(LogTemp, Log, TEXT("High-toxicity psychoacoustic filter stage: muffle highs, amplify heartbeat bass, and trigger hallucination whispers."));
		}
	}
	else if (bHallucinationLayerActive)
	{
		bHallucinationLayerActive = false;
	}
}

void AAudioDirector::ApplyFinalPurgeMix(bool bPurgeActive)
{
	if (bPurgeActive)
	{
		if (!bFinalPurgeMixActive)
		{
			bFinalPurgeMixActive = true;
			UE_LOG(LogTemp, Warning, TEXT("Final purge mix engaged: global alarms, low-frequency dread bed, and escalating system sirens."));
		}
	}
	else if (bFinalPurgeMixActive)
	{
		bFinalPurgeMixActive = false;
	}
}
