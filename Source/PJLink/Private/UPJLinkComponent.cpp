// UPJLinkComponent.cpp
#include "UPJLinkComponent.h"
#include "PJLinkNetworkManager.h"
#include "PJLinkStateMachine.h"
#include "PJLinkPresetManager.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

// 로그 카테고리 정의
DEFINE_LOG_CATEGORY_STATIC(LogPJLinkComponent, Log, All);

UPJLinkComponent::UPJLinkComponent()
{
    // 컴포넌트 기본 설정
    PrimaryComponentTick.bCanEverTick = true;  // Tick 활성화
    PrimaryComponentTick.TickInterval = 0.1f;  // 0.1초마다 Tick
    bAutoActivate = true;

    // 프로젝터 정보 기본값
    ProjectorInfo.Name = TEXT("Projector");
    ProjectorInfo.IPAddress = TEXT("192.168.1.100");
    ProjectorInfo.Port = 4352;

    // 이전 상태 변수 초기화
    PreviousPowerStatus = EPJLinkPowerStatus::Unknown;
    PreviousInputSource = EPJLinkInputSource::Unknown;
    bPreviousConnectionState = false;

    // 상태 머신 생성
    StateMachine = nullptr;
}

void UPJLinkComponent::BeginPlay()
{
    Super::BeginPlay();

    PJLINK_LOG_INFO(TEXT("PJLink Component Begin Play: %s"), *GetOwner()->GetName());

    // 기존 객체 정리 (중복 방지)
    if (NetworkManager)
    {
        // 응답 이벤트 바인딩
        NetworkManager->OnResponseReceived.AddDynamic(this, &UPJLinkComponent::HandleResponseReceived);

        // 통신 로그 이벤트 바인딩
        NetworkManager->OnCommunicationLog.AddDynamic(this, &UPJLinkComponent::HandleCommunicationLog);

        // 확장된 오류 이벤트 바인딩
        NetworkManager->OnExtendedError.AddDynamic(this, &UPJLinkComponent::HandleExtendedError);

        // 디버깅 설정 전달
        NetworkManager->bLogCommunication = bVerboseLogging;
    }

    if (StateMachine)
    {
        StateMachine->OnStateChanged.RemoveDynamic(this, &UPJLinkComponent::HandleStateChanged);
        StateMachine = nullptr;
    }

    // 네트워크 매니저 생성
    NetworkManager = NewObject<UPJLinkNetworkManager>(this, NAME_None, RF_Transient);
    if (NetworkManager)
    {
        // 응답 이벤트 바인딩
        NetworkManager->OnResponseReceived.AddDynamic(this, &UPJLinkComponent::HandleResponseReceived);

        // 통신 로그 이벤트 바인딩
        NetworkManager->OnCommunicationLog.AddDynamic(this, &UPJLinkComponent::HandleCommunicationLog);

        // 디버깅 설정 전달
        NetworkManager->bLogCommunication = bVerboseLogging;
    }
    else
    {
        PJLINK_LOG_ERROR(TEXT("Failed to create NetworkManager for component %s"), *GetOwner()->GetName());
        return; // 생성 실패 시 초기화 중단
    }

    // 상태 머신 생성
    StateMachine = NewObject<UPJLinkStateMachine>(this, NAME_None, RF_Transient);
    if (StateMachine)
    {
        // 상태 변경 이벤트 바인딩
        StateMachine->OnStateChanged.AddDynamic(this, &UPJLinkComponent::HandleStateChanged);
        // 초기 상태 설정
        StateMachine->SetState(EPJLinkProjectorState::Disconnected);
    }
    else
    {
        PJLINK_LOG_ERROR(TEXT("Failed to create StateMachine for component %s"), *GetOwner()->GetName());
        return; // 생성 실패 시 초기화 중단
    }

    // 초기 상태 설정
    PreviousPowerStatus = EPJLinkPowerStatus::Unknown;
    PreviousInputSource = EPJLinkInputSource::Unknown;
    bPreviousConnectionState = false;

    // 자동 연결 설정이 있는 경우
    if (bAutoConnect && !ProjectorInfo.IPAddress.IsEmpty())
    {
        PJLINK_LOG_INFO(TEXT("Auto-connecting to projector: %s - %s:%d"),
            *ProjectorInfo.Name, *ProjectorInfo.IPAddress, ProjectorInfo.Port);

        // 연결 시도는 지연 실행으로 처리 (BeginPlay 완료 후 수행)
        UWorld* World = GetWorld();
        if (World)
        {
            FTimerHandle ConnectTimerHandle;
            World->GetTimerManager().SetTimer(
                ConnectTimerHandle,
                this,
                &UPJLinkComponent::Connect,
                0.5f,
                false);
        }
        else
        {
            PJLINK_LOG_ERROR(TEXT("Failed to get World reference for auto-connect"));
            // 대체 연결 시도
            Connect();
        }
    }

    // 주기적 상태 확인 설정
    if (bPeriodicStatusCheck && StatusCheckInterval > 0.0f)
    {
        UWorld* World = GetWorld();
        if (World)
        {
            World->GetTimerManager().SetTimer(
                StatusCheckTimerHandle,
                this,
                &UPJLinkComponent::CheckStatus,
                FMath::Max(StatusCheckInterval, 1.0f), // 최소 1초 간격 보장
                true);
        }
        else
        {
            PJLINK_LOG_ERROR(TEXT("Failed to get World reference for status check timer"));
        }
    }
}

void UPJLinkComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    PJLINK_LOG_INFO(TEXT("PJLink Component End Play: %s, Reason: %d"),
        *GetOwner()->GetName(), static_cast<int32>(EndPlayReason));

    // 1. 먼저 타이머 정지
    UWorld* World = GetWorld();
    if (World)
    {
        World->GetTimerManager().ClearTimer(StatusCheckTimerHandle);
    }
    else
    {
        PJLINK_LOG_WARNING(TEXT("Failed to get World reference in EndPlay"));
    }

    // 2. 네트워크 매니저 참조 저장 (제거 전)
    UPJLinkNetworkManager* NetworkManagerToDisconnect = NetworkManager;

    // 3. 연결 해제
    if (NetworkManagerToDisconnect && NetworkManagerToDisconnect->IsConnected())
    {
        PJLINK_LOG_INFO(TEXT("Disconnecting from projector during EndPlay: %s"), *ProjectorInfo.Name);
        NetworkManagerToDisconnect->DisconnectFromProjector();
    }

    // 4. 이벤트 연결 해제 - 참조 해제 전에 먼저 수행
    if (NetworkManagerToDisconnect)
    {
        NetworkManagerToDisconnect->OnResponseReceived.RemoveDynamic(this, &UPJLinkComponent::HandleResponseReceived);
        NetworkManagerToDisconnect->OnCommunicationLog.RemoveDynamic(this, &UPJLinkComponent::HandleCommunicationLog);
    }

    // 5. 상태 머신 이벤트 연결 해제
    if (StateMachine)
    {
        StateMachine->OnStateChanged.RemoveDynamic(this, &UPJLinkComponent::HandleStateChanged);
    }

    // 6. 참조 해제
    NetworkManager = nullptr;
    StateMachine = nullptr;

    Super::EndPlay(EndPlayReason);
}

void UPJLinkComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // 블루프린트용 간단한 상태 업데이트 (이벤트는 발생시키지 않음)
    if (NetworkManager && IsConnected())
    {
        // 마지막 상태 확인 후 일정 시간 경과한 경우 상태 재확인
        static float TimeSinceLastStatusCheck = 0.0f;
        TimeSinceLastStatusCheck += DeltaTime;

        if (TimeSinceLastStatusCheck >= 5.0f)  // 5초마다 자동 상태 확인
        {
            TimeSinceLastStatusCheck = 0.0f;

            // 블루프린트에서의 빈번한 폴링을 대신하는 자동 상태 확인
            RequestStatus();
        }
    }
}

bool UPJLinkComponent::Connect()
{
    if (!IsComponentValid())
    {
        PJLINK_LOG_ERROR(TEXT("Cannot connect - component not valid"));
        return false;
    }

    if (StateMachine)
    {
        StateMachine->SetState(EPJLinkProjectorState::Connecting);
    }

    if (StateMachine)
    {
        StateMachine->SetState(EPJLinkProjectorState::Connecting);
    }

    if (NetworkManager)
    {
        bool bResult = NetworkManager->ConnectToProjector(ProjectorInfo, ConnectionTimeout);

        // 연결 결과에 따라 상태 머신 업데이트
        if (bResult)
        {
            PJLINK_LOG_INFO(TEXT("Successfully connected to projector: %s"), *ProjectorInfo.Name);

            if (StateMachine)
            {
                StateMachine->UpdateFromConnectionStatus(true);
            }

            // 연결 상태가 실제로 변경되었는지 확인
            if (!bPreviousConnectionState)
            {
                OnConnectionChanged.Broadcast(true);
                bPreviousConnectionState = true;
            }

            // 연결 성공 후 초기 상태 요청
            RequestStatus();
        }
        else
        {
            PJLINK_LOG_WARNING(TEXT("Failed to connect to projector: %s"), *ProjectorInfo.Name);

            if (StateMachine)
            {
                StateMachine->SetState(EPJLinkProjectorState::Disconnected);
            }

            // 연결 상태가 실제로 변경되었는지 확인
            if (bPreviousConnectionState)
            {
                OnConnectionChanged.Broadcast(false);
                bPreviousConnectionState = false;
            }
        }

        return bResult;
    }

    bool UPJLinkComponent::IsComponentValid() const
    {
        // 핵심 객체들이 유효한지 확인
        return IsValid(this) && NetworkManager && StateMachine;
    }

    PJLINK_LOG_ERROR(TEXT("NetworkManager is null, cannot connect"));

    if (StateMachine)
    {
        StateMachine->SetState(EPJLinkProjectorState::Disconnected);
    }

    return false;
}

    PJLINK_LOG_ERROR(TEXT("NetworkManager is null, cannot connect"));

    if (StateMachine)
    {
        StateMachine->SetState(EPJLinkProjectorState::Disconnected);
    }

    return false;
}

