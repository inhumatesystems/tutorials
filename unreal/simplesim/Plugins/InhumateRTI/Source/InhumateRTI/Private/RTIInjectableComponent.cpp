// Copyright 2025 Inhumate AB. All Rights Reserved.

#include "RTIInjectableComponent.h"
#include "RTIInjectionBehaviourComponent.h"
#include "RuntimeStateEnum.h"

URTIInjectableComponent::URTIInjectableComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    bAutoActivate = true;
}

bool URTIInjectableComponent::IsConcurrent_Implementation()
{
    return InjectActor != nullptr;
}

FString URTIInjectableComponent::GetParameterDefaultValue(const FString& ParameterName)
{
    for (auto parameter : Parameters) {
        if (parameter.Name == ParameterName) return parameter.DefaultValue;
    }
    return "";
}

AActor *URTIInjectableComponent::SpawnActorForInjection_Implementation(const FInjection& Injection)
{
    if (InjectActor != nullptr) {
        AActor *Actor = GetWorld()->SpawnActor<AActor>(InjectActor);
        Actor->AttachToActor(GetOwner(), FAttachmentTransformRules::KeepWorldTransform);
        return Actor;
    } else {
        return nullptr;
    }
}

void URTIInjectableComponent::Inject(const inhumate::rti::proto::InjectionOperation_Inject *inject)
{
    auto subsystem = GetSubsystem();
    if (!subsystem) return;
    auto state = subsystem->GetState();
    if (state != ERuntimeState::UNKNOWN && state != ERuntimeState::LOADING && state != ERuntimeState::READY &&
        state != ERuntimeState::RUNNING && state != ERuntimeState::PAUSED) {
        UE_LOG(LogRTI, Log, TEXT("Not injecting %s while %s"), *Name, *UEnum::GetValueAsString(state));
        return;
    }
    if (!IsConcurrent() && Injections.Num() > 0 && Injections[Injections.Num() - 1].State <= EInjectionState::RUNNING) {
        UE_LOG(LogRTI, Log, TEXT("Already injected: %s %s"), *Name, *Injections[Injections.Num() - 1].Id);
        return;
    }
    FInjection& Injection = Injections.Emplace_GetRef();
    Injection.Id = FGuid::NewGuid().ToString();
    Injection.Injectable = Name;
    if (inject != nullptr) {
        Injection.EnableTime = inject->enable_time();
        for (auto& pair : inject->parameter_values()) {
            Injection.ParameterValues.Add(pair.first.c_str(), pair.second.c_str()); 
        }
    }
    AActor *SpawnedActor = SpawnActorForInjection(Injection);
    if (SpawnedActor != nullptr) {
        TArray<URTIInjectionBehaviourComponent *> ActorBehaviours;
        SpawnedActor->GetComponents<URTIInjectionBehaviourComponent>(ActorBehaviours, true);
        if (ActorBehaviours.Num() == 0) {
            UE_LOG(LogRTI, Log, TEXT("Spawned injection actor has no behaviour component: %s"), *Name);
        } else {
            InjectionBehaviours.Add(Injection.Id, {ActorBehaviours});
        }
    }
    TArray<URTIInjectionBehaviourComponent *> LocalBehaviours;
    GetBehaviours(Injection, LocalBehaviours);
    for (const auto behaviour : LocalBehaviours) {
        behaviour->Inject(Injection, this);
    }
    if ((inject != nullptr && inject->disabled()) ||
        (inject == nullptr && AutoInjection < EAutoInjection::START_ENABLED) || !EnableInjection(Injection)) {
        UpdateState(Injection, EInjectionState::DISABLED);
    }
    UE_LOG(LogRTI, Log, TEXT("Inject %s"), *Name);
}

bool URTIInjectableComponent::EnableInjection(FInjection &Injection)
{
    if (Injection.State < EInjectionState::ENABLED) {
        auto behavioursEnabled = true;
        TArray<URTIInjectionBehaviourComponent *> LocalBehaviours;
        GetBehaviours(Injection, LocalBehaviours);
        for (auto Behaviour : LocalBehaviours) {
            if (!Behaviour->Enable()) {
                behavioursEnabled = false;
            }
        }
        if (!behavioursEnabled) {
            for (auto behaviour : LocalBehaviours)
                behaviour->Disable();
            return false;
        }
        Injection.EnableTime = GetSubsystem()->GetTime();
        if (StartMode == EInjectableControlMode::IMMEDIATE) {
            for (auto behaviour : LocalBehaviours)
                behaviour->Begin();
            Injection.StartTime = GetSubsystem()->GetTime();
            if (EndMode == EInjectableControlMode::IMMEDIATE) {
                for (auto behaviour : LocalBehaviours)
                    behaviour->End();
                Injection.EndTime = GetSubsystem()->GetTime();
                UpdateState(Injection, EInjectionState::END);
            } else {
                UpdateState(Injection, EInjectionState::RUNNING);
            }
        } else {
            UpdateState(Injection, EInjectionState::ENABLED);
        }
    }
    return true;
}

