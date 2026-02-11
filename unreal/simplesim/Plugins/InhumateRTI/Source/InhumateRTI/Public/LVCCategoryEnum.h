// Copyright 2025 Inhumate AB. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "inhumaterti.hpp"

#include "LVCCategoryEnum.generated.h"

// Blueprint-usable mapping of inhumate::rti::proto::LVCCategory

UENUM(BlueprintType)
enum class ELVCCategory : uint8 {
  UNKNOWN       UMETA(DisplayName="Unknown"),
  LIVE          UMETA(DisplayName="Live"),
  VIRTUAL       UMETA(DisplayName="Virtual"),
  CONSTRUCTIVE  UMETA(DisplayName="Constructive"),
};

static ELVCCategory LVCCategoryFromPb(const inhumate::rti::proto::LVCCategory pbCategory) {
    switch (pbCategory) {
        case inhumate::rti::proto::LVCCategory::UNKNOWN_LVC_CATEGORY: return ELVCCategory::UNKNOWN;
        case inhumate::rti::proto::LVCCategory::LIVE: return ELVCCategory::LIVE;
        case inhumate::rti::proto::LVCCategory::VIRTUAL: return ELVCCategory::VIRTUAL;
        case inhumate::rti::proto::LVCCategory::CONSTRUCTIVE: return ELVCCategory::CONSTRUCTIVE;
    }
    return ELVCCategory::UNKNOWN;
}

static inhumate::rti::proto::LVCCategory PbCategoryFromLVCCategory(ELVCCategory category) {
    inhumate::rti::proto::LVCCategory pbCategory = inhumate::rti::proto::LVCCategory::UNKNOWN_LVC_CATEGORY;
    switch (category) {
        case ELVCCategory::LIVE: pbCategory = inhumate::rti::proto::LVCCategory::LIVE; break;
        case ELVCCategory::VIRTUAL: pbCategory = inhumate::rti::proto::LVCCategory::VIRTUAL; break;
        case ELVCCategory::CONSTRUCTIVE: pbCategory = inhumate::rti::proto::LVCCategory::CONSTRUCTIVE; break;
    }
    return pbCategory;
}