void UPJLinkComponent::Disconnect()
{
    if (NetworkManager)
    {
        bool bWasConnected = NetworkManager->IsConnected();

        if (bWasConnected)
        {
            UE_LOG(LogPJLinkComponent, Log, TEXT("Disconnecting from projector: %s"), *ProjectorInfo.Name);
            NetworkManager->DisconnectFromProjector();

            // 연결 상태 변경 감지
            if (bPreviousConnectionState != false)
            {
                OnConnectionChanged.Broadcast(false);
                bPreviousConnectionState = false;
            }

            if (StateMachine)
            {
                StateMachine->SetState(EPJLinkProjectorState::Disconnected);
            }
        }
    }
}

bool UPJLinkComponent::IsConnected() const
{
    if (NetworkManager)
    {
        return NetworkManager->IsConnected();
    }
    return false;
}

bool UPJLinkComponent::PowerOn()
{
    if (!IsComponentValid())
    {
        PJLINK_LOG_ERROR(TEXT("Cannot power on - component not valid"));
        return false;
    }

    if (NetworkManager && NetworkManager->IsConnected())
    {
        UE_LOG(LogPJLinkComponent, Log, TEXT("Power ON command sent to projector: %s"), *ProjectorInfo.Name);
        return NetworkManager->PowerOn();
    }

    UE_LOG(LogPJLinkComponent, Warning, TEXT("Cannot power on - not connected to projector: %s"), *ProjectorInfo.Name);
    return false;
}

bool UPJLinkComponent::PowerOff()
{
    if (!IsComponentValid())
    {
        PJLINK_LOG_ERROR(TEXT("Cannot power off - component not valid"));
        return false;
    }

    if (NetworkManager && NetworkManager->IsConnected())
    {
        UE_LOG(LogPJLinkComponent, Log, TEXT("Power OFF command sent to projector: %s"), *ProjectorInfo.Name);
        return NetworkManager->PowerOff();
    }

    UE_LOG(LogPJLinkComponent, Warning, TEXT("Cannot power off - not connected to projector: %s"), *ProjectorInfo.Name);
    return false;
}

bool UPJLinkComponent::SwitchInputSource(EPJLinkInputSource InputSource)
{
    if (!IsComponentValid())
    {
        PJLINK_LOG_ERROR(TEXT("Cannot switch input source - component not valid"));
        return false;
    }

    if (NetworkManager && NetworkManager->IsConnected())
    {
        UE_LOG(LogPJLinkComponent, Log, TEXT("Switch input source command sent to projector: %s - Input: %d"),
            *ProjectorInfo.Name, static_cast<int32>(InputSource));
        return NetworkManager->SwitchInputSource(InputSource);
    }

    UE_LOG(LogPJLinkComponent, Warning, TEXT("Cannot switch input - not connected to projector: %s"), *ProjectorInfo.Name);
    return false;
}

bool UPJLinkComponent::RequestStatus()
{
    if (!IsComponentValid())
    {
        PJLINK_LOG_ERROR(TEXT("Cannot request status - component not valid"));
        return false;
    }

    if (NetworkManager && NetworkManager->IsConnected())
    {
        UE_LOG(LogPJLinkComponent, Verbose, TEXT("Status request sent to projector: %s"), *ProjectorInfo.Name);
        return NetworkManager->RequestStatus();
    }

    UE_LOG(LogPJLinkComponent, Warning, TEXT("Cannot request status - not connected to projector: %s"), *ProjectorInfo.Name);
    return false;
}

FPJLinkProjectorInfo UPJLinkComponent::GetProjectorInfo() const
{
    if (NetworkManager)
    {
        return NetworkManager->GetProjectorInfo();
    }

    return ProjectorInfo;
}

