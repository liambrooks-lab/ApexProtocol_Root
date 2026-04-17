#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ApexGameMode.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FApexProgressionEvent);

UCLASS()
class APEXPROTOCOL_API AApexGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AApexGameMode();

	UFUNCTION(BlueprintCallable, Category = "Progression")
	void RegisterSafeRoomEntry(FName SectorID);

	UFUNCTION(BlueprintCallable, Category = "Progression")
	void RegisterCheckpoint(const FTransform& CheckpointTransform, FName SectorID, float ToxicityLevel);

	UFUNCTION(BlueprintCallable, Category = "Progression")
	FName GetLastSafeRoomSector() const;

	UFUNCTION(BlueprintCallable, Category = "Progression")
	bool HasCheckpointData() const;

	UFUNCTION(BlueprintCallable, Category = "Progression")
	FTransform GetLastCheckpointTransform() const;

	UFUNCTION(BlueprintCallable, Category = "Progression")
	float GetLastCheckpointToxicity() const;

	UFUNCTION(BlueprintCallable, Category = "Progression")
	void RegisterSubroutineShutdown(FName SectorID);

	UFUNCTION(BlueprintCallable, Category = "Progression")
	int32 GetShutdownProgress() const;

	UFUNCTION(BlueprintCallable, Category = "Progression")
	int32 GetRequiredShutdownCount() const;

	UFUNCTION(BlueprintCallable, Category = "Progression")
	bool HasCoreBreachAccess() const;

	UFUNCTION(BlueprintCallable, Category = "Progression")
	int32 GetRemainingShutdownCount() const;

	UFUNCTION(BlueprintCallable, Category = "Progression")
	FString GetObjectiveReadout() const;

	UFUNCTION(BlueprintCallable, Category = "Progression")
	void BeginCoreBreachSequence();

	UFUNCTION(BlueprintCallable, Category = "Progression")
	bool IsCoreBreachSequenceStarted() const;

	UFUNCTION(BlueprintCallable, Category = "Progression")
	void CompleteApexProtocol();

	UFUNCTION(BlueprintCallable, Category = "Progression")
	bool IsApexProtocolCompleted() const;

	UFUNCTION(BlueprintCallable, Category = "Progression")
	void ActivateEscapeVector();

	UFUNCTION(BlueprintCallable, Category = "Progression")
	bool IsEscapeVectorActivated() const;

	UPROPERTY(BlueprintAssignable, Category = "Progression")
	FApexProgressionEvent OnCoreBreachSequenceStartedEvent;

	UPROPERTY(BlueprintAssignable, Category = "Progression")
	FApexProgressionEvent OnEscapeVectorActivatedEvent;

	UPROPERTY(BlueprintAssignable, Category = "Progression")
	FApexProgressionEvent OnApexProtocolCompletedEvent;

	UFUNCTION(BlueprintImplementableEvent, Category = "Progression")
	void OnCoreBreachSequenceStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Progression")
	void OnEscapeVectorActivated();

	UFUNCTION(BlueprintImplementableEvent, Category = "Progression")
	void OnApexProtocolCompleted();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression")
	FName LastSafeRoomSector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression")
	FTransform LastCheckpointTransform;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression")
	float LastCheckpointToxicity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression")
	bool bHasCheckpointData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression")
	TArray<FName> ShutdownSectors;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression")
	int32 RequiredSubroutineShutdowns;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression")
	bool bCoreBreachSequenceStarted;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression")
	bool bApexProtocolCompleted;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression")
	bool bEscapeVectorActivated;
};
