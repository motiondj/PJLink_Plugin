// PJLinkTests.cpp
#include "PJLinkTests.h"
#include "PJLinkNetworkManager.h"
#include "PJLinkSubsystem.h"
#include "PJLinkPresetManager.h"
#include "PJLinkStateMachine.h"
#include "UPJLinkComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "TimerManager.h"
#include "PJLinkLog.h"

bool UPJLinkTests::TestConnection(const FString& IPAddress, int32 Port)
{
    PJLINK_LOG_INFO(TEXT("Starting connection test to %s:%d"), *IPAddress, Port);

    // 네트워크 매니저 생성
    UPJLinkNetworkManager* NetworkManager = NewObject<UPJLinkNetworkManager>();
    if (!NetworkManager)
    {
        PJLINK_LOG_ERROR(TEXT("Failed to create NetworkManager for test"));
        return false;
    }

    // 프로젝터 정보 설정
    FPJLinkProjectorInfo ProjectorInfo;
    ProjectorInfo.Name = TEXT("Test Projector");
    ProjectorInfo.IPAddress = IPAddress;
    ProjectorInfo.Port = Port;

    // 연결 시도
    bool bSuccess = NetworkManager->ConnectToProjector(ProjectorInfo);

    if (bSuccess)
    {
        PJLINK_LOG_INFO(TEXT("Connection test successful to %s:%d"), *IPAddress, Port);

        // 상태 요청
        bSuccess = NetworkManager->RequestStatus();

        if (bSuccess)
        {
            PJLINK_LOG_INFO(TEXT("Status request successful"));
        }
        else
        {
            PJLINK_LOG_ERROR(TEXT("Status request failed"));
        }

        // 연결 해제
        NetworkManager->DisconnectFromProjector();
    }
    else
    {
        PJLINK_LOG_ERROR(TEXT("Connection test failed to %s:%d"), *IPAddress, Port);
    }

    return bSuccess;
}

bool UPJLinkTests::TestCommands(const FPJLinkProjectorInfo& ProjectorInfo)
{
    PJLINK_LOG_INFO(TEXT("Starting commands test for projector %s at %s:%d"),
        *ProjectorInfo.Name, *ProjectorInfo.IPAddress, ProjectorInfo.Port);

    // 네트워크 매니저 생성
    UPJLinkNetworkManager* NetworkManager = NewObject<UPJLinkNetworkManager>();
    if (!NetworkManager)
    {
        PJLINK_LOG_ERROR(TEXT("Failed to create NetworkManager for test"));
        return false;
    }

    // 연결 시도
    bool bConnected = NetworkManager->ConnectToProjector(ProjectorInfo);
    if (!bConnected)
    {
        PJLINK_LOG_ERROR(TEXT("Failed to connect to projector for command test"));
        return false;
    }

    // 커맨드 테스트
    bool bSuccess = true;

    // 상태 요청
    bSuccess &= NetworkManager->RequestStatus();
    PJLINK_LOG_INFO(TEXT("Status request result: %s"), bSuccess ? TEXT("Success") : TEXT("Failed"));

    // 입력 소스 변경
    bSuccess &= NetworkManager->SwitchInputSource(EPJLinkInputSource::RGB);
    PJLINK_LOG_INFO(TEXT("Input source change result: %s"), bSuccess ? TEXT("Success") : TEXT("Failed"));

    // 전원 명령은 실제 환경에서 주의해서 테스트
    // 자동 테스트에서는 실제로 전원을 끄거나 켜지 않음

    // 연결 해제
    NetworkManager->DisconnectFromProjector();

    return bSuccess;
}

bool UPJLinkTests::TestEvents(const FPJLinkProjectorInfo& ProjectorInfo)
{
    PJLINK_LOG_INFO(TEXT("Starting events test for projector %s"), *ProjectorInfo.Name);

    // 이 테스트는 블루프린트에서 별도로 구현하는 것이 좋음
    // C++에서는 이벤트 핸들러를 동적으로 설정하고 테스트하기 어려움

    PJLINK_LOG_INFO(TEXT("Events test skipped in C++ - implement in Blueprint instead"));
    return true;
}

