// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

using System;
using System.Reflection;

public class NexradSpatialEditorTarget : TargetRules
{
	/*public static void SetProperty(object instance, string propertyName, object newValue)
	{
		Type type = instance.GetType();
		PropertyInfo prop = type.BaseType.GetProperty(propertyName);
		prop.SetValue(instance, newValue, null);
	}
	public static object GetProperty(object instance, string propertyName)
	{
		Type type = instance.GetType();
		PropertyInfo prop = type.BaseType.GetProperty(propertyName);
		return prop.GetValue(instance, null);
	}*/
	public NexradSpatialEditorTarget( TargetInfo Target) : base(Target)
	{
		Name = "NexradSpatialEditor";
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		ExtraModuleNames.AddRange( new string[] { "NexradSpatial" } );
		
		bUseUnityBuild = false;
		//bUsePCHFiles = false;
		
		
		//SourceFileWorkingSet.Provider = "None";
		//SourceFileWorkingSet.RepositoryPath = "";
		//SourceFileWorkingSet.GitPath = "";
		
		//object SourceFileWorkingSetAccessible = GetProperty((TargetRules)this,"SourceFileWorkingSet");
		//SetProperty(SourceFileWorkingSetAccessible,"Provider","None");
		//SetProperty(SourceFileWorkingSetAccessible,"RepositoryPath","");
		//SetProperty(SourceFileWorkingSetAccessible,"GitPath","");
	}
}
