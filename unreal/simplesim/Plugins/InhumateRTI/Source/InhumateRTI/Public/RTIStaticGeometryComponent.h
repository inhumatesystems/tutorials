// Copyright 2025 Inhumate AB. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "GeometryCategoryEnum.h"
#include "RTIGeometryComponent.h"

#include "RTIStaticGeometryComponent.generated.h"

UENUM(BlueprintType)
enum class EStaticGeometryShape : uint8 {
    AUTO UMETA(DisplayName = "Auto"),
    POINT UMETA(DisplayName = "Point"),
    POINT3D UMETA(DisplayName = "Point3D"),
    POLYGON_FROM_MESH_BOUNDS UMETA(DisplayName = "Polygon from mesh bounds"),
    MESH UMETA(DisplayName = "Mesh"),
    MESH_FLIPPED UMETA(DisplayName = "Mesh flipped normals"),
    MESH_FROM_COLLISION UMETA(DisplayName = "Mesh from collision"),
    LINE UMETA(DisplayName = "Line"),
    LINE3D UMETA(DisplayName = "Line3D"),
    SPLINE UMETA(DisplayName = "Spline"),
    SPLINE3D UMETA(DisplayName = "Spline3D")
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent), DisplayName = "RTI Static Geometry")
class INHUMATERTI_API URTIStaticGeometryComponent : public URTIGeometryComponent
{
    GENERATED_BODY()

    public:
    URTIStaticGeometryComponent();

    // Type of RTI shape to create from the Unreal components.
    // Set to "Auto" to use defaults (let the component try to figure it out).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    EStaticGeometryShape Shape;

    // A string that can be used to identify this geometry type in another application. OK to leave blank.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    FString Type;

    // Category can be used to distinguish geometry, e.g. to filter geometry in a scenario viewer.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    EGeometryCategory Category;

    // Color that should be used to represent this geometry. Not used if A == 0 (which is the default).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    FColor Color;

    // Opacity that should be used to represent this geometry. From 0 (transparent) to 1 (opaque).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Opacity = 1.f;

    // Title to show in the label of this geometry. Leave blank to not show a label.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    FString Title;

    // Color that should be used for the label. Not used if A == 0 (which is the default).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    FColor LabelColor;

    // Opacity that should be used for the label. From 0 (transparent) to 1 (opaque).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float LabelOpacity = 1.f;

    // If checked, the geometry should be rendered in wireframe instead of solid.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    bool Wireframe;

    // Width of lines to use (for wire frame or line/spline geometry)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    float LineWidth = 1.f;

    UFUNCTION(BlueprintCallable, Category = "RTI")
    void SetColor(const FColor &NewColor);

    // A string that can be used to identify this specific geometry instance in another application. OK to leave blank.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI", DisplayName = "Specific ID")
    FString SpecificId;

    protected:
    virtual void BeginPlay() override;
    void InitializeComponent();
    virtual void FillGeometryData(inhumate::rti::proto::Geometry& data) override;

};
