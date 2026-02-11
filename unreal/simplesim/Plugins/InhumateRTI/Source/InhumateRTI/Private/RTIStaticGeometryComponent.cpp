// Copyright 2025 Inhumate AB. All Rights Reserved.

#include "RTIStaticGeometryComponent.h"
#include "Components/ShapeComponent.h"
#include "GameFramework/Actor.h"

URTIStaticGeometryComponent::URTIStaticGeometryComponent()
: URTIGeometryComponent::URTIGeometryComponent()
{
    bWantsInitializeComponent = true;
    Shape = EStaticGeometryShape::AUTO;
}

void URTIStaticGeometryComponent::BeginPlay()
{
    Super::BeginPlay();
}

void URTIStaticGeometryComponent::SetColor(const FColor &NewColor)
{
    Color = NewColor;
    Publish();
}

void URTIStaticGeometryComponent::InitializeComponent()
{
    Super::InitializeComponent();
    if (SpecificId.IsEmpty()) {
        Super::Id = GetReadableName().Replace(TEXT(".RTIStaticGeometry"), TEXT(""));
        if (GetSubsystem() != nullptr) Super::Id = GetSubsystem()->Application + "." + Super::Id;
    } else {
        Super::Id = SpecificId;
    }
}

void URTIStaticGeometryComponent::FillGeometryData(inhumate::rti::proto::Geometry& data)
{
    data.set_id(TCHAR_TO_UTF8(*Id));
    if (GetSubsystem() && GetSubsystem()->RTI()) data.set_owner_client_id(GetSubsystem()->RTI()->client_id());
    data.set_usage(inhumate::rti::proto::Geometry::Usage::Geometry_Usage_SCENARIO);
    data.set_type(TCHAR_TO_UTF8(*Type));
    data.set_category(PbCategoryFromGeometryCategory(Category));

    auto RTIColor = UEToRTIColor(Color);
    if (RTIColor == nullptr) {
        TArray<UShapeComponent *> ShapeComponents;
        GetComponents<UShapeComponent>(ShapeComponents, true);
        for (auto ShapeComponent : ShapeComponents) {
            RTIColor = UEToRTIColor(ShapeComponent->ShapeColor);
            if (RTIColor != nullptr) break;
        }
    }
    data.set_allocated_color(RTIColor);

    data.set_transparency(1 - Opacity);
    data.set_title(TCHAR_TO_UTF8(*Title));
    data.set_allocated_label_color(UEToRTIColor(LabelColor));
    data.set_label_transparency(1 - LabelOpacity);
    data.set_wireframe(Wireframe);
    data.set_line_width(LineWidth);

    EStaticGeometryShape UseShape = Shape;

    if (UseShape == EStaticGeometryShape::AUTO) {
        TArray<UStaticMeshComponent *> MeshComponents;
        GetComponents<UStaticMeshComponent>(MeshComponents, true);
        if (MeshComponents.Num() > 0) UseShape = EStaticGeometryShape::MESH;
    }
    if (UseShape == EStaticGeometryShape::AUTO) {
        TArray<UShapeComponent *> ShapeComponents;
        GetComponents<UShapeComponent>(ShapeComponents, true);
        if (ShapeComponents.Num() > 0) UseShape = EStaticGeometryShape::MESH_FROM_COLLISION;
    }
    if (UseShape == EStaticGeometryShape::AUTO) {
        TArray<USplineComponent *> SplineComponents;
        GetComponents<USplineComponent>(SplineComponents, true);
        if (SplineComponents.Num() > 0) UseShape = EStaticGeometryShape::SPLINE3D;
    }
    if (UseShape == EStaticGeometryShape::AUTO) {
        UseShape = EStaticGeometryShape::POINT3D;
    }

    switch (UseShape) {
    case EStaticGeometryShape::POINT:
        data.set_allocated_point(CreatePoint2D(GetOwner()->GetActorLocation()));
        break;
    case EStaticGeometryShape::POINT3D:
        data.set_allocated_point3d(CreatePoint3D(GetOwner()->GetActorLocation()));
        break;
    case EStaticGeometryShape::POLYGON_FROM_MESH_BOUNDS: {
        TArray<UStaticMeshComponent *> MeshComponents;
        GetComponents<UStaticMeshComponent>(MeshComponents, true);
        if (MeshComponents.Num() == 1) {
            data.set_allocated_polygon(CreatePolygon(MeshComponents[0]));
        } else {
            UE_LOG(LogRTI, Warning, TEXT("Can't create polygon from %d static mesh components, needs to be exactly one"),
                   MeshComponents.Num());
        }
        break;
    }
    case EStaticGeometryShape::MESH: {
        TArray<UStaticMeshComponent *> MeshComponents;
        GetComponents<UStaticMeshComponent>(MeshComponents, true);
        if (MeshComponents.Num() > 0) {
            data.set_allocated_mesh(CreateMesh(MeshComponents));
        } else {
            UE_LOG(LogRTI, Warning, TEXT("Can't create mesh geometry - no static mesh"));
        }
        break;
    }
    case EStaticGeometryShape::MESH_FLIPPED: {
        TArray<UStaticMeshComponent *> MeshComponents;
        GetComponents<UStaticMeshComponent>(MeshComponents, true);
        if (MeshComponents.Num() > 0) {
            data.set_allocated_mesh(CreateMesh(MeshComponents, false, true));
        } else {
            UE_LOG(LogRTI, Warning, TEXT("Can't create mesh geometry - no static mesh"));
        }
        break;
    }
    case EStaticGeometryShape::MESH_FROM_COLLISION: {
        TArray<UShapeComponent *> ShapeComponents;
        GetComponents<UShapeComponent>(ShapeComponents, true);
        if (ShapeComponents.Num() > 0) {
            data.set_allocated_mesh(CreateMeshFromCollision(ShapeComponents));
        } else {
            UE_LOG(LogRTI, Warning, TEXT("Can't create mesh geometry - no shapes"));
        }
        break;
    }
    case EStaticGeometryShape::LINE: {
        TArray<USplineComponent *> SplineComponents;
        GetComponents<USplineComponent>(SplineComponents, true);
        if (SplineComponents.Num() > 0) {
            data.set_allocated_line(CreateLine2DFromSpline(SplineComponents));
        } else {
            UE_LOG(LogRTI, Warning, TEXT("Can't create line geometry - no splines"));
        }
        break;
    }
    case EStaticGeometryShape::LINE3D: {
        TArray<USplineComponent *> SplineComponents;
        GetComponents<USplineComponent>(SplineComponents, true);
        if (SplineComponents.Num() > 0) {
            data.set_allocated_line3d(CreateLine3DFromSpline(SplineComponents));
        } else {
            UE_LOG(LogRTI, Warning, TEXT("Can't create line3d geometry - no splines"));
        }
        break;
    }
    case EStaticGeometryShape::SPLINE: {
        TArray<USplineComponent *> SplineComponents;
        GetComponents<USplineComponent>(SplineComponents, true);
        if (SplineComponents.Num() > 0) {
            data.set_allocated_spline(CreateSpline2D(SplineComponents));
        } else {
            UE_LOG(LogRTI, Warning, TEXT("Can't create spline geometry - no splines"));
        }
        break;
    }
    case EStaticGeometryShape::SPLINE3D: {
        TArray<USplineComponent *> SplineComponents;
        GetComponents<USplineComponent>(SplineComponents, true);
        if (SplineComponents.Num() > 0) {
            data.set_allocated_spline3d(CreateSpline3D(SplineComponents));
        } else {
            UE_LOG(LogRTI, Warning, TEXT("Can't create spline3d geometry - no splines"));
        }
        break;
    }
    default:
        UE_LOG(LogRTI, Warning, TEXT("Unimplemented geometry shape %d"), UseShape);
    }
}
