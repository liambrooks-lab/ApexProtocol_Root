#pragma once

#include "CoreMinimal.h"
#include "../Base/A_ApexCharacterEntity.h"
#include "A_BioAsset.generated.h"

UCLASS()
class APEXPROTOCOL_API AA_BioAsset : public AApexCharacterEntity
{
	GENERATED_BODY()

public:
	AA_BioAsset();

	UFUNCTION(BlueprintCallable, Category = "BioAsset AI")
	void InvestigateAcousticDisturbance(FVector Location);
};
