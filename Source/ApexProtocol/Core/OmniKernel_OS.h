#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "OmniKernel_OS.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOmniKernelBoolEvent, bool, bStateActive);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOmniKernelEvent);

UCLASS()
class APEXPROTOCOL_API AOmniKernel_OS : public AInfo
{
	GENERATED_BODY()

public:
	AOmniKernel_OS();

	UFUNCTION(BlueprintCallable, Category = "Unified Architecture")
	void EvaluateFacilityState(float PlayerAcousticLevel, float SectorTemperature);

	UFUNCTION(BlueprintCallable, Category = "Unified Architecture")
	void ExecuteSectorLockdown(FName SectorID);

	UFUNCTION(BlueprintCallable, Category = "Unified Architecture")
	void ReportHackingDisturbance(FVector TargetCoordinates, int32 ThreatLevel);

	UFUNCTION(BlueprintCallable, Category = "Unified Architecture")
	void TriggerSectorGasFlood(FName SectorID, bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Unified Architecture")
	void UnlockCoreBreachPath();

	UFUNCTION(BlueprintCallable, Category = "Unified Architecture")
	void BeginFinalPurgeSequence();

	UFUNCTION(BlueprintCallable, Category = "Unified Architecture")
	bool IsFinalPurgeActive() const;

	UFUNCTION(BlueprintCallable, Category = "Unified Architecture")
	void ShutdownOmniKernel();

	UFUNCTION(BlueprintCallable, Category = "Unified Architecture")
	bool IsOmniKernelShutdown() const;

	UPROPERTY(BlueprintAssignable, Category = "Unified Architecture")
	FOmniKernelBoolEvent OnFinalPurgeStateChangedEvent;

	UPROPERTY(BlueprintAssignable, Category = "Unified Architecture")
	FOmniKernelEvent OnCoreBreachPathUnlockedEvent;

	UPROPERTY(BlueprintAssignable, Category = "Unified Architecture")
	FOmniKernelBoolEvent OnOmniKernelShutdownStateChangedEvent;

	UFUNCTION(BlueprintImplementableEvent, Category = "Unified Architecture")
	void OnFinalPurgeStateChanged(bool bNewFinalPurgeState);

	UFUNCTION(BlueprintImplementableEvent, Category = "Unified Architecture")
	void OnCoreBreachPathUnlocked();

	UFUNCTION(BlueprintImplementableEvent, Category = "Unified Architecture")
	void OnOmniKernelShutdownStateChanged(bool bIsShutdown);

private:
	void DispatchBiologicalAssets(FVector TargetCoordinates, int32 ThreatLevel);

	TMap<FName, float> SectorPowerGrid;
	TMap<FName, float> SectorToxicityGrid;
	bool bFinalPurgeActive;
	bool bOmniKernelShutdown;
};
