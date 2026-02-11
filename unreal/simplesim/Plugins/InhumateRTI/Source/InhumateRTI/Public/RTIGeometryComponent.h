// Copyright 2025 Inhumate AB. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "Components/ShapeComponent.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "CoreMinimal.h"
#include "RTISceneComponent.h"
#include "TimerManager.h"

#include "RTIGeometryComponent.generated.h"

// Registers entity and publishes create/update/destroy messages

UCLASS(Abstract)
class INHUMATERTI_API URTIGeometryComponent : public URTISceneComponent
{
    GENERATED_BODY()

    public:
    const size_t WARN_MESH_SIZE = 50000;
    const size_t WARN_MESSAGE_BYTES = 500000;

    URTIGeometryComponent();

    UPROPERTY(BlueprintReadOnly, Category = "RTI")
    FString Id;

    UPROPERTY(BlueprintReadWrite, Category = "RTI")
    bool Persistent = true;

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
    UFUNCTION(BlueprintCallable, Category = "RTI")
    void RequestUpdate();

    protected:

    UFUNCTION()
    void On_TryPublishCreate();

    UFUNCTION()
    void On_TryPublishUpdate();

    FTimerHandle RequestRegisterTimer;
    FTimerHandle RequestCreateTimer;
    FTimerHandle RequestUpdateTimer;

    void Publish();

    virtual void FillGeometryData(inhumate::rti::proto::Geometry &data);
    inhumate::rti::proto::Geometry_Point2D *CreatePoint2D(const FVector &location);
    inhumate::rti::proto::Geometry_Point3D *CreatePoint3D(const FVector &location);
    inhumate::rti::proto::Geometry_Polygon *CreatePolygon(const UStaticMeshComponent *component);
    inhumate::rti::proto::Geometry_Mesh *CreateMesh(const UStaticMesh *Mesh);
    inhumate::rti::proto::Geometry_Mesh *CreateMesh(const TArray<UStaticMeshComponent *> &components,
                                                    const bool local = false,
                                                    const bool bFlippedNormals = false);
    uint32 AddMesh(inhumate::rti::proto::Geometry_Mesh *mesh,
                   const UStaticMesh *Mesh,
                   uint32 Offset,
                   const FTransform *ComponentTransform,
                   const FTransform *ActorTransform,
                   const bool bFlippedNormals = false);
    inhumate::rti::proto::Geometry_Mesh *CreateMeshFromCollision(const TArray<UShapeComponent *> &components);
    inhumate::rti::proto::Geometry_Line2D *CreateLine2DFromSpline(const TArray<USplineComponent *> &components);
    inhumate::rti::proto::Geometry_Line3D *CreateLine3DFromSpline(const TArray<USplineComponent *> &components);
    inhumate::rti::proto::Geometry_Spline2D *CreateSpline2D(const TArray<USplineComponent *> &components);
    inhumate::rti::proto::Geometry_Spline3D *CreateSpline3D(const TArray<USplineComponent *> &components);

    void SetPoint2D(const FVector &location, inhumate::rti::proto::Geometry_Point2D *point);
    void SetPoint3D(const FVector &location, inhumate::rti::proto::Geometry_Point3D *point);

    public:
    static inhumate::rti::proto::Geometry_LocalPoint2D *UEToRTILocalPoint2D(const FVector &location);
    static inhumate::rti::proto::Geometry_LocalPoint3D *UEToRTILocalPoint3D(const FVector &location);
    static inhumate::rti::proto::Color *UEToRTIColor(const FColor &color);

    UPROPERTY(BlueprintReadWrite, Category = "RTI")
    bool Owned = true;

    protected:
    UPROPERTY(BlueprintReadonly, Category = "RTI")
    bool Published;

    bool UpdateRequested;
    bool Registered;
    friend class ARTISpawnerActor;

    template <class T, class AllocatorType>
    void GetComponents(TArray<T *, AllocatorType> &OutComponents, bool bIncludeFromAllDescendants = true) const
    {
        OutComponents.Reset();

        // First, let's look if we have children
        TArray<USceneComponent *> SceneComponents;
        GetChildrenComponents(bIncludeFromAllDescendants, SceneComponents);
        if (SceneComponents.Num() > 0) {
            for (auto SceneComponent : SceneComponents) {
                T *Component = Cast<T>(SceneComponent);
                if (Component != nullptr) OutComponents.Add(Component);
            }

            // Otherwise, include parent and our siblings
        } else {
            auto Parent = GetAttachParent();
            if (Parent) {
                T *Component = Cast<T>(Parent);
                if (Component != nullptr) OutComponents.Add(Component);
                Parent->GetChildrenComponents(bIncludeFromAllDescendants, SceneComponents);
                for (auto SceneComponent : SceneComponents) {
                    Component = Cast<T>(SceneComponent);
                    if (Component != nullptr) OutComponents.Add(Component);
                }
            } else {
                UE_LOG(LogRTI, Warning, TEXT("RTI Geometry %s is not attached to anything"), *Id);
            }
        }
    }
};
