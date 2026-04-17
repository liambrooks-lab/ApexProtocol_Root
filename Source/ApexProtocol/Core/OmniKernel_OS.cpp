#include "OmniKernel_OS.h"
#include "../Entities/Characters/A_BioAsset.h"
#include "../Entities/Environment/A_InteractiveNode.h"
#include "../Entities/Environment/A_SectorVolume.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

AOmniKernel_OS::AOmniKernel_OS()
{
	PrimaryActorTick.bCanEverTick = true;
	bFinalPurgeActive = false;
	bOmniKernelShutdown = false;
}

void AOmniKernel_OS::EvaluateFacilityState(float PlayerAcousticLevel, float SectorTemperature)
{
	if (bOmniKernelShutdown)
	{
		return;
	}

	const float AcousticThreshold = bFinalPurgeActive ? 55.0f : 80.0f;

	if (PlayerAcousticLevel > AcousticThreshold)
	{
		if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
		{
			const FVector PlayerCoordinates = PlayerPawn->GetActorLocation();
			int32 ThreatLevel = SectorTemperature >= 30.0f ? 4 : 3;

			if (bFinalPurgeActive)
			{
				ThreatLevel += 2;
			}

			DispatchBiologicalAssets(PlayerCoordinates, ThreatLevel);
		}
	}
}

void AOmniKernel_OS::ExecuteSectorLockdown(FName SectorID)
{
	SectorPowerGrid.Add(SectorID, 0.0f);

	TArray<AActor*> InteractiveNodes;
	UGameplayStatics::GetAllActorsOfClass(this, AA_InteractiveNode::StaticClass(), InteractiveNodes);

	for (AActor* NodeActor : InteractiveNodes)
	{
		if (AA_InteractiveNode* InteractiveNode = Cast<AA_InteractiveNode>(NodeActor))
		{
			if (InteractiveNode->GetSectorIdentifier() == SectorID)
			{
				InteractiveNode->SetNodeOperationalState(false);
			}
		}
	}
}

void AOmniKernel_OS::ReportHackingDisturbance(FVector TargetCoordinates, int32 ThreatLevel)
{
	DispatchBiologicalAssets(TargetCoordinates, ThreatLevel);
}

void AOmniKernel_OS::TriggerSectorGasFlood(FName SectorID, bool bEnabled)
{
	SectorToxicityGrid.Add(SectorID, bEnabled ? 1.0f : 0.0f);

	TArray<AActor*> SectorVolumes;
	UGameplayStatics::GetAllActorsOfClass(this, AA_SectorVolume::StaticClass(), SectorVolumes);

	for (AActor* SectorActor : SectorVolumes)
	{
		if (AA_SectorVolume* SectorVolume = Cast<AA_SectorVolume>(SectorActor))
		{
			if (SectorVolume->GetSectorIdentifier() == SectorID)
			{
				SectorVolume->SetGasFlooded(bEnabled);
			}
		}
	}
}

void AOmniKernel_OS::UnlockCoreBreachPath()
{
	TArray<AActor*> InteractiveNodes;
	UGameplayStatics::GetAllActorsOfClass(this, AA_InteractiveNode::StaticClass(), InteractiveNodes);

	for (AActor* NodeActor : InteractiveNodes)
	{
		if (AA_InteractiveNode* InteractiveNode = Cast<AA_InteractiveNode>(NodeActor))
		{
			if (InteractiveNode->RequiresCoreBreachAccess())
			{
				InteractiveNode->SetCoreBreachAuthorized(true);
				InteractiveNode->SetNodeOperationalState(true);
			}
		}
	}

	OnCoreBreachPathUnlockedEvent.Broadcast();
	OnCoreBreachPathUnlocked();
}

void AOmniKernel_OS::BeginFinalPurgeSequence()
{
	if (bFinalPurgeActive || bOmniKernelShutdown)
	{
		return;
	}

	bFinalPurgeActive = true;

	TArray<AActor*> SectorVolumes;
	UGameplayStatics::GetAllActorsOfClass(this, AA_SectorVolume::StaticClass(), SectorVolumes);

	for (AActor* SectorActor : SectorVolumes)
	{
		if (AA_SectorVolume* SectorVolume = Cast<AA_SectorVolume>(SectorActor))
		{
			const FName SectorID = SectorVolume->GetSectorIdentifier();

			if (!SectorID.IsNone() && !SectorVolume->IsSafeRoom())
			{
				ExecuteSectorLockdown(SectorID);
				TriggerSectorGasFlood(SectorID, true);
			}
		}
	}

	OnFinalPurgeStateChangedEvent.Broadcast(true);
	OnFinalPurgeStateChanged(true);
}

bool AOmniKernel_OS::IsFinalPurgeActive() const
{
	return bFinalPurgeActive;
}

void AOmniKernel_OS::ShutdownOmniKernel()
{
	if (bOmniKernelShutdown)
	{
		return;
	}

	bOmniKernelShutdown = true;
	bFinalPurgeActive = false;

	TArray<AActor*> SectorVolumes;
	UGameplayStatics::GetAllActorsOfClass(this, AA_SectorVolume::StaticClass(), SectorVolumes);

	for (AActor* SectorActor : SectorVolumes)
	{
		if (AA_SectorVolume* SectorVolume = Cast<AA_SectorVolume>(SectorActor))
		{
			const FName SectorID = SectorVolume->GetSectorIdentifier();

			if (!SectorID.IsNone())
			{
				TriggerSectorGasFlood(SectorID, false);
			}
		}
	}

	OnFinalPurgeStateChangedEvent.Broadcast(false);
	OnOmniKernelShutdownStateChangedEvent.Broadcast(true);
	OnFinalPurgeStateChanged(false);
	OnOmniKernelShutdownStateChanged(true);
}

bool AOmniKernel_OS::IsOmniKernelShutdown() const
{
	return bOmniKernelShutdown;
}

void AOmniKernel_OS::DispatchBiologicalAssets(FVector TargetCoordinates, int32 ThreatLevel)
{
	TArray<AActor*> BiologicalAssets;
	UGameplayStatics::GetAllActorsOfClass(this, AA_BioAsset::StaticClass(), BiologicalAssets);

	if (BiologicalAssets.Num() == 0)
	{
		return;
	}

	BiologicalAssets.Sort(
		[&TargetCoordinates](const AActor* Left, const AActor* Right)
		{
			if (Left == nullptr || Right == nullptr)
			{
				return Left != nullptr;
			}

			const float LeftDistanceSquared = FVector::DistSquared(Left->GetActorLocation(), TargetCoordinates);
			const float RightDistanceSquared = FVector::DistSquared(Right->GetActorLocation(), TargetCoordinates);
			return LeftDistanceSquared < RightDistanceSquared;
		});

	const int32 AssetsToDispatch = FMath::Clamp(ThreatLevel, 1, BiologicalAssets.Num());

	for (int32 AssetIndex = 0; AssetIndex < AssetsToDispatch; ++AssetIndex)
	{
		if (AA_BioAsset* BioAsset = Cast<AA_BioAsset>(BiologicalAssets[AssetIndex]))
		{
			BioAsset->InvestigateAcousticDisturbance(TargetCoordinates);
		}
	}
}
