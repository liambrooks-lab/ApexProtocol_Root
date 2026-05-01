#include "ApexEndgameDirector.h"
#include "ApexGameMode.h"
#include "OmniKernel_OS.h"
#include "../Entities/Characters/A_BioAsset.h"
#include "../Systems/AudioDirector.h"
#include "Kismet/GameplayStatics.h"

AApexEndgameDirector::AApexEndgameDirector()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AApexEndgameDirector::BeginPlay()
{
	Super::BeginPlay();

	CachedGameMode = GetWorld() != nullptr ? Cast<AApexGameMode>(GetWorld()->GetAuthGameMode()) : nullptr;
	CachedOmniKernel = Cast<AOmniKernel_OS>(UGameplayStatics::GetActorOfClass(this, AOmniKernel_OS::StaticClass()));

	if (CachedGameMode != nullptr)
	{
		CachedGameMode->OnCoreBreachSequenceStartedEvent.AddDynamic(this, &AApexEndgameDirector::HandleCoreBreachStarted);
		CachedGameMode->OnEscapeVectorActivatedEvent.AddDynamic(this, &AApexEndgameDirector::HandleEscapeVectorActivated);
		CachedGameMode->OnApexProtocolCompletedEvent.AddDynamic(this, &AApexEndgameDirector::HandleApexProtocolCompleted);
	}

	if (CachedOmniKernel != nullptr)
	{
		CachedOmniKernel->OnFinalPurgeStateChangedEvent.AddDynamic(this, &AApexEndgameDirector::HandleFinalPurgeChanged);
		CachedOmniKernel->OnCoreBreachPathUnlockedEvent.AddDynamic(this, &AApexEndgameDirector::HandleCoreBreachPathUnlocked);
		CachedOmniKernel->OnOmniKernelShutdownStateChangedEvent.AddDynamic(this, &AApexEndgameDirector::HandleOmniKernelShutdownChanged);
	}
}

void AApexEndgameDirector::HandleCoreBreachStarted()
{
	OnCoreBreachSequenceStarted_BP();
}

void AApexEndgameDirector::HandleEscapeVectorActivated()
{
	OnEscapeVectorActivated_BP();
}

void AApexEndgameDirector::HandleApexProtocolCompleted()
{
	// === ENDGAME SEQUENCE ===
	// 1. Shut down OmniKernel (ceases all facility hostility)
	if (CachedOmniKernel != nullptr && !CachedOmniKernel->IsOmniKernelShutdown())
	{
		CachedOmniKernel->ShutdownOmniKernel();
	}

	// 2. Deactivate all BioAssets (stop all AI movement and sensing)
	DeactivateAllBioAssets();

	// 3. Fire Blueprint cinematic event
	OnApexProtocolCompleted_BP();
	OnEndgameCinematicTriggered();
}

void AApexEndgameDirector::HandleFinalPurgeChanged(bool bPurgeActive)
{
	OnFinalPurgeStateChanged_BP(bPurgeActive);
}

void AApexEndgameDirector::HandleCoreBreachPathUnlocked()
{
	OnCoreBreachPathUnlocked_BP();
}

void AApexEndgameDirector::HandleOmniKernelShutdownChanged(bool bIsShutdown)
{
	OnOmniKernelShutdown_BP(bIsShutdown);
}

// ─────────────────────────────────────────────────────────────
// DeactivateAllBioAssets — Master kill switch for all facility hostiles
// ─────────────────────────────────────────────────────────────
void AApexEndgameDirector::DeactivateAllBioAssets()
{
	TArray<AActor*> BioAssetActors;
	UGameplayStatics::GetAllActorsOfClass(this, AA_BioAsset::StaticClass(), BioAssetActors);

	for (AActor* Actor : BioAssetActors)
	{
		if (AA_BioAsset* BioAsset = Cast<AA_BioAsset>(Actor))
		{
			BioAsset->Deactivate();
		}
	}

	UE_LOG(LogTemp, Log, TEXT("AApexEndgameDirector: Deactivated %d BioAssets."), BioAssetActors.Num());
}
