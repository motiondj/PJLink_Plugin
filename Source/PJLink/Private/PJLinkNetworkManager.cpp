// PJLinkNetworkManager.cpp
#include "PJLinkNetworkManager.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Common/TcpSocketBuilder.h"
#include "Async/Async.h"
#include "HAL/RunnableThread.h"
#include "Misc/ScopeLock.h"
#include "Misc/CString.h"
#include "Misc/SecureHash.h" // FMD5를 위한 헤더
#include <string>  // std::string 사용을 위한 헤더

UPJLinkNetworkManager::UPJLinkNetworkManager()
    : Socket(nullptr)
    , ReceiverThread(nullptr)
    , bThreadStop(false)
    , bConnected(false)
    , LastErrorCode(EPJLinkErrorCode::None)
    , LastErrorMessage(TEXT(""))
{
    // 응답 큐 처리 타이머 설정 - 0.1초마다 큐 확인
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            ResponseQueueTimerHandle,
            this,
            &UPJLinkNetworkManager::ProcessResponseQueue,
            0.1f,
            true);
    }
}

// PJLinkNetworkManager.cpp의 ProcessResponseQueue 함수 최적화
void UPJLinkNetworkManager::ProcessResponseQueue()
{
    // 유효하지 않은 객체 체크
    if (!IsValid(this))
    {
        return;
    }

    FPJLinkResponseQueueItem Item;
    int32 ProcessedItems = 0;
    const int32 MaxItemsPerFrame = 10;
    bool bShouldReschedule = false;

    // 큐에 항목이 있으면 처리
    {
        FScopeLock QueueLock(&ResponseQueueLock); // 큐 접근 시 락 사용

        while (!ResponseQueue.IsEmpty() && ProcessedItems < MaxItemsPerFrame)
        {
            ResponseQueue.Dequeue(Item);
            ProcessedItems++;

            // 락 밖에서 처리하기 위해 복사본 생성에 대한 비용을 줄이기 위해
            // 각 항목을 처리하는 로직은 바깥에서 실행
            Item.WeakThis = TWeakObjectPtr<UPJLinkNetworkManager>(this);

            // 여기서 항목 처리
            ProcessResponseItem(Item);
        }

        // 큐에 항목이 더 있는지 확인
        bShouldReschedule = !ResponseQueue.IsEmpty();
    }

    // 아직 처리할 항목이 남아있으면 다음 프레임에 계속 처리
    if (bShouldReschedule)
    {
        if (UWorld* World = GetWorld())
        {
            // 성능 최적화: 호출 빈도 제한 (최소 16ms = 약 60fps)
            static const float MinProcessInterval = 0.016f;
            World->GetTimerManager().SetTimer(
                ResponseQueueTimerHandle,
                this,
                &UPJLinkNetworkManager::ProcessResponseQueue,
                MinProcessInterval,
                false);
        }
    }
}

// 응답 항목 처리를 별도 함수로 분리
void UPJLinkNetworkManager::ProcessResponseItem(const FPJLinkResponseQueueItem& Item)
{
    // 통신 로그 이벤트인지 확인
    if (Item.ResponseText.StartsWith(TEXT("COMMUNICATION_LOG")))
    {
        // 통신 로그 이벤트 처리
        TArray<FString> Parts;
        Item.ResponseText.ParseIntoArray(Parts, TEXT("|"));

        if (Parts.Num() >= 4)
        {
            bool bIsSending = (Parts[1] == TEXT("1"));
            if (OnCommunicationLog.IsBound())
            {
                OnCommunicationLog.Broadcast(bIsSending, Parts[2], Parts[3]);
            }
        }
    }
    // 오류 이벤트인지 확인
    else if (Item.ResponseText.StartsWith(TEXT("ERROR")))
    {
        // 오류 이벤트 처리
        TArray<FString> Parts;
        Item.ResponseText.ParseIntoArray(Parts, TEXT("|"));

        if (Parts.Num() >= 3)
        {
            EPJLinkErrorCode ErrorCode = static_cast<EPJLinkErrorCode>(FCString::Atoi(*Parts[1]));
            if (OnExtendedError.IsBound())
            {
                OnExtendedError.Broadcast(ErrorCode, Parts[2], Item.Command);
            }
        }
    }
    else
    {
        // 일반 응답 이벤트 처리
        if (OnResponseReceived.IsBound())
        {
            OnResponseReceived.Broadcast(Item.Command, Item.Status, Item.ResponseText);
        }
    }
}

UPJLinkNetworkManager::~UPJLinkNetworkManager()
{
    // 모든 리소스 안전하게 정리
    Shutdown();
}

void UPJLinkNetworkManager::Shutdown()
{
    // 타이머 정지
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(ResponseQueueTimerHandle);
    }

    // 1. 먼저 스레드 종료 신호 설정
    bThreadStop.store(true, std::memory_order_release);

    // 2. 스레드 종료 대기
    FRunnableThread* ThreadToDelete = nullptr;
    {
        FScopeLock Lock(&SocketCriticalSection);
        ThreadToDelete = ReceiverThread;
        ReceiverThread = nullptr;
    }

    // 임계 영역 밖에서 스레드 종료 (교착 상태 방지)
    if (ThreadToDelete)
    {
        ThreadToDelete->Kill(true);
        delete ThreadToDelete;
    }

    // 3. 소켓 종료
    FSocket* SocketToClose = nullptr;
    {
        FScopeLock Lock(&SocketCriticalSection);
        SocketToClose = Socket;
        Socket = nullptr;
    }

    // 임계 영역 밖에서 소켓 종료 (교착 상태 방지)
    if (SocketToClose)
    {
        ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
        if (SocketSubsystem)
        {
            SocketToClose->Close();
            SocketSubsystem->DestroySocket(SocketToClose);
        }
    }

    // 4. 연결 상태 업데이트
    bConnected.store(false, std::memory_order_release);

    // 5. 큐 비우기
    {
        FScopeLock QueueLock(&ResponseQueueLock);
        FPJLinkResponseQueueItem DummyItem;
        while (ResponseQueue.Dequeue(DummyItem)) {}
    }

    // 6. 프로젝터 정보 초기화
    {
        FScopeLock InfoLock(&ProjectorInfoLock);
        CurrentProjectorInfo.bIsConnected = false;
        CurrentProjectorInfo.PowerStatus = EPJLinkPowerStatus::Unknown;
        CurrentProjectorInfo.CurrentInputSource = EPJLinkInputSource::Unknown;
    }
}

