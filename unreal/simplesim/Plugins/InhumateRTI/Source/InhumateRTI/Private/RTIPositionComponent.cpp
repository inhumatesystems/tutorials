// Copyright 2025 Inhumate AB. All Rights Reserved.

#include "RTIPositionComponent.h"
#include "RTICoordinateConversionInterface.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "Components/PrimitiveComponent.h"

URTIPositionComponent::URTIPositionComponent()
: URTIEntityStateComponent(inhumate::rti::POSITION_CHANNEL, true)
{
    MinPublishInterval = 1.f;
    MaxPublishInterval = 10.f;
    PositionThreshold = 1.0f;
    RotationThreshold = 0.1f;
    VelocityThreshold = 10.f;
    Interpolate = true;
    PreventReversal = false;
    MaxInterpolateInterval = 2.f;
    PositionSmoothing = 1.f;
    RotationSmoothing = 1.f;
    PrimaryComponentTick.bCanEverTick = true;
    TimeSinceLastPublish = MAX_FLT;
    LastPositionTime = -MAX_FLT;
    PreviousPositionTime = -MAX_FLT;
    LastRotationTime = -MAX_FLT;
    PreviousRotationTime = -MAX_FLT;
    bAutoActivate = true;
}

void URTIPositionComponent::OnMessage(const FString &content)
{
    auto message = Parse<inhumate::rti::proto::EntityPosition>(content);
    ReceivePosition(message);
}

void URTIPositionComponent::ReceivePosition(const inhumate::rti::proto::EntityPosition &message, const bool first)
{
    // UE_LOG(LogRTI, Log, TEXT("Got position %s %.1f %.1f %.1f"), *FString(message.id().c_str()), message.local().x(), message.local().y(), message.local().z());
    if (IsReceiving()) {
        if (message.has_local_rotation() || message.has_euler_rotation()) {
            FRotator Rotation = GetMessageRotation(message, GetWorld());
            PreviousRotation = LastRotation;
            PreviousRotationTime = LastRotationTime;
            LastRotation = Rotation.Quaternion();
            LastRotationTime = GetWorld()->GetTimeSeconds();
            if (!Interpolate || first)
                GetOwner()->SetActorRotation(Rotation, ETeleportType::TeleportPhysics);
        }
        if (message.has_local() || message.has_geodetic()) {
            FVector Location = GetMessageLocation(message, GetWorld());
            PreviousPositionTime = LastPositionTime;
            PreviousPosition = LastPosition;
            LastPosition = Location;
            LastPositionTime = GetWorld()->GetTimeSeconds();
            if (!Interpolate || first)
                GetOwner()->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
        }
        LastVelocityValid = false;
        if (message.has_velocity()) {
            LastVelocity = RTIVelocityToUE(message.velocity());
            LastVelocityValid = true;
        }
        LastAccelerationValid = false;
        if (message.has_acceleration()) {
            LastAcceleration = RTIVelocityToUE(message.acceleration());
            LastAccelerationValid = true;
        }
        LastAngularVelocityValid = false;
        if (message.has_angular_velocity()) {
            LastAngularVelocity = RTIEulerRotationToUE(message.angular_velocity());
            LastAngularVelocityValid = true;
        }
    }
}

void URTIPositionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!IsActive()) return;

    TimeSinceLastPublish += DeltaTime;
    float time = GetWorld()->GetTimeSeconds();
    FVector LocalVelocity = GetOwner()->GetActorRotation().GetInverse().RotateVector(GetOwner()->GetVelocity());
    if (IsPublishing() && TimeSinceLastPublish > MinPublishInterval
            && (TimeSinceLastPublish > MaxPublishInterval
                || PositionThreshold < 1e-5f || RotationThreshold < 1e-5f || VelocityThreshold < 1e-5f
                || FVector::Distance(GetOwner()->GetActorLocation(), LastPosition) > PositionThreshold
                || GetOwner()->GetActorQuat().AngularDistance(LastRotation) * 180.0f / PI > RotationThreshold
                || (LastVelocityValid && FVector::Distance(LocalVelocity, LastVelocity) > VelocityThreshold )
            )) {
        // UE_LOG(LogRTI, Log, TEXT("Publish position %s %.1f %.1f %.1f"), *Entity->Id, Location.X, Location.Y, Location.Z);
        TimeSinceLastPublish = 0;
        inhumate::rti::proto::EntityPosition message;
        SetPositionMessageFromActor(message, GetOwner());
        message.set_id(TCHAR_TO_UTF8(*Entity->Id));
        Publish(message);
        LastPosition = GetOwner()->GetActorLocation();
        LastPositionTime = time;
        LastRotation = GetOwner()->GetActorQuat();
        LastRotationTime = time;
    } else if (IsReceiving() && Interpolate && DeltaTime > 0.1) {
        // Too large delta time (e.g. when editor loses focus) = don't interpolate
        if (LastPositionTime > 1e-5f) GetOwner()->SetActorLocation(LastPosition, false, nullptr, ETeleportType::TeleportPhysics);
        if (LastRotationTime > 1e-5f) GetOwner()->SetActorRotation(FRotator(LastRotation), ETeleportType::TeleportPhysics);
    } else if (IsReceiving() && Interpolate) {
        if (LastVelocityValid && LastAccelerationValid) {
            FVector NewVelocity = LastVelocity + LastAcceleration * DeltaTime;
            if (PreventReversal && FVector::DotProduct(NewVelocity, LastVelocity) <= 0) {
                LastVelocity = FVector::ZeroVector;
            } else {
                LastVelocity = NewVelocity;
            }
        }
        FVector targetPosition = GetOwner()->GetActorLocation();
        if (time - LastPositionTime < MaxInterpolateInterval) {
            if (LastPositionTime > 1e-5f && LastVelocityValid) {
                // Interpolate using velocity
                targetPosition = LastPosition + GetOwner()->GetActorQuat() * LastVelocity * (time - LastPositionTime);
            } else if (LastPositionTime > 1e-5f && PreviousPositionTime > 1e-5f && LastPositionTime - PreviousPositionTime > 1e-5f && LastPositionTime - PreviousPositionTime < MaxInterpolateInterval) {
                // or else lerp based on last and previous position
                targetPosition =
                FMath::Lerp(LastPosition,
                            LastPosition + (LastPosition - PreviousPosition),
                            (time - LastPositionTime) / (LastPositionTime - PreviousPositionTime));
            } else if (LastPositionTime > 1e-5f) {
                targetPosition = LastPosition;
            }

        }
        FVector setPosition = FMath::Lerp(GetOwner()->GetActorLocation(), targetPosition, DeltaTime * 10.f / PositionSmoothing);
        GetOwner()->SetActorLocation(setPosition, false, nullptr, ETeleportType::TeleportPhysics);

        FQuat targetRotation = GetOwner()->GetActorQuat();
        if (time - LastRotationTime < MaxInterpolateInterval) {
            if (LastRotationTime > 1e-5f && LastAngularVelocityValid) {
                // Interpolate using angular velocity
                targetRotation = (LastAngularVelocity * (time - LastRotationTime)).Quaternion() * LastRotationTime;
            } else if (LastRotationTime > 1e-5f && PreviousRotationTime > 1e-5f && LastRotationTime - PreviousRotationTime > 1e-5f && LastRotationTime - PreviousRotationTime < MaxInterpolateInterval) {
                // or else slerp based on last and previous rotation
                targetRotation =
                FQuat::Slerp(LastRotation,
                            (LastRotation * PreviousRotation.Inverse()) * LastRotation,
                            (time - LastRotationTime) / (LastRotationTime - PreviousRotationTime));
            } else if (LastRotationTime > 1e-5f) {
                targetRotation = LastRotation;
            }
        } else if (LastRotationTime > 1e-5f) {
            targetRotation = LastRotation;
        }
        FQuat setRotation = FQuat::Slerp(GetOwner()->GetActorQuat(), targetRotation, DeltaTime * 10.f / RotationSmoothing);
        GetOwner()->SetActorRotation(FRotator(setRotation), ETeleportType::TeleportPhysics);
    }
}

