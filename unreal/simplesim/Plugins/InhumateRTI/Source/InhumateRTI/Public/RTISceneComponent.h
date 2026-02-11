// Copyright 2025 Inhumate AB. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"

#include "RTISubsystem.h"

#include "RTISceneComponent.generated.h"

UCLASS(Abstract)
class INHUMATERTI_API URTISceneComponent : public USceneComponent
{
	GENERATED_BODY()

protected:
    inhumate::rti::RTIClient *RTI();
    URTISubsystem *GetSubsystem();
};
