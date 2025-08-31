// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Modules/ModuleManager.h"


class FGameSerializerModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

DECLARE_LOG_CATEGORY_EXTERN(LogSaveGame, Log, All);