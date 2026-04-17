#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TerminalWidget_Native.generated.h"

UCLASS()
class APEXPROTOCOL_API UTerminalWidget_Native : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "Terminal Interface")
	bool AttemptNodeOverride(FString CommandSyntax);

	UFUNCTION(BlueprintCallable, Category = "Terminal Interface")
	void SetTargetNode(class AA_InteractiveNode* InTargetNode);

	UFUNCTION(BlueprintCallable, Category = "Terminal Interface")
	void SetStatusReadout(const FString& StatusText);

	UFUNCTION(BlueprintCallable, Category = "Terminal Interface")
	void SetObjectiveReadout(const FString& ObjectiveText);

	UFUNCTION(BlueprintCallable, Category = "Terminal Interface")
	void SetTelemetryReadout(const FString& TelemetryText);

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* ReadoutDisplay;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* ObjectiveDisplay;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* TelemetryDisplay;

	UPROPERTY(Transient)
	TObjectPtr<class AA_InteractiveNode> TargetNode;

	void UpdateDataVisualizer(float SignalStrength);
};
