// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class NexradSpatialTarget : TargetRules
{
	public NexradSpatialTarget( TargetInfo Target) : base(Target)
	{
		Name = "NexradSpatial";
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		ExtraModuleNames.AddRange( new string[] { "NexradSpatial" } );
		
		bUseUnityBuild = false;
		bUsePCHFiles = false;
			
		// bOverrideBuildEnvironment = true;
		// bUseLoggingInShipping = true;
		
		//SourceFileWorkingSet.Provider = "None";
		//SourceFileWorkingSet.RepositoryPath = "";
		//SourceFileWorkingSet.GitPath = "";
	}
}
