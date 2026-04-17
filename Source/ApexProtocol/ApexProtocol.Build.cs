using UnrealBuildTool;

public class ApexProtocol : ModuleRules
{
	public ApexProtocol(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"AIModule",
				"Core",
				"CoreUObject",
				"Engine",
				"GameplayTasks",
				"InputCore",
				"EnhancedInput",
				"NavigationSystem",
				"WebSockets",
				"UMG"
			});
	}
}
