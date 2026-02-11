// Copyright 2025 Inhumate AB. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"

#include "RTISubsystem.h"
#include "RTIEntityComponent.h"
#include "IdMessage.pb.h"

#include "RTIEntityStateComponent.generated.h"

// Base class for components that listen to messages for a specific entity id
// (which is provided by URTIEntityComponent)

UCLASS(Abstract, Blueprintable, meta=(BlueprintSpawnableComponent))
class INHUMATERTI_API URTIEntityStateComponent : public URTIComponent
{
	GENERATED_BODY()

public:
    URTIEntityStateComponent(const FObjectInitializer& init);
    URTIEntityStateComponent(const FString& channelName, const bool stateless = false, const bool ephemeral = false);

    bool IsPublishing();
    bool IsReceiving();

protected:
	virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    virtual void OnMessage(const FString& content) {}

    template<typename Message> 
    static Message Parse(const FString& content) {
        return inhumate::rti::RTIClient::Parse<Message>(TCHAR_TO_UTF8(*content));
    }

    void Publish(const google::protobuf::Message& message);

public:
    void Initialize();
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    static void ResetSubscriptions();

protected:
    URTIEntityComponent *Entity;
    FString ChannelName;
    bool Stateless;
    bool Ephemeral;

private:

    void Unsubscribe();
    void RegisterChannel();
    
    FString SubscribedEntityId;
    bool initialized;
    bool warnedNoEntity;
    static TMap< FString, bool> subscribed;
    static TMap< FString, inhumate::rti::messagecallback_p> subscription;
    static TMap< FString, TMap< FString, TArray<URTIEntityStateComponent*> > > instances;
    static TMap< FString, bool> warnedUnknown;
    static TMap< FString, bool> registeredChannel;

};
