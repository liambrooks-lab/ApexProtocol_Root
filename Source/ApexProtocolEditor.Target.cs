using UnrealBuildTool;
using System.Collections.Generic;

public class ApexProtocolEditorTarget : TargetRules
{
	public ApexProtocolEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		ExtraModuleNames.Add("ApexProtocol");
	}
}
