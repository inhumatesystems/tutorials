// Copyright 2025 Inhumate AB. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/GameInstance.h"

#include "RTISubsystem.h"

#include "RTISpawnerActor.generated.h"

// A spawner that listens to entity create messages and spawns the appropriate actor
// according to entity specific type

UCLASS(DisplayName = "RTI Spawner")
class INHUMATERTI_API ARTISpawnerActor : public AActor
{
    GENERATED_BODY()

    public:
    ARTISpawnerActor();

    // Map of entity types to actors to be spawned when an entity is created.
    // Use * for wildcarding, e.g. "car*" will match "car5" and "car/sedan/volvos60".
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    TMap<FString, TSubclassOf<AActor>> EntityTypeActorMapping;

    // Actor to spawn when an entity is created and no match was found in the entity type to actor map.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    TSubclassOf<AActor> UnknownActor;

    // If checked, the spawner will publish a request for entity updates when game starts.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    bool RequestUpdatesOnStart;

    // Can be implemented to override the default spawning behavior when an entity is created.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RTI")
    AActor *SpawnActorForEntity(const FString &Id,
                                const FString &Type,
                                const FVector &Location,
                                const FRotator &Rotation);

    // Can be implemented to override the default behavior when an entity is destroyed.
    UFUNCTION(BlueprintNativeEvent, Category = "RTI")
    void DestroyActorForEntity(AActor *Actor, const URTIEntityComponent* Entity);

    protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    public:
    virtual void Tick(float DeltaTime) override;

    protected:
    void OnEntity(const std::string &ChannelName, const inhumate::rti::proto::Entity &message);
    void CreateEntity(const inhumate::rti::proto::Entity &entity);

    inhumate::rti::RTIClient *RTI();
    URTISubsystem *GetSubsystem();

    private:
    void Subscribe();
    bool Subscribed;
    inhumate::rti::messagecallback_p subscription;
};
