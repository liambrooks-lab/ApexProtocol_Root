#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_ApexEntity.generated.h"

UCLASS(Abstract)
class APEXPROTOCOL_API AApexEntity : public AActor
{
	GENERATED_BODY()

public:
	AApexEntity();

	FORCEINLINE float GetThermalSignature() const
	{
		return ThermalSignature;
	}

	FORCEINLINE bool IsNodeActive() const
	{
		return bIsActiveNode;
	}

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entity State")
	bool bIsActiveNode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entity State")
	float ThermalSignature;

	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
};