// UPJLinkComponent.cpp - HandleResponseReceived 함수 안전성 개선
void UPJLinkComponent::HandleResponseReceived(EPJLinkCommand Command, EPJLinkResponseStatus Status, const FString& Response)
{
    // 로그 출력 - 자세한 로깅 옵션 적용
    if (bVerboseLogging)
    {
        PJLINK_LOG_INFO(TEXT("[%s] Response - Command: %s, Status: %s, Response: %s"),
            *ProjectorInfo.Name,
            *GetCommandString(Command),
            *GetResponseStatusString(Status),
            *Response);
    }

    // 오류 상태 확인
    if (Status != EPJLinkResponseStatus::Success)
    {
        FString ErrorMessage = FString::Printf(TEXT("Error in command %s: %s - %s"),
            *GetCommandString(Command),
            *GetResponseStatusString(Status),
            *Response);

        PJLINK_LOG_WARNING(TEXT("%s"), *ErrorMessage);

        // 이벤트 바인딩 확인 후 방송
        if (OnErrorStatus.IsBound())
        {
            OnErrorStatus.Broadcast(ErrorMessage);
        }

        if (StateMachine)
        {
            StateMachine->SetErrorState(ErrorMessage);
        }

        // 명령 완료 이벤트 발생 (실패)
        if (OnCommandCompleted.IsBound())
        {
            OnCommandCompleted.Broadcast(Command, false);
        }
        return;
    }

    // 명령 완료 이벤트 발생 (성공)
    if (OnCommandCompleted.IsBound())
    {
        OnCommandCompleted.Broadcast(Command, true);
    }

    // 특정 명령에 대한 처리
    switch (Command)
    {
    case EPJLinkCommand::POWR:
        HandlePowerStatusChanged(Response);
        break;

    case EPJLinkCommand::INPT:
        HandleInputSourceChanged(Response);
        break;

        // 다른 명령들에 대한 처리 추가
    case EPJLinkCommand::NAME:
    case EPJLinkCommand::INF1:
    case EPJLinkCommand::INF2:
        // 정보 업데이트 처리
        HandleInfoResponse(Command, Response);
        break;

    default:
        // 기본 응답 이벤트 발생
        if (OnResponseReceived.IsBound())
        {
            OnResponseReceived.Broadcast(Command, Status, Response);
        }
        break;
    }
}

    // 입력 소스 변경 확인
    if (Command == EPJLinkCommand::INPT && CurrentInfo.CurrentInputSource != PreviousInputSource)
    {
        UE_LOG(LogPJLinkComponent, Log, TEXT("Input source changed from %d to %d"),
            static_cast<int32>(PreviousInputSource), static_cast<int32>(CurrentInfo.CurrentInputSource));

        OnInputSourceChanged.Broadcast(PreviousInputSource, CurrentInfo.CurrentInputSource);
        PreviousInputSource = CurrentInfo.CurrentInputSource;
    }

    // 기존 응답 이벤트 발생
    OnResponseReceived.Broadcast(Command, Status, Response);
}

void UPJLinkComponent::CheckStatus()
{
    bool bCurrentConnectionState = IsConnected();

    // 연결 상태 변경 감지
    if (bPreviousConnectionState != bCurrentConnectionState)
    {
        if (bVerboseLogging)
        {
            PJLINK_LOG_INFO(TEXT("[%s] Connection state changed to: %s"),
                *ProjectorInfo.Name, bCurrentConnectionState ? TEXT("Connected") : TEXT("Disconnected"));
        }

        OnConnectionChanged.Broadcast(bCurrentConnectionState);
        bPreviousConnectionState = bCurrentConnectionState;

        // 연결이 끊어지고 자동 재연결이 활성화된 경우
        if (!bCurrentConnectionState && bAutoConnect && bAutoReconnect)
        {
            static int32 ReconnectCount = 0;

            // 최대 시도 횟수 체크 (0은 무제한)
            if (MaxReconnectAttempts == 0 || ReconnectCount < MaxReconnectAttempts)
            {
                ReconnectCount++;
                PJLINK_LOG_INFO(TEXT("[%s] Connection lost. Attempting to reconnect (%d/%d)..."),
                    *ProjectorInfo.Name, ReconnectCount, MaxReconnectAttempts == 0 ? 999 : MaxReconnectAttempts);

                // 재연결 시도를 위한 타이머 설정
                UWorld* World = GetWorld();
                if (World)
                {
                    FTimerHandle ReconnectTimerHandle;
                    World->GetTimerManager().SetTimer(
                        ReconnectTimerHandle,
                        this,
                        &UPJLinkComponent::Connect,
                        ReconnectInterval,
                        false);
                }
                else
                {
                    PJLINK_LOG_ERROR(TEXT("Failed to get World reference for reconnect timer"));
                    // World 없이 즉시 재연결 시도
                    Connect();
                }
            }
            else if (MaxReconnectAttempts > 0)
            {
                PJLINK_LOG_WARNING(TEXT("[%s] Max reconnect attempts (%d) reached. Giving up."),
                    *ProjectorInfo.Name, MaxReconnectAttempts);
                // 재연결 카운트 리셋
                ReconnectCount = 0;
            }
        }
    }

    if (bCurrentConnectionState)
    {
        RequestStatus();
    }
}

