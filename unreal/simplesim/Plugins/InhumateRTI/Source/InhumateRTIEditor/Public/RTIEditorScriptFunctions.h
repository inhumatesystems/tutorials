// Copyright 2025 Inhumate AB. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RTIStaticGeometryComponent.h"
#include "RTIEntityComponent.h"
#include "RTIPositionComponent.h"

#include "RTIEditorScriptFunctions.generated.h"

UCLASS()
class INHUMATERTIEDITOR_API URTIEditorScriptFunctions : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:	
	UFUNCTION(BlueprintCallable, Category = "RTI | Level Utility")
	static URTIStaticGeometryComponent* AddRTIStaticGeometryToActor(AActor * Actor);

	UFUNCTION(BlueprintCallable, Category = "RTI | Level Utility")
    static URTIEntityComponent* AddRTIEntityToActor(AActor * Actor);

	UFUNCTION(BlueprintCallable, Category = "RTI | Level Utility")
    static URTIPositionComponent* AddRTIPositionToActor(AActor * Actor);
};
