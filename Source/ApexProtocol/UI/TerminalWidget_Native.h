#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TerminalWidget_Native.generated.h"

class AA_InteractiveNode;
class UTextBlock;

// ============================================================
// UTerminalWidget_Native — Diegetic Holo-Terminal Interface
//
// 100% real-time, dark-themed terminal overlay.
// Supports syntax-based hacking (command buffer) without pausing gameplay.
// ============================================================
UCLASS()
class APEXPROTOCOL_API UTerminalWidget_Native : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	// ─── Node Override Interface ───────────────────────────────────────
	/** Attempts to execute a command override on the current TargetNode.
	 *  @return true if the override was accepted by the node. */
	UFUNCTION(BlueprintCallable, Category = "Terminal Interface")
	bool AttemptNodeOverride(FString CommandSyntax);

	/** Sets the active target node for the terminal interface. */
	UFUNCTION(BlueprintCallable, Category = "Terminal Interface")
	void SetTargetNode(AA_InteractiveNode* InTargetNode);

	// ─── Display Readouts ──────────────────────────────────────────────
	UFUNCTION(BlueprintCallable, Category = "Terminal Interface")
	void SetStatusReadout(const FString& StatusText);

	UFUNCTION(BlueprintCallable, Category = "Terminal Interface")
	void SetObjectiveReadout(const FString& ObjectiveText);

	UFUNCTION(BlueprintCallable, Category = "Terminal Interface")
	void SetTelemetryReadout(const FString& TelemetryText);

	// ─── Command Buffer ─────────────────────────────────────────────────
	/** Appends a character to the pending command buffer (syntax-based hacking input). */
	UFUNCTION(BlueprintCallable, Category = "Terminal Interface")
	void AppendCommandCharacter(const FString& Character);

	/** Submits the current command buffer as a hack payload to the TargetNode. */
	UFUNCTION(BlueprintCallable, Category = "Terminal Interface")
	void SubmitCommand();

	/** Clears the command buffer and readout. */
	UFUNCTION(BlueprintCallable, Category = "Terminal Interface")
	void ClearCommandBuffer();

	/** Returns the current contents of the command buffer. */
	UFUNCTION(BlueprintPure, Category = "Terminal Interface")
	FORCEINLINE FString GetCommandBuffer() const { return PendingCommandBuffer; }

	// ─── Blueprint Events ──────────────────────────────────────────────
	/** Fires after a command is submitted — used for terminal UI animation/effects. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Terminal Interface")
	void OnCommandResult(bool bSuccess, const FString& ResultText);

	/** Fires when the data visualizer signal strength changes. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Terminal Interface")
	void OnDataVisualizerUpdated(float NormalizedSignal);

	/** Fires when a new target node is linked. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Terminal Interface")
	void OnTargetNodeLinked(AA_InteractiveNode* LinkedNode);

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ReadoutDisplay;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ObjectiveDisplay;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TelemetryDisplay;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CommandBufferDisplay;

	UPROPERTY(Transient)
	TObjectPtr<AA_InteractiveNode> TargetNode;

	/** Accumulated command syntax characters. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Terminal Interface")
	FString PendingCommandBuffer;

	void UpdateDataVisualizer(float SignalStrength);

	/** Max characters allowed in command buffer. */
	static constexpr int32 MaxCommandLength = 128;
};