bool UPJLinkComponent::SaveCurrentSettingsAsPreset(const FString& PresetName)
{
    if (PresetName.IsEmpty())
    {
        UE_LOG(LogPJLinkComponent, Error, TEXT("Cannot save preset with empty name"));
        return false;
    }

    // 현재 설정 가져오기
    FPJLinkProjectorInfo CurrentInfo = ProjectorInfo;

    // 프리셋 매니저 가져오기
    UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    if (!GameInstance)
    {
        UE_LOG(LogPJLinkComponent, Error, TEXT("Failed to get GameInstance"));
        return false;
    }

    UPJLinkPresetManager* PresetManager = GameInstance->GetSubsystem<UPJLinkPresetManager>();
    if (!PresetManager)
    {
        UE_LOG(LogPJLinkComponent, Error, TEXT("Failed to get PresetManager"));
        return false;
    }

    // 프리셋 저장
    bool bResult = PresetManager->SavePreset(PresetName, CurrentInfo);

    if (bResult)
    {
        UE_LOG(LogPJLinkComponent, Log, TEXT("Saved settings as preset: %s"), *PresetName);
    }
    else
    {
        UE_LOG(LogPJLinkComponent, Error, TEXT("Failed to save preset: %s"), *PresetName);
    }

    return bResult;
}

bool UPJLinkComponent::LoadPreset(const FString& PresetName)
{
    // 프리셋 매니저 가져오기
    UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    if (!GameInstance)
    {
        UE_LOG(LogPJLinkComponent, Error, TEXT("Failed to get GameInstance"));
        return false;
    }

    UPJLinkPresetManager* PresetManager = GameInstance->GetSubsystem<UPJLinkPresetManager>();
    if (!PresetManager)
    {
        UE_LOG(LogPJLinkComponent, Error, TEXT("Failed to get PresetManager"));
        return false;
    }

    // 연결되어 있다면 연결 해제
    if (IsConnected())
    {
        Disconnect();
    }

    // 프리셋 로드
    FPJLinkProjectorInfo LoadedInfo;
    bool bResult = PresetManager->LoadPreset(PresetName, LoadedInfo);

    if (bResult)
    {
        // 설정 적용
        ProjectorInfo = LoadedInfo;

        UE_LOG(LogPJLinkComponent, Log, TEXT("Loaded preset: %s - %s:%d"),
            *PresetName, *ProjectorInfo.IPAddress, ProjectorInfo.Port);

        // 자동 연결 옵션이 활성화되어 있으면 다시 연결
        if (bAutoConnect)
        {
            Connect();
        }
    }
    else
    {
        UE_LOG(LogPJLinkComponent, Error, TEXT("Failed to load preset: %s"), *PresetName);
    }

    return bResult;
}

TArray<FString> UPJLinkComponent::GetAvailablePresets()
{
    // 프리셋 매니저 가져오기
    UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    if (!GameInstance)
    {
        UE_LOG(LogPJLinkComponent, Error, TEXT("Failed to get GameInstance"));
        return TArray<FString>();
    }

    UPJLinkPresetManager* PresetManager = GameInstance->GetSubsystem<UPJLinkPresetManager>();
    if (!PresetManager)
    {
        UE_LOG(LogPJLinkComponent, Error, TEXT("Failed to get PresetManager"));
        return TArray<FString>();
    }

    // 모든 프리셋 이름 가져오기
    return PresetManager->GetAllPresetNames();
}

void UPJLinkComponent::HandleStateChanged(EPJLinkProjectorState OldState, EPJLinkProjectorState NewState)
{
    // 상태가 변경된 경우에만 로그 출력
    if (OldState != NewState)
    {
        if (bVerboseLogging)
        {
            PJLINK_LOG_INFO(TEXT("[%s] State changed from %s to %s"),
                *ProjectorInfo.Name,
                *StateMachine->GetStateNameFromState(OldState),
                *StateMachine->GetStateNameFromState(NewState));
        }

        // 준비 상태로 전환된 경우 특별한 처리
        if (NewState == EPJLinkProjectorState::ReadyForUse)
        {
            // 이미 OnProjectorReady 이벤트가 발생했는지 확인
            static bool bReadyEventFired = false;

            if (!bReadyEventFired)
            {
                bReadyEventFired = true;
                OnProjectorReady.Broadcast();
            }
        }
        else if (NewState == EPJLinkProjectorState::Disconnected)
        {
            // 연결이 끊어지면 Ready 이벤트 플래그 리셋
            static bool bReadyEventFired = false;
            bReadyEventFired = false;
        }
    }
}