bool UPJLinkTests::TestErrorRecovery(const FPJLinkProjectorInfo& ProjectorInfo)
{
    PJLINK_LOG_INFO(TEXT("Starting error recovery test for projector %s"), *ProjectorInfo.Name);

    // 네트워크 매니저 생성
    UPJLinkNetworkManager* NetworkManager = NewObject<UPJLinkNetworkManager>();
    if (!NetworkManager)
    {
        PJLINK_LOG_ERROR(TEXT("Failed to create NetworkManager for test"));
        return false;
    }

    // 잘못된 IP로 연결 시도
    FPJLinkProjectorInfo BadInfo = ProjectorInfo;
    BadInfo.IPAddress = TEXT("0.0.0.0"); // 연결할 수 없는 IP

    bool bFailedAsExpected = !NetworkManager->ConnectToProjector(BadInfo);

    if (bFailedAsExpected)
    {
        PJLINK_LOG_INFO(TEXT("Failed to connect to invalid IP as expected"));
    }
    else
    {
        PJLINK_LOG_ERROR(TEXT("Unexpectedly connected to invalid IP"));
        return false;
    }

    // 올바른 IP로 다시 연결
    bool bReconnected = NetworkManager->ConnectToProjector(ProjectorInfo);

    if (bReconnected)
    {
        PJLINK_LOG_INFO(TEXT("Successfully reconnected after failure"));
        NetworkManager->DisconnectFromProjector();
        return true;
    }
    else
    {
        PJLINK_LOG_ERROR(TEXT("Failed to reconnect after failure"));
        return false;
    }
}

bool UPJLinkTests::TestPresets()
{
    PJLINK_LOG_INFO(TEXT("Starting presets test"));

    // 게임 인스턴스 필요 - 이 테스트는 실행 중인 게임에서만 가능
    UWorld* World = GEngine->GetWorldFromContextObject(GEngine->GetCurrentPlayWorld(), EGetWorldErrorMode::LogAndReturnNull);
    if (!World)
    {
        PJLINK_LOG_ERROR(TEXT("No valid World for preset test"));
        return false;
    }

    UGameInstance* GameInstance = World->GetGameInstance();
    if (!GameInstance)
    {
        PJLINK_LOG_ERROR(TEXT("No GameInstance for preset test"));
        return false;
    }

    // 프리셋 매니저 가져오기
    UPJLinkPresetManager* PresetManager = GameInstance->GetSubsystem<UPJLinkPresetManager>();
    if (!PresetManager)
    {
        PJLINK_LOG_ERROR(TEXT("Failed to get PresetManager"));
        return false;
    }

    // 테스트용 프로젝터 정보 생성
    FPJLinkProjectorInfo TestInfo;
    TestInfo.Name = TEXT("Test Projector");
    TestInfo.IPAddress = TEXT("192.168.1.100");
    TestInfo.Port = 4352;

    // 프리셋 저장
    bool bSaved = PresetManager->SavePreset(TEXT("TestPreset"), TestInfo);
    if (!bSaved)
    {
        PJLINK_LOG_ERROR(TEXT("Failed to save preset"));
        return false;
    }

    // 프리셋 로드
    FPJLinkProjectorInfo LoadedInfo;
    bool bLoaded = PresetManager->LoadPreset(TEXT("TestPreset"), LoadedInfo);
    if (!bLoaded)
    {
        PJLINK_LOG_ERROR(TEXT("Failed to load preset"));
        return false;
    }

    // 정보 확인
    bool bMatch = (LoadedInfo.Name == TestInfo.Name) &&
        (LoadedInfo.IPAddress == TestInfo.IPAddress) &&
        (LoadedInfo.Port == TestInfo.Port);

    if (bMatch)
    {
        PJLINK_LOG_INFO(TEXT("Preset loaded successfully with matching info"));
    }
    else
    {
        PJLINK_LOG_ERROR(TEXT("Loaded preset info doesn't match original"));
        return false;
    }

    // 프리셋 삭제
    bool bDeleted = PresetManager->DeletePreset(TEXT("TestPreset"));
    if (!bDeleted)
    {
        PJLINK_LOG_ERROR(TEXT("Failed to delete preset"));
        return false;
    }

    PJLINK_LOG_INFO(TEXT("Presets test completed successfully"));
    return true;
}

