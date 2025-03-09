// PJLinkNetworkManager.h
#pragma once

#include "CoreMinimal.h"
#include "PJLinkTypes.h"
#include "HAL/Runnable.h"
#include "UObject/NoExportTypes.h"
#include "PJLinkNetworkManager.generated.h"

// 필요한 전방 선언 
class FSocket;
class FRunnableThread;

// 네트워크 응답 대리자
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPJLinkResponseDelegate, EPJLinkCommand, Command, EPJLinkResponseStatus, Status, const FString&, Response);

// 명령 추적을 위한 구조체
struct FPJLinkCommandInfo
{
    EPJLinkCommand Command;
    FString Parameter;
    double SendTime;
    float TimeoutSeconds;
    bool bResponseReceived;
};

/**
 * PJLink 네트워크 통신을 관리하는 클래스
 */
UCLASS(BlueprintType, Blueprintable)
class PJLINK_API UPJLinkNetworkManager : public UObject, public FRunnable
{
    GENERATED_BODY()

public:
    UPJLinkNetworkManager();
    virtual ~UPJLinkNetworkManager();

    // 플러그인 종료 시 호출
    void Shutdown();

    // 프로젝터에 연결
    UFUNCTION(BlueprintCallable, Category = "PJLink|Network")
    bool ConnectToProjector(const FPJLinkProjectorInfo& ProjectorInfo, float TimeoutSeconds = 5.0f);

    // 프로젝터 연결 해제
    UFUNCTION(BlueprintCallable, Category = "PJLink|Network")
    void DisconnectFromProjector();

    // 프로젝터에 명령 전송
    UFUNCTION(BlueprintCallable, Category = "PJLink|Network")
    bool SendCommand(EPJLinkCommand Command, const FString& Parameter = "");

    // 현재 연결된 프로젝터 정보 가져오기
    UFUNCTION(BlueprintCallable, Category = "PJLink|Network")
    FPJLinkProjectorInfo GetProjectorInfo() const;

    // 연결 상태 확인
    UFUNCTION(BlueprintCallable, Category = "PJLink|Network")
    bool IsConnected() const;

    // 프로젝터 전원 켜기
    UFUNCTION(BlueprintCallable, Category = "PJLink|Control")
    bool PowerOn();

    // 프로젝터 전원 끄기
    UFUNCTION(BlueprintCallable, Category = "PJLink|Control")
    bool PowerOff();

    // 입력 소스 변경
    UFUNCTION(BlueprintCallable, Category = "PJLink|Control")
    bool SwitchInputSource(EPJLinkInputSource InputSource);

    // 프로젝터 상태 요청
    UFUNCTION(BlueprintCallable, Category = "PJLink|Control")
    bool RequestStatus();

    // 응답 이벤트
    UPROPERTY(BlueprintAssignable, Category = "PJLink|Events")
    FPJLinkResponseDelegate OnResponseReceived;

    // 통신 로그 이벤트
    UPROPERTY(BlueprintAssignable, Category = "PJLink|Debug")
    FPJLinkCommunicationLogDelegate OnCommunicationLog;

    // 디버깅 관련 속성 추가
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|Debug")
    bool bLogCommunication = false;

    // private 영역에 로깅 함수 추가
    void LogCommunication(bool bIsSending, const FString& CommandOrResponse, const FString& RawData);

    // 확장된 오류 이벤트
    UPROPERTY(BlueprintAssignable, Category = "PJLink|Events")
    FPJLinkExtendedErrorDelegate OnExtendedError;

    // 오류 발생 함수
    void EmitError(EPJLinkErrorCode ErrorCode, const FString& ErrorMessage, EPJLinkCommand RelatedCommand = EPJLinkCommand::POWR);

    // 마지막 오류 메시지 가져오기
    UFUNCTION(BlueprintPure, Category = "PJLink|Error")
    FString GetLastErrorMessage() const { return LastErrorMessage; }

    // 자동 재연결 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|Connection")
    bool bAutoReconnect = true;

    // 재연결 시도 간격 (초)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|Connection", meta = (EditCondition = "bAutoReconnect", UIMin = 1.0, UIMax = 10.0))
    float ReconnectInterval = 3.0f;

    // 최대 재연결 시도 횟수 (0 = 무제한)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|Connection", meta = (EditCondition = "bAutoReconnect", UIMin = 0, UIMax = 10))
    int32 MaxReconnectAttempts = 3;

    // 타임아웃 처리 함수
    UFUNCTION(BlueprintCallable, Category = "PJLink|Network")
    bool SendCommandWithTimeout(EPJLinkCommand Command, const FString& Parameter = TEXT(""), float TimeoutSeconds = 5.0f);

