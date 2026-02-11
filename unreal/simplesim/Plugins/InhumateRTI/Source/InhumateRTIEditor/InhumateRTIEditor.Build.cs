// Copyright 2025 Inhumate AB. All Rights Reserved.

using UnrealBuildTool;

public class InhumateRTIEditor : ModuleRules
{
	public InhumateRTIEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Slate",
				"SlateCore",
				"Engine",
				"UnrealEd",
				"PropertyEditor",
				"BlueprintGraph",
                "Blutility",
                "UMG",
				"InhumateRTI",
				// ... add other public dependencies that you statically link with here ...
			}
		);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"EditorStyle",
				"AssetRegistry"
			}
		);
	}
}
