// Copyright 2025 Inhumate AB. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "RTICoordinateConversionInterface.generated.h"

USTRUCT(BlueprintType)
struct FGeodeticLocation {
    GENERATED_USTRUCT_BODY()

    UPROPERTY(BlueprintReadWrite, Category="RTI")
    float Longitude;

    UPROPERTY(BlueprintReadWrite, Category="RTI")
    float Latitude;

    UPROPERTY(BlueprintReadWrite, Category="RTI")
    float Altitude;
};

UINTERFACE(MinimalAPI, Blueprintable)
class URTIGeodeticCoordinateConversionInterface : public UInterface
{
    GENERATED_BODY()
};

class INHUMATERTI_API IRTIGeodeticCoordinateConversionInterface
{
    GENERATED_BODY()

    public:
    UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = RTI)
    void GeodeticToLocal(const FGeodeticLocation &GeodeticLocation, FVector &LocalLocation);

    UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = RTI)
    void LocalToGeodetic(const FVector &LocalLocation, FGeodeticLocation &GeodeticLocation);
};

UINTERFACE(MinimalAPI, Blueprintable)
class URTILocalCoordinateConversionInterface : public UInterface
{
    GENERATED_BODY()
};

class INHUMATERTI_API IRTILocalCoordinateConversionInterface
{
    GENERATED_BODY()

    public:
    UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = RTI)
    void RTIToLocalLocation(const FVector &RTILocation, FVector &LocalLocation);

    UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = RTI)
    void LocalLocationToRTI(const FVector &LocalLocation, FVector &RTILocation);

    UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = RTI)
    void RTIEulerToLocalRotation(const FRotator &RTIEuler, FRotator &LocalRotation);

    UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = RTI)
    void LocalRotationToRTIEuler(const FRotator &LocalRotation, FRotator &RTIEuler);
};
