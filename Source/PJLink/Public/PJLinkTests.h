// PJLinkTests.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "PJLinkTypes.h"
#include "PJLinkTests.generated.h"

/**
 * PJLink 플러그인 테스트를 위한 유틸리티 클래스
 */
UCLASS(BlueprintType)
class PJLINK_API UPJLinkTests : public UObject
{
    GENERATED_BODY()

public:
    /**
     * 연결 테스트 실행
     * 지정된 IP 주소와 포트로 프로젝터에 연결을 시도합니다.
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Tests")
    static bool TestConnection(const FString& IPAddress, int32 Port = 4352);

    /**
     * 명령 실행 테스트
     * 지정된 프로젝터에 여러 명령을 연속해서 보내고 결과를 확인합니다.
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Tests")
    static bool TestCommands(const FPJLinkProjectorInfo& ProjectorInfo);

    /**
     * 이벤트 시스템 테스트
     * 이벤트가 적절하게 발생하는지 테스트합니다.
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Tests")
    static bool TestEvents(const FPJLinkProjectorInfo& ProjectorInfo);

    /**
     * 오류 복구 테스트
     * 네트워크 오류 등의 상황에서 복구가 정상적으로 이루어지는지 테스트합니다.
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Tests")
    static bool TestErrorRecovery(const FPJLinkProjectorInfo& ProjectorInfo);

    /**
     * 프리셋 시스템 테스트
     * 프리셋의 저장 및 로드 기능을 테스트합니다.
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Tests")
    static bool TestPresets();

    /**
     * 상태 머신 테스트
     * 상태 머신의 상태 전환이 올바르게 동작하는지 테스트합니다.
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Tests")
    static bool TestStateMachine();

    /**
     * 전체 기능 테스트
     * 모든 테스트를 순차적으로 실행합니다.
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Tests")
    static bool RunAllTests(const FPJLinkProjectorInfo& ProjectorInfo);

    UFUNCTION(BlueprintCallable, Category = "PJLink|Tests")
    static bool TestTimeoutHandling(const FPJLinkProjectorInfo& ProjectorInfo);

    UFUNCTION(BlueprintCallable, Category = "PJLink|Tests")
    static bool TestDiagnosticSystem(const FPJLinkProjectorInfo& ProjectorInfo);

    /**
     * 타임아웃 처리 테스트
     * 명령 타임아웃이 올바르게 감지되고 처리되는지 테스트합니다.
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Tests")
    static bool TestTimeoutHandling(const FPJLinkProjectorInfo& ProjectorInfo);

    /**
     * 진단 시스템 테스트
     * 진단 데이터 수집 및 보고서 생성 기능을 테스트합니다.
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Tests")
    static bool TestDiagnosticSystem(const FPJLinkProjectorInfo& ProjectorInfo);
};
