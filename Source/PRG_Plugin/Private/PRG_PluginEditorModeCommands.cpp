// Copyright Epic Games, Inc. All Rights Reserved.

#include "PRG_PluginEditorModeCommands.h"
#include "PRG_PluginEditorMode.h"
#include "EditorStyleSet.h"

#define LOCTEXT_NAMESPACE "PRG_PluginEditorModeCommands"

FPRG_PluginEditorModeCommands::FPRG_PluginEditorModeCommands()
	: TCommands<FPRG_PluginEditorModeCommands>("PRG_PluginEditorMode",
		NSLOCTEXT("PRG_PluginEditorMode", "PRG_PluginEditorModeCommands", "PRG_Plugin Editor Mode"),
		NAME_None,
		FEditorStyle::GetStyleSetName())
{
}

void FPRG_PluginEditorModeCommands::RegisterCommands()
{
	TArray <TSharedPtr<FUICommandInfo>>& ToolCommands = Commands.FindOrAdd(NAME_Default);

	UI_COMMAND(RoomTool, "Room Tool", "Manage Rooms", EUserInterfaceActionType::None, FInputChord());
	ToolCommands.Add(RoomTool);
}

TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> FPRG_PluginEditorModeCommands::GetCommands()
{
	return FPRG_PluginEditorModeCommands::Get().Commands;
}

#undef LOCTEXT_NAMESPACE
