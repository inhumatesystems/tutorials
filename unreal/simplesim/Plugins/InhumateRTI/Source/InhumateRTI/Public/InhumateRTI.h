// Copyright 2025 Inhumate AB. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogRTI, Log, All);

class FInhumateRTIModule : public IModuleInterface
{
public:

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void*	LibraryHandle;
    void*	LibraryHandle2;
    void*	LibraryHandle3;
};
