// Copyright 2025 Inhumate AB. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RTIParameter.h"
#include "inhumaterti.hpp"

#include "RTIScenario.generated.h"

USTRUCT(BlueprintType)
struct FRTIScenario {

	GENERATED_BODY()

    // Unique name used to identify this scenario
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    FString Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI", meta = (AllowedClasses = "World"))
    FSoftObjectPath Level;

    // User-friendly description (multiline)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI", meta=(MultiLine=true))
    FString Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI", meta = (TitleProperty = "Name"))
    TArray<FRTIParameter> Parameters;

};

static void ScenarioToPb(const FRTIScenario& scenario, inhumate::rti::proto::Scenario* pbScenario) {
    pbScenario->set_name(TCHAR_TO_UTF8(*scenario.Name));
    pbScenario->set_description(TCHAR_TO_UTF8(*scenario.Description));
    for (auto param : scenario.Parameters) {
        auto pbParam = pbScenario->add_parameters();
        ParameterToPb(param, pbParam);
    }
}
