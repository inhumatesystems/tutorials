// Copyright 2025 Inhumate AB. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RTIComponent.h"
#include "EntityCategoryEnum.h"
#include "EntityDomainEnum.h"
#include "LVCCategoryEnum.h"

#include "RTIEntityComponent.generated.h"

// Registers entity and publishes create/update/destroy messages

UCLASS(meta=(Blueprintable, BlueprintSpawnableComponent), DisplayName="RTI Entity")
class INHUMATERTI_API URTIEntityComponent : public URTIComponent
{
	GENERATED_BODY()

public:	
	URTIEntityComponent();

    // Type of entity, used by applications to determine what type of object it represents.
    // E.g. something specific like "car/sedan/volvos60" or in a more generic case "person".
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    FString Type;

    // Category can be used by some applications to classify entities even if they don't disginguish between specific types.
    // Leave as "Unknown" if this is irrelevant or "Other" if no category is applicable.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    EEntityCategory Category;

    // Domain can be used by some applications to classify entities even if they don't disginguish between specific types.
    // Leave as "Unknown" if this is irrelevant or "Other" if no category is applicable.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    EEntityDomain Domain;

    // Live means "a real thing controlled by a real person".
    // Virtual means "a simulated thing controlled by a real person".
    // Constructive means "a simulated thing controlled by a computer / simulated behavior".
    // Leave as "Unknown" if this is irrelevant to your application.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    ELVCCategory LVC;

    // Size of this entity, in centimeters, X/Y/Z is length/width/height
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    FVector Size;

    // Geometric center (the "middle") if offset from the object origin, in centimeters, X/Y/Z is forward/right/up
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    FVector Center;
    
    // Color that should be used to represent this entity. Not used if A == 0 (which is the default).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    FColor Color;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    bool TitleFromActorName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI", meta=(EditCondition="!TitleFromActorName", EditConditionHides))
    FString Title;

    // Unique ID used to identify this entity. Leave blank to generate a random ID when the entity is created.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    FString Id;

    // If non-zero, an entity update message will be published periodically (at the specified interval in seconds).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    float PublishInterval = 0.f;

    // If TRUE, this entity is controlled by us and we should be publishing its state
    UFUNCTION(BlueprintPure, Category = "RTI")
    bool IsPublishing();

    // If TRUE, this entity is controlled remotely, e.g. playing back messages from a recording, or simulated by another application
    UFUNCTION(BlueprintPure, Category = "RTI")
    bool IsReceiving();

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCreatedEvent);
    UPROPERTY(BlueprintAssignable, DisplayName = "Created", Category = "RTI")
    FCreatedEvent CreatedEvent;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FUpdatedEvent);
    UPROPERTY(BlueprintAssignable, DisplayName = "Updated", Category = "RTI")
    FUpdatedEvent UpdatedEvent;

protected:
	virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category = "RTI")
    void RequestUpdate();

    void SetPropertiesFromEntityData(const inhumate::rti::proto::Entity& data);

    UPROPERTY(BlueprintReadonly, Category = "RTI")
    bool Persistent;

    UPROPERTY(BlueprintReadonly, Category = "RTI")
    bool Owned = true;

    UPROPERTY(BlueprintReadonly, Category = "RTI")
    bool Deleted;

    UPROPERTY(BlueprintReadonly, Category = "RTI")
    FString OwnerClientId;

    float LastOwnershipChangeTime;

protected:
    UPROPERTY(BlueprintReadonly, Category = "RTI")
    bool Published;

    void Publish();

    void FillEntityData(inhumate::rti::proto::Entity& data);

    float TimeSinceLastPublish;
    bool UpdateRequested;
    friend class ARTISpawnerActor;


};
