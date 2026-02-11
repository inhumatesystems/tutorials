// Copyright 2025 Inhumate AB. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"
#include "Components/ActorComponent.h"
#include "InhumateRTI.h"
#include "inhumaterti.hpp"
#include "RuntimeStateEnum.h"
#include "RTIScenario.h"

#include "RTISubsystem.generated.h"

struct FInjection;

namespace inhumate {
    namespace rti {
        constexpr auto UNREAL_INTEGRATION_VERSION = "0.2.0";
    }
}

// RTI game instance subsystem.
// Holds the connection (which is lazy-initialized), subscribes to runtime control,
// publishes connection and runtime control events (using dynamic multicast delegate)

UCLASS(DisplayName = "RTI Subsystem")
class INHUMATERTI_API URTISubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:

    static const int CONNECT_TIMEOUT = 5; // seconds

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

    inhumate::rti::RTIClient *RTI();

    // Properties that are normally set in project settings, but can be overridden runtime

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    FString Application = "";

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    FString Url = "";

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    FString Federation = "";

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    FString Secret = "";

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    FSoftObjectPath HomeLevel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    TArray<FRTIScenario> Scenarios;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    bool bUseCustomScenarioLoading = false;

    // Connection events

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FConnectedEvent);
    UPROPERTY(BlueprintAssignable, DisplayName = "Connected", Category = "RTI")
    FConnectedEvent ConnectedEvent;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDisconnectedEvent);
    UPROPERTY(BlueprintAssignable, DisplayName = "Disconnected", Category = "RTI")
    FDisconnectedEvent DisconnectedEvent;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FErrorEvent, FString, Channel, FString, Message);
    UPROPERTY(BlueprintAssignable, DisplayName = "Error", Category = "RTI")
    FErrorEvent ErrorEvent;

    // Runtime control events

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FStartEvent);
    UPROPERTY(BlueprintAssignable, DisplayName = "Start", Category = "RTI")
    FStartEvent StartEvent;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPauseEvent);
    UPROPERTY(BlueprintAssignable, DisplayName = "Pause", Category = "RTI")
    FPauseEvent PauseEvent;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FEndEvent);
    UPROPERTY(BlueprintAssignable, DisplayName = "End", Category = "RTI")
    FEndEvent EndEvent;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPlayEvent);
    UPROPERTY(BlueprintAssignable, DisplayName = "Play", Category = "RTI")
    FPlayEvent PlayEvent;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FStopEvent);
    UPROPERTY(BlueprintAssignable, DisplayName = "Stop", Category = "RTI")
    FStopEvent StopEvent;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FResetEvent);
    UPROPERTY(BlueprintAssignable, DisplayName = "Reset", Category = "RTI")
    FResetEvent ResetEvent;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLoadScenarioEvent, FString, Name, ERuntimeState, PreviousState);
    UPROPERTY(BlueprintAssignable, DisplayName = "Load Scenario", Category = "RTI")
    FLoadScenarioEvent LoadScenarioEvent;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FShutdownEvent);
    UPROPERTY(BlueprintAssignable, DisplayName = "Shutdown", Category = "RTI")
    FShutdownEvent ShutdownEvent;

    // Blueprint getters/setters

    UFUNCTION(BlueprintPure, Category = "RTI")
    FString ClientId();

    UFUNCTION(BlueprintPure, Category = "RTI")
    bool IsConnected();

    UFUNCTION(BlueprintCallable, Category = "RTI")
    ERuntimeState GetState();

    UFUNCTION(BlueprintCallable, Category = "RTI")
    void SetState(ERuntimeState state);

    UFUNCTION(BlueprintPure, Category = "RTI")
    float GetTime();

    UFUNCTION(BlueprintPure, Category = "RTI")
    float GetTimeScale();

    UFUNCTION(BlueprintPure, Category = "RTI")
    bool IsPersistentGeometryOwner();

    UFUNCTION(BlueprintPure, Category = "RTI")
    bool IsPersistentEntityOwner();

    UFUNCTION(BlueprintPure, Category = "RTI")
    bool HasPersistentEntityOwner();

    UFUNCTION(BlueprintPure, Category = "RTI")
    const FString& GetPersistentEntityOwnerClientId();

    UFUNCTION(BlueprintPure, Category = "RTI")
    const FString& GetCurrentScenarioName() { return CurrentScenarioName; }

    // Get scenario parameter value, or (if not specified in load message) default value
    UFUNCTION(BlueprintPure, Category = "RTI")
    FString GetScenarioParameterValue(const FString& ParameterName);

    // Connection functions

    UFUNCTION(BlueprintCallable, Category = "RTI")
    void Connect();

    UFUNCTION(BlueprintCallable, Category = "RTI")
    void Disconnect();

    // Runtime control functions

    UFUNCTION(BlueprintCallable, Category = "RTI")
    void PublishReset();

    UFUNCTION(BlueprintCallable, Category = "RTI")
    void PublishLoadScenario(const FString& ScenarioName);

    UFUNCTION(BlueprintCallable, Category = "RTI")
    void PublishStart();

    UFUNCTION(BlueprintCallable, Category = "RTI")
    void PublishPlay();

    UFUNCTION(BlueprintCallable, Category = "RTI")
    void PublishPause();

    UFUNCTION(BlueprintCallable, Category = "RTI")
    void PublishStop();

    // Reporting functions

    UFUNCTION(BlueprintCallable, Category = "RTI")
    void PublishString(const FString& Channel, const FString& Content);

    UFUNCTION(BlueprintCallable, Category = "RTI")
    void PublishError(const FString& ErrorMessage, const ERuntimeState State = ERuntimeState::UNKNOWN);

    UFUNCTION(BlueprintCallable, Category = "RTI")
    void PublishHeartbeat();

    UFUNCTION(BlueprintCallable, Category = "RTI")
    void PublishProgress(const uint8 Progress);

    UFUNCTION(BlueprintCallable, Category = "RTI")
    void PublishValue(const FString& Value, const bool Highlight = false, const bool Error = false);

    // Entities and geometry are centrally registered here so that the data survives level transitions

    void RegisterEntity(class URTIEntityComponent* Entity);
    void UnregisterEntity(class URTIEntityComponent* Entity);
    class URTIEntityComponent* GetEntityById(const FString& Id);
    TMap<FString, class URTIEntityComponent*> GetEntities() { return Entities; }

    bool RegisterGeometry(class URTIGeometryComponent* Geometry);
    void UnregisterGeometry(class URTIGeometryComponent* Geometry);
    class URTIGeometryComponent* GetGeometryById(const FString& Id);
    TMap<FString, class URTIGeometryComponent*> GetGeometries() { return Geometries; }

    bool GetInjectableAndInjection(const FString& injectionId, class URTIInjectableComponent** injectable, FInjection** injection);
    bool RegisterInjectable(class URTIInjectableComponent* Injectable);
    void UnregisterInjectable(class URTIInjectableComponent* Injectable);
    class URTIInjectableComponent* GetInjectableById(const FString& Id);
    TMap<FString, class URTIInjectableComponent*> GetInjectables() { return Injectables; }

    void WhenConnectedOnce(inhumate::rti::connectcallback_t callback);

