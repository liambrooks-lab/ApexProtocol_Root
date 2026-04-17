#include "ApexEndgameDirector.h"
#include "ApexGameMode.h"
#include "OmniKernel_OS.h"
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
	OnApexProtocolCompleted_BP();
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
