// Copyright 2025 Inhumate AB. All Rights Reserved.

#include "RTISubsystem.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Containers/Ticker.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "RTIEntityComponent.h"
#include "RTIEntityStateComponent.h"
#include "RTIGeometryComponent.h"
#include "RTIInjectableComponent.h"
#include "RTISettings.h"
#include <thread>

// see below
// #include "Runtime/AssetRegistry/Public/AssetRegistryModule.h"

using namespace std::placeholders;
FString GetProjectVersion(); // forward declaration

void URTISubsystem::Initialize(FSubsystemCollectionBase &Collection)
{
    // Initialize random seed (used for RTI instance id)
    srand(time(NULL));

    // Copy properties from settings
    auto settings = GetDefault<URTISettings>();
    if (settings) {
        AutoConnect = settings->AutoConnect;
        Application = settings->Application;
        Url = settings->Url;
        Federation = settings->Federation;
        Secret = settings->Secret;
        HomeLevel = settings->HomeLevel;
        Scenarios = settings->Scenarios;
    }

    // Get overrides from command-line arguments
    TArray<FString> Tokens;
    TArray<FString> Switches;
    FCommandLine::Parse(FCommandLine::GetOriginal(), Tokens, Switches);
    for (int i = 0; i < Tokens.Num(); i++) {
        auto Arg = Tokens[i];
        if (Arg.Equals(TEXT("-no-rti"), ESearchCase::IgnoreCase)) {
            InhibitConnect = true;
        } else if (Arg.Equals(TEXT("-rti"), ESearchCase::IgnoreCase) && i < Tokens.Num() - 1) {
            Url = Tokens[i + 1];
        } else if (Arg.Equals(TEXT("-rti-secret"), ESearchCase::IgnoreCase) && i < Tokens.Num() - 1) {
            Secret = Tokens[i + 1];
        } else if (Arg.Equals(TEXT("-rti-clientname"), ESearchCase::IgnoreCase) && i < Tokens.Num() - 1) {
            Application = Tokens[i + 1];
        }
    }

    if (Application.IsEmpty()) Application = FString(FApp::GetProjectName());

    auto TickDelegate = FTickerDelegate::CreateUObject(this, &URTISubsystem::Tick);
    TickDelegateHandle = FTSTicker::GetCoreTicker().AddTicker(TickDelegate);

    if (!HomeLevel.IsValid()) {
        UE_LOG(LogRTI, Log, TEXT("No home level defined in RTI settings"));
    }

    if (Scenarios.Num() == 0) {
        UE_LOG(LogRTI, Log, TEXT("No scenarios defined in RTI settings"));
        // If we haven't defined scenarios we could find levels in the project.
        // But: this works in an actor, but not here... lots of macro errors. Leaving it here in case a solution comes up.
        /*
        FAssetRegistryModule& AssetRegistryModule =
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry"); TArray<FAssetData>
        AssetData; FARFilter Filter; Filter.ClassNames.Add(TEXT("World"));
        Filter.PackagePaths.Add("/Game");
        Filter.bRecursivePaths = true;
        if (AssetRegistryModule.Get().GetAssets(Filter, AssetData)) {
            for (FAssetData data : AssetData) {
                //UE_LOG(LogTemp, Warning, TEXT("Asset name %s package %s class %s"),
        *data.AssetName.ToString(), *data.PackagePath.ToString(), *data.AssetClass.ToString());
        FString name = data.AssetName.ToString(); if (name != HomeLevel) { UE_LOG(LogRTI, Log,
        TEXT("Found scenario level %s"), *name); Scenarios.Add(name);
                }
            }
        } else {
            UE_LOG(LogRTI, Error, TEXT("RTI settings scenarios list is empty and failed to get a
        list of levels"));
        }
        */
    }
    if (AutoConnect) RTI();
    Initialized = true;
}

