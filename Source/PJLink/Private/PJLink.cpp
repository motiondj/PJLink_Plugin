// PJLink.cpp
#include "PJLink.h"
#include "Modules/ModuleManager.h"
#include "PJLinkLog.h"

#define LOCTEXT_NAMESPACE "FPJLinkModule"

void FPJLinkModule::StartupModule()
{
    // 모듈 시작시 로그 출력
    PJLINK_LOG_INFO(TEXT("PJLink Module Started"));
}

void FPJLinkModule::ShutdownModule()
{
    // 모듈 종료시 로그 출력
    PJLINK_LOG_INFO(TEXT("PJLink Module Shutdown"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FPJLinkModule, PJLink)