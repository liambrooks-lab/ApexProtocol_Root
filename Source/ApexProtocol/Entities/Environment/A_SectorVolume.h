#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_SectorVolume.generated.h"

class UBoxComponent;

UCLASS()
class APEXPROTOCOL_API AA_SectorVolume : public AActor
{
	GENERATED_BODY()

public:
	AA_SectorVolume();

	UFUNCTION(BlueprintCallable, Category = "Sector Systems")
	FName GetSectorIdentifier() const;

	UFUNCTION(BlueprintCallable, Category = "Sector Systems")
	float GetEnvironmentalExposureRate() const;

	UFUNCTION(BlueprintCallable, Category = "Sector Systems")
	float GetSectorTemperature() const;

	UFUNCTION(BlueprintCallable, Category = "Sector Systems")
	bool IsSafeRoom() const;

	UFUNCTION(BlueprintCallable, Category = "Sector Systems")
	void SetGasFlooded(bool bInGasFlooded);

	UFUNCTION(BlueprintCallable, Category = "Sector Systems")
	void SetSafeRoomState(bool bInSafeRoom);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sector Systems")
	TObjectPtr<UBoxComponent> SectorBounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sector Systems")
	FName SectorIdentifier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sector Systems")
	float BaseExposureRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sector Systems")
	float GasFloodExposureRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sector Systems")
	float SectorTemperature;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sector Systems")
	bool bIsSafeRoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sector Systems")
	bool bIsGasFlooded;
};
