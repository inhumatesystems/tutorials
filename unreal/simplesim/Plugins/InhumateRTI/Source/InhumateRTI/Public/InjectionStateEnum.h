// Copyright 2025 Inhumate AB. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "inhumaterti.hpp"

#include "InjectionStateEnum.generated.h"

// Blueprint-usable mapping of inhumate::rti::proto::Injection_State

UENUM(BlueprintType)
enum class EInjectionState : uint8 {
  UNKNOWN       UMETA(DisplayName="Unknown"),
  DISABLED      UMETA(DisplayName="Disabled"),
  ENABLED       UMETA(DisplayName="Enabled"),
  RUNNING       UMETA(DisplayName="Running"),
  END           UMETA(DisplayName="End"),
  STOPPED       UMETA(DisplayName="Stopped"),
  CANCELED      UMETA(DisplayName="Canceled"),
};

static EInjectionState InjectionStateFromPb(const inhumate::rti::proto::Injection_State pbState) {
    switch (pbState) {
        case inhumate::rti::proto::Injection_State_UNKNOWN: return EInjectionState::UNKNOWN;
        case inhumate::rti::proto::Injection_State_DISABLED: return EInjectionState::DISABLED;
        case inhumate::rti::proto::Injection_State_ENABLED: return EInjectionState::ENABLED;
        case inhumate::rti::proto::Injection_State_RUNNING: return EInjectionState::RUNNING;
        case inhumate::rti::proto::Injection_State_END: return EInjectionState::END;
        case inhumate::rti::proto::Injection_State_STOPPED: return EInjectionState::STOPPED;
        case inhumate::rti::proto::Injection_State_CANCELED: return EInjectionState::CANCELED;
    }
    return EInjectionState::UNKNOWN;
}

static inhumate::rti::proto::Injection_State PbInjectionState(EInjectionState state) {
    inhumate::rti::proto::Injection_State pbState = inhumate::rti::proto::Injection_State_UNKNOWN;
    switch (state) {
        case EInjectionState::UNKNOWN: pbState = inhumate::rti::proto::Injection_State_UNKNOWN; break;
        case EInjectionState::DISABLED: pbState = inhumate::rti::proto::Injection_State_DISABLED; break;
        case EInjectionState::ENABLED: pbState = inhumate::rti::proto::Injection_State_ENABLED; break;
        case EInjectionState::RUNNING: pbState = inhumate::rti::proto::Injection_State_RUNNING; break;
        case EInjectionState::END: pbState = inhumate::rti::proto::Injection_State_END; break;
        case EInjectionState::STOPPED: pbState = inhumate::rti::proto::Injection_State_STOPPED; break;
        case EInjectionState::CANCELED: pbState = inhumate::rti::proto::Injection_State_CANCELED; break;
    }
    return pbState;
}