// PJLinkNetworkManager.cpp의 ConnectToProjector 함수 변경
bool UPJLinkNetworkManager::ConnectToProjector(const FPJLinkProjectorInfo& ProjectorInfo, float TimeoutSeconds)
{
    // 진단 데이터 기록 시작
    PJLINK_CAPTURE_DIAGNOSTIC(ConnectionDiagnosticData,
        TEXT("Starting connection to %s:%d"), *ProjectorInfo.IPAddress, ProjectorInfo.Port);

    FScopeLock InfoLock(&ProjectorInfoLock);

    // 프로젝터 정보 저장 - 재연결을 위해 LastProjectorInfo도 업데이트
    CurrentProjectorInfo = ProjectorInfo;
    LastProjectorInfo = ProjectorInfo;

    // 기존 소켓 정리
    if (Socket)
    {
        PJLINK_CAPTURE_DIAGNOSTIC(ConnectionDiagnosticData, TEXT("Cleaning up existing socket connection"));
        DisconnectFromProjector();
    }

    // 소켓 생성
    if (!CreateSocket())
    {
        PJLINK_CAPTURE_DIAGNOSTIC(ConnectionDiagnosticData, TEXT("Failed to create socket"));
        return false;
    }

    // 서버 연결
    PJLINK_CAPTURE_DIAGNOSTIC(ConnectionDiagnosticData, TEXT("Attempting to connect to server"));
    if (!ConnectToServer(ProjectorInfo.IPAddress, ProjectorInfo.Port, TimeoutSeconds))
    {
        PJLINK_CAPTURE_DIAGNOSTIC(ConnectionDiagnosticData, TEXT("Failed to connect to server"));
        DisconnectFromProjector();
        return false;
    }

    // 인증 처리 (필요한 경우)
    if (ProjectorInfo.bRequiresAuthentication)
    {
        PJLINK_CAPTURE_DIAGNOSTIC(ConnectionDiagnosticData, TEXT("Authentication required, attempting..."));
        if (!HandleAuthentication(ProjectorInfo))
        {
            PJLINK_CAPTURE_DIAGNOSTIC(ConnectionDiagnosticData, TEXT("Authentication failed"));
            DisconnectFromProjector();
            return false;
        }
        PJLINK_CAPTURE_DIAGNOSTIC(ConnectionDiagnosticData, TEXT("Authentication successful"));
    }

    // 연결 상태 업데이트
    this->bConnected.store(true, std::memory_order_release);
    CurrentProjectorInfo.bIsConnected = true;
    PJLINK_CAPTURE_DIAGNOSTIC(ConnectionDiagnosticData, TEXT("Connection successful, bConnected set to true"));

    // 재연결 시도 카운트 리셋 - 연결 성공 시
    ReconnectAttemptCount = 0;

    // 수신 스레드 시작
    if (!StartReceiverThread())
    {
        PJLINK_CAPTURE_DIAGNOSTIC(ConnectionDiagnosticData, TEXT("Failed to start receiver thread"));
        DisconnectFromProjector();
        return false;
    }
    PJLINK_CAPTURE_DIAGNOSTIC(ConnectionDiagnosticData, TEXT("Receiver thread started successfully"));

    // 초기 상태 요청
    RequestStatus();

    PJLINK_LOG_INFO(TEXT("Successfully connected to projector at %s:%d"),
        *ProjectorInfo.IPAddress, ProjectorInfo.Port);
    PJLINK_CAPTURE_DIAGNOSTIC(ConnectionDiagnosticData, TEXT("Connection process completed successfully"));
    return true;
}

void UPJLinkNetworkManager::DisconnectFromProjector()
{
    // 스레드 종료 신호 설정
    bThreadStop.store(true, std::memory_order_release);

    // 스레드 종료 대기
    if (ReceiverThread)
    {
        ReceiverThread->Kill(true);
        delete ReceiverThread;
        ReceiverThread = nullptr;
    }

    // 소켓 종료 (스레드 안전하게)
    {
        FScopeLock SocketLock(&SocketCriticalSection);
        if (Socket)
        {
            ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
            if (SocketSubsystem)
            {
                Socket->Close();
                SocketSubsystem->DestroySocket(Socket);
            }
            Socket = nullptr;
        }
    }

    // 연결 상태 업데이트
    bConnected.store(false, std::memory_order_release);

    // 프로젝터 정보 업데이트
    {
        FScopeLock InfoLock(&ProjectorInfoLock);
        CurrentProjectorInfo.bIsConnected = false;
    }
}

// PJLinkNetworkManager.cpp에서
bool UPJLinkNetworkManager::SendCommand(EPJLinkCommand Command, const FString& Parameter)
{
    // 진단 데이터 기록 시작
    PJLINK_CAPTURE_DIAGNOSTIC(LastCommandDiagnosticData, TEXT("Sending command: %s, Parameter: %s"),
        *PJLinkHelpers::CommandToString(Command), *Parameter);

    // 호출 전 NULL 및 연결 상태 검사
    if (!Socket)
    {
        PJLINK_CAPTURE_DIAGNOSTIC(LastCommandDiagnosticData, TEXT("Cannot send command: Socket is null"));
        EmitError(EPJLinkErrorCode::SocketError, TEXT("Cannot send command: Socket is null"), Command);
        return false;
    }

    if (!bConnected.load(std::memory_order_acquire))
    {
        PJLINK_CAPTURE_DIAGNOSTIC(LastCommandDiagnosticData, TEXT("Cannot send command: Not connected"));
        EmitError(EPJLinkErrorCode::SocketError, TEXT("Cannot send command: Not connected"), Command);
        return false;
    }

    // 명령 문자열 생성
    FString CommandStr = BuildCommandString(Command, Parameter);
    if (CommandStr.IsEmpty())
    {
        PJLINK_CAPTURE_DIAGNOSTIC(LastCommandDiagnosticData, TEXT("Failed to build command string"));
        EmitError(EPJLinkErrorCode::CommandFailed, TEXT("Failed to build command string"), Command);
        return false;
    }

    // 통신 로깅 (명령 전송)
    if (bLogCommunication)
    {
        LogCommunication(true, PJLinkHelpers::CommandToString(Command), CommandStr);
    }

    // UTF-8로 변환
    FTCHARToUTF8 Utf8Command(*CommandStr);
    const uint8* SendData = (const uint8*)Utf8Command.Get();
    int32 DataLen = Utf8Command.Length();
    if (DataLen <= 0)
    {
        PJLINK_CAPTURE_DIAGNOSTIC(LastCommandDiagnosticData, TEXT("Command string conversion failed"));
        EmitError(EPJLinkErrorCode::CommandFailed, TEXT("Command string conversion failed"), Command);
        return false;
    }

    // 데이터 송신 (임계 영역 내에서)
    int32 BytesSent = 0;
    bool bSuccess = false;
    ESocketErrors LastError = ESocketErrors::SE_NO_ERROR;

    {
        FScopeLock ScopeLock(&SocketCriticalSection);
        // 두 번째 검사 (락 내부에서 다시 확인)
        if (!Socket || !bConnected.load(std::memory_order_acquire))
        {
            PJLINK_CAPTURE_DIAGNOSTIC(LastCommandDiagnosticData, TEXT("Socket disconnected while preparing to send"));
            EmitError(EPJLinkErrorCode::SocketError, TEXT("Socket disconnected while preparing to send"), Command);
            return false;
        }

        bSuccess = Socket->Send(SendData, DataLen, BytesSent);
        if (!bSuccess)
        {
            LastError = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLastErrorCode();
        }
    }

    if (!bSuccess || BytesSent != DataLen)
    {
        FString ErrorString = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetSocketError(LastError);
        FString ErrorMessage = FString::Printf(TEXT("Failed to send command. Error: %s"), *ErrorString);
        PJLINK_CAPTURE_DIAGNOSTIC(LastCommandDiagnosticData, TEXT("%s"), *ErrorMessage);
        EmitError(EPJLinkErrorCode::SocketError, ErrorMessage, Command);
        return false;
    }

    PJLINK_CAPTURE_DIAGNOSTIC(LastCommandDiagnosticData, TEXT("Command sent successfully. Bytes: %d"), BytesSent);
    PJLINK_LOG_VERBOSE(TEXT("Sent command: %s"), *CommandStr.TrimEnd());
    return true;
}

FPJLinkProjectorInfo UPJLinkNetworkManager::GetProjectorInfo() const
{
    FScopeLock InfoLock(&ProjectorInfoLock);
    return CurrentProjectorInfo;
}

bool UPJLinkNetworkManager::IsConnected() const
{
    // 원자적 변수 사용으로 락이 필요 없음
    return bConnected.load(std::memory_order_acquire);
}

bool UPJLinkNetworkManager::PowerOn()
{
    return SendCommandWithTimeout(EPJLinkCommand::POWR, TEXT("1"), 10.0f); // 전원 켜기는 더 긴 타임아웃
}
}

bool UPJLinkNetworkManager::PowerOff()
{
    return SendCommandWithTimeout(EPJLinkCommand::POWR, TEXT("0"), 5.0f);
}