// 프로젝터 전원 상태 가져오기
EPJLinkPowerStatus UPJLinkComponent::GetPowerStatus() const
{
    if (NetworkManager)
    {
        return NetworkManager->GetProjectorInfo().PowerStatus;
    }
    return EPJLinkPowerStatus::Unknown;
}

// 프로젝터 입력 소스 가져오기
EPJLinkInputSource UPJLinkComponent::GetInputSource() const
{
    if (NetworkManager)
    {
        return NetworkManager->GetProjectorInfo().CurrentInputSource;
    }
    return EPJLinkInputSource::Unknown;
}

// 프로젝터 이름 가져오기
FString UPJLinkComponent::GetProjectorName() const
{
    if (NetworkManager)
    {
        return NetworkManager->GetProjectorInfo().Name;
    }
    return ProjectorInfo.Name;
}

// 프로젝터가 사용 가능한 상태인지 확인 (전원 켜짐 상태)
bool UPJLinkComponent::IsProjectorReady() const
{
    if (NetworkManager && NetworkManager->IsConnected())
    {
        return NetworkManager->GetProjectorInfo().PowerStatus == EPJLinkPowerStatus::PoweredOn;
    }
    return false;
}

// 프로젝터가 특정 명령을 실행할 수 있는 상태인지 확인
bool UPJLinkComponent::CanExecuteCommand(EPJLinkCommand Command) const
{
    if (!NetworkManager || !NetworkManager->IsConnected())
    {
        return false;
    }

    if (StateMachine)
    {
        return StateMachine->CanPerformAction(Command);
    }

    // 상태 머신이 없는 경우 기본적인 체크
    EPJLinkPowerStatus PowerStatus = GetPowerStatus();

    switch (Command)
    {
    case EPJLinkCommand::POWR:
        // 전원 명령은 항상 가능
        return true;

    case EPJLinkCommand::INPT:
        // 입력 전환은 전원이 켜진 상태에서만 가능
        return PowerStatus == EPJLinkPowerStatus::PoweredOn;

    default:
        // 그 외 기본 상태 명령은 항상 가능
        return true;
    }
}

// 현재 상태 문자열로 가져오기
FString UPJLinkComponent::GetStateAsString() const
{
    if (StateMachine)
    {
        return StateMachine->GetStateName();
    }
    return TEXT("Unknown");
}

// 프로젝터 정보 설정 (런타임에서 변경할 때)
void UPJLinkComponent::SetProjectorInfo(const FPJLinkProjectorInfo& NewProjectorInfo)
{
    // 연결되어 있다면 먼저 연결 해제
    if (NetworkManager && NetworkManager->IsConnected())
    {
        Disconnect();
    }

    // 새 프로젝터 정보 설정
    ProjectorInfo = NewProjectorInfo;

    // 자동 연결이 활성화되어 있으면 새 정보로 다시 연결
    if (bAutoConnect)
    {
        Connect();
    }
}


// UPJLinkComponent.cpp에 다음 함수들 추가
FString UPJLinkComponent::GetCommandString(EPJLinkCommand Command) const
{
    switch (Command)
    {
    case EPJLinkCommand::POWR: return TEXT("POWR");
    case EPJLinkCommand::INPT: return TEXT("INPT");
    case EPJLinkCommand::AVMT: return TEXT("AVMT");
    case EPJLinkCommand::ERST: return TEXT("ERST");
    case EPJLinkCommand::LAMP: return TEXT("LAMP");
    case EPJLinkCommand::INST: return TEXT("INST");
    case EPJLinkCommand::NAME: return TEXT("NAME");
    case EPJLinkCommand::INF1: return TEXT("INF1");
    case EPJLinkCommand::INF2: return TEXT("INF2");
    case EPJLinkCommand::INFO: return TEXT("INFO");
    case EPJLinkCommand::CLSS: return TEXT("CLSS");
    default: return TEXT("Unknown");
    }
}

