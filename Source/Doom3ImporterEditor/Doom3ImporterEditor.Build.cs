// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class Doom3ImporterEditor : ModuleRules
{
	public Doom3ImporterEditor(ReadOnlyTargetRules target) : base(target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"UnrealEd",
				"AssetTools",
				"EditorStyle",
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"AssimpLibrary",
				"SkeletalMeshDescription",
				"EditorFramework",
				"MainFrame", 
				"SkeletalMeshUtilitiesCommon",
			}
			);
	}
}
