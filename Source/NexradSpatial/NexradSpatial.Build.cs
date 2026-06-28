// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System;
using System.Collections.Generic;
using System.IO;

public class NexradSpatial : ModuleRules
{
	public NexradSpatial(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "RenderCore", "Engine", "Slate", "SlateCore", "UMG", "HeadMountedDisplay" });

		PrivateDependencyModuleNames.AddRange(new string[] { "InputCore", "ImGui", "AudioCaptureCore", "ProceduralMeshComponent", "ImageWrapper", "RHI", "Json", "HTTP", "XRBase", "EnhancedInput", "AudioMixer", "MetasoundEngine", "MetasoundFrontend" });
		
		bEnableExceptions = true;
		bUseRTTI = true;
		bWarningsAsErrors = false;
		
		// string pluginsLocation = Path.Combine(ModuleDirectory, "../../Plugins");
		// // add UnrealHDF5 only if it exists
		// if(Directory.Exists(Path.Combine(pluginsLocation, "UnrealHDF5"))){
		// 	PrivateDependencyModuleNames.Add("UnrealHDF5");
		// }
		
		// define if exists
		if(Directory.Exists(Path.Combine(ModuleDirectory, "Radar/Deps/hdf5"))){
			PublicDefinitions.Add("HDF5=1");
		}
		
		PublicDefinitions.Add("_CRT_SECURE_NO_WARNINGS");
		
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.Add("UnrealEd");
		}
		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
