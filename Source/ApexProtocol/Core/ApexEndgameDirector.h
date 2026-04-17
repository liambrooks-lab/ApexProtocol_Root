#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "ApexEndgameDirector.generated.h"

class AApexGameMode;
class AOmniKernel_OS;

UCLASS()
class APEXPROTOCOL_API AApexEndgameDirector : public AInfo
{
	GENERATED_BODY()

public:
	AApexEndgameDirector();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleCoreBreachStarted();

	UFUNCTION()
	void HandleEscapeVectorActivated();

	UFUNCTION()
	void HandleApexProtocolCompleted();

	UFUNCTION()
	void HandleFinalPurgeChanged(bool bPurgeActive);

	UFUNCTION()
	void HandleCoreBreachPathUnlocked();

	UFUNCTION()
	void HandleOmniKernelShutdownChanged(bool bIsShutdown);

	UFUNCTION(BlueprintImplementableEvent, Category = "Endgame")
	void OnCoreBreachSequenceStarted_BP();

	UFUNCTION(BlueprintImplementableEvent, Category = "Endgame")
	void OnEscapeVectorActivated_BP();

	UFUNCTION(BlueprintImplementableEvent, Category = "Endgame")
	void OnApexProtocolCompleted_BP();

	UFUNCTION(BlueprintImplementableEvent, Category = "Endgame")
	void OnFinalPurgeStateChanged_BP(bool bPurgeActive);

	UFUNCTION(BlueprintImplementableEvent, Category = "Endgame")
	void OnCoreBreachPathUnlocked_BP();

	UFUNCTION(BlueprintImplementableEvent, Category = "Endgame")
	void OnOmniKernelShutdown_BP(bool bIsShutdown);

	UPROPERTY(Transient)
	TObjectPtr<AApexGameMode> CachedGameMode;

	UPROPERTY(Transient)
	TObjectPtr<AOmniKernel_OS> CachedOmniKernel;
};