bool UPJLinkNetworkManager::SwitchInputSource(EPJLinkInputSource InputSource)
{
    FString Parameter;

    switch (InputSource)
    {
    case EPJLinkInputSource::RGB:
        Parameter = TEXT("1");
        break;
    case EPJLinkInputSource::VIDEO:
        Parameter = TEXT("2");
        break;
    case EPJLinkInputSource::DIGITAL:
        Parameter = TEXT("3");
        break;
    case EPJLinkInputSource::STORAGE:
        Parameter = TEXT("4");
        break;
    case EPJLinkInputSource::NETWORK:
        Parameter = TEXT("5");
        break;
    default:
        return false;
    }

    return SendCommandWithTimeout(EPJLinkCommand::INPT, Parameter, 5.0f);
}

bool UPJLinkNetworkManager::RequestStatus()
{
    // 여러 상태 정보 요청
    bool bSuccess = true;

    bSuccess &= SendCommand(EPJLinkCommand::POWR);  // 전원 상태
    bSuccess &= SendCommand(EPJLinkCommand::INPT);  // 입력 소스
    bSuccess &= SendCommand(EPJLinkCommand::NAME);  // 프로젝터 이름
    bSuccess &= SendCommand(EPJLinkCommand::INF1);  // 제조사
    bSuccess &= SendCommand(EPJLinkCommand::INF2);  // 제품명
    bSuccess &= SendCommand(EPJLinkCommand::CLSS);  // 클래스 정보

    return bSuccess;
}

bool UPJLinkNetworkManager::Init()
{
    return true;
}

// PJLinkNetworkManager.cpp의 Run 함수 최적화
uint32 UPJLinkNetworkManager::Run()
{
    // 스레드당 버퍼 메모리 할당 (재사용)
    uint8 RecvBuffer[2048];

    // 재연결 상태 추적을 위한 이전 상태
    bool bWasConnected = true;

    // 스레드 로컬 캐시 - 직접 전역 상태 대신 사용
    FSocket* LocalSocket = nullptr;

    while (!bThreadStop.load(std::memory_order_acquire))
    {
        // 로컬 소켓 상태 업데이트 (락 최소화)
        if (LocalSocket != Socket)
        {
            FScopeLock ScopeLock(&SocketCriticalSection);
            LocalSocket = Socket;
        }

        // 소켓 유효성 확인
        bool bIsCurrentlyConnected = IsConnected();
        if (!LocalSocket || !bIsCurrentlyConnected)
        {
            // 연결 상태 변경 탐지 (스레드에서 한 번만 처리)
            if (bWasConnected && !bIsCurrentlyConnected)
            {
                bWasConnected = false;

                // 자동 재연결 설정이 활성화되어 있으면 재연결 시도
                if (bAutoReconnect && !bThreadStop.load(std::memory_order_acquire))
                {
                    PJLINK_CAPTURE_DIAGNOSTIC(ConnectionDiagnosticData,
                        TEXT("Connection lost in receiver thread. Setting up reconnection..."));

                    // 게임 스레드에서 재연결 시도 예약 (축약된 람다 사용)
                    AsyncTask(ENamedThreads::GameThread, [this]() {
                        if (UWorld* World = GetWorld())
                        {
                            if (!World->GetTimerManager().IsTimerActive(ReconnectTimerHandle))
                            {
                                AttemptReconnect();
                            }
                        }
                        });
                }
            }

            // 소켓 없이 계속 진행하면 안됨
            FPlatformProcess::Sleep(0.1f); // 더 긴 대기 시간으로 CPU 사용량 감소
            continue;
        }

        // 연결되었는지 확인
        bWasConnected = true;

        // 데이터 수신 대기
        int32 BytesRead = 0;
        bool bReadSuccess = false;

        {
            FScopeLock ScopeLock(&SocketCriticalSection);
            if (LocalSocket)
            {
                bReadSuccess = LocalSocket->Recv(RecvBuffer, sizeof(RecvBuffer) - 1, BytesRead, ESocketReceiveFlags::None);
            }
        }

        // 데이터 처리 또는 오류 처리
        if (!bReadSuccess)
        {
            // 연결 끊김 또는 오류 처리...
            // 기존 코드와 동일하게 유지
        }
        else if (BytesRead > 0)
        {
            // 수신 데이터 처리...
            // 기존 코드와 동일하게 유지
        }
        else
        {
            // 데이터가 없으면 짧게 대기 (CPU 사용량 감소)
            FPlatformProcess::Sleep(0.005f);
        }
    }

    // 스레드 종료 시 연결 상태 업데이트
    bConnected.store(false, std::memory_order_release);
    return 0;
}

void UPJLinkNetworkManager::Stop()
{
    bThreadStop.store(true, std::memory_order_release);
}

void UPJLinkNetworkManager::Exit()
{
    // 스레드 종료 시 필요한 정리 작업
}

// PJLinkNetworkManager.cpp의 BuildCommandString 함수 최적화
FString UPJLinkNetworkManager::BuildCommandString(EPJLinkCommand Command, const FString& Parameter)
{
    // 메모리 할당 최적화: 필요한 크기를 미리 예약
    FString CommandStr;
    CommandStr.Reserve(20 + Parameter.Len()); // 명령어 + 파라미터 + 여유 공간

    // 클래스 접두사
    CommandStr += (CurrentProjectorInfo.DeviceClass == EPJLinkClass::Class2) ? TEXT("%2") : TEXT("%1");

    // 명령 추가 - switch 대신 lookup table 사용
    static const TCHAR* CommandStrings[] = {
        TEXT("POWR"), // EPJLinkCommand::POWR
        TEXT("INPT"), // EPJLinkCommand::INPT
        TEXT("AVMT"), // EPJLinkCommand::AVMT
        TEXT("ERST"), // EPJLinkCommand::ERST
        TEXT("LAMP"), // EPJLinkCommand::LAMP
        TEXT("INST"), // EPJLinkCommand::INST
        TEXT("NAME"), // EPJLinkCommand::NAME
        TEXT("INF1"), // EPJLinkCommand::INF1
        TEXT("INF2"), // EPJLinkCommand::INF2
        TEXT("INFO"), // EPJLinkCommand::INFO
        TEXT("CLSS")  // EPJLinkCommand::CLSS
    };

    // 명령 인덱스가 유효한지 확인
    uint8 CmdIndex = static_cast<uint8>(Command);
    if (CmdIndex < UE_ARRAY_COUNT(CommandStrings))
    {
        CommandStr += CommandStrings[CmdIndex];
    }
    else
    {
        // 예외 처리
        PJLINK_LOG_ERROR(TEXT("Invalid command index: %d"), CmdIndex);
        return FString();
    }

    // 파라미터 추가 (있을 경우)
    if (!Parameter.IsEmpty())
    {
        CommandStr += TEXT(" ");
        CommandStr += Parameter;
    }

    // 종료 문자 추가
    CommandStr += TEXT("\r");

    return CommandStr;
}

