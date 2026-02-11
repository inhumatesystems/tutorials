// Copyright 2025 Inhumate AB. All Rights Reserved.

#include "RTISpawnerActor.h"
#include "RTIEntityComponent.h"
#include "RTIPositionComponent.h"
#include "Engine/World.h"

using namespace std::placeholders;

ARTISpawnerActor::ARTISpawnerActor()
{
    RequestUpdatesOnStart = true;
    PrimaryActorTick.bCanEverTick = true;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));
}

AActor *ARTISpawnerActor::SpawnActorForEntity_Implementation(const FString &Id,
                                                             const FString &Type,
                                                             const FVector &Location,
                                                             const FRotator &Rotation)
{
    TSubclassOf<AActor> ActorClass = nullptr;
    if (ActorClass == nullptr && EntityTypeActorMapping.Contains(Type)) {
        UE_LOG(LogRTI, Log, TEXT("Create entity id %s type %s"), *Id, *Type);
        ActorClass = EntityTypeActorMapping[Type];
    }
    if (ActorClass == nullptr) {
        TArray<FString> Keys;
        if (EntityTypeActorMapping.GetKeys(Keys) > 0) {
            for (auto Key : Keys) {
                int32 Index;
                if (Key.FindChar('*', Index) && Key.Mid(0, Index) == Type.Mid(0, Index)) {
                    UE_LOG(LogRTI, Log, TEXT("Create entity id %s type %s (matching %s)"), *Id, *Type, *Key);
                    ActorClass = EntityTypeActorMapping[Key];
                    break;
                }
            }
        }
    }
    if (ActorClass == nullptr && UnknownActor != nullptr) {
        UE_LOG(LogRTI, Warning, TEXT("Create entity id %s unknown type %s"), *Id, *Type);
        ActorClass = UnknownActor;
    }
    if (ActorClass == nullptr) {
        UE_LOG(LogRTI, Error, TEXT("Unknown entity type %s"), *Type);
        return nullptr;
    }
    AActor *Actor = GetWorld()->SpawnActor<AActor>(ActorClass, Location, Rotation);
    Actor->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
    return Actor;
}

void ARTISpawnerActor::DestroyActorForEntity_Implementation(AActor *Actor, const URTIEntityComponent* Entity)
{
    UE_LOG(LogRTI, Log, TEXT("Destroy entity %s"), *Entity->Id);
    Actor->Destroy();
}

void ARTISpawnerActor::BeginPlay()
{
    Super::BeginPlay();
    Subscribe();
}

void ARTISpawnerActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    if (Subscribed && subscription) RTI()->Unsubscribe(subscription);
}

void ARTISpawnerActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (!Subscribed) Subscribe();
}

void ARTISpawnerActor::OnEntity(const std::string &channelName, const inhumate::rti::proto::Entity &message)
{
    auto subsystem = GetSubsystem();
    FString Id = FString(message.id().c_str());
    URTIEntityComponent *entity = subsystem->GetEntityById(Id);
    if (entity != nullptr) {
        if (!entity->Persistent && !entity->Owned) {
            if (message.deleted()) {
                entity->Deleted = true;
                subsystem->UnregisterEntity(entity);
                DestroyActorForEntity(entity->GetOwner(), entity);
            } else {
                UE_LOG(LogRTI, Log, TEXT("Update entity %s"), *Id);
                entity->SetPropertiesFromEntityData(message);
                entity->UpdatedEvent.Broadcast();
            }
        }
    } else if (!message.deleted()) {
        CreateEntity(message);
    }
}

void ARTISpawnerActor::CreateEntity(const inhumate::rti::proto::Entity &entity)
{
    FString Id = FString(entity.id().c_str());
    FString Type = FString(entity.type().c_str());

    FVector Location = GetActorLocation();
    FRotator Rotation = GetActorRotation();
    if (entity.has_position()) {
        Location = URTIPositionComponent::GetMessageLocation(entity.position(), GetWorld());
        Rotation = URTIPositionComponent::GetMessageRotation(entity.position(), GetWorld());
    }
    AActor *Actor = SpawnActorForEntity(Id, Type, Location, Rotation);
    if (Actor == nullptr) return;

    // Check that there is an RTIEntityComponent
    URTIEntityComponent *EntityComponent =
    Cast<URTIEntityComponent>(Actor->GetComponentByClass(URTIEntityComponent::StaticClass()));
    if (EntityComponent) {
        EntityComponent->Id = Id;
        EntityComponent->SetPropertiesFromEntityData(entity);
        EntityComponent->Published = true;
        auto rti = RTI();
        EntityComponent->Owned = entity.owner_client_id() == rti->client_id();
        EntityComponent->OwnerClientId = FString(entity.owner_client_id().c_str());
        // Actor->Rename(*(Type + TEXT(" ") + Id));
        // This would be nice but crashes the engine when playing via Play > Standalone Game in the
        // editor menu #if WITH_EDITOR
        //             Actor->SetActorLabel(*(Type + TEXT(" ") + Id));
        // #endif

        TArray<URTIEntityStateComponent *> Components;
        Actor->GetComponents<URTIEntityStateComponent>(Components, true);
        for (URTIEntityStateComponent *Component : Components) {
            Component->Initialize();
        }
        if (entity.has_position()) {
            URTIPositionComponent *PositionComponent =
            Cast<URTIPositionComponent>(Actor->GetComponentByClass(URTIPositionComponent::StaticClass()));
            if (PositionComponent) PositionComponent->ReceivePosition(entity.position(), true);
        }
        GetSubsystem()->RegisterEntity(EntityComponent);
        EntityComponent->CreatedEvent.Broadcast();
    } else {
        UE_LOG(LogRTI, Error, TEXT("RTIEntityComponent missing in actor for type %s"), *Type);
        DestroyActorForEntity(Actor, nullptr);
    }
}

inhumate::rti::RTIClient *ARTISpawnerActor::RTI()
{
    auto subsystem = GetSubsystem();
    if (!subsystem) {
        UE_LOG(LogRTI, Error, TEXT("Subsystem is null"));
        return nullptr;
    }
    return subsystem->RTI();
}

URTISubsystem *ARTISpawnerActor::GetSubsystem()
{
    auto gameInstance = GetGameInstance();
    if (!gameInstance) {
        UE_LOG(LogRTI, Error, TEXT("GameInstance is null"));
        return nullptr;
    }
    return gameInstance->GetSubsystem<URTISubsystem>();
}

void ARTISpawnerActor::Subscribe()
{
    auto rti = RTI();
    if (rti && rti->connected()) {
        std::function<void(const std::string &, const inhumate::rti::proto::Entity &)> callback = std::bind(&ARTISpawnerActor::OnEntity, this, _1, _2);
        subscription = rti->Subscribe(inhumate::rti::ENTITY_CHANNEL, callback);
        if (RequestUpdatesOnStart) {
            inhumate::rti::proto::EntityOperation message;
            message.set_allocated_request_update(new google::protobuf::Empty());
            rti->Publish(inhumate::rti::ENTITY_OPERATION_CHANNEL, message);
        }
        Subscribed = true;
    }
}
