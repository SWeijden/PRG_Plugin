// Copyright Epic Games, Inc. All Rights Reserved.

#include "PRG_PluginModule.h"
#include "PRG_PluginEditorModeCommands.h"

#define LOCTEXT_NAMESPACE "PRG_PluginModule"

void FPRG_PluginModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	FPRG_PluginEditorModeCommands::Register();
}

void FPRG_PluginModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	FPRG_PluginEditorModeCommands::Unregister();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FPRG_PluginModule, PRG_PluginEditorMode)