// Copyright 2025 Inhumate AB. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RTISubsystem.h"

#include "RTIComponent.generated.h"

// Base class for components that want convenient access to the RTI

UCLASS(Abstract)
class INHUMATERTI_API URTIComponent : public UActorComponent
{
	GENERATED_BODY()

protected:
    inhumate::rti::RTIClient *RTI();
    URTISubsystem *GetSubsystem();

};
