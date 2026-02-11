// Copyright 2025 Inhumate AB. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "RTIComponent.h"
#include "RTIParameter.h"
#include "InjectionStateEnum.h"

#include "RTIInjectableComponent.generated.h"

UENUM(BlueprintType)
enum class EInjectableControlMode : uint8 {
    IMMEDIATE UMETA(DisplayName = "Immediate"),
    AUTO UMETA(DisplayName = "Auto"),
    MANUAL UMETA(DisplayName = "Manual"),
    AUTO_OR_MANUAL UMETA(DisplayName = "Auto or manual"),
};

UENUM(BlueprintType)
enum class EAutoInjection : uint8 {
    ON_DEMAND UMETA(DisplayName = "On demand"),
    START_DISABLED UMETA(DisplayName = "Start disabled"),
    START_ENABLED UMETA(DisplayName = "Start enabled"),
};

USTRUCT(BlueprintType)
struct FInjection
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Rti")
    FString Id;

    UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Rti")
    FString Injectable;
    
    UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Rti")
    EInjectionState State;

    UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Rti")
    FString Title;

    UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Rti")
    TMap<FString, FString> ParameterValues;

    UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Rti")
    float EnableTime;

    UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Rti")
    float StartTime;

    UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Rti")
    float EndTime;
};

UCLASS(Blueprintable, meta=(BlueprintSpawnableComponent), DisplayName="RTI Injectable")
class INHUMATERTI_API URTIInjectableComponent : public URTIComponent
{
	GENERATED_BODY()

public:	
	URTIInjectableComponent();

    // Unique name used to identify this injectable
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    FString Name;

    // How injections start - use auto if a trigger / condition calls Begin()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    EInjectableControlMode StartMode;

    // How injections end - use auto if a trigger / condition / fixed duration calls End()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    EInjectableControlMode EndMode;

    // Actor class to spawn when this is injected.
    // Use an actor class if you want concurrent injections (multiple running at the same time).
    // For non-concurrent injections, add component(s) deriving from RTIInjectionBase to the actor that contains this RTIInjectable.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    TSubclassOf<AActor> InjectActor;

    // Whether to be started on demand by user, or automatically at scenario start (and if so, disabled/enabled)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI")
    EAutoInjection AutoInjection;

    // User-friendly description (multiline)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI", meta=(MultiLine=true))
    FString Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTI", meta = (TitleProperty = "Name"))
    TArray<FRTIParameter> Parameters;

    // If TRUE, multiple injections can be active at the same time
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RTI")
    bool IsConcurrent();

    UFUNCTION(BlueprintPure, Category = "RTI")
    FString GetParameterDefaultValue(const FString& ParameterName);

    // Can be implemented to override the default spawning behavior when an injection is created.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RTI")
    AActor *SpawnActorForInjection(const FInjection &Injection);

    void Inject(const inhumate::rti::proto::InjectionOperation_Inject* inject);
    bool EnableInjection(FInjection& Injection);
    void DisableInjection(FInjection& Injection);
    void StartInjection(FInjection& Injection);
    void EndInjection(FInjection& Injection);
    void StopInjection(FInjection& Injection);
    void CancelInjection(FInjection& Injection);
    void ScheduleInjection(const float EnableTime, FInjection &Injection);
    void UpdateTitle(const FString& Title, FInjection &Injection);

    void Publish();
    void PublishClearInjections();
    void PublishInjections();
    void PublishInjection(const FInjection& Injection);

    FInjection* GetInjection(const FString& id);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    void AutoInject();

    void UpdateState(FInjection& Injection, EInjectionState state);
    void GetBehaviours(const FInjection& Injection, TArray<class URTIInjectionBehaviourComponent*>& outBehaviours);

    TArray<FInjection> Injections;
    TArray<class URTIInjectionBehaviourComponent*> Behaviours;
    TMap<FString, TArray<class URTIInjectionBehaviourComponent*>> InjectionBehaviours;

    float LastRtiTime;

};

static inhumate::rti::proto::Injectable_ControlMode PbControlMode(EInjectableControlMode mode) {
    switch (mode) {
        case EInjectableControlMode::IMMEDIATE: return inhumate::rti::proto::Injectable_ControlMode_IMMEDIATE;
        case EInjectableControlMode::AUTO: return inhumate::rti::proto::Injectable_ControlMode_AUTO;
        case EInjectableControlMode::MANUAL: return inhumate::rti::proto::Injectable_ControlMode_MANUAL;
        case EInjectableControlMode::AUTO_OR_MANUAL: return inhumate::rti::proto::Injectable_ControlMode_AUTO_OR_MANUAL;
    }
    return inhumate::rti::proto::Injectable_ControlMode_IMMEDIATE;
}