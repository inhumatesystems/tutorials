// Copyright 2025 Inhumate AB. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "RTIComponent.h"
#include "RTISubsystem.h"

#include "RTIListenerComponent.generated.h"

// Generic RTI listener that can do pub/sub of string-based messages (i.e. in blueprints)

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent), DisplayName="RTI Listener")
class INHUMATERTI_API URTIListenerComponent : public URTIComponent
{
	GENERATED_BODY()

public:

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    TArray<FString> Channels;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FConnectedEvent);
    UPROPERTY(BlueprintAssignable, DisplayName = "Connected", Category = "RTI")
    FConnectedEvent ConnectedEvent;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDisconnectedEvent);
    UPROPERTY(BlueprintAssignable, DisplayName = "Disconnected", Category = "RTI")
    FDisconnectedEvent DisconnectedEvent;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FErrorEvent, FString, Channel, FString, Message);
    UPROPERTY(BlueprintAssignable, DisplayName = "Error", Category = "RTI")
    FErrorEvent ErrorEvent;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMessageEvent, FString, Channel, FString, Message);
    UPROPERTY(BlueprintAssignable, DisplayName = "Message", Category = "RTI")
    FMessageEvent MessageEvent;

    UFUNCTION(BlueprintCallable, Category = "RTI")
    void Subscribe(const FString& Channel);

    UFUNCTION(BlueprintCallable, Category = "RTI")
    void Unsubscribe(const FString& Channel);

    UFUNCTION(BlueprintCallable, Category = "RTI")
    void PublishString(const FString& Channel, const FString& Content);

    UFUNCTION(BlueprintCallable, Category = "RTI")
    void PublishError(const FString& ErrorMessage);

protected:

    UFUNCTION()
    void OnConnected();

    UFUNCTION()
    void OnDisconnected();

    UFUNCTION()
    void OnError(FString channel, FString message);

	virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    void SubscribeToChannels();

private:

    TMap<FString, inhumate::rti::messagecallback_p> Subscriptions;

};
