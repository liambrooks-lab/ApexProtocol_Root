#include "RealTimeNetworking.h"
#include "IWebSocket.h"
#include "Modules/ModuleManager.h"
#include "WebSocketsModule.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "TimerManager.h"
#include "Engine/World.h"

// ============================================================
// URealTimeNetworking Implementation
// ============================================================

URealTimeNetworking::URealTimeNetworking()
	: ReconnectAttempts(0)
{
}

void URealTimeNetworking::InitializeSocketConnection(const FString& ServerURL)
{
	if (!FModuleManager::Get().IsModuleLoaded(TEXT("WebSockets")))
	{
		FModuleManager::LoadModuleChecked<FWebSocketsModule>(TEXT("WebSockets"));
	}

	CachedServerURL = ServerURL;
	ReconnectAttempts = 0;

	SocketConnection = FWebSocketsModule::Get().CreateWebSocket(ServerURL, TEXT("ws"));

	if (SocketConnection.IsValid())
	{
		SocketConnection->OnMessage().AddUObject(this, &URealTimeNetworking::OnMessageReceived);
		SocketConnection->OnConnectionError().AddUObject(this, &URealTimeNetworking::OnConnectionError);
		SocketConnection->OnConnected().AddUObject(this, &URealTimeNetworking::OnConnected);
		SocketConnection->OnClosed().AddUObject(this, &URealTimeNetworking::OnClosed);
		SocketConnection->Connect();

		UE_LOG(LogTemp, Log, TEXT("URealTimeNetworking: Initiating WebSocket connection to %s"), *ServerURL);
	}
}

void URealTimeNetworking::BroadcastStateChange(const FString& NodeID, bool bIsActive)
{
	if (!SocketConnection.IsValid() || !SocketConnection->IsConnected())
	{
		return;
	}

	// Construct JSON payload
	const TSharedRef<FJsonObject> PayloadObj = MakeShared<FJsonObject>();
	PayloadObj->SetStringField(TEXT("type"), TEXT("node_state"));
	PayloadObj->SetStringField(TEXT("nodeId"), NodeID);
	PayloadObj->SetBoolField(TEXT("isActive"), bIsActive);
	PayloadObj->SetNumberField(TEXT("timestamp"), FPlatformTime::Seconds());

	FString PayloadString;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&PayloadString);
	FJsonSerializer::Serialize(PayloadObj, Writer);

	SocketConnection->Send(PayloadString);
}

void URealTimeNetworking::BroadcastSectorUpdate(const FString& SectorID, float Temperature, bool bGasFlooded, float PowerLevel)
{
	if (!SocketConnection.IsValid() || !SocketConnection->IsConnected())
	{
		return;
	}

	const TSharedRef<FJsonObject> PayloadObj = MakeShared<FJsonObject>();
	PayloadObj->SetStringField(TEXT("type"), TEXT("sector_update"));
	PayloadObj->SetStringField(TEXT("sectorId"), SectorID);
	PayloadObj->SetNumberField(TEXT("temperature"), Temperature);
	PayloadObj->SetBoolField(TEXT("gasFlooded"), bGasFlooded);
	PayloadObj->SetNumberField(TEXT("powerLevel"), PowerLevel);
	PayloadObj->SetNumberField(TEXT("timestamp"), FPlatformTime::Seconds());

	FString PayloadString;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&PayloadString);
	FJsonSerializer::Serialize(PayloadObj, Writer);

	SocketConnection->Send(PayloadString);
}

void URealTimeNetworking::CloseConnection()
{
	if (SocketConnection.IsValid() && SocketConnection->IsConnected())
	{
		SocketConnection->Close();
	}
}

bool URealTimeNetworking::IsConnected() const
{
	return SocketConnection.IsValid() && SocketConnection->IsConnected();
}

// ─────────────────────────────────────────────────────────────
// WebSocket Callbacks
// ─────────────────────────────────────────────────────────────
void URealTimeNetworking::OnConnected()
{
	ReconnectAttempts = 0;
	UE_LOG(LogTemp, Log, TEXT("URealTimeNetworking: WebSocket connected successfully."));
	OnConnectionStatusChanged.Broadcast(true);
}

void URealTimeNetworking::OnMessageReceived(const FString& Message)
{
	// Parse incoming JSON
	TSharedPtr<FJsonObject> JsonObj;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Message);

	if (!FJsonSerializer::Deserialize(Reader, JsonObj) || !JsonObj.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("URealTimeNetworking: Failed to parse incoming message: %s"), *Message);
		return;
	}

	const FString MessageType = JsonObj->GetStringField(TEXT("type"));

	if (MessageType == TEXT("node_state"))
	{
		const FString NodeID = JsonObj->GetStringField(TEXT("nodeId"));
		const bool bIsActive = JsonObj->GetBoolField(TEXT("isActive"));

		// Broadcast to local listeners — InteractiveNodes, OmniKernel, etc.
		OnRemoteStateUpdate.Broadcast(NodeID, bIsActive);
	}
	else if (MessageType == TEXT("facility_alert"))
	{
		// Server-driven facility events (e.g., admin-triggered lockdowns)
		UE_LOG(LogTemp, Warning, TEXT("URealTimeNetworking: Facility alert received: %s"), *Message);
	}
}

void URealTimeNetworking::OnConnectionError(const FString& ErrorMessage)
{
	UE_LOG(LogTemp, Warning, TEXT("URealTimeNetworking: Connection error: %s"), *ErrorMessage);
	AttemptReconnect();
}

void URealTimeNetworking::OnClosed(int32 StatusCode, const FString& Reason, bool bWasClean)
{
	UE_LOG(LogTemp, Log, TEXT("URealTimeNetworking: Connection closed (Code: %d, Reason: %s, Clean: %s)"),
		StatusCode, *Reason, bWasClean ? TEXT("Yes") : TEXT("No"));

	OnConnectionStatusChanged.Broadcast(false);

	if (!bWasClean)
	{
		AttemptReconnect();
	}
}

// ─────────────────────────────────────────────────────────────
// Exponential Backoff Reconnect
// ─────────────────────────────────────────────────────────────
void URealTimeNetworking::AttemptReconnect()
{
	if (ReconnectAttempts >= MaxReconnectAttempts)
	{
		UE_LOG(LogTemp, Error, TEXT("URealTimeNetworking: Max reconnection attempts (%d) exhausted. Giving up."), MaxReconnectAttempts);
		return;
	}

	if (CachedServerURL.IsEmpty())
	{
		return;
	}

	ReconnectAttempts++;
	const float Delay = BaseReconnectDelay * FMath::Pow(2.0f, static_cast<float>(ReconnectAttempts - 1));

	UE_LOG(LogTemp, Log, TEXT("URealTimeNetworking: Reconnect attempt %d/%d in %.1f seconds."),
		ReconnectAttempts, MaxReconnectAttempts, Delay);

	// Use a world timer if available; otherwise log and skip
	UWorld* World = GEngine != nullptr ? GEngine->GetCurrentPlayWorld() : nullptr;
	if (World != nullptr)
	{
		World->GetTimerManager().SetTimer(
			ReconnectTimerHandle,
			[this]()
			{
				InitializeSocketConnection(CachedServerURL);
			},
			Delay,
			false
		);
	}
}
