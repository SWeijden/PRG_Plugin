// Copyright Epic Games, Inc. All Rights Reserved.

#include "PRG_PluginEditorMode.h"
#include "PRG_PluginEditorModeToolkit.h"
#include "EdModeInteractiveToolsContext.h"
#include "InteractiveToolManager.h"
#include "PRG_PluginEditorModeCommands.h"


//////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////// 
// AddYourTool Step 1 - include the header file for your Tools here
//////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////// 
#include "Tools/PRG_PluginRoomTool.h"
#include "BaseGizmos/TransformGizmoUtil.h"

// step 2: register a ToolBuilder in FPRG_PluginEditorMode::Enter() below

#define LOCTEXT_NAMESPACE "PRG_PluginEditorMode"

const FEditorModeID UPRG_PluginEditorMode::EM_PRG_PluginEditorModeId = TEXT("EM_PRG_PluginEditorMode");

FString UPRG_PluginEditorMode::RoomToolName = TEXT("PRG_Plugin_RoomTool");

UPRG_PluginEditorMode::UPRG_PluginEditorMode()
{
	FModuleManager::Get().LoadModule("EditorStyle");

	// appearance and icon in the editing mode ribbon can be customized here
	Info = FEditorModeInfo(UPRG_PluginEditorMode::EM_PRG_PluginEditorModeId,
		LOCTEXT("ModeName", "Room Generator"),
		FSlateIcon(),
		true);
}


UPRG_PluginEditorMode::~UPRG_PluginEditorMode()
{
}


void UPRG_PluginEditorMode::ActorSelectionChangeNotify()
{
}

void UPRG_PluginEditorMode::Enter()
{
	UEdMode::Enter();

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	// AddYourTool Step 2 - register the ToolBuilders for your Tools here.
	// The string name you pass to the ToolManager is used to select/activate your ToolBuilder later.
	//////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////// 
	const FPRG_PluginEditorModeCommands& SampleToolCommands = FPRG_PluginEditorModeCommands::Get();

	RegisterTool(SampleToolCommands.RoomTool, RoomToolName, NewObject<UPRG_PluginRoomToolBuilder>(this)/*, EToolsContextScope::EdMode*/);

	// register gizmo helper
	UE::TransformGizmoUtil::RegisterTransformGizmoContextObject(GetInteractiveToolsContext());

	// Set and use active tool type
	if (GetToolManager()->SelectActiveToolType(EToolSide::Left, RoomToolName))
		GetToolManager()->ActivateTool(EToolSide::Left);
}

void UPRG_PluginEditorMode::CreateToolkit()
{
	Toolkit = MakeShareable(new FPRG_PluginEditorModeToolkit);
}

TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> UPRG_PluginEditorMode::GetModeCommands() const
{
	return FPRG_PluginEditorModeCommands::Get().GetCommands();
}

#undef LOCTEXT_NAMESPACE
