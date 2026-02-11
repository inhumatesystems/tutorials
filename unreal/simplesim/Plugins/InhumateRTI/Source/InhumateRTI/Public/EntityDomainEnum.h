// Copyright 2025 Inhumate AB. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "inhumaterti.hpp"

#include "EntityDomainEnum.generated.h"

// Blueprint-usable mapping of inhumate::rti::proto::EntityDomain

UENUM(BlueprintType)
enum class EEntityDomain : uint8 {
  UNKNOWN       UMETA(DisplayName="Unknown"),
  OTHER         UMETA(DisplayName="Other"),
  LAND          UMETA(DisplayName="Land"),
  SEA           UMETA(DisplayName="Sea"),
  AIR           UMETA(DisplayName="Air"),
  SPACE         UMETA(DisplayName="Space"),
};

static EEntityDomain EntityDomainFromPb(const inhumate::rti::proto::EntityDomain pbDomain) {
    switch (pbDomain) {
        case inhumate::rti::proto::EntityDomain::UNKNOWN_DOMAIN: return EEntityDomain::UNKNOWN;
        case inhumate::rti::proto::EntityDomain::OTHER_DOMAIN: return EEntityDomain::OTHER;
        case inhumate::rti::proto::EntityDomain::LAND: return EEntityDomain::LAND;
        case inhumate::rti::proto::EntityDomain::SEA: return EEntityDomain::SEA;
        case inhumate::rti::proto::EntityDomain::AIR: return EEntityDomain::AIR;
        case inhumate::rti::proto::EntityDomain::SPACE: return EEntityDomain::SPACE;
    }
    return EEntityDomain::UNKNOWN;
}

static inhumate::rti::proto::EntityDomain PbDomainFromEntityDomain(EEntityDomain domain) {
    inhumate::rti::proto::EntityDomain pbDomain = inhumate::rti::proto::EntityDomain::UNKNOWN_DOMAIN;
    switch (domain) {
        case EEntityDomain::OTHER: pbDomain = inhumate::rti::proto::EntityDomain::OTHER_DOMAIN; break;
        case EEntityDomain::LAND: pbDomain = inhumate::rti::proto::EntityDomain::LAND; break;
        case EEntityDomain::SEA: pbDomain = inhumate::rti::proto::EntityDomain::SEA; break;
        case EEntityDomain::AIR: pbDomain = inhumate::rti::proto::EntityDomain::AIR; break;
        case EEntityDomain::SPACE: pbDomain = inhumate::rti::proto::EntityDomain::SPACE; break;
    }
    return pbDomain;
}