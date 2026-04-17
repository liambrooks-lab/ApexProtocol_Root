#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "AudioDirector.generated.h"

UCLASS()
class APEXPROTOCOL_API AAudioDirector : public AInfo
{
	GENERATED_BODY()

public:
	AAudioDirector();

	UFUNCTION(BlueprintCallable, Category = "Audio Architecture")
	void ApplyPsychoacousticFilters(float ToxicityLevel);

	UFUNCTION(BlueprintCallable, Category = "Audio Architecture")
	void ApplyFinalPurgeMix(bool bPurgeActive);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio Architecture")
	bool bHallucinationLayerActive;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio Architecture")
	bool bFinalPurgeMixActive;
};
