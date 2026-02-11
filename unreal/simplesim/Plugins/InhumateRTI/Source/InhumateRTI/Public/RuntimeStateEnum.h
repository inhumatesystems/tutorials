// Copyright 2025 Inhumate AB. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "inhumaterti.hpp"

#include "RuntimeStateEnum.generated.h"

// Blueprint-usable mapping of inhumate::rti::proto::RuntimeState

UENUM(BlueprintType)
enum class ERuntimeState : uint8 {
  UNKNOWN           UMETA(DisplayName="Unknown"),
  INITIAL           UMETA(DisplayName="Initial"),
  LOADING           UMETA(DisplayName="Loading"),
  READY             UMETA(DisplayName="Ready"),
  RUNNING           UMETA(DisplayName="Running"),
  PLAYBACK          UMETA(DisplayName="Playback"),
  PAUSED            UMETA(DisplayName="Paused"),
  PLAYBACK_PAUSED   UMETA(DisplayName="Playback paused"),
  END               UMETA(DisplayName="End"),
  PLAYBACK_END      UMETA(DisplayName="Playback end"),
  STOPPING          UMETA(DisplayName="Stopping"),
  STOPPED           UMETA(DisplayName="Stopped"),
  PLAYBACK_STOPPED  UMETA(DisplayName="Playback stopped")
};

static ERuntimeState RuntimeStateFromPbState(const inhumate::rti::proto::RuntimeState pbState) {
    switch (pbState) {
        case inhumate::rti::proto::RuntimeState::INITIAL: return ERuntimeState::INITIAL;
        case inhumate::rti::proto::RuntimeState::LOADING: return ERuntimeState::LOADING;
        case inhumate::rti::proto::RuntimeState::READY: return ERuntimeState::READY;
        case inhumate::rti::proto::RuntimeState::RUNNING: return ERuntimeState::RUNNING;
        case inhumate::rti::proto::RuntimeState::PLAYBACK: return ERuntimeState::PLAYBACK;
        case inhumate::rti::proto::RuntimeState::PAUSED: return ERuntimeState::PAUSED;
        case inhumate::rti::proto::RuntimeState::PLAYBACK_PAUSED: return ERuntimeState::PLAYBACK_PAUSED;
        case inhumate::rti::proto::RuntimeState::END: return ERuntimeState::END;
        case inhumate::rti::proto::RuntimeState::PLAYBACK_END: return ERuntimeState::PLAYBACK_END;
        case inhumate::rti::proto::RuntimeState::STOPPING: return ERuntimeState::STOPPING;
        case inhumate::rti::proto::RuntimeState::STOPPED: return ERuntimeState::STOPPED;
        case inhumate::rti::proto::RuntimeState::PLAYBACK_STOPPED: return ERuntimeState::PLAYBACK_STOPPED;
    }
    return ERuntimeState::UNKNOWN;
}

static inhumate::rti::proto::RuntimeState PbStateFromRuntimeState(ERuntimeState state) {
    inhumate::rti::proto::RuntimeState pbState = inhumate::rti::proto::RuntimeState::UNKNOWN;
    switch (state) {
        case ERuntimeState::INITIAL: pbState = inhumate::rti::proto::RuntimeState::INITIAL; break;
        case ERuntimeState::LOADING: pbState = inhumate::rti::proto::RuntimeState::LOADING; break;
        case ERuntimeState::READY: pbState = inhumate::rti::proto::RuntimeState::READY; break;
        case ERuntimeState::RUNNING: pbState = inhumate::rti::proto::RuntimeState::RUNNING; break;
        case ERuntimeState::PLAYBACK: pbState = inhumate::rti::proto::RuntimeState::PLAYBACK; break;
        case ERuntimeState::PAUSED: pbState = inhumate::rti::proto::RuntimeState::PAUSED; break;
        case ERuntimeState::PLAYBACK_PAUSED: pbState = inhumate::rti::proto::RuntimeState::PLAYBACK_PAUSED; break;
        case ERuntimeState::END: pbState = inhumate::rti::proto::RuntimeState::END; break;
        case ERuntimeState::PLAYBACK_END: pbState = inhumate::rti::proto::RuntimeState::PLAYBACK_END; break;
        case ERuntimeState::STOPPING: pbState = inhumate::rti::proto::RuntimeState::STOPPING; break;
        case ERuntimeState::STOPPED: pbState = inhumate::rti::proto::RuntimeState::STOPPED; break;
        case ERuntimeState::PLAYBACK_STOPPED: pbState = inhumate::rti::proto::RuntimeState::PLAYBACK_STOPPED; break;
    }
    return pbState;
}