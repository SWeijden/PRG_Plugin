// Copyright Epic Games, Inc. All Rights Reserved.

#include "PRG_PluginEditorModeToolkit.h"
#include "PRG_PluginEditorMode.h"
#include "Engine/Selection.h"

#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "EditorModeManager.h"

#define LOCTEXT_NAMESPACE "PRG_PluginEditorModeToolkit"

FPRG_PluginEditorModeToolkit::FPRG_PluginEditorModeToolkit()
{
}

void FPRG_PluginEditorModeToolkit::Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode)
{
	FModeToolkit::Init(InitToolkitHost, InOwningMode);
}

/*void FPRG_PluginEditorModeToolkit::GetToolPaletteNames(TArray<FName>& PaletteNames) const
{
	PaletteNames.Add(NAME_Default);
}*/

//void FPRG_PluginEditorModeToolkit::OnToolStarted(UInteractiveToolManager* Manager, UInteractiveTool* Tool)
//{
//	//UpdateActiveToolProperties();
//
//	//UInteractiveTool* CurTool = GetScriptableEditorMode()->GetToolManager(EToolsContextScope::EdMode)->GetActiveTool(EToolSide::Left);
//	//CurTool->OnPropertySetsModified.AddSP(this, &FModelingToolsEditorModeToolkit::UpdateActiveToolProperties);
//	//CurTool->OnPropertyModifiedDirectlyByTool.AddSP(this, &FModelingToolsEditorModeToolkit::InvalidateCachedDetailPanelState);
//
//	//ModeHeaderArea->SetVisibility(EVisibility::Collapsed);
//	//ActiveToolName = CurTool->GetToolInfo().ToolDisplayName;
//
//	//// try to update icon
//	//FString ActiveToolIdentifier = GetScriptableEditorMode()->GetToolManager(EToolsContextScope::EdMode)->GetActiveToolName(EToolSide::Left);
//	//ActiveToolIdentifier.InsertAt(0, ".");
//	//FName ActiveToolIconName = ISlateStyle::Join(FModelingToolsManagerCommands::Get().GetContextName(), TCHAR_TO_ANSI(*ActiveToolIdentifier));
//	//ActiveToolIcon = FModelingToolsEditorModeStyle::Get()->GetOptionalBrush(ActiveToolIconName);
//
//
//	//GetToolkitHost()->AddViewportOverlayWidget(ViewportOverlayWidget.ToSharedRef());
//
//	//// disable LOD level picker once Tool is active
//	//AssetLODMode->SetEnabled(false);
//	//AssetLODModeLabel->SetEnabled(false);
//}
//
//void FPRG_PluginEditorModeToolkit::OnToolEnded(UInteractiveToolManager* Manager, UInteractiveTool* Tool)
//{
//	//if (IsHosted())
//	//{
//	//	GetToolkitHost()->RemoveViewportOverlayWidget(ViewportOverlayWidget.ToSharedRef());
//	//}
//
//	//ModeDetailsView->SetObject(nullptr);
//	//ActiveToolName = FText::GetEmpty();
//	//ModeHeaderArea->SetVisibility(EVisibility::Visible);
//	//ModeHeaderArea->SetText(LOCTEXT("SelectToolLabel", "Select a Tool from the Toolbar"));
//	//ClearNotification();
//	//ClearWarning();
//	//UInteractiveTool* CurTool = GetScriptableEditorMode()->GetToolManager(EToolsContextScope::EdMode)->GetActiveTool(EToolSide::Left);
//	//if (CurTool)
//	//{
//	//	CurTool->OnPropertySetsModified.RemoveAll(this);
//	//	CurTool->OnPropertyModifiedDirectlyByTool.RemoveAll(this);
//	//}
//
//	//// re-enable LOD level picker
//	//AssetLODMode->SetEnabled(true);
//	//AssetLODModeLabel->SetEnabled(true);
//}



FName FPRG_PluginEditorModeToolkit::GetToolkitFName() const
{
	return FName("PRG_PluginEditorMode");
}

FText FPRG_PluginEditorModeToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("DisplayName", "PRG_PluginEditorMode Toolkit");
}

#undef LOCTEXT_NAMESPACE
