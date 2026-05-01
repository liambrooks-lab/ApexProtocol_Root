#include "A_PlayerCharacter.h"
#include "../../Core/ApexGameMode.h"
#include "../../Core/OmniKernel_OS.h"
#include "../../Core/ApexPlayerController.h"
#include "../../Entities/Environment/A_InteractiveNode.h"
#include "../../Entities/Environment/A_SectorVolume.h"
#include "../../Systems/AudioDirector.h"
#include "../../Systems/ToxicityManager.h"
#include "../../UI/TerminalWidget_Native.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Math/RotationMatrix.h"

// ============================================================
// Constructor
// ============================================================
AA_PlayerCharacter::AA_PlayerCharacter()
{
	BiologicalSystems = CreateDefaultSubobject<UToxicityManager>(TEXT("BiologicalSystems"));

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 0.0f;
	CameraBoom->SocketOffset = FVector(0.0f, 0.0f, 64.0f);
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	TerminalWidgetClass = UTerminalWidget_Native::StaticClass();
	AutoPossessPlayer = EAutoReceiveInput::Player0;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	bUseControllerRotationYaw = true;
	bTerminalVisible = false;
	InteractionRange = 350.0f;
	AcousticOutputLevel = 0.0f;
	CurrentSectorTemperature = 21.0f;
	bInSafeRoom = false;
	CurrentSectorID = NAME_None;
	LastCheckpointSectorID = NAME_None;
	bBiologicalCollapseTriggered = false;

	// Movement modifiers
	bIsCrouching = false;
	bIsSprinting = false;
	CrouchAcousticMultiplier = 0.25f;
	SprintAcousticMultiplier = 2.0f;
	CrouchWalkSpeed = 130.0f;
	SprintSpeed = 475.0f;
	NormalWalkSpeed = 275.0f;

	// Input latency
	MaxInputLatencySeconds = 0.35f;
	RawForwardInput = 0.0f;
	RawRightInput = 0.0f;

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->MaxWalkSpeed = NormalWalkSpeed;
}

// ============================================================
// BeginPlay — Cache references, bind ToxicityManager delegates
// ============================================================
void AA_PlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	CachedOmniKernel = Cast<AOmniKernel_OS>(UGameplayStatics::GetActorOfClass(this, AOmniKernel_OS::StaticClass()));
	CachedAudioDirector = Cast<AAudioDirector>(UGameplayStatics::GetActorOfClass(this, AAudioDirector::StaticClass()));

	// Bind ToxicityManager threshold delegates
	if (BiologicalSystems != nullptr)
	{
		BiologicalSystems->OnHallucinationThresholdReached.AddDynamic(this, &AA_PlayerCharacter::HandleHallucinationThreshold);
		BiologicalSystems->OnHallucinationRecovered.AddDynamic(this, &AA_PlayerCharacter::HandleHallucinationRecovered);
		BiologicalSystems->OnCriticalToxicityReached.AddDynamic(this, &AA_PlayerCharacter::HandleCriticalToxicity);
		BiologicalSystems->OnBiologicalCollapseEvent.AddDynamic(this, &AA_PlayerCharacter::HandleBiologicalCollapseDelegate);
	}

	if (AApexGameMode* ApexGameMode = GetWorld() != nullptr ? Cast<AApexGameMode>(GetWorld()->GetAuthGameMode()) : nullptr)
	{
		if (!ApexGameMode->HasCheckpointData())
		{
			ApexGameMode->RegisterCheckpoint(GetActorTransform(), CurrentSectorID, 0.0f);
		}
	}
}

// ============================================================
// Tick
// ============================================================
void AA_PlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AcousticOutputLevel = FMath::Max(0.0f, AcousticOutputLevel - (25.0f * DeltaTime));
	UpdateSectorState(DeltaTime);
	RefreshAudioState();
	TickInputLatencyBuffer(DeltaTime);

	if (BiologicalSystems != nullptr && BiologicalSystems->GetCurrentToxicity() >= 100.0f && !bBiologicalCollapseTriggered)
	{
		HandleBiologicalCollapse();
	}
}

