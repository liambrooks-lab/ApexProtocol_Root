#pragma once

#include "CoreMinimal.h"
#include "IWebSocket.h"
#include "UObject/NoExportTypes.h"
#include "RealTimeNetworking.generated.h"

UCLASS()
class APEXPROTOCOL_API URealTimeNetworking : public UObject
{
	GENERATED_BODY()

public:
	URealTimeNetworking();

	UFUNCTION(BlueprintCallable, Category = "Unified Architecture | Networking")
	void InitializeSocketConnection(const FString& ServerURL);

	UFUNCTION(BlueprintCallable, Category = "Unified Architecture | Networking")
	void BroadcastStateChange(const FString& NodeID, bool bIsActive);

private:
	void OnMessageReceived(const FString& Message);
	void OnConnectionError(const FString& ErrorMessage);

	TSharedPtr<IWebSocket> SocketConnection;
};
