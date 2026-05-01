#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_SectorVolume.generated.h"

class UBoxComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSectorVolumeOverlapEvent, AA_SectorVolume*, SectorVolume, AActor*, OverlappingActor);

// ============================================================
// AA_SectorVolume — Environmental Zone Definition
//
// Defines a spatial sector with environmental properties:
//   • Base toxin exposure rate
//   • Gas flood toxin exposure rate (stackable via OmniKernel)
//   • Sector temperature (thermal signature input)
//   • Safe room flag (checkpoint registration, antidote drip)
//
// Broadcasts overlap events for OmniKernel sector tracking.
// ============================================================
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
	bool IsGasFlooded() const;

	UFUNCTION(BlueprintCallable, Category = "Sector Systems")
	void SetGasFlooded(bool bInGasFlooded);

	UFUNCTION(BlueprintCallable, Category = "Sector Systems")
	void SetSafeRoomState(bool bInSafeRoom);

	/** Dynamically modify sector temperature (e.g., fire event, OmniKernel thermal escalation). */
	UFUNCTION(BlueprintCallable, Category = "Sector Systems")
	void SetSectorTemperature(float NewTemperature);

	// ─── Events ────────────────────────────────────────────────────────
	UPROPERTY(BlueprintAssignable, Category = "Sector Systems")
	FSectorVolumeOverlapEvent OnPlayerEnteredSector;

	UPROPERTY(BlueprintAssignable, Category = "Sector Systems")
	FSectorVolumeOverlapEvent OnPlayerExitedSector;

	UFUNCTION(BlueprintImplementableEvent, Category = "Sector Systems")
	void OnGasFloodStateChanged(bool bNewGasFloodState);

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

private:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