void URTIPositionComponent::SetPositionMessageFromActor(inhumate::rti::proto::EntityPosition &message, AActor *actor)
{
    AActor *LevelBP = (AActor *)actor->GetWorld()->GetLevelScriptActor();

    if (LevelBP && LevelBP->Implements<URTILocalCoordinateConversionInterface>()) {
        IRTILocalCoordinateConversionInterface *Conversion = Cast<IRTILocalCoordinateConversionInterface>(LevelBP);
        FVector RTILocation;
        Conversion->Execute_LocalLocationToRTI(LevelBP, actor->GetActorLocation(), RTILocation);
        auto local = new inhumate::rti::proto::EntityPosition_LocalPosition();
        local->set_x(RTILocation.X);
        local->set_y(RTILocation.Y);
        local->set_z(RTILocation.Z);
        message.set_allocated_local(local);
        FRotator RTIRotation;
        Conversion->Execute_LocalRotationToRTIEuler(LevelBP, actor->GetActorRotation(), RTIRotation);
        auto euler = new inhumate::rti::proto::EntityPosition_EulerRotation();
        euler->set_roll(RTIRotation.Roll);
        euler->set_pitch(RTIRotation.Pitch);
        euler->set_yaw(RTIRotation.Yaw);
        message.set_allocated_euler_rotation(euler);
    } else {
        message.set_allocated_local(UEToRTILocalPosition(actor->GetActorLocation()));
        message.set_allocated_local_rotation(UEToRTILocalRotation(actor->GetActorQuat()));
        message.set_allocated_euler_rotation(UEToRTIEulerRotation(actor->GetActorRotation()));
    }

    if (LevelBP && LevelBP->Implements<URTIGeodeticCoordinateConversionInterface>()) {
        IRTIGeodeticCoordinateConversionInterface *Conversion =
        Cast<IRTIGeodeticCoordinateConversionInterface>(LevelBP);
        FGeodeticLocation GeoLocation;
        Conversion->Execute_LocalToGeodetic(LevelBP, actor->GetActorLocation(), GeoLocation);
        auto geo = new inhumate::rti::proto::EntityPosition_GeodeticPosition();
        geo->set_longitude(GeoLocation.Longitude);
        geo->set_latitude(GeoLocation.Latitude);
        geo->set_altitude(GeoLocation.Altitude);
        message.set_allocated_geodetic(geo);
    }

    message.set_allocated_velocity(UEToRTIVelocity(actor->GetActorRotation().GetInverse().RotateVector(actor->GetVelocity())));
    auto Primitive = Cast<UPrimitiveComponent>(actor->GetRootComponent());
    if (Primitive) {
        FVector AngvelDegrees = Primitive->GetPhysicsAngularVelocityInDegrees();
        auto angvel = new inhumate::rti::proto::EntityPosition_EulerRotation();
        angvel->set_roll(-AngvelDegrees.X);
        angvel->set_pitch(-AngvelDegrees.Y);
        angvel->set_yaw(AngvelDegrees.Z);
        message.set_allocated_angular_velocity(angvel);
    }
}

FVector URTIPositionComponent::GetMessageLocation(const inhumate::rti::proto::EntityPosition &message, UWorld *world)
{
    AActor *LevelBP = world ? (AActor *)world->GetLevelScriptActor() : nullptr;
    if (message.has_local()) {
        if (LevelBP && LevelBP->Implements<URTILocalCoordinateConversionInterface>()) {
            IRTILocalCoordinateConversionInterface *Conversion = Cast<IRTILocalCoordinateConversionInterface>(LevelBP);
            FVector RTILocation;
            RTILocation.X = message.local().x();
            RTILocation.Y = message.local().y();
            RTILocation.Z = message.local().z();
            FVector LocalLocation;
            Conversion->Execute_RTIToLocalLocation(LevelBP, RTILocation, LocalLocation);
            return LocalLocation;
        }
        return RTILocalPositionToUE(message.local());
    } else if (message.has_geodetic()) {
        if (LevelBP && LevelBP->Implements<URTIGeodeticCoordinateConversionInterface>()) {
            IRTIGeodeticCoordinateConversionInterface *Conversion = Cast<IRTIGeodeticCoordinateConversionInterface>(LevelBP);
            FGeodeticLocation GeoLocation;
            GeoLocation.Longitude = message.geodetic().longitude();
            GeoLocation.Latitude = message.geodetic().latitude();
            GeoLocation.Altitude = message.geodetic().altitude();
            FVector LocalLocation;
            Conversion->Execute_GeodeticToLocal(LevelBP, GeoLocation, LocalLocation);
            return LocalLocation;
        }
    }
    return FVector();
}

