// Copyright 2025 Inhumate AB. All Rights Reserved.

#include "RTIEditorScriptFunctions.h"

URTIStaticGeometryComponent* URTIEditorScriptFunctions::AddRTIStaticGeometryToActor(AActor *Actor) {
    auto Component = NewObject<URTIStaticGeometryComponent>(Actor, FName(TEXT("RTI Static Geometry")));
    Actor->AddInstanceComponent(Component);
    Component->AttachToComponent(Actor->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
    if (!Actor->IsRootComponentMovable()) Component->SetMobility(EComponentMobility::Static);
    Component->RegisterComponent();
    return Component;
}

URTIEntityComponent* URTIEditorScriptFunctions::AddRTIEntityToActor(AActor *Actor) {
    auto Component = NewObject<URTIEntityComponent>(Actor, FName(TEXT("RTI Entity")));
    Actor->AddInstanceComponent(Component);
    Component->RegisterComponent();
    return Component;
}

URTIPositionComponent* URTIEditorScriptFunctions::AddRTIPositionToActor(AActor *Actor) {
    auto Component = NewObject<URTIPositionComponent>(Actor, FName(TEXT("RTI Position")));
    Actor->AddInstanceComponent(Component);
    Component->RegisterComponent();
    return Component;
}
