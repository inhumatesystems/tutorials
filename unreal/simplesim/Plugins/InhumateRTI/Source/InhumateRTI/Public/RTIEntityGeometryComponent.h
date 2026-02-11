// Copyright 2025 Inhumate AB. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "RTIGeometryComponent.h"

#include "RTIEntityGeometryComponent.generated.h"

UENUM(BlueprintType)
enum class EEntityGeometryShape : uint8 {
    AUTO UMETA(DisplayName = "Auto"),
    POINT3D UMETA(DisplayName = "Point3D"),
    MESH UMETA(DisplayName = "Mesh"),
    MESH_FROM_COLLISION UMETA(DisplayName = "Mesh from collision")
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent), DisplayName = "RTI Entity Geometry")
class INHUMATERTI_API URTIEntityGeometryComponent : public URTIGeometryComponent
{
    GENERATED_BODY()

    public:
    URTIEntityGeometryComponent();

    // Entity type this geometry should be used for
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    FString EntityType;

    // Type of RTI shape to create from the Unreal components.
    // Set to "Auto" to use defaults (let the component try to figure it out).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    EEntityGeometryShape Shape;

    // If the shape/mesh created has size 1x1x1, this can be checked, and geometry representation will be scaled to entity dimensions (length x width x height).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    bool ScaleToEntityDimensions;

    // Color that should be used to represent this geometry. Not used if A == 0 (which is the default).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    FColor Color;

    // Opacity that should be used to represent this geometry. From 0 (transparent) to 1 (opaque).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Opacity = 1.f;

    // Color that should be used for the label. Not used if A == 0 (which is the default).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    FColor LabelColor;

    // Opacity that should be used for the label. From 0 (transparent) to 1 (opaque).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float LabelOpacity = 1.f;

    // Set this to explicitly use a specific mesh (instead of finding it within components which is the default)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    UStaticMesh *StaticMesh;

    protected:
    virtual void InitializeComponent() override;
    virtual void FillGeometryData(inhumate::rti::proto::Geometry& data) override;
};
