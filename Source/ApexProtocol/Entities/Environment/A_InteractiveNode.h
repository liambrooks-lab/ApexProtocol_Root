#pragma once

#include "CoreMinimal.h"
#include "../Base/A_ApexEntity.h"
#include "A_InteractiveNode.generated.h"

UENUM(BlueprintType)
enum class EApexNodeFunction : uint8
{
	Door UMETA(DisplayName = "Door"),
	GasControl UMETA(DisplayName = "Gas Control"),
	SafeRoomRelay UMETA(DisplayName = "Safe Room Relay"),
	SecurityLock UMETA(DisplayName = "Security Lock"),
	CoreBreachConsole UMETA(DisplayName = "Core Breach Console"),
	MainframeCore UMETA(DisplayName = "Mainframe Core"),
	EscapeVectorRelay UMETA(DisplayName = "Escape Vector Relay"),
	Terminal UMETA(DisplayName = "Terminal")
};

UCLASS()
class APEXPROTOCOL_API AA_InteractiveNode : public AApexEntity
{
	GENERATED_BODY()

public:
	AA_InteractiveNode();

	UFUNCTION(BlueprintCallable, Category = "Node Systems")
	void ReceiveHackPayload(FString SyntaxCommand);

	UFUNCTION(BlueprintCallable, Category = "Node Systems")
	bool ExecuteOverrideCommand(const FString& SyntaxCommand);

	UFUNCTION(BlueprintCallable, Category = "Node Systems")
	FName GetNodeIdentifier() const;

	UFUNCTION(BlueprintCallable, Category = "Node Systems")
	FName GetSectorIdentifier() const;

	UFUNCTION(BlueprintCallable, Category = "Node Systems")
	FName GetControlledSectorIdentifier() const;

	UFUNCTION(BlueprintCallable, Category = "Node Systems")
	EApexNodeFunction GetNodeFunction() const;

	UFUNCTION(BlueprintCallable, Category = "Node Systems")
	void SetNodeOperationalState(bool bInIsActive);

	UFUNCTION(BlueprintCallable, Category = "Node Systems")
	bool IsDoorOpen() const;

	UFUNCTION(BlueprintCallable, Category = "Node Systems")
	FString GetNodeStatusReadout() const;

	UFUNCTION(BlueprintCallable, Category = "Node Systems")
	bool IsSubroutineShutdownNode() const;

	UFUNCTION(BlueprintCallable, Category = "Node Systems")
	bool HasCompletedSubroutineShutdown() const;

	UFUNCTION(BlueprintCallable, Category = "Node Systems")
	bool RequiresCoreBreachAccess() const;

	UFUNCTION(BlueprintCallable, Category = "Node Systems")
	bool HasCoreBreachAuthorization() const;

	UFUNCTION(BlueprintCallable, Category = "Node Systems")
	void SetCoreBreachAuthorized(bool bInAuthorized);

	UFUNCTION(BlueprintImplementableEvent, Category = "Node Systems")
	void OnDoorStateChanged(bool bNewDoorState);

	UFUNCTION(BlueprintImplementableEvent, Category = "Node Systems")
	void OnGasFloodStateChanged(bool bNewGasFloodState);

	UFUNCTION(BlueprintImplementableEvent, Category = "Node Systems")
	void OnOperationalStateChanged(bool bNewOperationalState);

	UFUNCTION(BlueprintImplementableEvent, Category = "Node Systems")
	void OnOverrideExecuted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Node Systems")
	void OnCoreAccessStateChanged(bool bNewAuthorizedState);

	UFUNCTION(BlueprintImplementableEvent, Category = "Node Systems")
	void OnMainframePurged();

	UFUNCTION(BlueprintImplementableEvent, Category = "Node Systems")
	void OnEscapeVectorReleased();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Node Systems")
	FName NodeIdentifier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Node Systems")
	FName SectorIdentifier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Node Systems")
	FName ControlledSectorIdentifier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Node Systems")
	EApexNodeFunction NodeFunction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Node Systems")
	bool bDoorOpen;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Node Systems")
	bool bGasFloodEnabled;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Node Systems")
	bool bTriggersSubroutineShutdown;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Node Systems")
	bool bSubroutineShutdownCompleted;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Node Systems")
	bool bRequiresCoreBreachAccess;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Node Systems")
	bool bCoreBreachAuthorized;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Node Systems")
	bool bMainframePurged;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Node Systems")
	bool bEscapeVectorReleased;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Node Systems")
	FString RequiredOverrideSyntax;
};
