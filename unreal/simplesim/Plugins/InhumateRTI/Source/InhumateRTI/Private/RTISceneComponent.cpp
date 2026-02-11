// Copyright 2025 Inhumate AB. All Rights Reserved.

#include "RTISceneComponent.h"

inhumate::rti::RTIClient *URTISceneComponent::RTI()
{
    return URTISubsystem::GetRTIForComponent(this);
}

URTISubsystem *URTISceneComponent::GetSubsystem()
{
    return URTISubsystem::GetSubsystemForComponent(this);
}

