// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PJLink : ModuleRules
{
    public PJLink(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
            new string[] {
                // ... 추가 경로
            }
        );

        PrivateIncludePaths.AddRange(
            new string[] {
                // ... 추가 경로
            }
        );

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "InputCore",
                "Projects",
                "Sockets", // 소켓 통신을 위해 필요함
                "Networking", // 네트워킹 기능을 위해 필요함
                "Json",
                "JsonUtilities",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Slate",
                "SlateCore",
                // 필요한 추가 모듈
            }
        );
    }
}