bool UPJLinkTests::TestStateMachine()
{
    PJLINK_LOG_INFO(TEXT("Starting state machine test"));

    // 상태 머신 생성
    UPJLinkStateMachine* StateMachine = NewObject<UPJLinkStateMachine>();
    if (!StateMachine)
    {
        PJLINK_LOG_ERROR(TEXT("Failed to create StateMachine for test"));
        return false;
    }

    // 초기 상태 확인
    if (StateMachine->GetCurrentState() != EPJLinkProjectorState::Disconnected)
    {
        PJLINK_LOG_ERROR(TEXT("Initial state is not Disconnected"));
        return false;
    }

    // 상태 변경 테스트
    StateMachine->SetState(EPJLinkProjectorState::Connecting);
    if (StateMachine->GetCurrentState() != EPJLinkProjectorState::Connecting)
    {
        PJLINK_LOG_ERROR(TEXT("Failed to change state to Connecting"));
        return false;
    }

    StateMachine->SetState(EPJLinkProjectorState::Connected);
    if (StateMachine->GetCurrentState() != EPJLinkProjectorState::Connected)
    {
        PJLINK_LOG_ERROR(TEXT("Failed to change state to Connected"));
        return false;
    }

    // 전원 상태 업데이트 테스트
    StateMachine->UpdateFromPowerStatus(EPJLinkPowerStatus::PoweredOn);
    if (StateMachine->GetCurrentState() != EPJLinkProjectorState::ReadyForUse)
    {
        PJLINK_LOG_ERROR(TEXT("Failed to update state from power status"));
        return false;
    }

    // 오류 상태 테스트
    StateMachine->SetErrorState(TEXT("Test Error"));
    if (StateMachine->GetCurrentState() != EPJLinkProjectorState::Error)
    {
        PJLINK_LOG_ERROR(TEXT("Failed to set error state"));
        return false;
    }

    if (StateMachine->GetErrorMessage() != TEXT("Test Error"))
    {
        PJLINK_LOG_ERROR(TEXT("Error message not set correctly"));
        return false;
    }

    PJLINK_LOG_INFO(TEXT("State machine test completed successfully"));
    return true;
}

// PJLinkTests.cpp의 RunAllTests 함수 수정
bool UPJLinkTests::RunAllTests(const FPJLinkProjectorInfo& ProjectorInfo)
{
    PJLINK_LOG_INFO(TEXT("Starting all tests"));

    bool bSuccess = true;

    // 각 테스트 실행
    bSuccess &= TestConnection(ProjectorInfo.IPAddress, ProjectorInfo.Port);
    PJLINK_LOG_INFO(TEXT("Connection test result: %s"), bSuccess ? TEXT("Pass") : TEXT("Fail"));

    bSuccess &= TestCommands(ProjectorInfo);
    PJLINK_LOG_INFO(TEXT("Commands test result: %s"), bSuccess ? TEXT("Pass") : TEXT("Fail"));

    bSuccess &= TestErrorRecovery(ProjectorInfo);
    PJLINK_LOG_INFO(TEXT("Error recovery test result: %s"), bSuccess ? TEXT("Pass") : TEXT("Fail"));

    // 새 테스트 추가
    bSuccess &= TestTimeoutHandling(ProjectorInfo);
    PJLINK_LOG_INFO(TEXT("Timeout handling test result: %s"), bSuccess ? TEXT("Pass") : TEXT("Fail"));

    bSuccess &= TestDiagnosticSystem(ProjectorInfo);
    PJLINK_LOG_INFO(TEXT("Diagnostic system test result: %s"), bSuccess ? TEXT("Pass") : TEXT("Fail"));

    bSuccess &= TestPresets();
    PJLINK_LOG_INFO(TEXT("Presets test result: %s"), bSuccess ? TEXT("Pass") : TEXT("Fail"));

    bSuccess &= TestStateMachine();
    PJLINK_LOG_INFO(TEXT("State machine test result: %s"), bSuccess ? TEXT("Pass") : TEXT("Fail"));

    // 이벤트 테스트는 블루프린트에서 실행
    PJLINK_LOG_INFO(TEXT("Events test skipped - run in Blueprint"));

    PJLINK_LOG_INFO(TEXT("All tests completed with result: %s"), bSuccess ? TEXT("PASS") : TEXT("FAIL"));
    return bSuccess;
}