bool UPJLinkNetworkManager::ParseResponse(const FString& ResponseString, EPJLinkCommand& OutCommand, FString& OutParameter, EPJLinkResponseStatus& OutStatus)
{
    // PJLink 응답 형식: %응답클래스COMMAND=Parameter\r

    // 기본값 설정
    OutStatus = EPJLinkResponseStatus::Unknown;

    // 응답 유효성 검사
    if (ResponseString.IsEmpty() || ResponseString.Len() < 7)
    {
        return false;
    }

    // 응답 클래스 및 형식 확인
    if (!ResponseString.StartsWith(TEXT("%")) || ResponseString[2] != TEXT('='))
    {
        // 오류 응답 확인 (ERR1, ERR2, ERR3, ERR4, ERRA 등)
        if (ResponseString.StartsWith(TEXT("ERR")))
        {
            if (ResponseString.Contains(TEXT("ERR1")))
            {
                OutStatus = EPJLinkResponseStatus::UndefinedCommand;
            }
            else if (ResponseString.Contains(TEXT("ERR2")))
            {
                OutStatus = EPJLinkResponseStatus::OutOfParameter;
            }
            else if (ResponseString.Contains(TEXT("ERR3")))
            {
                OutStatus = EPJLinkResponseStatus::UnavailableTime;
            }
            else if (ResponseString.Contains(TEXT("ERR4")))
            {
                OutStatus = EPJLinkResponseStatus::ProjectorFailure;
            }
            else if (ResponseString.Contains(TEXT("ERRA")))
            {
                OutStatus = EPJLinkResponseStatus::AuthenticationError;
            }

            return true;
        }

        return false;
    }

    // 명령 파싱
    FString CommandStr = ResponseString.Mid(2, 4);

    // 명령 타입 결정
    if (CommandStr.Equals(TEXT("POWR")))
    {
        OutCommand = EPJLinkCommand::POWR;
    }
    else if (CommandStr.Equals(TEXT("INPT")))
    {
        OutCommand = EPJLinkCommand::INPT;
    }
    else if (CommandStr.Equals(TEXT("AVMT")))
    {
        OutCommand = EPJLinkCommand::AVMT;
    }
    else if (CommandStr.Equals(TEXT("ERST")))
    {
        OutCommand = EPJLinkCommand::ERST;
    }
    else if (CommandStr.Equals(TEXT("LAMP")))
    {
        OutCommand = EPJLinkCommand::LAMP;
    }
    else if (CommandStr.Equals(TEXT("INST")))
    {
        OutCommand = EPJLinkCommand::INST;
    }
    else if (CommandStr.Equals(TEXT("NAME")))
    {
        OutCommand = EPJLinkCommand::NAME;
    }
    else if (CommandStr.Equals(TEXT("INF1")))
    {
        OutCommand = EPJLinkCommand::INF1;
    }
    else if (CommandStr.Equals(TEXT("INF2")))
    {
        OutCommand = EPJLinkCommand::INF2;
    }
    else if (CommandStr.Equals(TEXT("INFO")))
    {
        OutCommand = EPJLinkCommand::INFO;
    }
    else if (CommandStr.Equals(TEXT("CLSS")))
    {
        OutCommand = EPJLinkCommand::CLSS;
    }
    else
    {
        // 알 수 없는 명령
        return false;
    }

    // 파라미터 추출
    int32 EqualsPos = ResponseString.Find(TEXT("="));
    if (EqualsPos != INDEX_NONE)
    {
        OutParameter = ResponseString.Mid(EqualsPos + 1).TrimEnd();

        // 마지막 캐리지 리턴 제거
        if (OutParameter.EndsWith(TEXT("\r")))
        {
            OutParameter = OutParameter.LeftChop(1);
        }
    }

    // 성공 응답
    OutStatus = EPJLinkResponseStatus::Success;

    return true;

    // 응답이 성공적으로 파싱되었다면 타임아웃 취소
    if (OutStatus != EPJLinkResponseStatus::Unknown)
    {
        FScopeLock Lock(&CommandTrackingLock);

        // 해당 명령이 트래킹 중인지 확인
        if (FPJLinkCommandInfo* CommandInfo = PendingCommands.Find(OutCommand))
        {
            // 응답 받음 표시
            CommandInfo->bResponseReceived = true;

            // 타임아웃 타이머 취소
            if (FTimerHandle* TimerHandle = CommandTimeoutHandles.Find(OutCommand))
            {
                if (UWorld* World = GetWorld())
                {
                    World->GetTimerManager().ClearTimer(*TimerHandle);
                }
                CommandTimeoutHandles.Remove(OutCommand);
            }

            // 적절한 시간 내에 응답이 왔는지 확인
            double ResponseTime = FPlatformTime::Seconds() - CommandInfo->SendTime;
            PJLINK_CAPTURE_DIAGNOSTIC(LastCommandDiagnosticData,
                TEXT("Response received for %s in %.2f seconds"),
                *PJLinkHelpers::CommandToString(OutCommand), ResponseTime);

            PJLINK_LOG_VERBOSE(TEXT("Response received for %s in %.2f seconds"),
                *PJLinkHelpers::CommandToString(OutCommand), ResponseTime);

            // 트래킹에서 제거
            PendingCommands.Remove(OutCommand);
        }
    }

    return (OutStatus != EPJLinkResponseStatus::Unknown);
}

void UPJLinkNetworkManager::UpdateProjectorInfo(EPJLinkCommand Command, const FString& Response)
{
    FScopeLock InfoLock(&ProjectorInfoLock);

    switch (Command)
    {
    case EPJLinkCommand::POWR:
        // 전원 상태 업데이트
        if (Response.Equals(TEXT("0")))
        {
            CurrentProjectorInfo.PowerStatus = EPJLinkPowerStatus::PoweredOff;
        }
        else if (Response.Equals(TEXT("1")))
        {
            CurrentProjectorInfo.PowerStatus = EPJLinkPowerStatus::PoweredOn;
        }
        else if (Response.Equals(TEXT("2")))
        {
            CurrentProjectorInfo.PowerStatus = EPJLinkPowerStatus::CoolingDown;
        }
        else if (Response.Equals(TEXT("3")))
        {
            CurrentProjectorInfo.PowerStatus = EPJLinkPowerStatus::WarmingUp;
        }
        else
        {
            CurrentProjectorInfo.PowerStatus = EPJLinkPowerStatus::Unknown;
        }
        break;

    case EPJLinkCommand::INPT:
        // 입력 소스 업데이트
        if (Response.StartsWith(TEXT("1")))
        {
            CurrentProjectorInfo.CurrentInputSource = EPJLinkInputSource::RGB;
        }
        else if (Response.StartsWith(TEXT("2")))
        {
            CurrentProjectorInfo.CurrentInputSource = EPJLinkInputSource::VIDEO;
        }
        else if (Response.StartsWith(TEXT("3")))
        {
            CurrentProjectorInfo.CurrentInputSource = EPJLinkInputSource::DIGITAL;
        }
        else if (Response.StartsWith(TEXT("4")))
        {
            CurrentProjectorInfo.CurrentInputSource = EPJLinkInputSource::STORAGE;
        }
        else if (Response.StartsWith(TEXT("5")))
        {
            CurrentProjectorInfo.CurrentInputSource = EPJLinkInputSource::NETWORK;
        }
        else
        {
            CurrentProjectorInfo.CurrentInputSource = EPJLinkInputSource::Unknown;
        }
        break;

    case EPJLinkCommand::NAME:
        // 프로젝터 이름 업데이트
        CurrentProjectorInfo.Name = Response;
        break;

    case EPJLinkCommand::INF1:
        // 제조사 정보 업데이트
        CurrentProjectorInfo.ManufacturerName = Response;
        break;

    case EPJLinkCommand::INF2:
        // 제품명 업데이트
        CurrentProjectorInfo.ProductName = Response;
        break;

    case EPJLinkCommand::CLSS:
        // 클래스 정보 업데이트 (1 또는 2)
        if (Response.Equals(TEXT("1")))
        {
            CurrentProjectorInfo.DeviceClass = EPJLinkClass::Class1;
        }
        else if (Response.Equals(TEXT("2")))
        {
            CurrentProjectorInfo.DeviceClass = EPJLinkClass::Class2;
        }
        break;
    }
}

