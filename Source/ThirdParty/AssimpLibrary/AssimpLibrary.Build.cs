// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class AssimpLibrary : ModuleRules
{
	public AssimpLibrary(ReadOnlyTargetRules target) : base(target)
	{
		Type = ModuleType.External; // 声明为外部第三方模块
		//添加包含路径
		string assimpIncludePath = Path.Combine(ModuleDirectory, "include");
		PublicSystemIncludePaths.Add(assimpIncludePath);

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			//添加链接库路径
			string assimpLibraryPath = Path.Combine(ModuleDirectory, "lib","assimp-vc143-mt.lib");
			PublicAdditionalLibraries.Add(assimpLibraryPath);
			//添加动态链接库路径
			// string assimpDllPath = Path.Combine(ModuleDirectory,"bin","assimp-vc143-mt.dll");
			// RuntimeDependencies.Add(assimpDllPath);
		}
	}
}
