#include "TerminalWidget_Native.h"
#include "Components/TextBlock.h"
#include "../Entities/Environment/A_InteractiveNode.h"

// ============================================================
// NativeConstruct — Initialize terminal readouts
// ============================================================
void UTerminalWidget_Native::NativeConstruct()
{
	Super::NativeConstruct();

	PendingCommandBuffer.Empty();
	SetStatusReadout(TEXT("OMNI-KERNEL TERMINAL: AWAITING INPUT..."));
	SetObjectiveReadout(TEXT("PRIMARY OBJECTIVE: LINK NOT ESTABLISHED"));
	SetTelemetryReadout(TEXT("TELEMETRY: SIGNAL NOISE FLOOR STABLE"));

	if (CommandBufferDisplay != nullptr)
	{
		CommandBufferDisplay->SetText(FText::FromString(TEXT("> _")));
	}
}

// ============================================================
// AttemptNodeOverride — Execute syntax command against TargetNode
// ============================================================
bool UTerminalWidget_Native::AttemptNodeOverride(FString CommandSyntax)
{
	if (TargetNode == nullptr)
	{
		SetStatusReadout(TEXT("ERROR: NO TARGET NODE LINKED"));
		UpdateDataVisualizer(0.0f);
		OnCommandResult(false, TEXT("NO TARGET NODE LINKED"));
		return false;
	}

	if (!TargetNode->IsNodeActive())
	{
		SetStatusReadout(FString::Printf(TEXT("%s | NODE OFFLINE"), *TargetNode->GetNodeIdentifier().ToString()));
		UpdateDataVisualizer(10.0f);
		OnCommandResult(false, TEXT("NODE OFFLINE"));
		return false;
	}

	if (TargetNode->ExecuteOverrideCommand(CommandSyntax))
	{
		const FString ResultText = FString::Printf(TEXT("%s | %s"),
			*TargetNode->GetNodeIdentifier().ToString(),
			*TargetNode->GetNodeStatusReadout());
		SetStatusReadout(ResultText);
		UpdateDataVisualizer(100.0f);
		OnCommandResult(true, ResultText);
		return true;
	}

	const FString FailText = FString::Printf(TEXT("%s | OVERRIDE REJECTED: SYNTAX MISMATCH"),
		*TargetNode->GetNodeIdentifier().ToString());
	SetStatusReadout(FailText);
	UpdateDataVisualizer(20.0f);
	OnCommandResult(false, FailText);
	return false;
}

// ============================================================
// SetTargetNode — Link to an interactive node
// ============================================================
void UTerminalWidget_Native::SetTargetNode(AA_InteractiveNode* InTargetNode)
{
	TargetNode = InTargetNode;

	const FString TargetLabel = TargetNode != nullptr ? TargetNode->GetNodeIdentifier().ToString() : TEXT("NO NODE");
	const FString Status = TargetNode != nullptr ? TargetNode->GetNodeStatusReadout() : TEXT("NO ACTIVE LINK");
	SetStatusReadout(FString::Printf(TEXT("TARGET NODE: %s | %s"), *TargetLabel, *Status));

	OnTargetNodeLinked(InTargetNode);
}

// ============================================================
// Command Buffer — Syntax-based hacking input
// ============================================================
void UTerminalWidget_Native::AppendCommandCharacter(const FString& Character)
{
	if (PendingCommandBuffer.Len() >= MaxCommandLength)
	{
		return;
	}

	PendingCommandBuffer.Append(Character);

	if (CommandBufferDisplay != nullptr)
	{
		CommandBufferDisplay->SetText(FText::FromString(FString::Printf(TEXT("> %s_"), *PendingCommandBuffer)));
	}
}

void UTerminalWidget_Native::SubmitCommand()
{
	if (PendingCommandBuffer.IsEmpty())
	{
		SetStatusReadout(TEXT("EMPTY COMMAND BUFFER — NO PAYLOAD TRANSMITTED"));
		OnCommandResult(false, TEXT("EMPTY COMMAND"));
		return;
	}

	const FString SubmittedCommand = PendingCommandBuffer;
	PendingCommandBuffer.Empty();

	if (CommandBufferDisplay != nullptr)
	{
		CommandBufferDisplay->SetText(FText::FromString(TEXT("> _")));
	}

	// Route the command through the standard override pipeline
	AttemptNodeOverride(SubmittedCommand);
}

void UTerminalWidget_Native::ClearCommandBuffer()
{
	PendingCommandBuffer.Empty();

	if (CommandBufferDisplay != nullptr)
	{
		CommandBufferDisplay->SetText(FText::FromString(TEXT("> _")));
	}
}

// ============================================================
// Display Readout Setters
// ============================================================
void UTerminalWidget_Native::SetStatusReadout(const FString& StatusText)
{
	if (ReadoutDisplay != nullptr)
	{
		ReadoutDisplay->SetText(FText::FromString(StatusText));
	}
}

void UTerminalWidget_Native::SetObjectiveReadout(const FString& ObjectiveText)
{
	if (ObjectiveDisplay != nullptr)
	{
		ObjectiveDisplay->SetText(FText::FromString(ObjectiveText));
	}
}

void UTerminalWidget_Native::SetTelemetryReadout(const FString& TelemetryText)
{
	if (TelemetryDisplay != nullptr)
	{
		TelemetryDisplay->SetText(FText::FromString(TelemetryText));
	}
}

// ============================================================
// UpdateDataVisualizer — Normalizes signal and fires BP event
// ============================================================
void UTerminalWidget_Native::UpdateDataVisualizer(float SignalStrength)
{
	const float NormalizedSignal = FMath::Clamp(SignalStrength / 100.0f, 0.0f, 1.0f);
	OnDataVisualizerUpdated(NormalizedSignal);
}