void UPJLinkComponent::BeginPlay()
{
    Super::BeginPlay();

    PJLINK_LOG_INFO(TEXT("PJLink Component Begin Play: %s"), *GetOwner()->GetName());

    // 기존 객체 정리 (중복 방지)
    if (NetworkManager)
    {
        NetworkManager->OnResponseReceived.RemoveDynamic(this, &UPJLinkComponent::HandleResponseReceived);
        if (NetworkManager->OnCommunicationLog.IsBound())
        {
            NetworkManager->OnCommunicationLog.RemoveDynamic(this, &UPJLinkComponent::HandleCommunicationLog);
        }
        NetworkManager = nullptr;
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

void UPJLinkNetworkManager::LogCommunication(bool bIsSending, const FString& CommandOrResponse, const FString& RawData)
{
    // 로그에 출력
    FString DirectionStr = bIsSending ? TEXT("SEND") : TEXT("RECV");
    PJLINK_LOG_INFO(TEXT("[%s] %s: %s"), *DirectionStr, *CommandOrResponse, *RawData.TrimEnd());

    // 이벤트 발생 (큐에 넣어서 게임 스레드에서 처리)
    ResponseQueue.Enqueue(FPJLinkResponseQueueItem(
        EPJLinkCommand::POWR, // 더미 명령
        EPJLinkResponseStatus::Success, // 더미 상태
        FString::Printf(TEXT("COMMUNICATION_LOG|%d|%s|%s"), bIsSending, *CommandOrResponse, *RawData)
    ));
}

FString UPJLinkNetworkManager::GetCommandName(EPJLinkCommand Command) const
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

void UPJLinkNetworkManager::EmitError(EPJLinkErrorCode ErrorCode, const FString& ErrorMessage, EPJLinkCommand RelatedCommand)
{
    // 오류 로깅
    PJLINK_LOG_ERROR(TEXT("Error [%s]: %s"),
        *UEnum::GetValueAsString(ErrorCode),
        *ErrorMessage);

    // 마지막 오류 정보 저장
    LastErrorCode = ErrorCode;
    LastErrorMessage = ErrorMessage;

    // 오류 이벤트 발생 (큐에 추가)
    ResponseQueue.Enqueue(FPJLinkResponseQueueItem(
        RelatedCommand,
        EPJLinkResponseStatus::ProjectorFailure,
        FString::Printf(TEXT("ERROR|%d|%s"), static_cast<int32>(ErrorCode), *ErrorMessage),
        TWeakObjectPtr<UPJLinkNetworkManager>(this)
    ));
}

// PJLinkNetworkManager.cpp - 함수 추가
void UPJLinkNetworkManager::AttemptReconnect()
{
    // 재연결 시도 횟수 증가
    ReconnectAttemptCount++;

    // 최대 시도 횟수 확인
    if (MaxReconnectAttempts > 0 && ReconnectAttemptCount > MaxReconnectAttempts)
    {
        PJLINK_LOG_WARNING(TEXT("Maximum reconnect attempts (%d) reached. Giving up."), MaxReconnectAttempts);
        EmitError(EPJLinkErrorCode::ConnectionFailed,
            FString::Printf(TEXT("Failed to reconnect after %d attempts"), ReconnectAttemptCount - 1),
            EPJLinkCommand::POWR);
        ReconnectAttemptCount = 0;

        // 연결 상태 최종 업데이트
        bConnected.store(false, std::memory_order_release);

        // 오류 상태 이벤트 발생
        FPJLinkResponseQueueItem Item(
            EPJLinkCommand::POWR,
            EPJLinkResponseStatus::NoResponse,
            TEXT("Reconnection attempts exhausted"),
            TWeakObjectPtr<UPJLinkNetworkManager>(this)
        );
        ResponseQueue.Enqueue(Item);

        return;
    }

    // 연결 상태 확인
    if (bConnected.load(std::memory_order_acquire))
    {
        PJLINK_LOG_INFO(TEXT("Already connected, canceling reconnect attempt"));
        ReconnectAttemptCount = 0;
        return;
    }

    PJLINK_LOG_INFO(TEXT("Attempting to reconnect (attempt %d/%s)..."),
        ReconnectAttemptCount,
        MaxReconnectAttempts > 0 ? *FString::FromInt(MaxReconnectAttempts) : TEXT("∞"));

    // 이전 소켓 정리 확인
    {
        FScopeLock SocketLock(&SocketCriticalSection);
        if (Socket)
        {
            PJLINK_LOG_VERBOSE(TEXT("Cleaning up existing socket before reconnect"));
            ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
            if (SocketSubsystem)
            {
                Socket->Close();
                SocketSubsystem->DestroySocket(Socket);
            }
            Socket = nullptr;
        }
    }

    // 이전 연결 정보로 재연결 시도
    if (ConnectToProjector(LastProjectorInfo))
    {
        PJLINK_LOG_INFO(TEXT("Reconnection successful on attempt %d"), ReconnectAttemptCount);
        ReconnectAttemptCount = 0;

        // 연결 성공 이벤트 발생
        FPJLinkResponseQueueItem Item(
            EPJLinkCommand::POWR,
            EPJLinkResponseStatus::Success,
            TEXT("Reconnection successful"),
            TWeakObjectPtr<UPJLinkNetworkManager>(this)
        );
        ResponseQueue.Enqueue(Item);
    }
    else
    {
        // 연결 실패 - 백오프 지연 적용 (지수적 백오프)
        float BackoffDelay = ReconnectInterval * FMath::Min(1.0f, 1.0f + (ReconnectAttemptCount * 0.2f));

        PJLINK_LOG_WARNING(TEXT("Reconnection attempt %d failed. Will retry in %.1f seconds."),
            ReconnectAttemptCount, BackoffDelay);

        // 실패 이벤트 발생
        FPJLinkResponseQueueItem Item(
            EPJLinkCommand::POWR,
            EPJLinkResponseStatus::ProjectorFailure,
            FString::Printf(TEXT("Reconnection attempt %d failed"), ReconnectAttemptCount),
            TWeakObjectPtr<UPJLinkNetworkManager>(this)
        );
        ResponseQueue.Enqueue(Item);

        if (bAutoReconnect)
        {
            if (UWorld* World = GetWorld())
            {
                World->GetTimerManager().SetTimer(
                    ReconnectTimerHandle,
                    this,
                    &UPJLinkNetworkManager::AttemptReconnect,
                    BackoffDelay,
                    false);
            }
        }
        else
        {
            PJLINK_LOG_WARNING(TEXT("Reconnection attempt %d failed. Auto-reconnect is disabled."), ReconnectAttemptCount);
            ReconnectAttemptCount = 0;
        }
    }
}

bool UPJLinkNetworkManager::SendCommandWithTimeout(EPJLinkCommand Command, const FString& Parameter, float TimeoutSeconds)
{
    // 명령 전송 시간 기록
    double SendTime = FPlatformTime::Seconds();

    // 명령 트래킹 정보 기록
    FScopeLock Lock(&CommandTrackingLock);
    FPJLinkCommandInfo CommandInfo;
    CommandInfo.Command = Command;
    CommandInfo.Parameter = Parameter;
    CommandInfo.SendTime = SendTime;
    CommandInfo.TimeoutSeconds = TimeoutSeconds;
    CommandInfo.bResponseReceived = false;

    // 명령 트래킹 맵에 추가
    PendingCommands.Add(Command, CommandInfo);

    // 실제 명령 전송
    bool bResult = SendCommand(Command, Parameter);

    // 전송 실패 시 트래킹에서 제거
    if (!bResult)
    {
        PendingCommands.Remove(Command);
        return false;
    }

    // 타임아웃 타이머 설정
    if (UWorld* World = GetWorld())
    {
        FTimerDelegate TimerDelegate;
        TimerDelegate.BindUObject(this, &UPJLinkNetworkManager::HandleCommandTimeout, Command);

        FTimerHandle& TimerHandle = CommandTimeoutHandles.FindOrAdd(Command);
        World->GetTimerManager().SetTimer(TimerHandle, TimerDelegate, TimeoutSeconds, false);
    }

    return true;
}

void UPJLinkNetworkManager::HandleCommandTimeout(EPJLinkCommand Command)
{
    FScopeLock Lock(&CommandTrackingLock);

    // 해당 명령이 아직 응답을 받지 못했는지 확인
    if (FPJLinkCommandInfo* CommandInfo = PendingCommands.Find(Command))
    {
        if (!CommandInfo->bResponseReceived)
        {
            // 타임아웃 처리
            PJLINK_LOG_WARNING(TEXT("Command timeout: %s after %.1f seconds"),
                *GetCommandName(Command), CommandInfo->TimeoutSeconds);

            // 오류 이벤트 발생
            EmitError(EPJLinkErrorCode::Timeout,
                FString::Printf(TEXT("Command timeout: %s"), *GetCommandName(Command)),
                Command);

            // 응답 큐에 타임아웃 항목 추가
            FPJLinkResponseQueueItem Item(
                Command,
                EPJLinkResponseStatus::NoResponse,
                TEXT("Command timed out"),
                TWeakObjectPtr<UPJLinkNetworkManager>(this)
            );
            ResponseQueue.Enqueue(Item);
        }

        // 타임아웃 후 명령 정보 제거
        PendingCommands.Remove(Command);
    }

    // 타이머 핸들 제거
    CommandTimeoutHandles.Remove(Command);
}

// 응답 처리 시 타임아웃 취소 로직 - ParseResponse 함수 내에 추가할 코드
bool UPJLinkNetworkManager::ParseResponse(const FString& ResponseString, EPJLinkCommand& OutCommand, FString& OutParameter, EPJLinkResponseStatus& OutStatus)
{
    // 기존 파싱 로직...

    // 응답이 성공적으로 파싱되었다면 타임아웃 취소
    if (OutStatus != EPJLinkResponseStatus::Unknown)
    {
        FScopeLock Lock(&CommandTrackingLock);

        // 해당 명령이 트래킹 중인지 확인
        if (FPJLinkCommandInfo* CommandInfo = PendingCommands.Find(OutCommand))
        {
            // 응답 받음 표시
            CommandInfo->bResponseReceived = true;

            // 타임아웃 타이머 취소
            if (FTimerHandle* TimerHandle = CommandTimeoutHandles.Find(OutCommand))
            {
                if (UWorld* World = GetWorld())
                {
                    World->GetTimerManager().ClearTimer(*TimerHandle);
                }
                CommandTimeoutHandles.Remove(OutCommand);
            }

            // 적절한 시간 내에 응답이 왔는지 확인
            double ResponseTime = FPlatformTime::Seconds() - CommandInfo->SendTime;
            PJLINK_LOG_VERBOSE(TEXT("Response received for %s in %.2f seconds"),
                *GetCommandName(OutCommand), ResponseTime);

            // 트래킹에서 제거
            PendingCommands.Remove(OutCommand);
        }
    }

    return (OutStatus != EPJLinkResponseStatus::Unknown);
}

FString UPJLinkNetworkManager::GenerateDiagnosticReport() const
{
    FString Report = TEXT("PJLink Network Manager Diagnostic Report\n");
    Report += TEXT("==============================================\n\n");

    Report += TEXT("1. Connection Diagnostic Data\n");
    Report += TEXT("----------------------------\n");
    Report += ConnectionDiagnosticData.GenerateReport();
    Report += TEXT("\n\n");

    Report += TEXT("2. Last Command Diagnostic Data\n");
    Report += TEXT("------------------------------\n");
    Report += LastCommandDiagnosticData.GenerateReport();
    Report += TEXT("\n\n");

    Report += TEXT("3. Current Status\n");
    Report += TEXT("---------------\n");
    Report += FString::Printf(TEXT("Connected: %s\n"), IsConnected() ? TEXT("Yes") : TEXT("No"));

    // 프로젝터 정보 추가
    FPJLinkProjectorInfo Info = GetProjectorInfo();
    Report += FString::Printf(TEXT("Projector Name: %s\n"), *Info.Name);
    Report += FString::Printf(TEXT("IP Address: %s\n"), *Info.IPAddress);
    Report += FString::Printf(TEXT("Port: %d\n"), Info.Port);
    Report += FString::Printf(TEXT("Power Status: %s\n"), *UEnum::GetValueAsString(Info.PowerStatus));
    Report += FString::Printf(TEXT("Input Source: %s\n"), *UEnum::GetValueAsString(Info.CurrentInputSource));

    // 오류 정보 추가
    Report += TEXT("\n4. Last Error\n");
    Report += TEXT("------------\n");
    Report += FString::Printf(TEXT("Error Code: %s\n"), *UEnum::GetValueAsString(LastErrorCode));
    Report += FString::Printf(TEXT("Error Message: %s\n"), *LastErrorMessage);

    return Report;
}

// 소켓 생성 함수
bool UPJLinkNetworkManager::CreateSocket()
{
    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    if (!SocketSubsystem)
    {
        return HandleError(EPJLinkErrorCode::SocketCreationFailed, TEXT("Failed to get socket subsystem"));
    }

    // 소켓 생성
    Socket = SocketSubsystem->CreateSocket(NAME_Stream, TEXT("PJLinkSocket"), true);
    if (!Socket)
    {
        return HandleError(EPJLinkErrorCode::SocketCreationFailed, TEXT("Failed to create socket"));
    }

    // 소켓 설정 구성
    Socket->SetNoDelay(true);
    return true;
}

// 서버 주소 구성 및 연결 시도 함수
// PJLinkNetworkManager.cpp에서 ConnectToServer 함수 개선
bool UPJLinkNetworkManager::ConnectToServer(const FString& IPAddress, int32 Port, float TimeoutSeconds)
{
    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    if (!SocketSubsystem)
    {
        return HandleError(EPJLinkErrorCode::ConnectionFailed, TEXT("Failed to get socket subsystem"));
    }

    // IP 주소 파싱
    FIPv4Address IP;
    if (!FIPv4Address::Parse(IPAddress, IP))
    {
        return HandleError(EPJLinkErrorCode::InvalidIP,
            FString::Printf(TEXT("Invalid IP address: %s"), *IPAddress));
    }

    // 서버 주소 생성
    TSharedRef<FInternetAddr> ServerAddr = SocketSubsystem->CreateInternetAddr();
    ServerAddr->SetIp(IP.Value);
    ServerAddr->SetPort(Port);

    // 타임아웃 값 설정 (기본값이 너무 크면 사용자 경험이 나빠질 수 있음)
    if (TimeoutSeconds <= 0.0f)
    {
        TimeoutSeconds = 5.0f; // 기본 타임아웃
    }

    // 연결 전 소켓이 유효한지 확인
    if (!Socket)
    {
        return HandleError(EPJLinkErrorCode::SocketCreationFailed, TEXT("Socket is null before connection attempt"));
    }

    // 소켓에 타임아웃 설정
    Socket->SetNonBlocking(false);
    if (TimeoutSeconds > 0.0f)
    {
        Socket->SetReceiveTimeout(TimeoutSeconds);
        Socket->SetSendTimeout(TimeoutSeconds);
    }

    // 연결 시도
    bool bConnected = Socket->Connect(*ServerAddr);
    if (!bConnected)
    {
        ESocketErrors LastError = SocketSubsystem->GetLastErrorCode();

        // 이미 연결된 경우 처리 (가끔 발생할 수 있음)
        if (LastError == SE_EINPROGRESS || LastError == SE_EWOULDBLOCK)
        {
            // 비동기 연결 처리 (논블로킹 소켓의 경우)
            PJLINK_LOG_INFO(TEXT("Connection in progress to %s:%d"), *IPAddress, Port);
            return true;
        }

        return HandleError(EPJLinkErrorCode::ConnectionFailed,
            FString::Printf(TEXT("Failed to connect to %s:%d - Error: %s"),
                *IPAddress, Port, *SocketSubsystem->GetSocketError(LastError)));
    }

    PJLINK_LOG_INFO(TEXT("Successfully connected to %s:%d"), *IPAddress, Port);
    return true;
}

// 인증 처리 함수
bool UPJLinkNetworkManager::HandleAuthentication(const FPJLinkProjectorInfo& ProjectorInfo)
{
    // 인증 챌린지 수신
    uint8 RecvBuffer[256];
    int32 BytesRead = 0;
    bool bReadSuccess = Socket->Recv(RecvBuffer, sizeof(RecvBuffer) - 1, BytesRead, ESocketReceiveFlags::None);

    if (!bReadSuccess || BytesRead <= 0)
    {
        PJLINK_LOG_ERROR(TEXT("Failed to receive authentication challenge"));
        EmitError(EPJLinkErrorCode::AuthenticationFailed, TEXT("Failed to receive authentication challenge"));
        return false;
    }

    // NULL 종료 문자 추가
    RecvBuffer[BytesRead] = 0;
    FString ResponseString = UTF8_TO_TCHAR((const char*)RecvBuffer);

    // 인증이 필요한지 확인
    if (ResponseString.Contains(TEXT("PJLINK 1")))
    {
        // 인증 필요 없음
        PJLINK_LOG_INFO(TEXT("No authentication required"));
        return true;
    }
    else if (ResponseString.Contains(TEXT("PJLINK 0")))
    {
        // 인증 필요
        PJLINK_LOG_INFO(TEXT("Authentication required"));

        // 챌린지 추출
        FString Challenge;
        if (!ResponseString.Split(TEXT("PJLINK 0 "), nullptr, &Challenge))
        {
            PJLINK_LOG_ERROR(TEXT("Invalid authentication challenge format"));
            EmitError(EPJLinkErrorCode::AuthenticationFailed, TEXT("Invalid authentication challenge format"));
            return false;
        }

        // 공백 및 개행 제거
        Challenge = Challenge.TrimStartAndEnd();

        // MD5 해시 생성: Challenge + Password
        FString HashSource = Challenge + ProjectorInfo.Password;

        // UTF-8로 변환
        FTCHARToUTF8 Utf8HashSource(*HashSource);

        // MD5 해시 계산
        uint8 Digest[16];
        FMD5 Md5Gen;
        Md5Gen.Update((uint8*)Utf8HashSource.Get(), Utf8HashSource.Length());
        Md5Gen.Final(Digest);

        // 해시를 16진수 문자열로 변환
        FString AuthResponse;
        for (int32 i = 0; i < 16; i++)
        {
            AuthResponse += FString::Printf(TEXT("%02x"), Digest[i]);
        }

        // 인증 응답 전송
        PJLINK_LOG_VERBOSE(TEXT("Sending authentication response: %s"), *AuthResponse);

        // UTF-8로 변환
        FTCHARToUTF8 Utf8Auth(*AuthResponse);
        int32 BytesSent = 0;
        bool bSendSuccess = Socket->Send((uint8*)Utf8Auth.Get(), Utf8Auth.Length(), BytesSent);

        if (!bSendSuccess || BytesSent != Utf8Auth.Length())
        {
            PJLINK_LOG_ERROR(TEXT("Failed to send authentication response"));
            EmitError(EPJLinkErrorCode::AuthenticationFailed, TEXT("Failed to send authentication response"));
            return false;
        }

        return true;
    }
    else
    {
        // 알 수 없는 응답
        PJLINK_LOG_ERROR(TEXT("Unknown response: %s"), *ResponseString);
        EmitError(EPJLinkErrorCode::InvalidResponse,
            FString::Printf(TEXT("Unknown response: %s"), *ResponseString));
        return false;
    }
}

// 수신 스레드 시작 함수
bool UPJLinkNetworkManager::StartReceiverThread()
{
    // 스레드 중지 플래그 초기화
    bThreadStop = false;

    // 새 스레드 생성
    ReceiverThread = FRunnableThread::Create(this, TEXT("PJLinkReceiverThread"));
    if (!ReceiverThread)
    {
        PJLINK_LOG_ERROR(TEXT("Failed to create receiver thread"));
        EmitError(EPJLinkErrorCode::UnknownError, TEXT("Failed to create receiver thread"));
        return false;
    }

    return true;
}

bool UPJLinkNetworkManager::HandleError(EPJLinkErrorCode ErrorCode, const FString& ErrorMessage, EPJLinkCommand RelatedCommand)
{
    PJLINK_LOG_ERROR(TEXT("%s"), *ErrorMessage);
    EmitError(ErrorCode, ErrorMessage, RelatedCommand);
    return false;
}

// PJLinkNetworkManager.cpp에 추가
FString UPJLinkNetworkManager::GenerateDiagnosticReport() const
{
    FString Report = TEXT("PJLink Network Manager Diagnostic Report\n");
    Report += TEXT("==============================================\n\n");

    Report += TEXT("1. Connection Diagnostic Data\n");
    Report += TEXT("----------------------------\n");
    Report += ConnectionDiagnosticData.GenerateReport();
    Report += TEXT("\n\n");

    Report += TEXT("2. Last Command Diagnostic Data\n");
    Report += TEXT("------------------------------\n");
    Report += LastCommandDiagnosticData.GenerateReport();
    Report += TEXT("\n\n");

    Report += TEXT("3. Current Status\n");
    Report += TEXT("---------------\n");
    Report += FString::Printf(TEXT("Connected: %s\n"), IsConnected() ? TEXT("Yes") : TEXT("No"));

    // 프로젝터 정보 추가
    FPJLinkProjectorInfo Info = GetProjectorInfo();
    Report += FString::Printf(TEXT("Projector Name: %s\n"), *Info.Name);
    Report += FString::Printf(TEXT("IP Address: %s\n"), *Info.IPAddress);
    Report += FString::Printf(TEXT("Port: %d\n"), Info.Port);
    Report += FString::Printf(TEXT("Power Status: %s\n"), *PJLinkHelpers::PowerStatusToString(Info.PowerStatus));
    Report += FString::Printf(TEXT("Input Source: %s\n"), *PJLinkHelpers::InputSourceToString(Info.CurrentInputSource));

    // 오류 정보 추가
    Report += TEXT("\n4. Last Error\n");
    Report += TEXT("------------\n");
    Report += FString::Printf(TEXT("Error Code: %s\n"), *UEnum::GetValueAsString(LastErrorCode));
    Report += FString::Printf(TEXT("Error Message: %s\n"), *LastErrorMessage);

    return Report;
}

// PJLinkNetworkManager.cpp에 추가
bool UPJLinkNetworkManager::SendCommandWithTimeout(EPJLinkCommand Command, const FString& Parameter, float TimeoutSeconds)
{
    // 명령 전송 시간 기록
    double SendTime = FPlatformTime::Seconds();

    // 명령 트래킹 정보 기록
    FScopeLock Lock(&CommandTrackingLock);
    FPJLinkCommandInfo CommandInfo;
    CommandInfo.Command = Command;
    CommandInfo.Parameter = Parameter;
    CommandInfo.SendTime = SendTime;
    CommandInfo.TimeoutSeconds = TimeoutSeconds;
    CommandInfo.bResponseReceived = false;

    PJLINK_CAPTURE_DIAGNOSTIC(LastCommandDiagnosticData,
        TEXT("Setting up command timeout tracking: %s, Timeout: %.1f seconds"),
        *PJLinkHelpers::CommandToString(Command), TimeoutSeconds);

    // 명령 트래킹 맵에 추가
    PendingCommands.Add(Command, CommandInfo);

    // 실제 명령 전송
    bool bResult = SendCommand(Command, Parameter);

    // 전송 실패 시 트래킹에서 제거
    if (!bResult)
    {
        PJLINK_CAPTURE_DIAGNOSTIC(LastCommandDiagnosticData,
            TEXT("Command send failed, removing from tracking: %s"),
            *PJLinkHelpers::CommandToString(Command));
        PendingCommands.Remove(Command);
        return false;
    }

    // 타임아웃 타이머 설정
    if (UWorld* World = GetWorld())
    {
        FTimerDelegate TimerDelegate;
        TimerDelegate.BindUObject(this, &UPJLinkNetworkManager::HandleCommandTimeout, Command);

        FTimerHandle& TimerHandle = CommandTimeoutHandles.FindOrAdd(Command);
        World->GetTimerManager().SetTimer(TimerHandle, TimerDelegate, TimeoutSeconds, false);

        PJLINK_CAPTURE_DIAGNOSTIC(LastCommandDiagnosticData,
            TEXT("Timeout timer set for command: %s (%.1f seconds)"),
            *PJLinkHelpers::CommandToString(Command), TimeoutSeconds);
    }
    else
    {
        PJLINK_CAPTURE_DIAGNOSTIC(LastCommandDiagnosticData,
            TEXT("Failed to set timeout timer - World not available"));
    }

    return true;
}

// PJLinkNetworkManager.cpp에서
void UPJLinkNetworkManager::HandleCommandTimeout(EPJLinkCommand Command)
{
    FScopeLock Lock(&CommandTrackingLock);

    // 해당 명령이 아직 응답을 받지 못했는지 확인
    FPJLinkCommandInfo* CommandInfo = PendingCommands.Find(Command);
    if (!CommandInfo)
    {
        // 이미 제거된 경우 (응답을 받았거나 다른 이유로 제거됨)
        PJLINK_LOG_VERBOSE(TEXT("Timeout handler called for command %s, but command is not pending anymore"),
            *PJLinkHelpers::CommandToString(Command));
        return;
    }

    if (!CommandInfo->bResponseReceived)
    {
        // 타임아웃 처리
        FString ErrorMessage = FString::Printf(TEXT("Command timeout: %s after %.1f seconds"),
            *PJLinkHelpers::CommandToString(Command), CommandInfo->TimeoutSeconds);

        PJLINK_LOG_WARNING(TEXT("%s"), *ErrorMessage);
        PJLINK_CAPTURE_DIAGNOSTIC(LastCommandDiagnosticData, TEXT("%s"), *ErrorMessage);

        // 오류 이벤트 발생
        EmitError(EPJLinkErrorCode::Timeout, ErrorMessage, Command);

        // 응답 큐에 타임아웃 항목 추가
        FPJLinkResponseQueueItem Item(
            Command,
            EPJLinkResponseStatus::NoResponse,
            TEXT("Command timed out"),
            TWeakObjectPtr<UPJLinkNetworkManager>(this)
        );
        ResponseQueue.Enqueue(Item);
    }

    // 타임아웃 후 명령 정보 제거
    PendingCommands.Remove(Command);

    // 타이머 핸들 제거
    FTimerHandle* TimerHandle = CommandTimeoutHandles.Find(Command);
    if (TimerHandle)
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(*TimerHandle);
        }
        CommandTimeoutHandles.Remove(Command);
    }
}

