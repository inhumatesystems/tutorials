// Copyright 2025 Inhumate AB. All Rights Reserved.

#include "RTIListenerComponent.h"

void URTIListenerComponent::Subscribe(const FString &Channel)
{
    auto rti = RTI();
    if (Subscriptions.Contains(Channel)) {
        UE_LOG(LogRTI, Warning, TEXT("Already subscribed to channel %s"), *Channel);
    } else if (rti && rti->connected()) {
        auto subscription =
        rti->Subscribe(TCHAR_TO_UTF8(*Channel), [this](const std::string &Channel, const std::string &message) {
            MessageEvent.Broadcast(FString(Channel.c_str()), FString(message.c_str()));
        });
        if (subscription) Subscriptions.Add(Channel, subscription);
    } else {
        if (!Channels.Contains(Channel)) {
            Channels.Add(Channel);
            UE_LOG(LogRTI, Log, TEXT("Postponed subscribe to RTI channel %s"), *Channel);
        }
    }
}

void URTIListenerComponent::Unsubscribe(const FString &Channel)
{
    auto rti = RTI();
    if (rti && Subscriptions.Contains(Channel)) {
        rti->Unsubscribe(Subscriptions[Channel]);
        Subscriptions.Remove(Channel);
    }
}

void URTIListenerComponent::PublishString(const FString &Channel, const FString &Content)
{
    auto rti = RTI();
    if (rti && rti->connected()) {
        rti->Publish(TCHAR_TO_UTF8(*Channel), TCHAR_TO_UTF8(*Content));
    } else {
        UE_LOG(LogRTI, Error, TEXT("URTIListenerComponent::PublishString called when not connected"));
    }
}

void URTIListenerComponent::PublishError(const FString &ErrorMessage) {
    const auto Rti = GetSubsystem();
    if (Rti) Rti->PublishError(ErrorMessage);
}

void URTIListenerComponent::OnConnected()
{
    SubscribeToChannels();
    ConnectedEvent.Broadcast();
}

void URTIListenerComponent::OnDisconnected()
{
    DisconnectedEvent.Broadcast();
}

void URTIListenerComponent::OnError(FString channel, FString message)
{
    ErrorEvent.Broadcast(channel, message);
}

void URTIListenerComponent::BeginPlay()
{
    Super::BeginPlay();
    auto subsystem = GetSubsystem();
    if (subsystem) {
        subsystem->ConnectedEvent.AddDynamic(this, &URTIListenerComponent::OnConnected);
        subsystem->DisconnectedEvent.AddDynamic(this, &URTIListenerComponent::OnDisconnected);
        subsystem->ErrorEvent.AddDynamic(this, &URTIListenerComponent::OnError);
        if (subsystem->IsConnected()) SubscribeToChannels();
    }
}

void URTIListenerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    auto rti = RTI();
    if (rti) {
        for (auto &item : Subscriptions) {
            rti->Unsubscribe(item.Value);
        }
    }
    Subscriptions.Empty();
}

void URTIListenerComponent::SubscribeToChannels()
{
    auto rti = RTI();
    if (rti && rti->connected()) {
        for (auto &channel : Channels) {
            if (!Subscriptions.Contains(channel)) {
                Subscribe(channel);
            }
        }
    }
}