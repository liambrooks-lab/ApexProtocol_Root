using UnrealBuildTool;
using System.Collections.Generic;

public class ApexProtocolTarget : TargetRules
{
    public ApexProtocolTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        ExtraModuleNames.AddRange( new string[] { "ApexProtocol" } );
    }
}