// ============================================================
// Input Binding
// ============================================================
void AA_PlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AA_PlayerCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AA_PlayerCharacter::MoveRight);
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AA_PlayerCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &AA_PlayerCharacter::LookUpAtRate);
	PlayerInputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &AA_PlayerCharacter::Interact);
	PlayerInputComponent->BindAction(TEXT("HoloTerminal"), IE_Pressed, this, &AA_PlayerCharacter::ToggleTerminal);
	PlayerInputComponent->BindAction(TEXT("VoiceCommand_Override"), IE_Pressed, this, &AA_PlayerCharacter::VoiceOverride);
	PlayerInputComponent->BindAction(TEXT("Crouch"), IE_Pressed, this, &AA_PlayerCharacter::ToggleCrouch);
	PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Pressed, this, &AA_PlayerCharacter::StartSprint);
	PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Released, this, &AA_PlayerCharacter::StopSprint);
}

// ============================================================
// Input Latency Ring Buffer
// At high toxicity, inputs are deferred by a scaled delay.
// This simulates neurological impairment without time dilation.
// ============================================================
void AA_PlayerCharacter::TickInputLatencyBuffer(float DeltaTime)
{
	// Process any mature samples in the buffer
	for (int32 i = InputLatencyBuffer.Num() - 1; i >= 0; --i)
	{
		InputLatencyBuffer[i].DelayRemaining -= DeltaTime;

		if (InputLatencyBuffer[i].DelayRemaining <= 0.0f)
		{
			ApplyDeferredMovement(InputLatencyBuffer[i].ForwardValue, InputLatencyBuffer[i].RightValue);
			InputLatencyBuffer.RemoveAt(i);
		}
	}
}

void AA_PlayerCharacter::ApplyDeferredMovement(float ForwardValue, float RightValue)
{
	if (Controller == nullptr)
	{
		return;
	}

	const FRotator ControlRotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);

	if (!FMath::IsNearlyZero(ForwardValue))
	{
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(ForwardDirection, ForwardValue);
	}

	if (!FMath::IsNearlyZero(RightValue))
	{
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(RightDirection, RightValue);
	}
}

// ============================================================
// Movement — With Latency Injection & Acoustic Modulation
// ============================================================
void AA_PlayerCharacter::MoveForward(float Value)
{
	if (Controller == nullptr || FMath::IsNearlyZero(Value))
	{
		return;
	}

	// Determine acoustic modifier
	float AcousticMod = 1.0f;
	if (bIsCrouching) AcousticMod = CrouchAcousticMultiplier;
	else if (bIsSprinting) AcousticMod = SprintAcousticMultiplier;

	ReportAcousticEvent(40.0f * AcousticMod);

	// If toxicity is above hallucination threshold, defer input
	const float LatencyScalar = BiologicalSystems != nullptr ? BiologicalSystems->GetInputLatencyScalar() : 0.0f;

	if (LatencyScalar > 0.01f)
	{
		const float Delay = LatencyScalar * MaxInputLatencySeconds;
		InputLatencyBuffer.Add(FDeferredInputSample(Value, 0.0f, Delay));
	}
	else
	{
		const FRotator ControlRotation = Controller->GetControlRotation();
		const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(ForwardDirection, Value);
	}
}

void AA_PlayerCharacter::MoveRight(float Value)
{
	if (Controller == nullptr || FMath::IsNearlyZero(Value))
	{
		return;
	}

	float AcousticMod = 1.0f;
	if (bIsCrouching) AcousticMod = CrouchAcousticMultiplier;
	else if (bIsSprinting) AcousticMod = SprintAcousticMultiplier;

	ReportAcousticEvent(35.0f * AcousticMod);

	const float LatencyScalar = BiologicalSystems != nullptr ? BiologicalSystems->GetInputLatencyScalar() : 0.0f;

	if (LatencyScalar > 0.01f)
	{
		const float Delay = LatencyScalar * MaxInputLatencySeconds;
		InputLatencyBuffer.Add(FDeferredInputSample(0.0f, Value, Delay));
	}
	else
	{
		const FRotator ControlRotation = Controller->GetControlRotation();
		const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(RightDirection, Value);
	}
}

void AA_PlayerCharacter::TurnAtRate(float Value)
{
	AddControllerYawInput(Value);
}

void AA_PlayerCharacter::LookUpAtRate(float Value)
{
	AddControllerPitchInput(Value);
}

// ============================================================
// Crouch / Sprint — Movement Modes with Acoustic Coupling
// ============================================================
void AA_PlayerCharacter::ToggleCrouch()
{
	bIsCrouching = !bIsCrouching;
	bIsSprinting = false; // Mutually exclusive

	GetCharacterMovement()->MaxWalkSpeed = bIsCrouching ? CrouchWalkSpeed : NormalWalkSpeed;
}

