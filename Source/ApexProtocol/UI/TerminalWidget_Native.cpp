#include "TerminalWidget_Native.h"
#include "Components/TextBlock.h"
#include "../Entities/Environment/A_InteractiveNode.h"

void UTerminalWidget_Native::NativeConstruct()
{
	Super::NativeConstruct();

	SetStatusReadout(TEXT("OMNI-KERNEL TERMINAL: AWAITING INPUT..."));
	SetObjectiveReadout(TEXT("PRIMARY OBJECTIVE: LINK NOT ESTABLISHED"));
	SetTelemetryReadout(TEXT("TELEMETRY: SIGNAL NOISE FLOOR STABLE"));
}

bool UTerminalWidget_Native::AttemptNodeOverride(FString CommandSyntax)
{
	if (TargetNode != nullptr && TargetNode->ExecuteOverrideCommand(CommandSyntax))
	{
		SetStatusReadout(FString::Printf(TEXT("%s | %s"), *TargetNode->GetNodeIdentifier().ToString(), *TargetNode->GetNodeStatusReadout()));
		UpdateDataVisualizer(100.0f);
		return true;
	}

	SetStatusReadout(TEXT("OVERRIDE REJECTED"));
	UpdateDataVisualizer(20.0f);
	return false;
}

void UTerminalWidget_Native::SetTargetNode(AA_InteractiveNode* InTargetNode)
{
	TargetNode = InTargetNode;

	if (ReadoutDisplay != nullptr)
	{
		const FString TargetLabel = TargetNode != nullptr ? TargetNode->GetNodeIdentifier().ToString() : TEXT("NO NODE");
		const FString Status = TargetNode != nullptr ? TargetNode->GetNodeStatusReadout() : TEXT("NO ACTIVE LINK");
		SetStatusReadout(FString::Printf(TEXT("TARGET NODE: %s | %s"), *TargetLabel, *Status));
	}
}

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

void UTerminalWidget_Native::UpdateDataVisualizer(float SignalStrength)
{
	(void)SignalStrength;
}
