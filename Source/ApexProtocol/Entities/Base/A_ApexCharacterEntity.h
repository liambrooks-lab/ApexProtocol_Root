#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "A_ApexCharacterEntity.generated.h"

UCLASS(Abstract)
class APEXPROTOCOL_API AApexCharacterEntity : public ACharacter
{
	GENERATED_BODY()

public:
	AApexCharacterEntity();

	FORCEINLINE float GetThermalSignature() const
	{
		return ThermalSignature;
	}

	FORCEINLINE bool IsEntityActive() const
	{
		return bIsEntityActive;
	}

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entity State")
	bool bIsEntityActive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entity State")
	float ThermalSignature;

	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
};
