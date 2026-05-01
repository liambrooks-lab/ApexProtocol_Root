#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "RealTimeNetworking.generated.h"

class IWebSocket;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNetworkStateUpdateEvent, FString, NodeID, bool, bIsActive);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNetworkConnectionEvent, bool, bConnected);

// ============================================================
// URealTimeNetworking — Facility State Synchronization Layer
//
// WebSocket-based real-time sync of environmental variables.
// All node state changes are broadcast to the server.
// Incoming messages route facility state updates to local systems.
//
// If no server is running, the system compiles cleanly and
// all methods safely no-op (checked via SocketConnection validity).
// ============================================================
UCLASS()
class APEXPROTOCOL_API URealTimeNetworking : public UObject
{
	GENERATED_BODY()

public:
	URealTimeNetworking();

	/** Initialize and connect the WebSocket to the given server URL. */
	UFUNCTION(BlueprintCallable, Category = "Unified Architecture|Networking")
	void InitializeSocketConnection(const FString& ServerURL);

	/** Broadcast a node state change to the connected server. */
	UFUNCTION(BlueprintCallable, Category = "Unified Architecture|Networking")
	void BroadcastStateChange(const FString& NodeID, bool bIsActive);

	/** Broadcast a sector environmental update (gas, temperature, power). */
	UFUNCTION(BlueprintCallable, Category = "Unified Architecture|Networking")
	void BroadcastSectorUpdate(const FString& SectorID, float Temperature, bool bGasFlooded, float PowerLevel);

	/** Gracefully close the WebSocket connection. */
	UFUNCTION(BlueprintCallable, Category = "Unified Architecture|Networking")
	void CloseConnection();

	/** Returns whether the WebSocket is currently connected. */
	UFUNCTION(BlueprintPure, Category = "Unified Architecture|Networking")
	bool IsConnected() const;

	// ─── Events ────────────────────────────────────────────────────────
	/** Fires when a remote node state update is received from the server. */
	UPROPERTY(BlueprintAssignable, Category = "Unified Architecture|Networking")
	FNetworkStateUpdateEvent OnRemoteStateUpdate;

	/** Fires on connection/disconnection. */
	UPROPERTY(BlueprintAssignable, Category = "Unified Architecture|Networking")
	FNetworkConnectionEvent OnConnectionStatusChanged;

private:
	void OnMessageReceived(const FString& Message);
	void OnConnectionError(const FString& ErrorMessage);
	void OnConnected();
	void OnClosed(int32 StatusCode, const FString& Reason, bool bWasClean);
	void AttemptReconnect();

	TSharedPtr<IWebSocket> SocketConnection;

	/** Cached server URL for reconnection attempts. */
	FString CachedServerURL;

	/** Reconnection backoff state. */
	int32 ReconnectAttempts;
	static constexpr int32 MaxReconnectAttempts = 5;
	static constexpr float BaseReconnectDelay = 1.0f;

	FTimerHandle ReconnectTimerHandle;
};
