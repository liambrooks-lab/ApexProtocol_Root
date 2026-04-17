#include "RealTimeNetworking.h"
#include "Modules/ModuleManager.h"
#include "WebSocketsModule.h"

URealTimeNetworking::URealTimeNetworking()
{
}

void URealTimeNetworking::InitializeSocketConnection(const FString& ServerURL)
{
	if (!FModuleManager::Get().IsModuleLoaded(TEXT("WebSockets")))
	{
		FModuleManager::LoadModuleChecked<FWebSocketsModule>(TEXT("WebSockets"));
	}

	SocketConnection = FWebSocketsModule::Get().CreateWebSocket(ServerURL);

	if (SocketConnection.IsValid())
	{
		SocketConnection->OnMessage().AddUObject(this, &URealTimeNetworking::OnMessageReceived);
		SocketConnection->OnConnectionError().AddUObject(this, &URealTimeNetworking::OnConnectionError);
		SocketConnection->Connect();
	}
}

void URealTimeNetworking::BroadcastStateChange(const FString& NodeID, bool bIsActive)
{
	if (SocketConnection.IsValid() && SocketConnection->IsConnected())
	{
		const FString Payload = FString::Printf(
			TEXT("{\"NodeID\":\"%s\",\"bIsActive\":%s}"),
			*NodeID,
			bIsActive ? TEXT("true") : TEXT("false"));

		SocketConnection->Send(Payload);
	}
}

void URealTimeNetworking::OnMessageReceived(const FString& Message)
{
	// Future implementation will parse incoming JSON payloads and route state updates to subscribed systems.
}

void URealTimeNetworking::OnConnectionError(const FString& ErrorMessage)
{
	// Future implementation will parse error JSON/details and trigger reconnection or failover handling.
}