// PJLinkNetworkManager.cpp에 추가
void UPJLinkNetworkManager::AttemptReconnect()
{
    // 재연결 시도 횟수 증가
    ReconnectAttemptCount++;

    // 최대 시도 횟수 확인
    if (MaxReconnectAttempts > 0 && ReconnectAttemptCount > MaxReconnectAttempts)
    {
        PJLINK_CAPTURE_DIAGNOSTIC(ConnectionDiagnosticData,
            TEXT("Maximum reconnect attempts (%d) reached. Giving up."), MaxReconnectAttempts);
        PJLINK_LOG_WARNING(TEXT("Maximum reconnect attempts (%d) reached. Giving up."), MaxReconnectAttempts);

        EmitError(EPJLinkErrorCode::ConnectionFailed,
            FString::Printf(TEXT("Failed to reconnect after %d attempts"), ReconnectAttemptCount - 1),
            EPJLinkCommand::POWR);

        ReconnectAttemptCount = 0;

        // 연결 상태 최종 업데이트
        bConnected.store(false, std::memory_order_release);

        // 오류 상태 이벤트 발생
        FPJLinkResponseQueueItem Item(
            EPJLinkCommand::POWR,
            EPJLinkResponseStatus::NoResponse,
            TEXT("Reconnection attempts exhausted")
        );
        ResponseQueue.Enqueue(Item);

        return;
    }

    // 연결 상태 확인
    if (bConnected.load(std::memory_order_acquire))
    {
        PJLINK_CAPTURE_DIAGNOSTIC(ConnectionDiagnosticData,
            TEXT("Already connected, canceling reconnect attempt"));
        PJLINK_LOG_INFO(TEXT("Already connected, canceling reconnect attempt"));
        ReconnectAttemptCount = 0;
        return;
    }

    PJLINK_CAPTURE_DIAGNOSTIC(ConnectionDiagnosticData,
        TEXT("Attempting to reconnect (attempt %d/%s)..."),
        ReconnectAttemptCount,
        MaxReconnectAttempts > 0 ? *FString::FromInt(MaxReconnectAttempts) : TEXT("∞"));

    PJLINK_LOG_INFO(TEXT("Attempting to reconnect (attempt %d/%s)..."),
        ReconnectAttemptCount,
        MaxReconnectAttempts > 0 ? *FString::FromInt(MaxReconnectAttempts) : TEXT("∞"));

    // 이전 소켓 정리 확인
    {
        FScopeLock SocketLock(&SocketCriticalSection);
        if (Socket)
        {
            PJLINK_CAPTURE_DIAGNOSTIC(ConnectionDiagnosticData,
                TEXT("Cleaning up existing socket before reconnect"));
            PJLINK_LOG_VERBOSE(TEXT("Cleaning up existing socket before reconnect"));

            ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
            if (SocketSubsystem)
            {
                Socket->Close();
                SocketSubsystem->DestroySocket(Socket);
            }
            Socket = nullptr;
        }
    }

    // 이전 연결 정보로 재연결 시도
    if (ConnectToProjector(LastProjectorInfo))
    {
        PJLINK_CAPTURE_DIAGNOSTIC(ConnectionDiagnosticData,
            TEXT("Reconnection successful on attempt %d"), ReconnectAttemptCount);
        PJLINK_LOG_INFO(TEXT("Reconnection successful on attempt %d"), ReconnectAttemptCount);
        ReconnectAttemptCount = 0;

        // 연결 성공 이벤트 발생
        FPJLinkResponseQueueItem Item(
            EPJLinkCommand::POWR,
            EPJLinkResponseStatus::Success,
            TEXT("Reconnection successful")
        );
        ResponseQueue.Enqueue(Item);
    }
    else
    {
        // 연결 실패 - 백오프 지연 적용 (지수적 백오프)
        float BackoffDelay = ReconnectInterval * FMath::Min(1.0f, 1.0f + (ReconnectAttemptCount * 0.2f));

        PJLINK_CAPTURE_DIAGNOSTIC(ConnectionDiagnosticData,
            TEXT("Reconnection attempt %d failed. Will retry in %.1f seconds."),
            ReconnectAttemptCount, BackoffDelay);

        PJLINK_LOG_WARNING(TEXT("Reconnection attempt %d failed. Will retry in %.1f seconds."),
            ReconnectAttemptCount, BackoffDelay);

        // 실패 이벤트 발생
        FPJLinkResponseQueueItem Item(
            EPJLinkCommand::POWR,
            EPJLinkResponseStatus::ProjectorFailure,
            FString::Printf(TEXT("Reconnection attempt %d failed"), ReconnectAttemptCount)
        );
        ResponseQueue.Enqueue(Item);

        if (bAutoReconnect)
        {
            if (UWorld* World = GetWorld())
            {
                World->GetTimerManager().SetTimer(
                    ReconnectTimerHandle,
                    this,
                    &UPJLinkNetworkManager::AttemptReconnect,
                    BackoffDelay,
                    false);
            }
        }
        else
        {
            PJLINK_CAPTURE_DIAGNOSTIC(ConnectionDiagnosticData,
                TEXT("Reconnection attempt %d failed. Auto-reconnect is disabled."), ReconnectAttemptCount);
            PJLINK_LOG_WARNING(TEXT("Reconnection attempt %d failed. Auto-reconnect is disabled."), ReconnectAttemptCount);
            ReconnectAttemptCount = 0;
        }
    }
}