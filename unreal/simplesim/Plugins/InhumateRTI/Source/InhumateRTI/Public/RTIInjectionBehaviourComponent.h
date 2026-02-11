// Copyright 2025 Inhumate AB. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RTIComponent.h"
#include "InjectionStateEnum.h"

#include "RTIInjectionBehaviourComponent.generated.h"

// Base class for components that implement injection behavior

struct FInjection;
UCLASS(Abstract, Blueprintable, meta=(BlueprintSpawnableComponent))
class INHUMATERTI_API URTIInjectionBehaviourComponent : public URTIComponent
{
	GENERATED_BODY()

public:
    URTIInjectionBehaviourComponent(const FObjectInitializer& init);
    URTIInjectionBehaviourComponent();

    UFUNCTION(BlueprintPure, Category = "RTI")
    bool IsRunning() const { return Running; }

    // Called when injection is disabled.
    // Destroy/disable/de-active triggers/conditions etc here.
    UFUNCTION(BlueprintNativeEvent, Category = "RTI")
    bool Disable();

    // Called when injection is enabled.
    // Create/enable/active triggers/conditions etc here.
    UFUNCTION(BlueprintNativeEvent, Category = "RTI")
    bool Enable();

    // Called when injection is manually started.
    UFUNCTION(BlueprintNativeEvent, Category = "RTI")
    bool Start();

    // Called when injection is manually stopped.
    UFUNCTION(BlueprintNativeEvent, Category = "RTI")
    bool Stop();

    // Called when injection is canceled.
    UFUNCTION(BlueprintNativeEvent, Category = "RTI")
    bool Cancel();

    // Called when injection is scheduled.
    UFUNCTION(BlueprintNativeEvent, Category = "RTI")
    void Schedule();

    // Override to implement start of injection behaviour.
    // Called by Start() default implementation.
    // Should be called by triggers/conditions.
    // Should set Running to true.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RTI")
    bool Begin();

    // Called by Stop() and Cancel() default implementation.
    // Should be called by implemented behaviour when it ends.
    // Should set Running to false.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RTI")
    bool End();

    UFUNCTION(BlueprintPure, Category = "RTI")
    EInjectionState GetState() const;

    UFUNCTION(BlueprintPure, Category = "RTI")
    FString GetTitle() const;

    // Update and publish title
    UFUNCTION(BlueprintCallable, Category = "RTI")
    void SetTitle(const FString& Title);

    // Get parameter value, or (if not specified in injection) default value
    UFUNCTION(BlueprintPure, Category = "RTI")
    FString GetParameterValue(const FString& ParameterName);

    void Publish();
    void Inject(FInjection& Injection, class URTIInjectableComponent* injectable);
    FInjection* GetInjection() const;

protected:
    UPROPERTY(BlueprintReadWrite, Category = "RTI")
    bool Running;

    UPROPERTY(BlueprintReadOnly, Category = "RTI")
    FString InjectionId;

    UPROPERTY(BlueprintReadOnly, Category = "RTI")
    class URTIInjectableComponent* Injectable;

};