void AA_PlayerCharacter::StartSprint()
{
	bIsSprinting = true;
	bIsCrouching = false;
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
}

void AA_PlayerCharacter::StopSprint()
{
	bIsSprinting = false;
	GetCharacterMovement()->MaxWalkSpeed = NormalWalkSpeed;
}

// ============================================================
// ToxicityManager Delegate Handlers
// ============================================================
void AA_PlayerCharacter::HandleHallucinationThreshold(float ToxicityLevel)
{
	if (CachedAudioDirector != nullptr)
	{
		const float Normalized = BiologicalSystems != nullptr ? BiologicalSystems->GetInputLatencyScalar() : 0.0f;
		CachedAudioDirector->SetHallucinationIntensity(Normalized);
	}

	OnHallucinationOnset(ToxicityLevel);
}

void AA_PlayerCharacter::HandleHallucinationRecovered(float ToxicityLevel)
{
	if (CachedAudioDirector != nullptr)
	{
		CachedAudioDirector->SetHallucinationIntensity(0.0f);
	}

	OnHallucinationCleared();
}

void AA_PlayerCharacter::HandleCriticalToxicity(float ToxicityLevel)
{
	// Involuntary gasp — loud acoustic emission at critical toxicity
	ReportAcousticEvent(70.0f);
	OnCriticalToxicityOnset(ToxicityLevel);
}

void AA_PlayerCharacter::HandleBiologicalCollapseDelegate(float ToxicityLevel)
{
	if (!bBiologicalCollapseTriggered)
	{
		HandleBiologicalCollapse();
	}
}

