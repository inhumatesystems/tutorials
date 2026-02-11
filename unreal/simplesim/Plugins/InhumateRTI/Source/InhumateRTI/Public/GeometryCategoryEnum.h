// Copyright 2025 Inhumate AB. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "inhumaterti.hpp"

#include "GeometryCategoryEnum.generated.h"

// Blueprint-usable mapping of inhumate::rti::proto::Geometry::Category
// TODO this really needs to be generated...

UENUM(BlueprintType)
enum class EGeometryCategory : uint8 {
    UNKNOWN_GEOMETRY = 0 UMETA(DisplayName = "Unknown geometry"),
    PHYSICAL_GEOMETRY = 10 UMETA(DisplayName = "Physical geometry"),

    GROUND = 11 UMETA(DisplayName = "Ground"),
    VEGETATION = 12 UMETA(DisplayName = "Vegetation"),
    STRUCTURE = 13 UMETA(DisplayName = "Structure"),
    ROAD = 14 UMETA(DisplayName = "Road"),

    ABSTRACT_GEOMETRY = 100 UMETA(DisplayName = "Abstract geometry"),
    ZONE = 101 UMETA(DisplayName = "Zone"),
    TRIGGER = 102 UMETA(DisplayName = "Trigger"),
    EDGE = 103 UMETA(DisplayName = "Edge"),
    PATH = 104 UMETA(DisplayName = "Path"),
    PLANNED_PATH = 105 UMETA(DisplayName = "Planned path"),
    HISTORIC_PATH = 106 UMETA(DisplayName = "Historic path"),

    SENSOR_COVERAGE = 151 UMETA(DisplayName = "Sensor coverage"),
    SENSOR_HIT = 152 UMETA(DisplayName = "Sensor hit"),

    // Debug

    DEBUG_GEOMETRY = 200 UMETA(DisplayName = "Debug geometry"),
};

static EGeometryCategory GeometryCategoryFromPb(const inhumate::rti::proto::Geometry::Category pbCategory) {
    return (EGeometryCategory)pbCategory;
    /*
    switch (pbCategory) {
        case inhumate::rti::proto::GeometryOperation::UNKNOWN_GEOMETRY: return EGeometryCategory::UNKNOWN_GEOMETRY;
        case inhumate::rti::proto::GeometryOperation::PHYSICAL_GEOMETRY: return EGeometryCategory::PHYSICAL_GEOMETRY;
    }
    return EGeometryCategory::UNKNOWN;
    */
}

static inhumate::rti::proto::Geometry::Category PbCategoryFromGeometryCategory(EGeometryCategory category)
{
    return (inhumate::rti::proto::Geometry::Category)category;
    /*
    inhumate::rti::proto::Geometry::Category pbCategory = inhumate::rti::proto::GeometryOperation::UNKNOWN_GEOMETRY;
    switch (category) {
        case EGeometryCategory::PHYSICAL_GEOMETRY: pbCategory = inhumate::rti::proto::GeometryOperation::PHYSICAL_GEOMETRY; break;
    }
    return pbCategory;
    */
}