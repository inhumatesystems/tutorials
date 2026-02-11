// Copyright 2025 Inhumate AB. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RTIEntityStateComponent.h"
#include "RTISubsystem.h"

#include "RTIPositionComponent.generated.h"

// Publishes/subscribes to entity positions and updates actor location/rotation

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent), DisplayName="RTI Position")
class INHUMATERTI_API URTIPositionComponent : public URTIEntityStateComponent
{
	GENERATED_BODY()

public:
    URTIPositionComponent();

    // Minimum interval (in seconds) between published position updates
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    float MinPublishInterval;

    // Maximum interval (in seconds) between published position updates
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    float MaxPublishInterval;

    // Minimum distance (in cm) to move to publish update
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    float PositionThreshold;

    // Minimum angular distance (in degrees) to rotate to publish update
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    float RotationThreshold;

    // Minimum velocity (in cm/s) change to publish update
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    float VelocityThreshold;

    // If checked, position will be interpolated between incoming position updates
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    bool Interpolate;

    // When interpolating, avoid reversing direction
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    bool PreventReversal;

    // Stop interpolating if time since last message exceeds this value (in seconds)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    float MaxInterpolateInterval;

    // Amount of smoothing (lerping) to use for position interpolation. Experiment until it "looks good".
    UPROPERTY(EditAnywhere,Category = "RTI" , meta=(UIMin = "0.01", UIMax = "10.0"))
    float PositionSmoothing;

    // Amount of smoothing (slerping) to use for rotation interpolation. Experiment until it "looks good".
    UPROPERTY(EditAnywhere, Category = "RTI", meta = (UIMin = "0.01", UIMax = "10.0"))
    float RotationSmoothing;

    UFUNCTION(BlueprintPure, Category = "RTI")
    FVector GetVelocity() { return LastVelocity; }

    UFUNCTION(BlueprintPure, Category = "RTI")
    FVector GetAcceleration() { return LastAcceleration; }

    UFUNCTION(BlueprintPure, Category = "RTI")
    FVector GetAngularVelocity() { return LastAngularVelocity.Euler(); }

protected:
    virtual void OnMessage(const FString& content) override;

public:
    void ReceivePosition(const inhumate::rti::proto::EntityPosition &message, const bool first = false);
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    static void SetPositionMessageFromActor(inhumate::rti::proto::EntityPosition& message, AActor *actor);
    static FVector GetMessageLocation(const inhumate::rti::proto::EntityPosition& message, UWorld *world);
    static FRotator GetMessageRotation(const inhumate::rti::proto::EntityPosition& message, UWorld *world);

    static inhumate::rti::proto::EntityPosition_LocalPosition* UEToRTILocalPosition(const FVector& vector);
    static FVector RTILocalPositionToUE(const inhumate::rti::proto::EntityPosition_LocalPosition& position);
    static inhumate::rti::proto::EntityPosition_LocalRotation* UEToRTILocalRotation(const FQuat& quat);
    static FQuat RTILocalRotationToUE(const inhumate::rti::proto::EntityPosition_LocalRotation& rotation);
    static inhumate::rti::proto::EntityPosition_EulerRotation * UEToRTIEulerRotation(const FRotator& quat);
    static FRotator RTIEulerRotationToUE(const inhumate::rti::proto::EntityPosition_EulerRotation& rotation);
    static inhumate::rti::proto::EntityPosition_VelocityVector* UEToRTIVelocity(const FVector& velocity);
    static FVector RTIVelocityToUE(const inhumate::rti::proto::EntityPosition_VelocityVector& velocity);

private:

    float TimeSinceLastPublish;
    float LastPositionTime;
    FVector LastPosition;
    float LastRotationTime;
    FQuat LastRotation;
    float PreviousPositionTime;
    FVector PreviousPosition;
    float PreviousRotationTime;
    FQuat PreviousRotation;

    FVector LastVelocity;
    bool LastVelocityValid;
    FVector LastAcceleration;
    bool LastAccelerationValid;
    FRotator LastAngularVelocity;
    bool LastAngularVelocityValid;

};
