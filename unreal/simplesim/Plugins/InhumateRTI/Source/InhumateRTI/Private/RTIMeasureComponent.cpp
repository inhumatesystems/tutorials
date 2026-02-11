// Copyright 2025 Inhumate AB. All Rights Reserved.

#include "RTIMeasureComponent.h"

void URTIMeasureComponent::Measure(float Value) {
    auto rti = RTI();
    if (!rti || !rti->connected()) return;
    if (!MyMeasure) {
        MyMeasure = TUniquePtr<inhumate::rti::proto::Measure>(new inhumate::rti::proto::Measure());
        MyMeasure->set_id(TCHAR_TO_UTF8(*Id));
        MyMeasure->set_title(TCHAR_TO_UTF8(*Title));
        MyMeasure->set_unit(TCHAR_TO_UTF8(*Unit));
        MyMeasure->set_channel(TCHAR_TO_UTF8(*Channel));
        MyMeasure->set_interval(Interval);
    }
    rti->Measure(*MyMeasure.Get(), Value);
}