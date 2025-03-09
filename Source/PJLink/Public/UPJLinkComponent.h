// UPJLinkComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PJLinkTypes.h"
#include "UPJLinkComponent.generated.h"

// 전방 선언으로 변경 (포인터로만 사용하므로)
class UPJLinkNetworkManager;
class UPJLinkStateMachine;

/**
 * 액터에 부착하여 PJLink 프로젝터 제어 기능을 제공하는 컴포넌트
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent), DisplayName = "PJLink Projector Component")
class PJLINK_API UPJLinkComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPJLinkComponent();

    // UActorComponent 인터페이스
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // 에디터에서 설정할 프로젝터 정보
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|Connection",
        meta = (DisplayName = "Projector Information", ToolTip = "기본 프로젝터 연결 정보"))
    FPJLinkProjectorInfo ProjectorInfo;

    // 자동 연결 여부
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|Connection",
        meta = (DisplayName = "Auto Connect at Start", ToolTip = "게임 시작 시 자동으로 프로젝터에 연결 시도"))
    bool bAutoConnect = false;

    // 재연결 시도 관련 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|Connection",
        meta = (EditCondition = "bAutoConnect", DisplayName = "Auto Reconnect",
            ToolTip = "연결이 끊어졌을 때 자동으로 재연결 시도"))
    bool bAutoReconnect = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|Connection",
        meta = (EditCondition = "bAutoReconnect", UIMin = 1, UIMax = 30, ClampMin = 1, ClampMax = 60,
            DisplayName = "Reconnect Interval (seconds)", ToolTip = "자동 재연결 시도 간격(초)"))
    float ReconnectInterval = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|Connection",
        meta = (EditCondition = "bAutoReconnect", UIMin = 1, UIMax = 10, ClampMin = 1, ClampMax = 20,
            DisplayName = "Max Reconnect Attempts", ToolTip = "최대 재연결 시도 횟수 (0 = 무제한)"))
    int32 MaxReconnectAttempts = 3;

    // 연결 타임아웃 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|Connection",
        meta = (UIMin = 1, UIMax = 30, ClampMin = 1, ClampMax = 60,
            DisplayName = "Connection Timeout (seconds)", ToolTip = "연결 시도 제한 시간"))
    float ConnectionTimeout = 5.0f;

    // 주기적 상태 확인 여부
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|Status",
        meta = (DisplayName = "Periodic Status Check", ToolTip = "주기적으로 프로젝터 상태 확인"))
    bool bPeriodicStatusCheck = true;

    // 상태 확인 주기 (초)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|Status",
        meta = (EditCondition = "bPeriodicStatusCheck", UIMin = 1.0, UIMax = 30.0, ClampMin = 1.0, ClampMax = 60.0,
            DisplayName = "Status Check Interval (seconds)", ToolTip = "상태 확인 주기(초)"))
    float StatusCheckInterval = 5.0f;

    // 디버깅 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|Debug",
        meta = (DisplayName = "Enable Verbose Logging", ToolTip = "자세한 로그 메시지 활성화"))
    bool bVerboseLogging = false;

    // 연결 제어 함수들
    UFUNCTION(BlueprintCallable, Category = "PJLink")
    bool Connect();

    UFUNCTION(BlueprintCallable, Category = "PJLink")
    void Disconnect();

    UFUNCTION(BlueprintPure, Category = "PJLink")
    bool IsConnected() const;

    // 프로젝터 제어 함수들
    UFUNCTION(BlueprintCallable, Category = "PJLink|Control")
    bool PowerOn();

    UFUNCTION(BlueprintCallable, Category = "PJLink|Control")
    bool PowerOff();

    UFUNCTION(BlueprintCallable, Category = "PJLink|Control")
    bool SwitchInputSource(EPJLinkInputSource InputSource);

    UFUNCTION(BlueprintCallable, Category = "PJLink|Control")
    bool RequestStatus();

    // 프로젝터 정보 가져오기
    UFUNCTION(BlueprintPure, Category = "PJLink")
    FPJLinkProjectorInfo GetProjectorInfo() const;

    // 응답 이벤트 (블루프린트에서 바인딩 가능)
    UPROPERTY(BlueprintAssignable, Category = "PJLink|Events")
    FPJLinkResponseDelegate OnResponseReceived;

    // 상태 변경 이벤트
    UPROPERTY(BlueprintAssignable, Category = "PJLink|Events")
    FPJLinkPowerStatusChangedDelegate OnPowerStatusChanged;

    UPROPERTY(BlueprintAssignable, Category = "PJLink|Events")
    FPJLinkInputSourceChangedDelegate OnInputSourceChanged;

    UPROPERTY(BlueprintAssignable, Category = "PJLink|Events")
    FPJLinkConnectionChangedDelegate OnConnectionChanged;

    UPROPERTY(BlueprintAssignable, Category = "PJLink|Events")
    FPJLinkErrorStatusDelegate OnErrorStatus;

    // 프리셋 관련 함수
    UFUNCTION(BlueprintCallable, Category = "PJLink|Presets")
    bool SaveCurrentSettingsAsPreset(const FString& PresetName);

    UFUNCTION(BlueprintCallable, Category = "PJLink|Presets")
    bool LoadPreset(const FString& PresetName);

    UFUNCTION(BlueprintCallable, Category = "PJLink|Presets")
    TArray<FString> GetAvailablePresets();

    // 상태 머신 가져오기
    UFUNCTION(BlueprintCallable, Category = "PJLink|State")
    UPJLinkStateMachine* GetStateMachine() const { return StateMachine; }

    // 프로젝터 전원 상태 가져오기
    UFUNCTION(BlueprintPure, Category = "PJLink|Status")
    EPJLinkPowerStatus GetPowerStatus() const;

    // 프로젝터 입력 소스 가져오기
    UFUNCTION(BlueprintPure, Category = "PJLink|Status")
    EPJLinkInputSource GetInputSource() const;

    // 프로젝터 이름 가져오기
    UFUNCTION(BlueprintPure, Category = "PJLink|Status")
    FString GetProjectorName() const;

    // 프로젝터가 사용 가능한 상태인지 확인 (전원 켜짐 상태)
    UFUNCTION(BlueprintPure, Category = "PJLink|Status")
    bool IsProjectorReady() const;

    // 프로젝터가 특정 명령을 실행할 수 있는 상태인지 확인
    UFUNCTION(BlueprintPure, Category = "PJLink|Status")
    bool CanExecuteCommand(EPJLinkCommand Command) const;

    // 현재 상태 문자열로 가져오기
    UFUNCTION(BlueprintPure, Category = "PJLink|Status")
    FString GetStateAsString() const;

    // 프로젝터 정보 설정 (런타임에서 변경할 때)
    UFUNCTION(BlueprintCallable, Category = "PJLink|Configuration")
    void SetProjectorInfo(const FPJLinkProjectorInfo& NewProjectorInfo);

    // 대신 정적 헬퍼 함수 사용을 위한 Blueprint 함수 추가:
    UFUNCTION(BlueprintPure, Category = "PJLink|Utility", meta = (DisplayName = "Power Status To String"))
    static FString BP_PowerStatusToString(EPJLinkPowerStatus PowerStatus)
    {
        return PJLinkHelpers::PowerStatusToString(PowerStatus);
    }

    UFUNCTION(BlueprintPure, Category = "PJLink|Utility", meta = (DisplayName = "Input Source To String"))
    static FString BP_InputSourceToString(EPJLinkInputSource InputSource)
    {
        return PJLinkHelpers::InputSourceToString(InputSource);
    }

    // 프로젝터 명령 완료 이벤트
    UPROPERTY(BlueprintAssignable, Category = "PJLink|Events")
    FPJLinkCommandCompletedDelegate OnCommandCompleted;

    // 디버깅 메시지 이벤트
    UPROPERTY(BlueprintAssignable, Category = "PJLink|Debug")
    FPJLinkDebugMessageDelegate OnDebugMessage;

    // 통신 로그 이벤트
    UPROPERTY(BlueprintAssignable, Category = "PJLink|Debug")
    FPJLinkCommunicationLogDelegate OnCommunicationLog;

    // 디버깅 메시지 발생 함수
    UFUNCTION(BlueprintCallable, Category = "PJLink|Debug")
    void EmitDebugMessage(const FString& Category, const FString& Message);

    UFUNCTION(BlueprintPure, Category = "PJLink")
    bool IsComponentValid() const;

    // UPJLinkComponent.cpp - 함수 구현
    bool UPJLinkComponent::IsComponentValid() const
    {
        // 핵심 객체들이 유효한지 확인
        return IsValid(this) && NetworkManager && StateMachine;
    }

    // 확장된 오류 이벤트
    UPROPERTY(BlueprintAssignable, Category = "PJLink|Events")
    FPJLinkExtendedErrorDelegate OnExtendedError;

    // 마지막 오류 정보 가져오기
    UFUNCTION(BlueprintPure, Category = "PJLink|Error")
    void GetLastError(EPJLinkErrorCode& OutErrorCode, FString& OutErrorMessage) const;

    // 진단 보고서 생성
    UFUNCTION(BlueprintCallable, Category = "PJLink|Diagnostic")
    FString GenerateDiagnosticReport() const;

    // 네트워크 매니저 접근 (고급 진단용)
    UFUNCTION(BlueprintCallable, Category = "PJLink|Advanced")
    UPJLinkNetworkManager* GetNetworkManager() const { return NetworkManager; }

    // 블루프린트에서 쉽게 사용할 수 있는 상태 체크 함수
    UFUNCTION(BlueprintPure, Category = "PJLink|Status")
    bool IsPoweredOn() const { return GetPowerStatus() == EPJLinkPowerStatus::PoweredOn; }

    UFUNCTION(BlueprintPure, Category = "PJLink|Status")
    bool IsPoweredOff() const { return GetPowerStatus() == EPJLinkPowerStatus::PoweredOff; }

    UFUNCTION(BlueprintPure, Category = "PJLink|Status")
    bool IsWarmingUp() const { return GetPowerStatus() == EPJLinkPowerStatus::WarmingUp; }

    UFUNCTION(BlueprintPure, Category = "PJLink|Status")
    bool IsCoolingDown() const { return GetPowerStatus() == EPJLinkPowerStatus::CoolingDown; }

    // 블루프린트 노드로 상태 문자열 가져오기
    UFUNCTION(BlueprintPure, Category = "PJLink|Status", meta = (DisplayName = "Get Power Status As String"))
    FString GetPowerStatusString() const { return PJLinkHelpers::PowerStatusToString(GetPowerStatus()); }

    UFUNCTION(BlueprintPure, Category = "PJLink|Status", meta = (DisplayName = "Get Input Source As String"))
    FString GetInputSourceString() const { return PJLinkHelpers::InputSourceToString(GetInputSource()); }

    // 단순화된 입력 소스 전환 (열거형 대신 인덱스 기반)
    UFUNCTION(BlueprintCallable, Category = "PJLink|Control", meta = (DisplayName = "Switch Input Source By Index"))
    bool SwitchInputSourceByIndex(int32 InputSourceIndex)
    {
        if (InputSourceIndex < 0 || InputSourceIndex > 4)
        {
            return false;
        }

        static const EPJLinkInputSource InputSources[] = {
            EPJLinkInputSource::RGB,      // 0
            EPJLinkInputSource::VIDEO,    // 1
            EPJLinkInputSource::DIGITAL,  // 2
            EPJLinkInputSource::STORAGE,  // 3
            EPJLinkInputSource::NETWORK   // 4
        };

        return SwitchInputSource(InputSources[InputSourceIndex]);
    }

    // 연결 상태 정보를 한 번에 가져오는 노드
    UFUNCTION(BlueprintCallable, Category = "PJLink|Status")
    void GetConnectionStatus(bool& bIsConnected, bool& bIsPoweredOn, EPJLinkInputSource& CurrentSource, FString& ProjectorName)
    {
        bIsConnected = IsConnected();
        bIsPoweredOn = IsPoweredOn();
        CurrentSource = GetInputSource();
        ProjectorName = GetProjectorName();
    }

    // 진단 보고서 생성 (다이제스트 모드)
    UFUNCTION(BlueprintCallable, Category = "PJLink|Diagnostic")
    FString GenerateDiagnosticDigest(int32 MaxEntries = 10)
    {
        if (!NetworkManager)
        {
            return TEXT("Network Manager not available");
        }

        FString Report = TEXT("PJLink Component Diagnostic Digest\n");
        Report += TEXT("=====================================\n\n");

        Report += TEXT("Component: ") + GetName() + TEXT("\n");
        Report += TEXT("Owner: ") + GetOwner()->GetName() + TEXT("\n\n");

        Report += TEXT("Connection Status: ") + FString(IsConnected() ? TEXT("Connected") : TEXT("Disconnected")) + TEXT("\n");
        Report += TEXT("Power Status: ") + GetPowerStatusString() + TEXT("\n");
        Report += TEXT("Input Source: ") + GetInputSourceString() + TEXT("\n\n");

        // 네트워크 매니저의 진단 데이터 요약
        if (NetworkManager)
        {
            Report += TEXT("Last Commands:\n");
            Report += NetworkManager->GetLastCommandDiagnosticData().GenerateDigestReport(MaxEntries);
            Report += TEXT("\n");
        }

        return Report;
    }

private:
    // 네트워크 매니저 인스턴스
    UPROPERTY()
    UPJLinkNetworkManager* NetworkManager;

    // 상태 확인 타이머 핸들
    FTimerHandle StatusCheckTimerHandle;

    // 응답 처리 함수
    UFUNCTION()
    void HandleResponseReceived(EPJLinkCommand Command, EPJLinkResponseStatus Status, const FString& Response);

    // 주기적 상태 확인
    void CheckStatus();

    // 이전 상태 추적을 위한 변수
    EPJLinkPowerStatus PreviousPowerStatus;
    EPJLinkInputSource PreviousInputSource;
    bool bPreviousConnectionState;

    // 상태 머신
    UPROPERTY()
    UPJLinkStateMachine* StateMachine;

    // 상태 변경 처리 함수
    UFUNCTION()
    void HandleStateChanged(EPJLinkProjectorState OldState, EPJLinkProjectorState NewState);

    // 프로젝터 준비 완료 이벤트 (전원 켜짐 상태가 되었을 때)
    UPROPERTY(BlueprintAssignable, Category = "PJLink|Events")
    FOnPJLinkProjectorReadyDelegate OnProjectorReady;

    // 프로젝터 작업 완료 이벤트 (명령 실행 완료 시)
    UPROPERTY(BlueprintAssignable, Category = "PJLink|Events")
    FOnPJLinkCommandCompletedDelegate OnCommandCompleted;

    // 명령 문자열 가져오기
    FString GetCommandString(EPJLinkCommand Command) const;

    // 응답 상태 문자열 가져오기
    FString GetResponseStatusString(EPJLinkResponseStatus Status) const;

    // 전원 상태 변경 처리
    void HandlePowerStatusChanged(const FString& Response);

    // 입력 소스 변경 처리
    void HandleInputSourceChanged(const FString& Response);

    // 정보 응답 처리
    void HandleInfoResponse(EPJLinkCommand Command, const FString& Response);

    // 통신 로그 핸들러
    UFUNCTION()
    void HandleCommunicationLog(bool bIsSending, const FString& CommandOrResponse, const FString& RawData);
};
