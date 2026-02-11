// Copyright 2025 Inhumate AB. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "inhumaterti.hpp"

#include "RTIParameter.generated.h"

UENUM(BlueprintType)
enum class ERTIParameterType : uint8 {
    STRING              UMETA(DisplayName="String"),
    TEXT                UMETA(DisplayName="Text"),
    FLOAT               UMETA(DisplayName="Float"),
    INTEGER             UMETA(DisplayName="Integer"),
    SWITCH              UMETA(DisplayName="Switch"),
    CHECKBOX            UMETA(DisplayName="Checkbox"),
    CHOICE              UMETA(DisplayName="Choice"),
};

USTRUCT(BlueprintType)
struct FRTIParameter {

	GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    FString Name;

    // User-friendly label. If left blank, same as name.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    FString Label;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    FString Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    ERTIParameterType Type;

    // List of choices, separated by pipe (|)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI", meta=(EditCondition="Type==ERTIParameterType::CHOICE", EditConditionHides))
    FString Choices;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    FString DefaultValue;

};

static void ParameterToPb(const FRTIParameter& parameter, inhumate::rti::proto::Parameter* pbParameter) {
    pbParameter->set_name(TCHAR_TO_UTF8(*parameter.Name));
    pbParameter->set_label(TCHAR_TO_UTF8(*parameter.Label));
    pbParameter->set_description(TCHAR_TO_UTF8(*parameter.Description));
    pbParameter->set_default_value(TCHAR_TO_UTF8(*parameter.DefaultValue));
    switch (parameter.Type) {
        case ERTIParameterType::STRING:
            pbParameter->set_type("string");
            break;
        case ERTIParameterType::TEXT:
            pbParameter->set_type("text");
            break;
        case ERTIParameterType::FLOAT:
            pbParameter->set_type("float");
            break;
        case ERTIParameterType::INTEGER:
            pbParameter->set_type("integer");
            break;
        case ERTIParameterType::SWITCH:
            pbParameter->set_type("switch");
            break;
        case ERTIParameterType::CHECKBOX:
            pbParameter->set_type("checkbox");
            break;
        case ERTIParameterType::CHOICE:
            pbParameter->set_type(TCHAR_TO_UTF8(*(FString("choice|") + parameter.Choices)));
            break;
   }
}
