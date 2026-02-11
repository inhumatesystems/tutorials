// Copyright 2025 Inhumate AB. All Rights Reserved.

#include "RTIEntityGeometryComponent.h"
#include "GameFramework/Actor.h"

URTIEntityGeometryComponent::URTIEntityGeometryComponent() : URTIGeometryComponent::URTIGeometryComponent()
{
    bWantsInitializeComponent = true;
    Shape = EEntityGeometryShape::AUTO;
}

void URTIEntityGeometryComponent::InitializeComponent()
{
    Super::InitializeComponent();
    Id = EntityType;
}

void URTIEntityGeometryComponent::FillGeometryData(inhumate::rti::proto::Geometry& data)
{
    data.set_id(TCHAR_TO_UTF8(*Id));
    if (GetSubsystem() && GetSubsystem()->RTI()) data.set_owner_client_id(GetSubsystem()->RTI()->client_id());
    data.set_usage(inhumate::rti::proto::Geometry::Usage::Geometry_Usage_ENTITY);
    data.set_scalable(ScaleToEntityDimensions);
    data.set_allocated_color(UEToRTIColor(Color));
    data.set_transparency(1 - Opacity);
    data.set_allocated_label_color(UEToRTIColor(LabelColor));
    data.set_label_transparency(1 - LabelOpacity);

    auto UseShape = Shape;
    if (UseShape == EEntityGeometryShape::AUTO && StaticMesh != nullptr) {
        UseShape = EEntityGeometryShape::MESH;
    }
    if (UseShape == EEntityGeometryShape::AUTO) {
        TArray<UStaticMeshComponent *> MeshComponents;
        GetComponents<UStaticMeshComponent>(MeshComponents, true);
        if (MeshComponents.Num() > 0) UseShape = EEntityGeometryShape::MESH;
    }
    if (UseShape == EEntityGeometryShape::AUTO) {
        TArray<UShapeComponent *> ShapeComponents;
        GetComponents<UShapeComponent>(ShapeComponents, true);
        if (ShapeComponents.Num() > 0) UseShape = EEntityGeometryShape::MESH_FROM_COLLISION;
    }
    if (UseShape == EEntityGeometryShape::AUTO) {
        UseShape = EEntityGeometryShape::POINT3D;
    }
    switch (UseShape) {
    case EEntityGeometryShape::POINT3D:
        data.set_allocated_point3d(CreatePoint3D(GetOwner()->GetActorLocation()));
        break;
    case EEntityGeometryShape::MESH: {
        bool found = false;
        if (StaticMesh != nullptr) {
            data.set_allocated_mesh(CreateMesh(StaticMesh));
            found = true;
        } else {
            TArray<UStaticMeshComponent *> MeshComponents;
            GetComponents<UStaticMeshComponent>(MeshComponents);
            if (MeshComponents.Num() > 0) {
                data.set_allocated_mesh(CreateMesh(MeshComponents, true));
                found = true;
            }
        }
        if (!found) {
            UE_LOG(LogRTI, Warning, TEXT("Can't create mesh geometry - no static mesh"));
        }
        break;
    }
    case EEntityGeometryShape::MESH_FROM_COLLISION: {
        TArray<UShapeComponent *> ShapeComponents;
        GetComponents<UShapeComponent>(ShapeComponents, true);
        if (ShapeComponents.Num() > 0) {
            data.set_allocated_mesh(CreateMeshFromCollision(ShapeComponents));
        } else {
            UE_LOG(LogRTI, Warning, TEXT("Can't create mesh geometry - no shapes"));
        }
        break;
    }
    default:
        UE_LOG(LogRTI, Warning, TEXT("Unimplemented geometry shape %d"), UseShape);
    }
}
