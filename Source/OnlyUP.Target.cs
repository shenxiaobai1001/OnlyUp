using UnrealBuildTool;

public class OnlyUPTarget : TargetRules
{
	public OnlyUPTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("OnlyUP");
	}
}
