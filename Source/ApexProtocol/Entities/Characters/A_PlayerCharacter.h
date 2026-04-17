#pragma once

#include "CoreMinimal.h"
#include "../Base/A_ApexCharacterEntity.h"
#include "A_PlayerCharacter.generated.h"

class AAudioDirector;
class UCameraComponent;
class USpringArmComponent;
class UTerminalWidget_Native;
class UToxicityManager;
class AOmniKernel_OS;
class AA_SectorVolume;
class AA_InteractiveNode;

UCLASS()
class APEXPROTOCOL_API AA_PlayerCharacter : public AApexCharacterEntity
{
	GENERATED_BODY()

public:
	AA_PlayerCharacter();

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Recovery")
	void RestoreFromCheckpoint();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Systems")
	UToxicityManager* BiologicalSystems;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Terminal")
	bool bTerminalVisible;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Terminal")
	TSubclassOf<UTerminalWidget_Native> TerminalWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Terminal")
	TObjectPtr<UTerminalWidget_Native> ActiveTerminalWidget;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	float InteractionRange;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Senses")
	float AcousticOutputLevel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Senses")
	float CurrentSectorTemperature;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Survival")
	bool bInSafeRoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Survival")
	FName CurrentSectorID;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Survival")
	FName LastCheckpointSectorID;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Survival")
	bool bBiologicalCollapseTriggered;

	UPROPERTY(Transient)
	TObjectPtr<AA_InteractiveNode> FocusedNode;

	UPROPERTY(Transient)
	TObjectPtr<AA_SectorVolume> ActiveSectorVolume;

	UPROPERTY(Transient)
	TObjectPtr<AOmniKernel_OS> CachedOmniKernel;

	UPROPERTY(Transient)
	TObjectPtr<AAudioDirector> CachedAudioDirector;

	virtual void BeginPlay() override;

	void HandleBiologicalCollapse();
	void UpdateTerminalContextReadout() const;
	void RegisterSafeRoomCheckpoint();
	void UpdateSectorState(float DeltaTime);
	AA_SectorVolume* ResolveCurrentSectorVolume() const;
	AA_InteractiveNode* TraceInteractiveNode() const;
	void AcquireFocusedNode();
	void Interact();
	void VoiceOverride();
	void RefreshAudioState() const;
	void ReportAcousticEvent(float Loudness);
	void MoveForward(float Value);
	void MoveRight(float Value);
	void TurnAtRate(float Value);
	void LookUpAtRate(float Value);
	void ToggleTerminal();
};