FString UPJLinkComponent::GetResponseStatusString(EPJLinkResponseStatus Status) const
{
    switch (Status)
    {
    case EPJLinkResponseStatus::Success: return TEXT("Success");
    case EPJLinkResponseStatus::UndefinedCommand: return TEXT("UndefinedCommand");
    case EPJLinkResponseStatus::OutOfParameter: return TEXT("OutOfParameter");
    case EPJLinkResponseStatus::UnavailableTime: return TEXT("UnavailableTime");
    case EPJLinkResponseStatus::ProjectorFailure: return TEXT("ProjectorFailure");
    case EPJLinkResponseStatus::AuthenticationError: return TEXT("AuthenticationError");
    case EPJLinkResponseStatus::NoResponse: return TEXT("NoResponse");
    case EPJLinkResponseStatus::Unknown:
    default: return TEXT("Unknown");
    }
}

void UPJLinkComponent::HandlePowerStatusChanged(const FString& Response)
{
    // 현재 프로젝터 정보 가져오기
    FPJLinkProjectorInfo CurrentInfo = NetworkManager->GetProjectorInfo();
    EPJLinkPowerStatus CurrentPowerStatus = CurrentInfo.PowerStatus;

    if (CurrentPowerStatus != PreviousPowerStatus)
    {
        PJLINK_LOG_INFO(TEXT("[%s] Power status changed from %s to %s"),
            *ProjectorInfo.Name,
            *PJLinkHelpers::PowerStatusToString(PreviousPowerStatus),
            *PJLinkHelpers::PowerStatusToString(CurrentPowerStatus));

        // 이벤트 발생 (바인딩 확인)
        if (OnPowerStatusChanged.IsBound())
        {
            OnPowerStatusChanged.Broadcast(PreviousPowerStatus, CurrentPowerStatus);
        }
        PreviousPowerStatus = CurrentPowerStatus;

        // 상태 머신 업데이트
        if (StateMachine)
        {
            StateMachine->UpdateFromPowerStatus(CurrentPowerStatus);
        }

        // 프로젝터가 켜진 상태가 되면 준비 완료 이벤트 발생
        if (CurrentPowerStatus == EPJLinkPowerStatus::PoweredOn && OnProjectorReady.IsBound())
        {
            OnProjectorReady.Broadcast();
        }
    }
}

void UPJLinkComponent::HandleInputSourceChanged(const FString& Response)
{
    // 현재 프로젝터 정보 가져오기
    FPJLinkProjectorInfo CurrentInfo = NetworkManager->GetProjectorInfo();
    EPJLinkInputSource CurrentInputSource = CurrentInfo.CurrentInputSource;

    if (CurrentInputSource != PreviousInputSource)
    {
        PJLINK_LOG_INFO(TEXT("[%s] Input source changed from %s to %s"),
            *ProjectorInfo.Name,
            *InputSourceToString(PreviousInputSource),
            *InputSourceToString(CurrentInputSource));

        // 이벤트 발생 (바인딩 확인)
        if (OnInputSourceChanged.IsBound())
        {
            OnInputSourceChanged.Broadcast(PreviousInputSource, CurrentInputSource);
        }
        PreviousInputSource = CurrentInputSource;
    }
}

void UPJLinkComponent::HandleInfoResponse(EPJLinkCommand Command, const FString& Response)
{
    // 정보 업데이트 시 특별한 처리가 필요하면 여기에 구현
    switch (Command)
    {
    case EPJLinkCommand::NAME:
        PJLINK_LOG_INFO(TEXT("[%s] Projector name: %s"), *ProjectorInfo.Name, *Response);
        break;
    case EPJLinkCommand::INF1:
        PJLINK_LOG_INFO(TEXT("[%s] Manufacturer: %s"), *ProjectorInfo.Name, *Response);
        break;
    case EPJLinkCommand::INF2:
        PJLINK_LOG_INFO(TEXT("[%s] Product name: %s"), *ProjectorInfo.Name, *Response);
        break;
    }

    // 기본 응답 이벤트 발생
    OnResponseReceived.Broadcast(Command, EPJLinkResponseStatus::Success, Response);

}

void UPJLinkComponent::EmitDebugMessage(const FString& Category, const FString& Message)
{
    // 로그에 출력
    PJLINK_LOG_INFO(TEXT("[%s][%s] %s"), *ProjectorInfo.Name, *Category, *Message);

    // 이벤트 발생
    OnDebugMessage.Broadcast(Category, Message);
}

void UPJLinkComponent::HandleCommunicationLog(bool bIsSending, const FString& CommandOrResponse, const FString& RawData)
{
    // 컴포넌트의 통신 로그 이벤트로 전달
    OnCommunicationLog.Broadcast(bIsSending, CommandOrResponse, RawData);
}

