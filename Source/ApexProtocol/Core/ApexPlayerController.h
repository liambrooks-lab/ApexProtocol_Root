#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ApexPlayerController.generated.h"

UCLASS()
class APEXPROTOCOL_API AApexPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AApexPlayerController();

	UFUNCTION(BlueprintCallable, Category = "Recovery")
	void TriggerCheckpointRecovery();

protected:
	virtual void BeginPlay() override;
};
