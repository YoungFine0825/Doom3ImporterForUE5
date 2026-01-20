// Copyright Epic Games, Inc. All Rights Reserved.

#include "Doom3ImporterEditor.h"

void FDoom3ImporterEditor::StartupModule()
{
	UE_LOG(LogTemp,Log,TEXT("Doom3 importer plugin started!"));
}

void FDoom3ImporterEditor::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}
	
IMPLEMENT_MODULE(FDoom3ImporterEditor, Doom3ImporterEditor)