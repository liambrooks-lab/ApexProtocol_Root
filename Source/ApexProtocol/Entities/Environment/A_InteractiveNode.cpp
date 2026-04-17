#include "A_InteractiveNode.h"
#include "../../Core/ApexGameMode.h"
#include "../../Core/OmniKernel_OS.h"
#include "Kismet/GameplayStatics.h"

AA_InteractiveNode::AA_InteractiveNode()
{
	NodeIdentifier = NAME_None;
	SectorIdentifier = NAME_None;
	ControlledSectorIdentifier = NAME_None;
	NodeFunction = EApexNodeFunction::Terminal;
	bDoorOpen = false;
	bGasFloodEnabled = false;
	bTriggersSubroutineShutdown = false;
	bSubroutineShutdownCompleted = false;
	bRequiresCoreBreachAccess = false;
	bCoreBreachAuthorized = false;
	bMainframePurged = false;
	bEscapeVectorReleased = false;
	RequiredOverrideSyntax = TEXT("sudo override -f");
}

void AA_InteractiveNode::ReceiveHackPayload(FString SyntaxCommand)
{
	ExecuteOverrideCommand(SyntaxCommand);
}

bool AA_InteractiveNode::ExecuteOverrideCommand(const FString& SyntaxCommand)
{
	if (!bIsActiveNode || SyntaxCommand != RequiredOverrideSyntax)
	{
		return false;
	}

	if (bRequiresCoreBreachAccess && !bCoreBreachAuthorized)
	{
		return false;
	}

	const FName TargetSector = ControlledSectorIdentifier.IsNone() ? SectorIdentifier : ControlledSectorIdentifier;

	switch (NodeFunction)
	{
	case EApexNodeFunction::Door:
	case EApexNodeFunction::SecurityLock:
		bDoorOpen = !bDoorOpen;
		OnDoorStateChanged(bDoorOpen);
		break;

	case EApexNodeFunction::GasControl:
		bGasFloodEnabled = !bGasFloodEnabled;
		OnGasFloodStateChanged(bGasFloodEnabled);

		if (AOmniKernel_OS* OmniKernel = Cast<AOmniKernel_OS>(UGameplayStatics::GetActorOfClass(this, AOmniKernel_OS::StaticClass())))
		{
			OmniKernel->TriggerSectorGasFlood(TargetSector, bGasFloodEnabled);
		}
		break;

	case EApexNodeFunction::CoreBreachConsole:
		if (AApexGameMode* ApexGameMode = GetWorld() != nullptr ? Cast<AApexGameMode>(GetWorld()->GetAuthGameMode()) : nullptr)
		{
			if (ApexGameMode->HasCoreBreachAccess())
			{
				ApexGameMode->BeginCoreBreachSequence();

				if (AOmniKernel_OS* OmniKernel = Cast<AOmniKernel_OS>(UGameplayStatics::GetActorOfClass(this, AOmniKernel_OS::StaticClass())))
				{
					OmniKernel->BeginFinalPurgeSequence();
				}
			}
			else
			{
				return false;
			}
		}
		break;

	case EApexNodeFunction::MainframeCore:
		if (AApexGameMode* ApexGameMode = GetWorld() != nullptr ? Cast<AApexGameMode>(GetWorld()->GetAuthGameMode()) : nullptr)
		{
			if (!ApexGameMode->IsCoreBreachSequenceStarted())
			{
				return false;
			}

			ApexGameMode->ActivateEscapeVector();
			bMainframePurged = true;
			OnMainframePurged();

			if (AOmniKernel_OS* OmniKernel = Cast<AOmniKernel_OS>(UGameplayStatics::GetActorOfClass(this, AOmniKernel_OS::StaticClass())))
			{
				OmniKernel->ShutdownOmniKernel();
			}
		}
		break;

	case EApexNodeFunction::EscapeVectorRelay:
		if (AApexGameMode* ApexGameMode = GetWorld() != nullptr ? Cast<AApexGameMode>(GetWorld()->GetAuthGameMode()) : nullptr)
		{
			if (!ApexGameMode->IsEscapeVectorActivated())
			{
				return false;
			}

			ApexGameMode->CompleteApexProtocol();
			bEscapeVectorReleased = true;
			OnEscapeVectorReleased();
		}
		break;

	case EApexNodeFunction::SafeRoomRelay:
	case EApexNodeFunction::Terminal:
	default:
		break;
	}

	if (bTriggersSubroutineShutdown && !bSubroutineShutdownCompleted)
	{
		if (AApexGameMode* ApexGameMode = GetWorld() != nullptr ? Cast<AApexGameMode>(GetWorld()->GetAuthGameMode()) : nullptr)
		{
			ApexGameMode->RegisterSubroutineShutdown(TargetSector);
			bSubroutineShutdownCompleted = true;

			if (ApexGameMode->HasCoreBreachAccess())
			{
				if (AOmniKernel_OS* OmniKernel = Cast<AOmniKernel_OS>(UGameplayStatics::GetActorOfClass(this, AOmniKernel_OS::StaticClass())))
				{
					OmniKernel->UnlockCoreBreachPath();
				}
			}
		}
	}

	OnOverrideExecuted();

	return true;
}

