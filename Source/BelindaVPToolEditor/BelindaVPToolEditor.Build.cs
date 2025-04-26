// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using UnrealBuildTool.Rules;

public class BelindaVPToolEditor : ModuleRules
{
    public BelindaVPToolEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
            new string[] {
                "BelindaVPToolEditor/Public"
            }
            );


        PrivateIncludePaths.AddRange(
            new string[] {
				// ... add other private include paths required here ...
                "BelindaVPToolEditor/Private"
            }
            );


        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "BelindaVPTool",
                "InputCore",
                "Projects",
                "TimeManagement",
                "LiveLink",
                "LevelEditor",
                "MediaIOCore",
                "MediaFrameworkUtilities",
                "MediaFrameworkUtilitiesEditor",
                "RemoteControl"
				// ... add other public dependencies that you statically link with here ...
			}
            );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "UMG",
                "Slate",
                "SlateCore",
                "ToolMenus",
                "Blutility",
                "EditorScriptingUtilities",
                "UnrealEd",
                "UMGEditor",
                "BelindaVPTool",
                "LevelEditor",
                "MediaIOCore",
                "MediaFrameworkUtilities",
                "MediaFrameworkUtilitiesEditor",
                "RemoteControl"
				// ... add private dependencies that you statically link with here ...	
			}
            );

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.Add("EditorScriptingUtilities");
            PrivateDependencyModuleNames.Add("EditorStyle");
            PrivateDependencyModuleNames.Add("UnrealEd");
            PrivateDependencyModuleNames.Add("BelindaVPTool");
            PrivateDependencyModuleNames.Add("MediaFrameworkUtilities");
            PrivateDependencyModuleNames.Add("MediaFrameworkUtilitiesEditor");
            PrivateDependencyModuleNames.Add("RemoteControl");

        }

        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
				// ... add any modules that your module loads dynamically here ...
			}
            );
    }
}
