// Copyright 2025 Inhumate AB. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "RTIScenario.h"

#include "RTISettings.generated.h"

UCLASS(config = Engine)
class INHUMATERTI_API URTISettings : public UObject
{
	GENERATED_BODY()
	
public:

    // If this is NOT checked, you need to call the Connect() function in Blueprint or C++.
    UPROPERTY(EditAnywhere, config, Category = "Connection")
    bool AutoConnect = true;

    // A string that can be used to identify this application. Leave blank to use project name.
    UPROPERTY(EditAnywhere, config, Category = "Connection")
    FString Application = "";

    // Default URL to connect to. Leave blank to use client default.
    UPROPERTY(EditAnywhere, config, Category = "Connection")
    FString Url = "";

    // Federation to connecto to. OK to leave blank.
    UPROPERTY(EditAnywhere, config, Category = "Connection")
    FString Federation = "";

    // Default secret to use when connecting. OK to leave blank.
    UPROPERTY(EditAnywhere, config, Category = "Connection")
    FString Secret = "";

    // Level to return to on RESET runtime control message.
    UPROPERTY(EditAnywhere, config, Category = "Game", meta = (AllowedClasses = "World"))
    FSoftObjectPath HomeLevel;

    // List of scenarios. Level will be opened on LOAD SCENARIO runtime control message. 
    // Use * as scenario name to catch all names.
    UPROPERTY(EditAnywhere, config, Category = "Game", meta = (TitleProperty = "Name"))
    TArray<FRTIScenario> Scenarios;

};
