// Copyright 2025 Inhumate AB. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "RTIComponent.h"

#include "RTIMeasureComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent), DisplayName="RTI Measure")
class INHUMATERTI_API URTIMeasureComponent : public URTIComponent
{
	GENERATED_BODY()

public:

    // Unique ID of this measure
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    FString Id;

    // Title of this measure, to be presented to the user. OK to leave blank (in which case ID will be used).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    FString Title;

    // Unit of this measure, to be presented to the user. E.g. "kg", "seconds" or "m3". OK to leave blank.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    FString Unit;

    // Channel to publish measure on. Leave blank to use default.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    FString Channel;

    // Interval to collect and publish measurements periodically. Use 0 to publish instantly on measurement.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    float Interval;

    UFUNCTION(BlueprintCallable, Category = "RTI")
    void Measure(const float Value);

protected:
    TUniquePtr<inhumate::rti::proto::Measure> MyMeasure;

};