// ============================================================
// Terminal Toggle — Diegetic HoloTerminal (no pause)
// ============================================================
void AA_PlayerCharacter::ToggleTerminal()
{
	bTerminalVisible = !bTerminalVisible;

	if (bTerminalVisible && TerminalWidgetClass != nullptr && ActiveTerminalWidget == nullptr)
	{
		ActiveTerminalWidget = CreateWidget<UTerminalWidget_Native>(GetWorld(), TerminalWidgetClass);
		if (ActiveTerminalWidget != nullptr)
		{
			ActiveTerminalWidget->AddToViewport();
		}
	}

	if (ActiveTerminalWidget != nullptr)
	{
		if (bTerminalVisible)
		{
			AcquireFocusedNode();
			ActiveTerminalWidget->SetTargetNode(FocusedNode);
			ActiveTerminalWidget->SetVisibility(ESlateVisibility::Visible);
			UpdateTerminalContextReadout();
		}
		else
		{
			ActiveTerminalWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		PlayerController->bShowMouseCursor = bTerminalVisible;
		if (bTerminalVisible)
		{
			FInputModeGameAndUI InputMode;
			InputMode.SetHideCursorDuringCapture(false);
			PlayerController->SetInputMode(InputMode);
		}
		else
		{
			FInputModeGameOnly InputMode;
			PlayerController->SetInputMode(InputMode);
		}
	}
}

// ============================================================
// Interact / VoiceOverride
// ============================================================
void AA_PlayerCharacter::Interact()
{
	AcquireFocusedNode();

	if (FocusedNode != nullptr)
	{
		if (!bTerminalVisible)
		{
			ToggleTerminal();
		}
		else if (ActiveTerminalWidget != nullptr)
		{
			ActiveTerminalWidget->SetTargetNode(FocusedNode);
			UpdateTerminalContextReadout();
		}
		ReportAcousticEvent(35.0f);
	}
}

void AA_PlayerCharacter::VoiceOverride()
{
	AcquireFocusedNode();

	if (ActiveTerminalWidget != nullptr && FocusedNode != nullptr)
	{
		ActiveTerminalWidget->SetTargetNode(FocusedNode);
		UpdateTerminalContextReadout();
	}

	if (ActiveTerminalWidget != nullptr && bTerminalVisible)
	{
		if (ActiveTerminalWidget->AttemptNodeOverride(TEXT("sudo override -f")))
		{
			UpdateTerminalContextReadout();
			ReportAcousticEvent(60.0f);
			if (CachedOmniKernel != nullptr)
			{
				CachedOmniKernel->ReportHackingDisturbance(GetActorLocation(), 2);
			}
		}
		else
		{
			UpdateTerminalContextReadout();
		}
	}
}

// ============================================================
// Audio State Refresh
// ============================================================
void AA_PlayerCharacter::RefreshAudioState() const
{
	if (BiologicalSystems != nullptr && CachedAudioDirector != nullptr)
	{
		CachedAudioDirector->ApplyPsychoacousticFilters(BiologicalSystems->GetCurrentToxicity());
		if (CachedOmniKernel != nullptr)
		{
			CachedAudioDirector->ApplyFinalPurgeMix(CachedOmniKernel->IsFinalPurgeActive());
		}
	}
}

// ============================================================
// Sector State Update
// ============================================================
void AA_PlayerCharacter::UpdateSectorState(float DeltaTime)
{
	const bool bWasInSafeRoom = bInSafeRoom;
	const FName PreviousSectorID = CurrentSectorID;

	ActiveSectorVolume = ResolveCurrentSectorVolume();
	bInSafeRoom = false;
	CurrentSectorID = NAME_None;
	CurrentSectorTemperature = 21.0f;

	if (ActiveSectorVolume != nullptr)
	{
		bInSafeRoom = ActiveSectorVolume->IsSafeRoom();
		CurrentSectorID = ActiveSectorVolume->GetSectorIdentifier();
		CurrentSectorTemperature = ActiveSectorVolume->GetSectorTemperature();

		if (BiologicalSystems != nullptr)
		{
			const float ExposureRate = bInSafeRoom ? 0.0f : ActiveSectorVolume->GetEnvironmentalExposureRate();
			BiologicalSystems->SetEnvironmentalExposureRate(ExposureRate);
			if (bInSafeRoom)
			{
				BiologicalSystems->AdministerAntidote(2.5f * DeltaTime);
			}
		}

		if (bInSafeRoom)
		{
			AcousticOutputLevel = FMath::Min(AcousticOutputLevel, 10.0f);
			if (!bWasInSafeRoom || PreviousSectorID != CurrentSectorID)
			{
				RegisterSafeRoomCheckpoint();
			}
		}
	}
	else if (BiologicalSystems != nullptr)
	{
		BiologicalSystems->SetEnvironmentalExposureRate(0.0f);
	}
}

// ============================================================
// Safe Room Checkpoint
// ============================================================
void AA_PlayerCharacter::RegisterSafeRoomCheckpoint()
{
	if (!bInSafeRoom || BiologicalSystems == nullptr || CurrentSectorID.IsNone())
	{
		return;
	}

	if (AApexGameMode* ApexGameMode = GetWorld() != nullptr ? Cast<AApexGameMode>(GetWorld()->GetAuthGameMode()) : nullptr)
	{
		ApexGameMode->RegisterSafeRoomEntry(CurrentSectorID);
		ApexGameMode->RegisterCheckpoint(GetActorTransform(), CurrentSectorID, BiologicalSystems->GetCurrentToxicity());
		LastCheckpointSectorID = CurrentSectorID;

		if (ActiveTerminalWidget != nullptr && bTerminalVisible)
		{
			ActiveTerminalWidget->SetStatusReadout(FString::Printf(TEXT("CHECKPOINT SYNCHRONIZED: %s"), *CurrentSectorID.ToString()));
			UpdateTerminalContextReadout();
		}
	}
}

// ============================================================
// Biological Collapse / Checkpoint Recovery
// ============================================================
void AA_PlayerCharacter::HandleBiologicalCollapse()
{
	bBiologicalCollapseTriggered = true;

	if (bTerminalVisible)
	{
		ToggleTerminal();
	}

	if (AApexPlayerController* ApexController = Cast<AApexPlayerController>(GetController()))
	{
		ApexController->TriggerCheckpointRecovery();
	}
}

void AA_PlayerCharacter::RestoreFromCheckpoint()
{
	if (AApexGameMode* ApexGameMode = GetWorld() != nullptr ? Cast<AApexGameMode>(GetWorld()->GetAuthGameMode()) : nullptr)
	{
		if (ApexGameMode->HasCheckpointData())
		{
			SetActorTransform(ApexGameMode->GetLastCheckpointTransform());
			LastCheckpointSectorID = ApexGameMode->GetLastSafeRoomSector();
			if (BiologicalSystems != nullptr)
			{
				BiologicalSystems->SetEnvironmentalExposureRate(0.0f);
				BiologicalSystems->SetCurrentToxicity(ApexGameMode->GetLastCheckpointToxicity());
			}
		}
	}

	AcousticOutputLevel = 0.0f;
	bBiologicalCollapseTriggered = false;
	InputLatencyBuffer.Empty(); // Flush deferred inputs on recovery

	GetCharacterMovement()->StopMovementImmediately();

	if (ActiveTerminalWidget != nullptr)
	{
		ActiveTerminalWidget->SetStatusReadout(TEXT("BIOLOGICAL STATE RESTORED FROM CHECKPOINT"));
		UpdateTerminalContextReadout();
	}
}

// ============================================================
// Terminal Context Readout
// ============================================================
void AA_PlayerCharacter::UpdateTerminalContextReadout() const
{
	if (ActiveTerminalWidget == nullptr)
	{
		return;
	}

	if (AApexGameMode* ApexGameMode = GetWorld() != nullptr ? Cast<AApexGameMode>(GetWorld()->GetAuthGameMode()) : nullptr)
	{
		ActiveTerminalWidget->SetObjectiveReadout(ApexGameMode->GetObjectiveReadout());
	}

	const FString SectorLabel = CurrentSectorID.IsNone() ? TEXT("UNTRACKED") : CurrentSectorID.ToString();
	const float ToxicityValue = BiologicalSystems != nullptr ? BiologicalSystems->GetCurrentToxicity() : 0.0f;
	const float ExposureRate = BiologicalSystems != nullptr ? BiologicalSystems->GetEnvironmentalExposureRate() : 0.0f;
	const FString SafeRoomState = bInSafeRoom ? TEXT("SAFE ROOM") : TEXT("EXPOSED");
	const FString PurgeState = (CachedOmniKernel != nullptr && CachedOmniKernel->IsFinalPurgeActive()) ? TEXT("PURGE") : TEXT("STABLE");

	ActiveTerminalWidget->SetTelemetryReadout(
		FString::Printf(
			TEXT("SECTOR:%s | TOX:%.1f | EXP:%.2f | %s | %s"),
			*SectorLabel, ToxicityValue, ExposureRate, *SafeRoomState, *PurgeState));

	if (FocusedNode == nullptr)
	{
		if (CurrentSectorID.IsNone())
		{
			ActiveTerminalWidget->SetStatusReadout(TEXT("OMNI-KERNEL TERMINAL: LINK ESTABLISHED"));
		}
		else
		{
			ActiveTerminalWidget->SetStatusReadout(FString::Printf(TEXT("SECTOR %s LINK ESTABLISHED"), *CurrentSectorID.ToString()));
		}
	}
}

// ============================================================
// Acoustic Event Reporting
// ============================================================
void AA_PlayerCharacter::ReportAcousticEvent(float Loudness)
{
	AcousticOutputLevel = FMath::Clamp(FMath::Max(AcousticOutputLevel, Loudness), 0.0f, 100.0f);

	if (CachedOmniKernel != nullptr)
	{
		CachedOmniKernel->EvaluateFacilityState(AcousticOutputLevel, CurrentSectorTemperature);
	}
}

// ============================================================
// Interaction Tracing
// ============================================================
AA_InteractiveNode* AA_PlayerCharacter::TraceInteractiveNode() const
{
	if (FollowCamera == nullptr)
	{
		return nullptr;
	}

	const FVector TraceStart = FollowCamera->GetComponentLocation();
	const FVector TraceEnd = TraceStart + (FollowCamera->GetForwardVector() * InteractionRange);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ApexNodeTrace), false, this);

	if (GetWorld() != nullptr && GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		return Cast<AA_InteractiveNode>(HitResult.GetActor());
	}

	return nullptr;
}

void AA_PlayerCharacter::AcquireFocusedNode()
{
	FocusedNode = TraceInteractiveNode();
}

AA_SectorVolume* AA_PlayerCharacter::ResolveCurrentSectorVolume() const
{
	TArray<AActor*> OverlappingActors;
	GetOverlappingActors(OverlappingActors, AA_SectorVolume::StaticClass());

	AA_SectorVolume* ResolvedSector = nullptr;
	float BestDistanceSquared = TNumericLimits<float>::Max();

	for (AActor* OverlappingActor : OverlappingActors)
	{
		if (AA_SectorVolume* SectorVolume = Cast<AA_SectorVolume>(OverlappingActor))
		{
			const float DistanceSquared = FVector::DistSquared(SectorVolume->GetActorLocation(), GetActorLocation());
			if (DistanceSquared < BestDistanceSquared)
			{
				BestDistanceSquared = DistanceSquared;
				ResolvedSector = SectorVolume;
			}
		}
	}

	return ResolvedSector;
}
