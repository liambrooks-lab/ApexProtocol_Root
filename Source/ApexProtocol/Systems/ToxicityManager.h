#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ToxicityManager.generated.h"

UCLASS(ClassGroup=(Custom), BlueprintSpawnableComponent)
class APEXPROTOCOL_API UToxicityManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UToxicityManager();

	FORCEINLINE float GetCurrentToxicity() const
	{
		return CurrentToxicity;
	}

	FORCEINLINE float GetEnvironmentalExposureRate() const
	{
		return EnvironmentalExposureRate;
	}

	UFUNCTION(BlueprintCallable, Category = "Survival Systems")
	void ApplyEnvironmentalExposure(float ExposureRate);

	UFUNCTION(BlueprintCallable, Category = "Survival Systems")
	void AdministerAntidote(float NeutralizationFactor);

	UFUNCTION(BlueprintCallable, Category = "Survival Systems")
	void SetEnvironmentalExposureRate(float ExposureRate);

	UFUNCTION(BlueprintCallable, Category = "Survival Systems")
	void SetCurrentToxicity(float NewToxicity);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Biological Math")
	float CurrentToxicity;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Biological Math")
	float MetabolicClearanceRate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Biological Math")
	float EnvironmentalExposureRate;

	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};
