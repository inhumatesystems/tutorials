// Copyright 2025 Inhumate AB. All Rights Reserved.

#include "RTIComponent.h"
#include "RTISubsystem.h"

inhumate::rti::RTIClient *URTIComponent::RTI()
{
    return URTISubsystem::GetRTIForComponent(this);
}

URTISubsystem *URTIComponent::GetSubsystem()
{
    return URTISubsystem::GetSubsystemForComponent(this);
}