FRotator URTIPositionComponent::GetMessageRotation(const inhumate::rti::proto::EntityPosition &message, UWorld *world)
{
    AActor *LevelBP = world ? (AActor *)world->GetLevelScriptActor() : nullptr;
    if (LevelBP && LevelBP->Implements<URTILocalCoordinateConversionInterface>() && message.has_euler_rotation()) {
        IRTILocalCoordinateConversionInterface *Conversion = Cast<IRTILocalCoordinateConversionInterface>(LevelBP);
        FRotator RTIRotation;
        RTIRotation.Roll = message.euler_rotation().roll();
        RTIRotation.Pitch = message.euler_rotation().pitch();
        RTIRotation.Yaw = message.euler_rotation().yaw();
        FRotator LocalRotation;
        Conversion->Execute_RTIEulerToLocalRotation(LevelBP, RTIRotation, LocalRotation);
        return LocalRotation;
    }
    if (message.has_local_rotation()) {
        FQuat rotation = RTILocalRotationToUE(message.local_rotation());
        return FRotator(rotation);
    } else if (message.has_euler_rotation()) {
        FRotator rotation = RTIEulerRotationToUE(message.euler_rotation());
        return rotation;
    }
    return FRotator();
}

// In the RTI, for local positions and rotations, any cartesian coordinate system can theoretically
// be used. However, Inhumate products assume a Unity style coordinate system:
// - x is right / east
// - y is up
// - z is forward / north
// - 1 unit is 1 meter
// Unreal Engine 4 uses the following coordinate system:
// - x is forward / north
// - y is right / east
// - z is up
// - 1 unit is 1 centimeter

// The following methods convert between these coordinate systems.

inhumate::rti::proto::EntityPosition_LocalPosition *URTIPositionComponent::UEToRTILocalPosition(const FVector &location)
{
    auto local = new inhumate::rti::proto::EntityPosition_LocalPosition();
    local->set_x(location.Y / 100);
    local->set_y(location.Z / 100);
    local->set_z(location.X / 100);
    return local;
}

FVector URTIPositionComponent::RTILocalPositionToUE(const inhumate::rti::proto::EntityPosition_LocalPosition &local)
{
    FVector location;
    location.X = local.z() * 100;
    location.Y = local.x() * 100;
    location.Z = local.y() * 100;
    return location;
}

inhumate::rti::proto::EntityPosition_LocalRotation *URTIPositionComponent::UEToRTILocalRotation(const FQuat &quat)
{
    auto rot = new inhumate::rti::proto::EntityPosition_LocalRotation();
    rot->set_x(quat.Y);
    rot->set_y(quat.Z);
    rot->set_z(quat.X);
    rot->set_w(quat.W);
    return rot;
}

FQuat URTIPositionComponent::RTILocalRotationToUE(const inhumate::rti::proto::EntityPosition_LocalRotation &local)
{
   return FQuat(local.z(), local.x(), local.y(), local.w());
}

inhumate::rti::proto::EntityPosition_EulerRotation *URTIPositionComponent::UEToRTIEulerRotation(const FRotator &Rotation)
{
    auto rotation = new inhumate::rti::proto::EntityPosition_EulerRotation();
    rotation->set_roll(Rotation.Roll);
    rotation->set_pitch(Rotation.Pitch);
    rotation->set_yaw(Rotation.Yaw);
    return rotation;
}

FRotator URTIPositionComponent::RTIEulerRotationToUE(const inhumate::rti::proto::EntityPosition_EulerRotation &rotation)
{
    FRotator Rotation;
    Rotation.Roll = rotation.roll();
    Rotation.Pitch = rotation.pitch();
    Rotation.Yaw = rotation.yaw();
    return Rotation;
}

inhumate::rti::proto::EntityPosition_VelocityVector *URTIPositionComponent::UEToRTIVelocity(const FVector &velocity)
{
    auto vel = new inhumate::rti::proto::EntityPosition_VelocityVector();
    vel->set_forward(velocity.X / 100);
    vel->set_right(velocity.Y / 100);
    vel->set_up(velocity.Z / 100);
    return vel;
}

FVector URTIPositionComponent::RTIVelocityToUE(const inhumate::rti::proto::EntityPosition_VelocityVector& velocity)
{
    FVector vel;
    vel.X = velocity.forward() * 100;
    vel.Y = velocity.right() * 100;
    vel.Z = velocity.up() * 100;
    return vel;
}
