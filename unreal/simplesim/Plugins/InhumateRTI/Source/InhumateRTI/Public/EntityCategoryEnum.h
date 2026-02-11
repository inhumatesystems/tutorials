// Copyright 2025 Inhumate AB. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "inhumaterti.hpp"

#include "EntityCategoryEnum.generated.h"

// Blueprint-usable mapping of inhumate::rti::proto::EntityCategory

UENUM(BlueprintType)
enum class EEntityCategory : uint8 {
  UNKNOWN       UMETA(DisplayName="Unknown"),
  OTHER         UMETA(DisplayName="Other"),
  PERSON        UMETA(DisplayName="Person"),
  ANIMAL        UMETA(DisplayName="Animal"),
  VEHICLE       UMETA(DisplayName="Vehicle"),
};

static EEntityCategory EntityCategoryFromPb(const inhumate::rti::proto::EntityCategory pbCategory) {
    switch (pbCategory) {
        case inhumate::rti::proto::EntityCategory::UNKNOWN_CATEGORY: return EEntityCategory::UNKNOWN;
        case inhumate::rti::proto::EntityCategory::OTHER_CATEGORY: return EEntityCategory::OTHER;
        case inhumate::rti::proto::EntityCategory::PERSON: return EEntityCategory::PERSON;
        case inhumate::rti::proto::EntityCategory::ANIMAL: return EEntityCategory::ANIMAL;
        case inhumate::rti::proto::EntityCategory::VEHICLE: return EEntityCategory::VEHICLE;
    }
    return EEntityCategory::UNKNOWN;
}

static inhumate::rti::proto::EntityCategory PbCategoryFromEntityCategory(EEntityCategory category) {
    inhumate::rti::proto::EntityCategory pbCategory = inhumate::rti::proto::EntityCategory::UNKNOWN_CATEGORY;
    switch (category) {
        case EEntityCategory::OTHER: pbCategory = inhumate::rti::proto::EntityCategory::OTHER_CATEGORY; break;
        case EEntityCategory::PERSON: pbCategory = inhumate::rti::proto::EntityCategory::PERSON; break;
        case EEntityCategory::ANIMAL: pbCategory = inhumate::rti::proto::EntityCategory::ANIMAL; break;
        case EEntityCategory::VEHICLE: pbCategory = inhumate::rti::proto::EntityCategory::VEHICLE; break;
    }
    return pbCategory;
}