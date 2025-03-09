// PJLink.h
#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

// 전방 선언
class UPJLinkSubsystem;
class UPJLinkBlueprintLibrary;
class UPJLinkProjectorInstance;
class UPJLinkComponent;
class UPJLinkPresetManager;
class UPJLinkStateMachine;
class UPJLinkNetworkManager;

// 모듈 클래스 선언
class PJLINK_API FPJLinkModule : public IModuleInterface
{
public:
    /** IModuleInterface 구현 */
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};