void URTIInjectableComponent::DisableInjection(FInjection &Injection)
{
    TArray<URTIInjectionBehaviourComponent *> LcoalBehaviours;
    GetBehaviours(Injection, LcoalBehaviours);
    for (auto Behaviour : LcoalBehaviours)
        Behaviour->Disable();
    UpdateState(Injection, EInjectionState::DISABLED);
}

void URTIInjectableComponent::StartInjection(FInjection &Injection)
{
    TArray<URTIInjectionBehaviourComponent *> LocalBehaviours;
    GetBehaviours(Injection, LocalBehaviours);
    if (LocalBehaviours.Num() == 0) {
        Injection.StartTime = GetSubsystem()->GetTime();
        if (EndMode == EInjectableControlMode::IMMEDIATE) {
            Injection.EndTime = GetSubsystem()->GetTime();
            UpdateState(Injection, EInjectionState::END);
        } else {
            UpdateState(Injection, EInjectionState::RUNNING);
        }
    } else {
        for (auto Behaviour : LocalBehaviours)
            Behaviour->Start();
        if (EndMode == EInjectableControlMode::IMMEDIATE) {
            Injection.EndTime = GetSubsystem()->GetTime();
            UpdateState(Injection, EInjectionState::END);
        }
    }
}

void URTIInjectableComponent::EndInjection(FInjection &Injection)
{
    TArray<URTIInjectionBehaviourComponent *> LocalBehaviours;
    GetBehaviours(Injection, LocalBehaviours);
    if (LocalBehaviours.Num() == 0) {
        Injection.EndTime = GetSubsystem()->GetTime();
        UpdateState(Injection, EInjectionState::END);
    } else {
        for (auto Behaviour : LocalBehaviours)
            Behaviour->End();
    }
}

void URTIInjectableComponent::StopInjection(FInjection &Injection)
{
    TArray<URTIInjectionBehaviourComponent *> LocalBehaviours;
    GetBehaviours(Injection, LocalBehaviours);
    for (auto Behaviour : LocalBehaviours)
        Behaviour->Stop();
    Injection.EndTime = GetSubsystem()->GetTime();
    UpdateState(Injection, EInjectionState::STOPPED);
}

void URTIInjectableComponent::CancelInjection(FInjection &Injection)
{
    TArray<URTIInjectionBehaviourComponent *> LocalBehaviours;
    GetBehaviours(Injection, LocalBehaviours);
    for (auto Behaviour : LocalBehaviours)
        Behaviour->Cancel();
    Injection.EndTime = GetSubsystem()->GetTime();
    UpdateState(Injection, EInjectionState::CANCELED);
}

void URTIInjectableComponent::ScheduleInjection(const float EnableTime, FInjection &Injection)
{
    Injection.EnableTime = EnableTime;
    TArray<URTIInjectionBehaviourComponent *> LocalBehaviours;
    GetBehaviours(Injection, LocalBehaviours);
    for (auto Behaviour : LocalBehaviours)
        Behaviour->Schedule();
    PublishInjection(Injection);
}

void URTIInjectableComponent::UpdateTitle(const FString &Title, FInjection &Injection)
{
    Injection.Title = Title;
    PublishInjection(Injection);
}

void URTIInjectableComponent::Publish()
{
    auto rti = RTI();
    if (!rti || !rti->connected()) return;
    inhumate::rti::proto::Injectable injectable;
    injectable.set_name(TCHAR_TO_UTF8(*Name));
    injectable.set_description(TCHAR_TO_UTF8(*Description));
    injectable.set_concurrent(IsConcurrent());
    injectable.set_start_mode(PbControlMode(StartMode));
    injectable.set_end_mode(PbControlMode(EndMode));
    for (auto parameter : Parameters) {
        auto pbParameter = injectable.add_parameters();
        ParameterToPb(parameter, pbParameter);
    }
    rti->Publish(inhumate::rti::INJECTABLE_CHANNEL, injectable);
}

void URTIInjectableComponent::PublishClearInjections()
{
    auto rti = RTI();
    if (!rti || !rti->connected()) return;
    inhumate::rti::proto::InjectionOperation message;
    message.set_clear(TCHAR_TO_UTF8(*Name));
    rti->Publish(inhumate::rti::INJECTION_OPERATION_CHANNEL, message);
}

void URTIInjectableComponent::PublishInjections()
{
    for (auto &injection : Injections)
        PublishInjection(injection);
}