FName AA_InteractiveNode::GetNodeIdentifier() const
{
	return NodeIdentifier.IsNone() ? GetFName() : NodeIdentifier;
}

FName AA_InteractiveNode::GetSectorIdentifier() const
{
	return SectorIdentifier;
}

FName AA_InteractiveNode::GetControlledSectorIdentifier() const
{
	return ControlledSectorIdentifier.IsNone() ? SectorIdentifier : ControlledSectorIdentifier;
}

void AA_InteractiveNode::SetNodeOperationalState(bool bInIsActive)
{
	bIsActiveNode = bInIsActive;
	OnOperationalStateChanged(bIsActiveNode);
}

EApexNodeFunction AA_InteractiveNode::GetNodeFunction() const
{
	return NodeFunction;
}

bool AA_InteractiveNode::IsDoorOpen() const
{
	return bDoorOpen;
}

FString AA_InteractiveNode::GetNodeStatusReadout() const
{
	const FString PowerState = bIsActiveNode ? TEXT("ONLINE") : TEXT("OFFLINE");

	switch (NodeFunction)
	{
	case EApexNodeFunction::Door:
	case EApexNodeFunction::SecurityLock:
		if (bRequiresCoreBreachAccess && !bCoreBreachAuthorized)
		{
			return FString::Printf(TEXT("%s | CORE ACCESS DENIED"), *PowerState);
		}

		return FString::Printf(TEXT("%s | %s"), *PowerState, bDoorOpen ? TEXT("ACCESS OPEN") : TEXT("ACCESS SEALED"));

	case EApexNodeFunction::GasControl:
		return FString::Printf(TEXT("%s | GAS %s"), *PowerState, bGasFloodEnabled ? TEXT("ENGAGED") : TEXT("CLEAR"));

	case EApexNodeFunction::SafeRoomRelay:
		return FString::Printf(TEXT("%s | SAFE ROOM CHECKPOINT"), *PowerState);

	case EApexNodeFunction::CoreBreachConsole:
		if (bRequiresCoreBreachAccess && !bCoreBreachAuthorized)
		{
			return FString::Printf(TEXT("%s | CORE ACCESS DENIED"), *PowerState);
		}

		return FString::Printf(TEXT("%s | PURGE CONSOLE ARMED"), *PowerState);

	case EApexNodeFunction::MainframeCore:
		return FString::Printf(TEXT("%s | %s"), *PowerState, bMainframePurged ? TEXT("MAINFRAME PURGED") : TEXT("MAINFRAME ACTIVE"));

	case EApexNodeFunction::EscapeVectorRelay:
		return FString::Printf(TEXT("%s | %s"), *PowerState, bEscapeVectorReleased ? TEXT("ESCAPE VECTOR RELEASED") : TEXT("EXTRACTION LOCKED"));

	case EApexNodeFunction::Terminal:
	default:
		if (bRequiresCoreBreachAccess && !bCoreBreachAuthorized)
		{
			return FString::Printf(TEXT("%s | CORE ACCESS DENIED"), *PowerState);
		}

		if (bTriggersSubroutineShutdown)
		{
			return FString::Printf(TEXT("%s | SUBROUTINE %s"), *PowerState, bSubroutineShutdownCompleted ? TEXT("SEVERED") : TEXT("ONLINE"));
		}

		return FString::Printf(TEXT("%s | READY"), *PowerState);
	}
}

bool AA_InteractiveNode::IsSubroutineShutdownNode() const
{
	return bTriggersSubroutineShutdown;
}

bool AA_InteractiveNode::HasCompletedSubroutineShutdown() const
{
	return bSubroutineShutdownCompleted;
}

bool AA_InteractiveNode::RequiresCoreBreachAccess() const
{
	return bRequiresCoreBreachAccess;
}

bool AA_InteractiveNode::HasCoreBreachAuthorization() const
{
	return bCoreBreachAuthorized;
}

void AA_InteractiveNode::SetCoreBreachAuthorized(bool bInAuthorized)
{
	bCoreBreachAuthorized = bInAuthorized;
	OnCoreAccessStateChanged(bCoreBreachAuthorized);
}