void URTISubsystem::Deinitialize()
{
    Initialized = false;
     if (TickDelegateHandle.IsValid()) {
         FTSTicker::GetCoreTicker().RemoveTicker(TickDelegateHandle);
         TickDelegateHandle.Reset();
     }
#if WITH_EDITOR
    try {
#endif
    if (rti && rti->connected()) {
        // Squeeze those last requests through (destroy geometry, entities etc)
        for (int i = 0; rti->Poll() > 0 && i < 1000; i++);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        for (int i = 0; rti->Poll() > 0 && i < 1000; i++);
        // Disconnect
        if (rti->connected()) rti->Disconnect();
        // Poll for a while so that we actually disconnect gracefully
        int count = 0;
        while (++count < 10) {
            rti->Poll();
            if (!rti->connected()) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    rti.Reset();
#if WITH_EDITOR
    } catch (std::exception& e) {
        UE_LOG(LogRTI, Error, TEXT("RTI deinitialize error (editor only): %s"), *FString(e.what()));
    }
#endif
}

FString URTISubsystem::ClientId() {
    if (rti) return FString(rti->client_id().c_str());
    return FString("");
}

bool URTISubsystem::IsConnected()
{
    return rti && rti->connected();
}

ERuntimeState URTISubsystem::GetState()
{
    return rti ? RuntimeStateFromPbState(rti->state()) : ERuntimeState::UNKNOWN;
}

void URTISubsystem::SetState(ERuntimeState state)
{
    if (rti) rti->set_state(PbStateFromRuntimeState(state));
}

float URTISubsystem::GetTime()
{
    return Time;
}

float URTISubsystem::GetTimeScale()
{
    return TimeScale;
}

bool URTISubsystem::IsPersistentGeometryOwner()
{
    return rti && PersistentGeometryOwnerClientId == FString(rti->client_id().c_str());
}

bool URTISubsystem::IsPersistentEntityOwner()
{
    return rti && PersistentEntityOwnerClientId == FString(rti->client_id().c_str());
}

bool URTISubsystem::HasPersistentEntityOwner()
{
    return PersistentEntityOwnerClientId.Len() > 0;
}

const FString &URTISubsystem::GetPersistentEntityOwnerClientId()
{
    return PersistentEntityOwnerClientId;
}


FString URTISubsystem::GetScenarioParameterValue(const FString& ParameterName)
{
    if (ScenarioParameterValues.Contains(ParameterName)) return ScenarioParameterValues[ParameterName];
    auto Scenario = GetCurrentScenario();
    if (Scenario != nullptr) {
        for (auto parameter : Scenario->Parameters) {
            if (parameter.Name == ParameterName) return parameter.DefaultValue;
        }
    }
    return "";
}

void URTISubsystem::Connect()
{
    if (InhibitConnect) {
        UE_LOG(LogRTI, Log, TEXT("RTI connect inhibited"));
    } else if (!rti && AutoConnect && RTI()) {
        // RTI() calls connect, so we're already done
    } else if (rti || (!AutoConnect && RTI())) {
        UE_LOG(LogRTI, Log, TEXT("RTI connecting %s %s to %s"), *Application,
               *FString(rti->client_id().c_str()),
               (Url.Len() > 0 ? *Url : *FString(inhumate::rti::RTI_DEFAULT_URL)));
        rti->Connect();
        Connecting = true;
        ConnectDateTime = FDateTime::UtcNow();
    } else {
        UE_LOG(LogRTI, Error, TEXT("No RTI!?"));
    }
}

void URTISubsystem::Disconnect()
{
    if (rti) rti->Disconnect();
}

void URTISubsystem::PublishReset()
{
    if (!IsConnected()) return;
    inhumate::rti::proto::RuntimeControl message;
    message.set_allocated_reset(new google::protobuf::Empty());
    RTI()->Publish(inhumate::rti::CONTROL_CHANNEL, message);
}

void URTISubsystem::PublishLoadScenario(const FString &ScenarioName)
{
    if (!IsConnected()) return;
    inhumate::rti::proto::RuntimeControl message;
    inhumate::rti::proto::RuntimeControl_LoadScenario *load = new inhumate::rti::proto::RuntimeControl_LoadScenario();
    load->set_name(TCHAR_TO_UTF8(*ScenarioName));
    message.set_allocated_load_scenario(load);
    RTI()->Publish(inhumate::rti::CONTROL_CHANNEL, message);
}

void URTISubsystem::PublishStart()
{
    if (!IsConnected()) return;
    inhumate::rti::proto::RuntimeControl message;
    message.set_allocated_start(new google::protobuf::Empty());
    RTI()->Publish(inhumate::rti::CONTROL_CHANNEL, message);
}

void URTISubsystem::PublishPlay()
{
    if (!IsConnected()) return;
    inhumate::rti::proto::RuntimeControl message;
    message.set_allocated_play(new google::protobuf::Empty());
    RTI()->Publish(inhumate::rti::CONTROL_CHANNEL, message);
}

void URTISubsystem::PublishPause()
{
    if (!IsConnected()) return;
    inhumate::rti::proto::RuntimeControl message;
    message.set_allocated_pause(new google::protobuf::Empty());
    RTI()->Publish(inhumate::rti::CONTROL_CHANNEL, message);
}

void URTISubsystem::PublishStop()
{
    if (!IsConnected()) return;
    inhumate::rti::proto::RuntimeControl message;
    message.set_allocated_stop(new google::protobuf::Empty());
    RTI()->Publish(inhumate::rti::CONTROL_CHANNEL, message);
}

void URTISubsystem::PublishString(const FString &Channel, const FString &Content)
{
    if (rti && rti->connected()) {
        rti->Publish(TCHAR_TO_UTF8(*Channel), TCHAR_TO_UTF8(*Content));
    } else {
        UE_LOG(LogRTI, Error, TEXT("URTISubsystem::PublishString called when not connected"));
    }
}

void URTISubsystem::PublishError(const FString &ErrorMessage, const ERuntimeState State)
{
    if (IsConnected()) 
    {
        inhumate::rti::proto::RuntimeState PbState = inhumate::rti::proto::RuntimeState::UNKNOWN;
        if (State == ERuntimeState::UNKNOWN)
            PbState = RTI()->state();
        else
            PbState = PbStateFromRuntimeState(State);
        
        RTI()->PublishError(TCHAR_TO_UTF8(*ErrorMessage), PbState);
    }
    UE_LOG(LogRTI, Error, TEXT("%s"), *ErrorMessage);
}

void URTISubsystem::PublishHeartbeat()
{
    if (!IsConnected()) return;
    RTI()->PublishHeartbeat();
}

void URTISubsystem::PublishProgress(const uint8 Progress)
{
    if (!IsConnected()) return;
    RTI()->PublishProgress(Progress);
}

void URTISubsystem::PublishValue(const FString &Value, const bool Highlight, const bool Error)
{
    if (!IsConnected()) return;
    RTI()->PublishValue(TCHAR_TO_UTF8(*Value), Highlight, Error);
}


void URTISubsystem::RegisterEntity(URTIEntityComponent *Entity)
{
    Entities.Add(Entity->Id);
    Entities[Entity->Id] = Entity;
}

void URTISubsystem::UnregisterEntity(URTIEntityComponent *Entity)
{
    Entities.Remove(Entity->Id);
}

URTIEntityComponent *URTISubsystem::GetEntityById(const FString &Id)
{
    if (Entities.Contains(Id)) return Entities[Id];
    return nullptr;
}

bool URTISubsystem::RegisterGeometry(URTIGeometryComponent *Geometry)
{
    if (Geometries.Contains(Geometry->Id) && Geometries[Geometry->Id] != Geometry) {
        return false;
    } else {
        Geometries.Add(Geometry->Id);
        Geometries[Geometry->Id] = Geometry;
        return true;
    }
}

void URTISubsystem::UnregisterGeometry(URTIGeometryComponent *Geometry)
{
    if (Geometries.Contains(Geometry->Id) && Geometries[Geometry->Id] == Geometry) {
        Geometries.Remove(Geometry->Id);
    }
}

URTIGeometryComponent *URTISubsystem::GetGeometryById(const FString &Id)
{
    if (Geometries.Contains(Id)) return Geometries[Id];
    return nullptr;
}

bool URTISubsystem::GetInjectableAndInjection(const FString& injectionId, URTIInjectableComponent** outInjectable, FInjection** outInjection)
{
    for (auto pair : Injectables) {
        auto injectable = pair.Value;
        auto injection = injectable->GetInjection(injectionId);
        if (injection != nullptr) {
            *outInjectable = injectable;
            *outInjection = injection;
            return true;
        }
    }
    return false;
}

bool URTISubsystem::RegisterInjectable(URTIInjectableComponent *Injectable)
{
    if (Injectables.Contains(Injectable->Name) && Injectables[Injectable->Name] != Injectable) {
        return false;
    } else {
        Injectables.Add(Injectable->Name);
        Injectables[Injectable->Name] = Injectable;
        return true;
    }
}

void URTISubsystem::UnregisterInjectable(URTIInjectableComponent *Injectable)
{
    if (Injectables.Contains(Injectable->Name) && Injectables[Injectable->Name] == Injectable) {
        Injectables.Remove(Injectable->Name);
    }
}

URTIInjectableComponent *URTISubsystem::GetInjectableById(const FString &Name)
{
    if (Injectables.Contains(Name)) return Injectables[Name];
    return nullptr;
}

void URTISubsystem::WhenConnectedOnce(inhumate::rti::connectcallback_t callback)
{
    if (IsConnected()) {
        callback();
    } else {
        inhumate::rti::connectcallback_p callback_ptr;
        callback_ptr = RTI()->OnConnected([this, callback, callback_ptr]() {
            callback();
            RTI()->OffConnected(callback_ptr);
        });
    }
}

inhumate::rti::RTIClient *URTISubsystem::RTI()
{
    // Lazy initialization
    if (!rti) {
        if (Application.Len() == 0) Application = TEXT("Unreal");
        rti = TUniquePtr<inhumate::rti::RTIClient>(
            new inhumate::rti::RTIClient(TCHAR_TO_UTF8(*Application), false, TCHAR_TO_UTF8(*Url), TCHAR_TO_UTF8(*Federation), TCHAR_TO_UTF8(*Secret))
        );
        rti->set_application_version(TCHAR_TO_UTF8(*GetProjectVersion()));
        rti->set_engine_version(TCHAR_TO_UTF8(*FString(FApp::GetBuildVersion()).Replace(TEXT("+"), TEXT(""))));
        if (FString(inhumate::rti::UNREAL_INTEGRATION_VERSION) != "0.0.1-dev-version") {
            rti->set_integration_version(inhumate::rti::UNREAL_INTEGRATION_VERSION);
        }
        rti->add_capability(inhumate::rti::RUNTIME_CONTROL_CAPABILITY);
        rti->add_capability(inhumate::rti::SCENARIO_CAPABILITY);
        rti->add_capability(inhumate::rti::TIME_SCALE_CAPABILITY);

        std::function<void(const std::string&, const inhumate::rti::proto::RuntimeControl&)> onRuntimeControl = std::bind(&URTISubsystem::OnRuntimeControl, this, _1, _2);
        rti->Subscribe(inhumate::rti::CONTROL_CHANNEL, onRuntimeControl);
        auto OwnControlChannel = FString::Printf(TEXT("@%s:%s"), *FString(rti->client_id().c_str()), *FString(inhumate::rti::CONTROL_CHANNEL));
        rti->Subscribe(TCHAR_TO_UTF8(*OwnControlChannel), onRuntimeControl);

        std::function<void(const std::string&, const inhumate::rti::proto::Scenarios&)> onScenarios = std::bind(&URTISubsystem::OnScenarios, this, _1, _2);
        rti->Subscribe(inhumate::rti::SCENARIOS_CHANNEL, onScenarios);

        inhumate::rti::proto::Channel entityChannel;
        entityChannel.set_name(inhumate::rti::ENTITY_CHANNEL);
        entityChannel.set_data_type("Entity");
        entityChannel.set_first_field_id(true);
        entityChannel.set_state(true);
        rti->RegisterChannel(entityChannel);
        std::function<void(const std::string &, const inhumate::rti::proto::Entity&)> onEntity = std::bind(&URTISubsystem::OnEntity, this, _1, _2);
        rti->Subscribe(inhumate::rti::ENTITY_CHANNEL, onEntity);
        inhumate::rti::proto::Channel entityOperationChannel;
        entityOperationChannel.set_name(inhumate::rti::ENTITY_OPERATION_CHANNEL);
        entityOperationChannel.set_data_type("EntityOperation");
        entityOperationChannel.set_ephemeral(true);
        rti->RegisterChannel(entityOperationChannel);
        std::function<void(const std::string &, const inhumate::rti::proto::EntityOperation &)> onEntityOperation = std::bind(&URTISubsystem::OnEntityOperation, this, _1, _2);
        rti->Subscribe(inhumate::rti::ENTITY_OPERATION_CHANNEL, onEntityOperation);

        inhumate::rti::proto::Channel geometryChannel;
        geometryChannel.set_name(inhumate::rti::GEOMETRY_CHANNEL);
        geometryChannel.set_data_type("Geometry");
        geometryChannel.set_first_field_id(true);
        geometryChannel.set_state(true);
        rti->RegisterChannel(geometryChannel);
        inhumate::rti::proto::Channel geometryOperationChannel;
        geometryOperationChannel.set_name(inhumate::rti::GEOMETRY_OPERATION_CHANNEL);
        geometryOperationChannel.set_data_type("GeometryOperation");
        geometryOperationChannel.set_ephemeral(true);
        rti->RegisterChannel(geometryOperationChannel);
        std::function<void(const std::string &, const inhumate::rti::proto::GeometryOperation &)> onGeometryOperation = std::bind(&URTISubsystem::OnGeometryOperation, this, _1, _2);
        rti->Subscribe(inhumate::rti::GEOMETRY_OPERATION_CHANNEL, onGeometryOperation);

        std::function<void(const std::string&, const inhumate::rti::proto::InjectableOperation&)> onInjectableOperation = std::bind(&URTISubsystem::OnInjectableOperation, this, _1, _2);
        rti->Subscribe(inhumate::rti::INJECTABLE_OPERATION_CHANNEL, onInjectableOperation);
        std::function<void(const std::string&, const inhumate::rti::proto::InjectionOperation&)> onInjectionOperation = std::bind(&URTISubsystem::OnInjectionOperation, this, _1, _2);
        rti->Subscribe(inhumate::rti::INJECTION_OPERATION_CHANNEL, onInjectionOperation);
        inhumate::rti::proto::Channel injectionChannel;
        injectionChannel.set_name(inhumate::rti::INJECTION_CHANNEL);
        injectionChannel.set_data_type("Injection");
        injectionChannel.set_first_field_id(true);
        injectionChannel.set_state(true);
        rti->RegisterChannel(injectionChannel);

        std::function<void(const std::string&, const std::string&)> onClientDisconnect = std::bind(&URTISubsystem::OnClientDisconnect, this, _1, _2);        
        rti->Subscribe(inhumate::rti::CLIENT_DISCONNECT_CHANNEL, onClientDisconnect);

        rti->OnConnected([this]() {
            Connecting = false;
            UE_LOG(LogRTI, Log, TEXT("RTI connected"));
            
            ConnectedEvent.Broadcast();

            if (PersistentGeometryOwnerClientId.IsEmpty()) QueryPersistentGeometryOwner();
            if (PersistentEntityOwnerClientId.IsEmpty()) QueryPersistentEntityOwner();
        });
        rti->OnDisconnected([this]() {
            Connecting = false;
            if (Initialized) {
                UE_LOG(LogRTI, Warning, TEXT("RTI disconnected unexpectedly"));
            } else {
                UE_LOG(LogRTI, Log, TEXT("RTI disconnected as expected"));
            }
            DisconnectedEvent.Broadcast();
        });
        rti->OnError([this](const std::string &channel, const std::string &error) {
            auto Channel = FString(channel.c_str());
            auto Error = FString(error.c_str());
            UE_LOG(LogRTI, Error, TEXT("RTI error %s %s"), *Channel, *Error);
            ErrorEvent.Broadcast(Channel, Error);
        });
        URTIEntityStateComponent::ResetSubscriptions();
        if (AutoConnect) Connect();
    }
    return rti.Get();
}

bool URTISubsystem::Tick(float DeltaTime)
{
    if (!bUseCustomScenarioLoading) {
        // Poor mans "callback" for when level finished loading - is there a better way?
        FString WorldName = GetWorld() ? GetWorld()->GetName() : "NULL";
        if (!WorldName.Equals(LastWorldName, ESearchCase::CaseSensitive)) {
            if (LoadingLevel) {
                LoadingLevel = false;
                Time = 0;
                if (WorldName.Equals(HomeLevel.GetAssetName(), ESearchCase::CaseSensitive)) {
                    UE_LOG(LogRTI, Log, TEXT("Home level loaded"));
                } else {
                    UE_LOG(LogRTI, Log, TEXT("Level loaded: %s"), *(GetWorld()->GetName()));
                    if (StateBeforeLoadingLevel == ERuntimeState::PLAYBACK) {
                        UGameplayStatics::SetGamePaused(GetWorld(), false);
                        SetState(ERuntimeState::PLAYBACK);
                    } else {
                        UGameplayStatics::SetGamePaused(GetWorld(), true);
                        SetState(ERuntimeState::READY);
                    }
                }
            } else if (!WorldName.Equals(HomeLevel.GetAssetName(), ESearchCase::CaseSensitive) &&
                       !LastWorldName.IsEmpty() && !HomeLevel.IsValid()) {
                UE_LOG(LogRTI, Warning, TEXT("World name changed %s"), *WorldName);
            }
            if (WorldName.Equals(HomeLevel.GetAssetName(), ESearchCase::CaseSensitive))
                SetState(ERuntimeState::INITIAL);
            LastWorldName = WorldName;
        }
    }

    if (rti && (rti->state() == inhumate::rti::proto::RuntimeState::RUNNING ||
                rti->state() == inhumate::rti::proto::RuntimeState::PLAYBACK)) {
        Time += DeltaTime;
    }

    if (rti && (rti->state() == inhumate::rti::proto::RuntimeState::RUNNING || rti->state() == inhumate::rti::proto::RuntimeState::UNKNOWN)
            && ((TimeSyncMasterClientId.IsEmpty() && FMath::Abs(Time - LastTimeSyncTime) >= 1.0f + FMath::RandRange(0.5f, 1.5f))
            || (TimeSyncMasterClientId == rti->client_id().c_str() && FMath::Abs(Time - LastTimeSyncTime) >= 1.0f))) {
        LastTimeSyncTime = Time;
        if (rti->connected()) {
            inhumate::rti::proto::RuntimeControl message;
            auto timesync = new inhumate::rti::proto::RuntimeControl_TimeSync();
            timesync->set_time(Time);
            timesync->set_time_scale(TimeScale);
            timesync->set_master_client_id(rti->client_id());
            message.set_allocated_time_sync(timesync);
            rti->Publish(inhumate::rti::CONTROL_CHANNEL, message);
        }
    }

    // Process up to 100 queued operations per tick
#if WITH_EDITOR
    try {
#endif
    for (int i = 0; Initialized && rti && rti->Poll() > 0 && i < 100; i++)
        ;
#if WITH_EDITOR
    } catch (std::exception& e) {
        UE_LOG(LogRTI, Error, TEXT("RTI poll error (editor only): %s"), *FString(e.what()));
    }
#endif

    // Retry and check for connection error (as we don't have exception handling in Unreal and websocketpp/asio uses that...)
    if (Connecting && (FDateTime::UtcNow() - ConnectDateTime).GetTotalSeconds() > CONNECT_TIMEOUT) {
        if (!rti) {
            Connecting = false;
            UE_LOG(LogRTI, Error, TEXT("RTI disappeared?"));
        } else if (ConnectRetries < 3) {
            ConnectRetries++;
            UE_LOG(LogRTI, Log, TEXT("RTI retry connection after %d seconds"), CONNECT_TIMEOUT);
            rti->Connect();
            ConnectDateTime = FDateTime::UtcNow();
        } else {
            Connecting = false;
            UE_LOG(LogRTI, Error, TEXT("RTI connection timeout after %d seconds"), CONNECT_TIMEOUT);
            ErrorEvent.Broadcast(FString("connection"), FString("Connection timeout"));
        }
    }

    if (rti && rti->connected() && RequestedGeometryOwnership && (FDateTime::UtcNow() - RequestGeometryOwnershipDateTime).GetTotalSeconds() > ClaimGeometryOwnershipWaitTime) {
        RequestedGeometryOwnership = false;
        if (PersistentGeometryOwnerClientId.IsEmpty()) {
            UE_LOG(LogRTI, Log, TEXT("Claiming persistent geometry ownership for %s"), *FString(rti->application().c_str()));
            PersistentGeometryOwnerClientId = FString(rti->client_id().c_str());
            PublishClaimPersistentGeometryOwnership();
        }
    }

    if (rti && rti->connected() && RequestedEntityOwnership && (FDateTime::UtcNow() - RequestEntityOwnershipDateTime).GetTotalSeconds() > ClaimEntityOwnershipWaitTime) {
        RequestedEntityOwnership = false;
        if (PersistentEntityOwnerClientId.IsEmpty()) {
            UE_LOG(LogRTI, Log, TEXT("Claiming persistent entity ownership for %s"), *FString(rti->application().c_str()));
            PersistentEntityOwnerClientId = FString(rti->client_id().c_str());
            PublishClaimPersistentEntityOwnership();
        }
    }

    return true;
}

void URTISubsystem::OnRuntimeControl(const std::string &channelName, const inhumate::rti::proto::RuntimeControl &message)
{
    switch (message.control_case()) {
    case inhumate::rti::proto::RuntimeControl::ControlCase::kStart:
        UE_LOG(LogRTI, Log, TEXT("Start"));
        if (!bUseCustomScenarioLoading) UGameplayStatics::SetGamePaused(GetWorld(), false);
        if (!HomeLevel.IsValid() && GetWorld()->GetName() == HomeLevel.GetAssetName() && !bUseCustomScenarioLoading) {
            PublishError("Cannot start - No level loaded", ERuntimeState::RUNNING);
        } else {
            if (GetState() != ERuntimeState::PAUSED && GetState() != ERuntimeState::PLAYBACK_PAUSED) {
                Time = 0;
                TimeSyncMasterClientId = "";
            }
            SetState(ERuntimeState::RUNNING);
            GetWorld()->GetWorldSettings()->SetTimeDilation(TimeScale);
            StartEvent.Broadcast();
        }
        break;
    case inhumate::rti::proto::RuntimeControl::ControlCase::kPause:
        UE_LOG(LogRTI, Log, TEXT("Pause"));
        if (!bUseCustomScenarioLoading) UGameplayStatics::SetGamePaused(GetWorld(), true);
        SetState(GetState() == ERuntimeState::PLAYBACK ? ERuntimeState::PLAYBACK_PAUSED : ERuntimeState::PAUSED);
        PauseEvent.Broadcast();
        break;
    case inhumate::rti::proto::RuntimeControl::ControlCase::kEnd:
        if (GetState() == ERuntimeState::RUNNING) {
            UE_LOG(LogRTI, Log, TEXT("End"));
            if (!bUseCustomScenarioLoading) UGameplayStatics::SetGamePaused(GetWorld(), true);
            SetState(ERuntimeState::END);
            EndEvent.Broadcast();
        } else if (GetState() == ERuntimeState::PLAYBACK) {
            UE_LOG(LogRTI, Log, TEXT("Playback end"));
            UGameplayStatics::SetGamePaused(GetWorld(), true);
            SetState(ERuntimeState::PLAYBACK_END);
            EndEvent.Broadcast();
        } else {
            UE_LOG(LogRTI, Warning, TEXT("Unexpected end from invalid state"));
        }
        break;
    case inhumate::rti::proto::RuntimeControl::ControlCase::kPlay:
        UE_LOG(LogRTI, Log, TEXT("Play"));
        if (!bUseCustomScenarioLoading) UGameplayStatics::SetGamePaused(GetWorld(), false);
        SetState(ERuntimeState::PLAYBACK);
        GetWorld()->GetWorldSettings()->SetTimeDilation(TimeScale);
        TimeSyncMasterClientId = "";
        PlayEvent.Broadcast();
        break;
    case inhumate::rti::proto::RuntimeControl::ControlCase::kStop:
        UE_LOG(LogRTI, Log, TEXT("Stop"));
        if (!bUseCustomScenarioLoading) UGameplayStatics::SetGamePaused(GetWorld(), true);
        for (auto &it : GetEntities()) {
            auto entity = it.Value;
            entity->GetOwner()->Destroy();
        }
        Entities.Reset();
        for (auto &it : GetInjectables()) {
            auto injectable = it.Value;
            injectable->GetOwner()->Destroy();
        }
        Injectables.Reset();

        switch (GetState()) {
            case ERuntimeState::PLAYBACK:
            case ERuntimeState::PLAYBACK_PAUSED:
            case ERuntimeState::PLAYBACK_END:
                SetState(ERuntimeState::PLAYBACK_STOPPED);
                break;
            default:
                SetState(ERuntimeState::STOPPED);
                break;
        }
        StopEvent.Broadcast();
        break;
    case inhumate::rti::proto::RuntimeControl::ControlCase::kReset:
        UE_LOG(LogRTI, Log, TEXT("Reset"));
        CurrentScenarioName = "";
        ScenarioParameterValues.Reset();
        if (!bUseCustomScenarioLoading) {
            UGameplayStatics::SetGamePaused(GetWorld(), true);
            if (!HomeLevel.IsValid()) {
                UE_LOG(LogRTI, Warning, TEXT("No home level specified"));
            } else if (GetWorld()->GetName() != HomeLevel.GetAssetName()) {
                SetState(ERuntimeState::STOPPING);
                UGameplayStatics::OpenLevel(GetWorld(), *HomeLevel.GetAssetName(), false, TEXT(""));
                LoadingLevel = true;
                // see Tick() for "callback" when loading is finished
            }
        }
        ResetEvent.Broadcast();
        break;
    case inhumate::rti::proto::RuntimeControl::ControlCase::kLoadScenario: {
        FString scenarioName = UTF8_TO_TCHAR(message.load_scenario().name().c_str());
        if (!GetScenario(scenarioName) && GetScenario("*")) {
            scenarioName = "*";
        }
        auto Scenario = GetScenario(scenarioName);
        if (Scenario != nullptr || bUseCustomScenarioLoading) {
            UE_LOG(LogRTI, Log, TEXT("Load scenario %s"), *scenarioName);
            CurrentScenarioName = scenarioName;
            ScenarioParameterValues.Reset();
            for (auto entry = message.load_scenario().parameter_values().begin(); entry != message.load_scenario().parameter_values().end(); entry++) {
                ScenarioParameterValues.Add(FString(entry->first.c_str()), FString(entry->second.c_str()));
            }
            auto previousState = GetState();
            LoadScenarioEvent.Broadcast(scenarioName, previousState);
            if (!bUseCustomScenarioLoading) {
                FString levelName = Scenario->Level.GetAssetName();
                FString worldName = GetWorld() ? GetWorld()->GetName() : "NULL";
                if (worldName == levelName) {
                    if (previousState == ERuntimeState::PLAYBACK) {
                        UE_LOG(LogRTI, Log, TEXT("Scenario already loaded for playback: %s"), *scenarioName);
                    } else {
                        // Kludge: go back to home then load level, because loading same level doesn't reset actors etc...
                        if (!HomeLevel.IsValid() && GetWorld()->GetName() != HomeLevel.GetAssetName() && !bUseCustomScenarioLoading) {
                            SetState(ERuntimeState::STOPPING);
                            UGameplayStatics::OpenLevel(GetWorld(), *HomeLevel.GetAssetName(), false, TEXT(""));
                            SetState(ERuntimeState::LOADING);
                            UGameplayStatics::OpenLevel(GetWorld(), *levelName, false,
                                                        previousState == ERuntimeState::PLAYBACK ? TEXT("") : TEXT("?paused=1"));
                            StateBeforeLoadingLevel = previousState;
                            LoadingLevel = true;
                            LastWorldName = HomeLevel.GetAssetName();
                        } else {
                            UE_LOG(LogRTI, Log, TEXT("Scenario already loaded: %s"), *scenarioName);
                        }
                    }
                } else {
                    SetState(ERuntimeState::LOADING);
                    UGameplayStatics::OpenLevel(GetWorld(), *levelName, false,
                                                previousState == ERuntimeState::PLAYBACK ? TEXT("") : TEXT("?paused=1"));
                    StateBeforeLoadingLevel = previousState;
                    LoadingLevel = true;
                    // see Tick() for "callback" when loading is finished
                }
            }
        } else {
            PublishError("Unknown scenario " + scenarioName);
            CurrentScenarioName = "";
            ScenarioParameterValues.Reset();
        }
        break;
    }
    case inhumate::rti::proto::RuntimeControl::ControlCase::kSetTimeScale:
        if (FMath::Abs(message.set_time_scale().time_scale()) < 1e-5f) {
            UE_LOG(LogRTI, Warning, TEXT("Invalid time scale %f"), TimeScale);
        } else {
            TimeScale = message.set_time_scale().time_scale();
            UE_LOG(LogRTI, Log, TEXT("Set time scale %.1f"), TimeScale);
            GetWorld()->GetWorldSettings()->SetTimeDilation(TimeScale);
        }
        break;
    case inhumate::rti::proto::RuntimeControl::ControlCase::kSeek:
        Time = message.seek().time();
        UE_LOG(LogRTI, Log, TEXT("Seek %.1f"), Time);
        break;
    case inhumate::rti::proto::RuntimeControl::ControlCase::kTimeSync: {
        TimeSyncMasterClientId = message.time_sync().master_client_id().c_str();
        LastReceivedTimeSyncTime = GetWorld()->GetRealTimeSeconds();
        auto diff = FMath::Abs(Time - (float)message.time_sync().time());
        if (diff > 0.5f) {
            UE_LOG(LogRTI, Warning, TEXT("Time sync diff %.3f"), diff);
            Time = (float)message.time_sync().time();
        }
        diff = FMath::Abs(TimeScale - (float)message.time_sync().time_scale());
        if (diff > 0.01f) {
            if (FMath::Abs(message.time_sync().time_scale()) < 1e-5f) {
                UE_LOG(LogRTI, Warning, TEXT("Sync invalid time scale %f"), TimeScale);
            } else {
                UE_LOG(LogRTI, Warning, TEXT("Sync time scale to %.1f"), message.time_sync().time_scale());
                TimeScale = (float)message.time_sync().time_scale();
                GetWorld()->GetWorldSettings()->SetTimeDilation(TimeScale);
            }
        }
        break;
    }
    case inhumate::rti::proto::RuntimeControl::ControlCase::kRequestCurrentScenario: {
        if (!CurrentScenarioName.IsEmpty()) {
            auto currentScenario = new inhumate::rti::proto::RuntimeControl_LoadScenario();
            currentScenario->set_name(TCHAR_TO_UTF8(*CurrentScenarioName));
            for (auto& pair : ScenarioParameterValues) {
                (*currentScenario->mutable_parameter_values())[TCHAR_TO_UTF8(*pair.Key)] = TCHAR_TO_UTF8(*pair.Value);
            }
            inhumate::rti::proto::RuntimeControl response;
            response.set_allocated_current_scenario(currentScenario);
            rti->Publish(inhumate::rti::CONTROL_CHANNEL, response);
        }
        break;
    }
    case inhumate::rti::proto::RuntimeControl::ControlCase::kShutdown:
        ShutdownEvent.Broadcast();
        break;
    }
}

void URTISubsystem::OnScenarios(const std::string &channelName, const inhumate::rti::proto::Scenarios &message)
{
    switch (message.which_case()) {
    case inhumate::rti::proto::Scenarios::WhichCase::kRequestScenarios:
        if (Scenarios.Num() == 0) {
            UE_LOG(LogRTI, Warning, TEXT("No scenarios defined in RTI settings"));
        } else {
            for (auto scenario : Scenarios) {
                auto pbScenario = new inhumate::rti::proto::Scenario();
                ScenarioToPb(scenario, pbScenario);
                inhumate::rti::proto::Scenarios scenarioMessage;
                scenarioMessage.set_allocated_scenario(pbScenario);
                rti->Publish(inhumate::rti::SCENARIOS_CHANNEL, scenarioMessage);
            }
        }
        break;
    }
}

void URTISubsystem::OnEntity(const std::string& channelName, const inhumate::rti::proto::Entity& message)
{
    FString Id = FString(message.id().c_str());
    URTIEntityComponent *entity = GetEntityById(Id);
    if (entity != nullptr && entity->Persistent && !entity->Owned) {
        if (message.deleted()) {
            UE_LOG(LogRTI, Warning, TEXT("Destroy deleted persistent entity %s"), *Id);
            entity->Deleted = true;
            UnregisterEntity(entity);
            entity->GetOwner()->Destroy();
        } else {
            UE_LOG(LogRTI, Warning, TEXT("Update persistent entity %s"), *Id);
            entity->SetPropertiesFromEntityData(message);
            entity->UpdatedEvent.Broadcast();
        }
    }
}


void URTISubsystem::OnEntityOperation(const std::string &channelName, const inhumate::rti::proto::EntityOperation &message)
{
    switch (message.which_case()) {
    case inhumate::rti::proto::EntityOperation::WhichCase::kRequestUpdate: {
        UE_LOG(LogRTI, Log, TEXT("Entity updates requested"));
        for (auto &it : GetEntities()) {
            auto entity = it.Value;
            if (entity->IsPublishing()) entity->RequestUpdate();
        }
        break;
    }
    case inhumate::rti::proto::EntityOperation::WhichCase::kTransferOwnership: {
        FString Id = FString(message.transfer_ownership().entity_id().c_str());
        auto entity = GetEntityById(Id);
        if (entity != nullptr) {
            FString TransferToId = FString(message.transfer_ownership().entity_id().c_str());
            if (entity->Owned && entity->OwnerClientId == ClientId() && TransferToId != ClientId()) {
                entity->Owned = false;
                entity->OwnerClientId = TransferToId;
                inhumate::rti::proto::EntityOperation opMessage;
                auto release = new inhumate::rti::proto::EntityOperation_EntityClient();
                release->set_entity_id(TCHAR_TO_UTF8(*Id));
                release->set_client_id(TCHAR_TO_UTF8(*ClientId()));
                opMessage.set_allocated_release_ownership(release);
                rti->Publish(inhumate::rti::ENTITY_OPERATION_CHANNEL, opMessage);
            } else if (!entity->Owned && entity->OwnerClientId != ClientId() && TransferToId == ClientId()) {
                entity->Owned = true;
                entity->OwnerClientId = ClientId();
                inhumate::rti::proto::EntityOperation opMessage;
                auto assume = new inhumate::rti::proto::EntityOperation_EntityClient();
                assume->set_entity_id(TCHAR_TO_UTF8(*Id));
                assume->set_client_id(TCHAR_TO_UTF8(*ClientId()));
                opMessage.set_allocated_assume_ownership(assume);
                rti->Publish(inhumate::rti::ENTITY_OPERATION_CHANNEL, opMessage);
            } else if (entity->Owned) {
                UE_LOG(LogRTI, Warning, TEXT("Weird ownership transfer of owned entity %s to %s"), *Id, *TransferToId);
            }
        }
        break;
    }
    case inhumate::rti::proto::EntityOperation::WhichCase::kAssumeOwnership: {
        FString Id = FString(message.assume_ownership().entity_id().c_str());
        FString AssumingId = FString(message.assume_ownership().client_id().c_str());
        auto entity = GetEntityById(Id);
        if (entity != nullptr && entity->Owned && AssumingId != ClientId()) {
            entity->Owned = false;
            entity->OwnerClientId = AssumingId;
            inhumate::rti::proto::EntityOperation opMessage;
            auto release = new inhumate::rti::proto::EntityOperation_EntityClient();
            release->set_entity_id(TCHAR_TO_UTF8(*Id));
            release->set_client_id(TCHAR_TO_UTF8(*ClientId()));
            opMessage.set_allocated_release_ownership(release);
            rti->Publish(inhumate::rti::ENTITY_OPERATION_CHANNEL, opMessage);
            entity->LastOwnershipChangeTime = GetWorld()->GetTimeSeconds();
        }
        break;
    }
    case inhumate::rti::proto::EntityOperation::WhichCase::kReleaseOwnership: {
        FString Id = FString(message.release_ownership().entity_id().c_str());
        FString ReleasingId = FString(message.release_ownership().client_id().c_str());
        auto entity = GetEntityById(Id);
        if (entity != nullptr) {
            if (entity->OwnerClientId == ReleasingId) {
                entity->OwnerClientId = "";
                UE_LOG(LogRTI, Warning, TEXT("No owner for entity %s"), *Id);
            }
            entity->LastOwnershipChangeTime = GetWorld()->GetTimeSeconds();
        }
        break;
    }
    case inhumate::rti::proto::EntityOperation::WhichCase::kRequestPersistentOwnership: {
        if (IsPersistentEntityOwner() && message.request_persistent_ownership().application() == rti->application()) {
            PublishClaimPersistentEntityOwnership();
        }
        break;
    }
    case inhumate::rti::proto::EntityOperation::WhichCase::kClaimPersistentOwnership: {
        if (message.claim_persistent_ownership().application() == rti->application()) {
            PersistentEntityOwnerClientId = FString(message.claim_persistent_ownership().client_id().c_str());
        }
        break;
    }
    }
}

void URTISubsystem::OnGeometryOperation(const std::string &channelName, const inhumate::rti::proto::GeometryOperation &message)
{
    switch (message.which_case()) {
    case inhumate::rti::proto::GeometryOperation::WhichCase::kRequestUpdate: {
        UE_LOG(LogRTI, Log, TEXT("Geometry updates requested"));
        for (auto &it : GetGeometries()) {
            auto geometry = it.Value;
            geometry->RequestUpdate();
        }
        break;
    }
    case inhumate::rti::proto::GeometryOperation::WhichCase::kRequestPersistentOwnership: {
        if (IsPersistentGeometryOwner() && message.request_persistent_ownership().application() == rti->application()) {
            PublishClaimPersistentGeometryOwnership();
        }
        break;
    }
    case inhumate::rti::proto::GeometryOperation::WhichCase::kClaimPersistentOwnership: {
        if (message.claim_persistent_ownership().application() == rti->application()) {
            PersistentGeometryOwnerClientId = FString(message.claim_persistent_ownership().client_id().c_str());
        }
        break;
    }
    }
}

void URTISubsystem::OnInjectableOperation(const std::string &channelName, const inhumate::rti::proto::InjectableOperation &message)
{
    switch (message.which_case()) {
    case inhumate::rti::proto::InjectableOperation::WhichCase::kRequestUpdate: {
        for (auto &it : GetInjectables()) {
            auto injectable = it.Value;
            injectable->Publish();
        }
        break;
    }
    }

}

void URTISubsystem::OnInjectionOperation(const std::string &channelName, const inhumate::rti::proto::InjectionOperation &message)
{
    URTIInjectableComponent* injectable;
    FInjection* injection;
    switch (message.which_case()) {
    case inhumate::rti::proto::InjectionOperation::WhichCase::kRequestUpdate: {
        for (auto pair : Injectables) {
            pair.Value->PublishInjections();
        }
        break;
    }
    case inhumate::rti::proto::InjectionOperation::WhichCase::kInject: {
        FString InjectableName = UTF8_TO_TCHAR(message.inject().injectable().c_str());
        if (Injectables.Contains(InjectableName)) {
            Injectables[InjectableName]->Inject(&message.inject());
        } else {
            UE_LOG(LogRTI, Log, TEXT("Unknown injectable: %s"), *InjectableName);
        }
        break;
    }
    case inhumate::rti::proto::InjectionOperation::WhichCase::kDisable: {
        if (GetInjectableAndInjection(FString(message.disable().c_str()), &injectable, &injection)) {
            injectable->DisableInjection(*injection);
        }
        break;
    }
    case inhumate::rti::proto::InjectionOperation::WhichCase::kEnable: {
        if (GetInjectableAndInjection(FString(message.enable().c_str()), &injectable, &injection)) {
            injectable->EnableInjection(*injection);
        }
        break;
    }
    case inhumate::rti::proto::InjectionOperation::WhichCase::kStart: {
        if (GetInjectableAndInjection(FString(message.start().c_str()), &injectable, &injection)) {
            injectable->StartInjection(*injection);
        }
        break;
    }
    case inhumate::rti::proto::InjectionOperation::WhichCase::kEnd: {
        if (GetInjectableAndInjection(FString(message.end().c_str()), &injectable, &injection)) {
            injectable->EndInjection(*injection);
        }
        break;
    }
    case inhumate::rti::proto::InjectionOperation::WhichCase::kStop: {
        if (GetInjectableAndInjection(FString(message.stop().c_str()), &injectable, &injection)) {
            injectable->StopInjection(*injection);
        }
        break;
    }
    case inhumate::rti::proto::InjectionOperation::WhichCase::kCancel: {
        if (GetInjectableAndInjection(FString(message.cancel().c_str()), &injectable, &injection)) {
            injectable->CancelInjection(*injection);
        }
        break;
    }
    case inhumate::rti::proto::InjectionOperation::WhichCase::kSchedule: {
        if (GetInjectableAndInjection(FString(message.schedule().injection_id().c_str()), &injectable, &injection)) {
            injectable->ScheduleInjection(message.schedule().enable_time(), *injection);
        }
        break;
    }
    case inhumate::rti::proto::InjectionOperation::WhichCase::kUpdateTitle: {
        if (GetInjectableAndInjection(FString(message.update_title().injection_id().c_str()), &injectable, &injection)) {
            injectable->UpdateTitle(FString(message.update_title().title().c_str()), *injection);
        }
        break;
    }
    }
}

void URTISubsystem::OnClientDisconnect(const std::string &channelName, const std::string &clientId)
{
    if (TimeSyncMasterClientId == clientId.c_str()) {
        UE_LOG(LogRTI, Log, TEXT("Time sync master disconnected"));
        TimeSyncMasterClientId = "";
    }
    if (PersistentEntityOwnerClientId == clientId.c_str()) {
        UE_LOG(LogRTI, Log, TEXT("Persistent entity owner disconnected"));
        PersistentEntityOwnerClientId = "";
        QueryPersistentEntityOwner();
    }
    if (PersistentGeometryOwnerClientId == clientId.c_str()) {
        UE_LOG(LogRTI, Log, TEXT("Persistent geometry owner disconnected"));
        PersistentGeometryOwnerClientId = "";
        QueryPersistentGeometryOwner();
    }
    // TODO if persistent entity owner, take ownership of persistent entities that disconnecting client owns
}

void URTISubsystem::QueryPersistentGeometryOwner()
{
    inhumate::rti::proto::GeometryOperation message;
    auto request = new inhumate::rti::proto::GeometryOperation_ApplicationClient();
    request->set_application(rti->application());
    request->set_client_id(rti->client_id());
    message.set_allocated_request_persistent_ownership(request);
    rti->Publish(inhumate::rti::GEOMETRY_OPERATION_CHANNEL, message);
    RequestedGeometryOwnership = true;
    RequestGeometryOwnershipDateTime = FDateTime::UtcNow();
    ClaimGeometryOwnershipWaitTime = 0.2 + (double)rand() / RAND_MAX * 0.5;
}

void URTISubsystem::PublishClaimPersistentGeometryOwnership()
{
    inhumate::rti::proto::GeometryOperation message;
    auto claim = new inhumate::rti::proto::GeometryOperation_ApplicationClient();
    claim->set_application(rti->application());
    claim->set_client_id(rti->client_id());
    message.set_allocated_claim_persistent_ownership(claim);
    rti->Publish(inhumate::rti::GEOMETRY_OPERATION_CHANNEL, message);
}

void URTISubsystem::QueryPersistentEntityOwner() {
    inhumate::rti::proto::EntityOperation message;
    auto request = new inhumate::rti::proto::EntityOperation_ApplicationClient();
    request->set_application(rti->application());
    request->set_client_id(rti->client_id());
    message.set_allocated_request_persistent_ownership(request);
    rti->Publish(inhumate::rti::ENTITY_OPERATION_CHANNEL, message);
    RequestedEntityOwnership = true;
    RequestEntityOwnershipDateTime = FDateTime::UtcNow();
    ClaimEntityOwnershipWaitTime = 0.2 + (double)rand() / RAND_MAX * 0.5;
}

void URTISubsystem::PublishClaimPersistentEntityOwnership()
{
    inhumate::rti::proto::EntityOperation message;
    auto claim = new inhumate::rti::proto::EntityOperation_ApplicationClient();
    claim->set_application(rti->application());
    claim->set_client_id(rti->client_id());
    message.set_allocated_claim_persistent_ownership(claim);
    rti->Publish(inhumate::rti::ENTITY_OPERATION_CHANNEL, message);
}

FRTIScenario* URTISubsystem::GetScenario(const FString& Name)
{
    for (auto& Scenario : Scenarios) {
        if (Scenario.Name == Name) return &Scenario;
    }
    return nullptr;
}

inhumate::rti::RTIClient* URTISubsystem::GetRTIForComponent(UActorComponent* Component)
{
    auto subsystem = GetSubsystemForComponent(Component);
    if (!subsystem) {
        UE_LOG(LogRTI, Error, TEXT("Subsystem is null"));
        return nullptr;
    }
    return subsystem->RTI();
}

URTISubsystem* URTISubsystem::GetSubsystemForComponent(UActorComponent* Component)
{
    auto owner = Component->GetOwner();
    if (!owner) {
        UE_LOG(LogRTI, Error, TEXT("Owner is null"));
        return nullptr;
    }
    auto gameInstance = owner->GetGameInstance();
    if (!gameInstance) {
        UE_LOG(LogRTI, Error, TEXT("GameInstance is null"));
        return nullptr;
    }
    return gameInstance->GetSubsystem<URTISubsystem>();
}


FString GetProjectVersion()
{
    FString ProjectVersion;
    GConfig->GetString(TEXT("/Script/EngineSettings.GeneralProjectSettings"),
                       TEXT("ProjectVersion"), ProjectVersion, GGameIni);
    return ProjectVersion;
}

