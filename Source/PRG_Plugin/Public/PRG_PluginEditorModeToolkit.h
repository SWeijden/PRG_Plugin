// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Toolkits/BaseToolkit.h"
#include "PRG_PluginEditorMode.h"

/**
 * This FModeToolkit just creates a basic UI panel that allows various InteractiveTools to
 * be initialized, and a DetailsView used to show properties of the active Tool.
 */
class FPRG_PluginEditorModeToolkit : public FModeToolkit
{
public:
	FPRG_PluginEditorModeToolkit();

	/** FModeToolkit interface */
	virtual void Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode) override;
	// NOTE: Removed to prevent tool selection buttons from being created (or their empty container)
	//virtual void GetToolPaletteNames(TArray<FName>& PaletteNames) const override;

	/** IToolkit interface */
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;

	// TODO: Check to handle auto-open of tool via here?
	//virtual void OnToolStarted(UInteractiveToolManager* Manager, UInteractiveTool* Tool) override;
	//virtual void OnToolEnded(UInteractiveToolManager* Manager, UInteractiveTool* Tool) override;
};