    // 진단 데이터 가져오기 함수 추가
    UFUNCTION(BlueprintCallable, Category = "PJLink|Diagnostic")
    FPJLinkDiagnosticData GetConnectionDiagnosticData() const { return ConnectionDiagnosticData; }

    UFUNCTION(BlueprintCallable, Category = "PJLink|Diagnostic")
    FPJLinkDiagnosticData GetLastCommandDiagnosticData() const { return LastCommandDiagnosticData; }

    // 진단 보고서 생성
    UFUNCTION(BlueprintCallable, Category = "PJLink|Diagnostic")
    FString GenerateDiagnosticReport() const;

protected:
    // FRunnable 인터페이스 구현
    virtual bool Init() override;
    virtual uint32 Run() override;
    virtual void Stop() override;
    virtual void Exit() override;

    // 명령 문자열 생성
    FString BuildCommandString(EPJLinkCommand Command, const FString& Parameter = "");

    // 응답 파싱
    // PJLinkNetworkManager.h에서 수정
    bool ParseResponse(const FString& ResponseString, EPJLinkCommand& Command, FString& Parameter, EPJLinkResponseStatus& Status);

    // 응답으로부터 프로젝터 정보 업데이트
    void UpdateProjectorInfo(EPJLinkCommand Command, const FString& Response);

    // PJLink 인증 처리
    bool HandleAuthentication(const FString& Challenge);

    // 타임아웃 처리 함수 (내부용)
    void HandleCommandTimeout(EPJLinkCommand Command);

    // 소켓 생성 함수
    bool CreateSocket();

    // 서버 주소 구성 및 연결 시도 함수
    bool ConnectToServer(const FString& IPAddress, int32 Port, float TimeoutSeconds);

    // 인증 처리 함수
    bool HandleAuthentication(const FPJLinkProjectorInfo& ProjectorInfo);

    // 수신 스레드 시작 함수
    bool StartReceiverThread();

    // 에러 처리 헬퍼 함수 (코드 중복 제거)
    bool HandleError(EPJLinkErrorCode ErrorCode, const FString& ErrorMessage, EPJLinkCommand RelatedCommand = EPJLinkCommand::POWR);

    // 타임아웃 처리 함수
    void HandleCommandTimeout(EPJLinkCommand Command);

    // 재연결 시도 함수
    void AttemptReconnect();

private:
    // 응답 큐 구조체 정의 (private 내부에 위치)
    struct FPJLinkResponseQueueItem
    {
        EPJLinkCommand Command;
        EPJLinkResponseStatus Status;
        FString ResponseText;
        TWeakObjectPtr<UPJLinkNetworkManager> WeakThis;

        FPJLinkResponseQueueItem() {}

        FPJLinkResponseQueueItem(
            EPJLinkCommand InCommand,
            EPJLinkResponseStatus InStatus,
            const FString& InResponseText,
            TWeakObjectPtr<UPJLinkNetworkManager> InWeakThis = nullptr)
            : Command(InCommand), Status(InStatus), ResponseText(InResponseText), WeakThis(InWeakThis) {
        }
    };

    // 응답 큐
    TQueue<FPJLinkResponseQueueItem> ResponseQueue;

    // 큐 동기화를 위한 임계 영역
    FCriticalSection ResponseQueueLock;

    // 명령 추적 맵 및 타이머 핸들
    TMap<EPJLinkCommand, FPJLinkCommandInfo> PendingCommands;
    TMap<EPJLinkCommand, FTimerHandle> CommandTimeoutHandles;
    FCriticalSection CommandTrackingLock;

    // 소켓 및 스레드 관련 변수
    FSocket* Socket;
    FRunnableThread* ReceiverThread;
    TAtomic<bool> bThreadStop;
    TAtomic<bool> bConnected;

    // 프로젝터 정보 - 락을 통해 보호됨
    FPJLinkProjectorInfo CurrentProjectorInfo;

    // 임계 영역들
    mutable FCriticalSection SocketCriticalSection;
    mutable FCriticalSection ProjectorInfoLock;
    mutable FCriticalSection ResponseQueueLock;

    // 명령어 대기열
    TQueue<TPair<EPJLinkCommand, FString>> CommandQueue;
 
    // 큐 처리 타이머
    FTimerHandle ResponseQueueTimerHandle;

    // 큐 처리 함수
    void ProcessResponseQueue();

    // 마지막 오류 정보
    EPJLinkErrorCode LastErrorCode;
    FString LastErrorMessage;

    // 재연결 관련 변수
    int32 ReconnectAttemptCount = 0;
    FTimerHandle ReconnectTimerHandle;
    FPJLinkProjectorInfo LastProjectorInfo;

    // 진단 데이터
    FPJLinkDiagnosticData ConnectionDiagnosticData;
    FPJLinkDiagnosticData LastCommandDiagnosticData;
     
 };