void URTIInjectableComponent::PublishInjection(const FInjection &Injection)
{
    auto rti = RTI();
    if (!rti || !rti->connected()) return;
    inhumate::rti::proto::Injection ProtoInjection;
    ProtoInjection.set_id(TCHAR_TO_UTF8(*Injection.Id));
    ProtoInjection.set_injectable(TCHAR_TO_UTF8(*Injection.Injectable));
    ProtoInjection.set_state(PbInjectionState(Injection.State));
    ProtoInjection.set_title(TCHAR_TO_UTF8(*Injection.Title));

    for (auto& Param : Injection.ParameterValues) {
        (*ProtoInjection.mutable_parameter_values())[TCHAR_TO_UTF8(*Param.Key)] = TCHAR_TO_UTF8(*Param.Value);    
    }

    ProtoInjection.set_enable_time(Injection.EnableTime);
    ProtoInjection.set_start_time(Injection.StartTime);
    ProtoInjection.set_end_time(Injection.EndTime);
    
    rti->Publish(inhumate::rti::INJECTION_CHANNEL, ProtoInjection);
}

FInjection *URTIInjectableComponent::GetInjection(const FString &id)
{
    for (auto &injection : Injections)
        if (id == injection.Id) return &injection;
    return nullptr;
}

void URTIInjectableComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!IsActive()) return;
    auto subsystem = GetSubsystem();
    if (!subsystem) return;

    auto RtiTime = subsystem->GetTime();
    for (auto& injection : Injections) {
        if (injection.State == EInjectionState::DISABLED && injection.EnableTime > 1e-5f &&
            RtiTime >= injection.EnableTime && LastRtiTime < injection.EnableTime) {
            EnableInjection(injection);
        }
        TArray<URTIInjectionBehaviourComponent *> LocalBehaviours;
        GetBehaviours(injection, LocalBehaviours);
        if (LocalBehaviours.Num() > 0) {
            bool anyRunning = false;
            for (auto behaviour : LocalBehaviours) {
                if (behaviour->IsRunning()) {
                    anyRunning = true;
                    if (injection.State < EInjectionState::RUNNING) {
                        injection.StartTime = RtiTime;
                        UpdateState(injection, EInjectionState::RUNNING);
                    }
                }
            }
            if (!anyRunning && injection.State == EInjectionState::RUNNING) {
                injection.EndTime = RtiTime;
                UpdateState(injection, EInjectionState::END);
            }
        }
    }
    LastRtiTime = RtiTime;
}

void URTIInjectableComponent::BeginPlay()
{
    Super::BeginPlay();
    if (Name.IsEmpty()) {
        Name = GetReadableName().Replace(TEXT(".RTIInjectable"), TEXT(""));
        Name = Name.Replace(*(GetOwner()->GetName() + "."), TEXT(""));
    }

    GetOwner()->GetComponents<URTIInjectionBehaviourComponent>(Behaviours);
    TArray<URTIInjectableComponent*> Injectables;
    GetOwner()->GetComponents<URTIInjectableComponent>(Injectables);
    if (InjectActor == nullptr && Behaviours.Num() == 0) {
        UE_LOG(LogRTI, Warning, TEXT("Injectable %s has no inject actor or components"), *Name);
    } else if (InjectActor != nullptr && Behaviours.Num() > 0) {
        UE_LOG(LogRTI, Warning, TEXT("Injectable %s has both inject actor and components in actor"), *Name);
    } else if (Behaviours.Num() > 0 && Injectables.Num() > 1) {
        UE_LOG(LogRTI, Warning, TEXT("Actor %s has multiple injectable and injection components - that's not gonna fly."), *GetOwner()->GetHumanReadableName());
    }

    auto subsystem = GetSubsystem();
    if (subsystem) {
        subsystem->RegisterInjectable(this);
        subsystem->WhenConnectedOnce([this]() {
            Publish();
            PublishClearInjections();
            // We delay the autoinject to get some time after PublishClearInjections().
            // Typically a lot of messages being pushed around at start...
            FTimerHandle TimerHandle;
            GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &URTIInjectableComponent::AutoInject, 0.1f, false);
        });
    }
}

void URTIInjectableComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    for (auto injection : Injections) {
        if (injection.State > EInjectionState::DISABLED && injection.State < EInjectionState::END) {
            CancelInjection(injection);
        }
    }
    auto subsystem = GetSubsystem();
    if (subsystem) {
        subsystem->UnregisterInjectable(this);
    }
}

void URTIInjectableComponent::AutoInject()
{
    if (Injections.Num() == 0 && AutoInjection > EAutoInjection::ON_DEMAND) {
        Inject(nullptr);
    }
}

void URTIInjectableComponent::UpdateState(FInjection &Injection, EInjectionState state)
{
    Injection.State = state;
    PublishInjection(Injection);
}

void URTIInjectableComponent::GetBehaviours(const FInjection &Injection,
                                            TArray<URTIInjectionBehaviourComponent *> &outBehaviours)
{
    if (InjectionBehaviours.Contains(Injection.Id)) {
        outBehaviours.Append(InjectionBehaviours[Injection.Id]);
    } else if (!IsConcurrent()) {
        outBehaviours.Append(Behaviours);
    }
}