protected:
    
    TUniquePtr<inhumate::rti::RTIClient> rti;

    FTSTicker::FDelegateHandle TickDelegateHandle;
    bool Tick(float deltaTime);

    void OnRuntimeControl(const std::string& channelName, const inhumate::rti::proto::RuntimeControl& message);
    void OnScenarios(const std::string& channelName, const inhumate::rti::proto::Scenarios& message);
    void OnEntity(const std::string &channelName, const inhumate::rti::proto::Entity& message);
    void OnEntityOperation(const std::string& channelName, const inhumate::rti::proto::EntityOperation& message);
    void OnGeometryOperation(const std::string& channelName, const inhumate::rti::proto::GeometryOperation& message);
    void OnInjectableOperation(const std::string& channelName, const inhumate::rti::proto::InjectableOperation& message);
    void OnInjectionOperation(const std::string& channelName, const inhumate::rti::proto::InjectionOperation& message);
    void OnClientDisconnect(const std::string& channelName, const std::string& clientId);
    void QueryPersistentGeometryOwner();
    void PublishClaimPersistentGeometryOwnership();
    void QueryPersistentEntityOwner();
    void PublishClaimPersistentEntityOwnership();

private:

    bool Initialized;
    bool AutoConnect = true;
    bool InhibitConnect;
    float Time = 0.0f;
    float TimeScale = 1.0f;
    FString LastWorldName;
    bool LoadingLevel;
    ERuntimeState StateBeforeLoadingLevel;
    TMap<FString, class URTIEntityComponent*> Entities;
    TMap<FString, class URTIGeometryComponent*> Geometries;
    TMap<FString, class URTIInjectableComponent*> Injectables;
    bool Connecting;
    FDateTime ConnectDateTime;
    int ConnectRetries;
    FString PersistentGeometryOwnerClientId;
    FString PersistentEntityOwnerClientId;

    bool RequestedGeometryOwnership;
    FDateTime RequestGeometryOwnershipDateTime;
    double ClaimGeometryOwnershipWaitTime;

    bool RequestedEntityOwnership;
    FDateTime RequestEntityOwnershipDateTime;
    double ClaimEntityOwnershipWaitTime;

    FString TimeSyncMasterClientId;
    float LastReceivedTimeSyncTime;
    float LastTimeSyncTime;
    
    FString CurrentScenarioName;
    TMap<FString, FString> ScenarioParameterValues;

public:

    // Internal (but public) getters for convenience

    const FRTIScenario* GetCurrentScenario() { return GetScenario(CurrentScenarioName); }
    FRTIScenario* GetScenario(const FString& Name);
    static inhumate::rti::RTIClient* GetRTIForComponent(UActorComponent* Component);
    static URTISubsystem* GetSubsystemForComponent(UActorComponent* Component);

};