// 더 자세한 테스트 함수만 유지하고 중복된 함수는 제거합니다
bool UPJLinkTests::TestTimeoutHandling(const FPJLinkProjectorInfo& ProjectorInfo)
{
    PJLINK_LOG_INFO(TEXT("Starting timeout handling test for projector %s"), *ProjectorInfo.Name);

    // 네트워크 매니저 생성
    UPJLinkNetworkManager* NetworkManager = NewObject<UPJLinkNetworkManager>();
    if (!NetworkManager)
    {
        PJLINK_LOG_ERROR(TEXT("Failed to create NetworkManager for test"));
        return false;
    }

    // 테스트용 짧은 타임아웃 설정 (1초)
    const float ShortTimeout = 1.0f;

    // 연결 시도
    bool bConnected = NetworkManager->ConnectToProjector(ProjectorInfo);
    if (!bConnected)
    {
        PJLINK_LOG_ERROR(TEXT("Failed to connect to projector for timeout test"));
        return false;
    }

    // 프로젝터 상태에 따라 테스트할 명령 선택
    EPJLinkCommand TestCommand = EPJLinkCommand::POWR;
    FString TestParam = TEXT("?"); // 잘못된 파라미터를 일부러 사용하여 오류 유도

    // 타임아웃 테스트를 위한 명령 전송
    PJLINK_LOG_INFO(TEXT("Sending command with intentionally invalid parameter to test timeout..."));
    bool bCommandSent = NetworkManager->SendCommandWithTimeout(TestCommand, TestParam, ShortTimeout);

    if (!bCommandSent)
    {
        PJLINK_LOG_ERROR(TEXT("Failed to send test command"));
        NetworkManager->DisconnectFromProjector();
        return false;
    }

    // 타임아웃 대기
    PJLINK_LOG_INFO(TEXT("Waiting for timeout period (%.1f seconds)..."), ShortTimeout + 0.5f);
    FPlatformProcess::Sleep(ShortTimeout + 0.5f);

    // 타임아웃 후 오류 상태 확인
    EPJLinkErrorCode LastErrorCode = NetworkManager->GetLastErrorCode();
    FString LastErrorMessage = NetworkManager->GetLastErrorMessage();

    bool bTimeoutDetected = (LastErrorCode == EPJLinkErrorCode::Timeout ||
        LastErrorCode == EPJLinkErrorCode::CommandFailed);

    if (bTimeoutDetected)
    {
        PJLINK_LOG_INFO(TEXT("Timeout successfully detected: %s"), *LastErrorMessage);
    }
    else
    {
        PJLINK_LOG_ERROR(TEXT("Timeout not properly detected. Error code: %d, Message: %s"),
            static_cast<int32>(LastErrorCode), *LastErrorMessage);
    }

    // 다음 명령이 정상 처리되는지 확인
    PJLINK_LOG_INFO(TEXT("Testing if next command works after timeout..."));
    bool bStatusRequestOk = NetworkManager->RequestStatus();

    if (bStatusRequestOk)
    {
        PJLINK_LOG_INFO(TEXT("Normal command works after timeout - test passed"));
    }
    else
    {
        PJLINK_LOG_ERROR(TEXT("Normal command failed after timeout - recovery issue"));
    }

    // 연결 해제
    NetworkManager->DisconnectFromProjector();

    return bTimeoutDetected && bStatusRequestOk;
}

bool UPJLinkTests::TestDiagnosticSystem(const FPJLinkProjectorInfo& ProjectorInfo)
{
    PJLINK_LOG_INFO(TEXT("Starting diagnostic system test"));

    // 네트워크 매니저 생성
    UPJLinkNetworkManager* NetworkManager = NewObject<UPJLinkNetworkManager>();
    if (!NetworkManager)
    {
        PJLINK_LOG_ERROR(TEXT("Failed to create NetworkManager for test"));
        return false;
    }

    // 다양한 작업 수행하여 진단 데이터 생성
    NetworkManager->ConnectToProjector(ProjectorInfo);
    NetworkManager->RequestStatus();
    NetworkManager->PowerOn();

    // 잘못된 명령으로 오류 생성
    NetworkManager->SendCommand(EPJLinkCommand::INPT, TEXT("INVALID"));

    // 진단 보고서 생성
    FString DiagnosticReport = NetworkManager->GenerateDiagnosticReport();

    // 보고서가 비어있지 않은지 확인
    if (DiagnosticReport.IsEmpty())
    {
        PJLINK_LOG_ERROR(TEXT("Diagnostic report is empty"));
        NetworkManager->DisconnectFromProjector();
        return false;
    }

    // 보고서에 필수 섹션이 포함되어 있는지 확인
    bool bHasConnectionData = DiagnosticReport.Contains(TEXT("Connection Diagnostic Data"));
    bool bHasCommandData = DiagnosticReport.Contains(TEXT("Last Command Diagnostic Data"));
    bool bHasStatusSection = DiagnosticReport.Contains(TEXT("Current Status"));
    bool bHasErrorSection = DiagnosticReport.Contains(TEXT("Last Error"));

    bool bAllSectionsPresent = bHasConnectionData && bHasCommandData &&
        bHasStatusSection && bHasErrorSection;

    if (bAllSectionsPresent)
    {
        PJLINK_LOG_INFO(TEXT("Diagnostic report contains all required sections"));
        PJLINK_LOG_VERBOSE(TEXT("Diagnostic Report:\n%s"), *DiagnosticReport);
    }
    else
    {
        PJLINK_LOG_ERROR(TEXT("Diagnostic report is missing some sections"));
    }

    // 연결 해제
    NetworkManager->DisconnectFromProjector();

    return bAllSectionsPresent;
}