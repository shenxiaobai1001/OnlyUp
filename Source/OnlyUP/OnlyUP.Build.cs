using UnrealBuildTool;

public class OnlyUP : ModuleRules
{
	public OnlyUP(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(new[]
		{
			ModuleDirectory,
			System.IO.Path.Combine(ModuleDirectory, "Gift"),
			System.IO.Path.Combine(ModuleDirectory, "Video"),
			System.IO.Path.Combine(ModuleDirectory, "Dance")
		});

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"UMG",
			"Slate",
			"SlateCore",
			"Niagara",
			"GameplayTags",
			"ALS",
			"MediaAssets",
			"MediaUtils",
			"AudioMixer"
		});
	}
}
