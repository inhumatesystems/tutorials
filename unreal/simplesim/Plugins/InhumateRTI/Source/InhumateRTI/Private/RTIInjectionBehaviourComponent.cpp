// Copyright 2025 Inhumate AB. All Rights Reserved.

#include "RTIInjectionBehaviourComponent.h"
#include "RTIInjectableComponent.h"

URTIInjectionBehaviourComponent::URTIInjectionBehaviourComponent(const FObjectInitializer& init)
{
    Running = false;
}

URTIInjectionBehaviourComponent::URTIInjectionBehaviourComponent()
{
    Running = false;
}

bool URTIInjectionBehaviourComponent::Disable_Implementation()
{
    return true;
}

bool URTIInjectionBehaviourComponent::Enable_Implementation()
{
    return true;
}

bool URTIInjectionBehaviourComponent::Start_Implementation()
{
    return Begin();
}

bool URTIInjectionBehaviourComponent::Stop_Implementation()
{
    return End();
}

bool URTIInjectionBehaviourComponent::Cancel_Implementation()
{
    if (Running) End();
    return Disable();
}

void URTIInjectionBehaviourComponent::Schedule_Implementation()
{
}

bool URTIInjectionBehaviourComponent::Begin_Implementation() {
    Running = true;
    return true;
}

bool URTIInjectionBehaviourComponent::End_Implementation() {
    Running = false;
    return true;
}

EInjectionState URTIInjectionBehaviourComponent::GetState() const
{
    return GetInjection() != nullptr ? GetInjection()->State : EInjectionState::UNKNOWN;
}

FString URTIInjectionBehaviourComponent::GetTitle() const
{
    return GetInjection() != nullptr ? GetInjection()->Title : TEXT("");
}

void URTIInjectionBehaviourComponent::SetTitle(const FString& Title)
{
    auto Injection = GetInjection();
    if (Injection == nullptr) return;
    Injection->Title = Title;
    Publish();
}

void URTIInjectionBehaviourComponent::Inject(FInjection& Injection, class URTIInjectableComponent* injectable)
{
    InjectionId = FString(Injection.Id);
    Injectable = injectable;
}

void URTIInjectionBehaviourComponent::Publish()
{
    auto Injection = GetInjection();
    if (Injection == nullptr) return;
    auto rti = RTI();
    if (!rti || !rti->connected()) return;

    inhumate::rti::proto::Injection ProtoInjection;
    ProtoInjection.set_id(TCHAR_TO_UTF8(*Injection->Id));
    ProtoInjection.set_injectable(TCHAR_TO_UTF8(*Injection->Injectable));
    ProtoInjection.set_state(PbInjectionState(Injection->State));
    ProtoInjection.set_title(TCHAR_TO_UTF8(*Injection->Title));

    for (auto& Param : Injection->ParameterValues) {
        (*ProtoInjection.mutable_parameter_values())[TCHAR_TO_UTF8(*Param.Key)] = TCHAR_TO_UTF8(*Param.Value);    
    }

    ProtoInjection.set_enable_time(Injection->EnableTime);
    ProtoInjection.set_start_time(Injection->StartTime);
    ProtoInjection.set_end_time(Injection->EndTime);
    
    rti->Publish(inhumate::rti::INJECTION_CHANNEL, ProtoInjection);
}

FInjection* URTIInjectionBehaviourComponent::GetInjection() const {
    if (Injectable != nullptr) return Injectable->GetInjection(InjectionId);
    return nullptr;
}

FString URTIInjectionBehaviourComponent::GetParameterValue(const FString& ParameterName) {
    auto injection = GetInjection();
    if (injection != nullptr && injection->ParameterValues.Contains(ParameterName)) {
        return FString(injection->ParameterValues[ParameterName]);
    } else if (Injectable != nullptr) {
        return Injectable->GetParameterDefaultValue(ParameterName);
    }
    return "";
}