void UPJLinkComponent::HandleExtendedError(EPJLinkErrorCode ErrorCode, const FString& ErrorMessage, EPJLinkCommand RelatedCommand)
{
    // 컴포넌트의 확장된 오류 이벤트로 전달
    if (OnExtendedError.IsBound())
    {
        OnExtendedError.Broadcast(ErrorCode, ErrorMessage, RelatedCommand);
    }

    // 디버그 메시지 발생
    EmitDebugMessage(TEXT("Error"), FString::Printf(TEXT("[%s] %s"),
        *UEnum::GetValueAsString(ErrorCode),
        *ErrorMessage));

    // 심각한 오류인 경우 상태 머신 업데이트
    if (ErrorCode != EPJLinkErrorCode::None && StateMachine)
    {
        StateMachine->SetErrorState(ErrorMessage);
    }
}

void UPJLinkComponent::GetLastError(EPJLinkErrorCode& OutErrorCode, FString& OutErrorMessage) const
{
    if (NetworkManager)
    {
        OutErrorCode = NetworkManager->GetLastErrorCode();
        OutErrorMessage = NetworkManager->GetLastErrorMessage();
    }
    else
    {
        OutErrorCode = EPJLinkErrorCode::UnknownError;
        OutErrorMessage = TEXT("Network manager not available");
    }
}

// UPJLinkComponent.cpp에 추가
FString UPJLinkComponent::GenerateDiagnosticReport() const
{
    FString Report = TEXT("PJLink Component Diagnostic Report\n");
    Report += TEXT("=====================================\n\n");

    // 기본 컴포넌트 정보
    Report += FString::Printf(TEXT("Component Name: %s\n"), *GetName());
    Report += FString::Printf(TEXT("Owner: %s\n"), *GetOwner()->GetName());
    Report += FString::Printf(TEXT("Valid: %s\n"), IsComponentValid() ? TEXT("Yes") : TEXT("No"));
    Report += FString::Printf(TEXT("Active: %s\n"), IsActive() ? TEXT("Yes") : TEXT("No"));

    // 프로젝터 정보
    Report += TEXT("\n1. Projector Configuration\n");
    Report += TEXT("-------------------------\n");
    Report += FString::Printf(TEXT("Projector Name: %s\n"), *ProjectorInfo.Name);
    Report += FString::Printf(TEXT("IP Address: %s\n"), *ProjectorInfo.IPAddress);
    Report += FString::Printf(TEXT("Port: %d\n"), ProjectorInfo.Port);
    Report += FString::Printf(TEXT("Requires Authentication: %s\n"), ProjectorInfo.bRequiresAuthentication ? TEXT("Yes") : TEXT("No"));

    // 연결 설정
    Report += TEXT("\n2. Connection Settings\n");
    Report += TEXT("---------------------\n");
    Report += FString::Printf(TEXT("Auto Connect: %s\n"), bAutoConnect ? TEXT("Yes") : TEXT("No"));
    Report += FString::Printf(TEXT("Auto Reconnect: %s\n"), bAutoReconnect ? TEXT("Yes") : TEXT("No"));
    Report += FString::Printf(TEXT("Reconnect Interval: %.1f seconds\n"), ReconnectInterval);
    Report += FString::Printf(TEXT("Max Reconnect Attempts: %d\n"), MaxReconnectAttempts);
    Report += FString::Printf(TEXT("Connection Timeout: %.1f seconds\n"), ConnectionTimeout);

    // 현재 상태
    Report += TEXT("\n3. Current Status\n");
    Report += TEXT("----------------\n");
    Report += FString::Printf(TEXT("Connected: %s\n"), IsConnected() ? TEXT("Yes") : TEXT("No"));
    Report += FString::Printf(TEXT("Power Status: %s\n"), *PJLinkHelpers::PowerStatusToString(GetPowerStatus()));
    Report += FString::Printf(TEXT("Input Source: %s\n"), *PJLinkHelpers::InputSourceToString(GetInputSource()));
    Report += FString::Printf(TEXT("State: %s\n"), *GetStateAsString());
    Report += FString::Printf(TEXT("Projector Ready: %s\n"), IsProjectorReady() ? TEXT("Yes") : TEXT("No"));

    // 에러 정보
    EPJLinkErrorCode ErrorCode;
    FString ErrorMessage;
    GetLastError(ErrorCode, ErrorMessage);

    Report += TEXT("\n4. Last Error\n");
    Report += TEXT("------------\n");
    Report += FString::Printf(TEXT("Error Code: %s\n"), *UEnum::GetValueAsString(ErrorCode));
    Report += FString::Printf(TEXT("Error Message: %s\n"), *ErrorMessage);

    // 네트워크 매니저 진단 정보 추가
    if (NetworkManager)
    {
        Report += TEXT("\n5. Network Manager Diagnostic\n");
        Report += TEXT("---------------------------\n");
        Report += NetworkManager->GenerateDiagnosticReport();
    }

    return Report;
}