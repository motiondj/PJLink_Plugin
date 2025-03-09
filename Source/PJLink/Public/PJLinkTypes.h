// PJLinkTypes.h
#pragma once

#include "CoreMinimal.h"
#include "PJLinkTypes.generated.h"

// PJLink 명령 클래스 (Class)
UENUM(BlueprintType)
enum class EPJLinkClass : uint8
{
    Class1 UMETA(DisplayName = "Class 1"),
    Class2 UMETA(DisplayName = "Class 2")
};

// PJLink 전원 상태
UENUM(BlueprintType)
enum class EPJLinkPowerStatus : uint8
{
    PoweredOff UMETA(DisplayName = "Powered Off", ToolTip = "Projector is completely off"),
    PoweredOn UMETA(DisplayName = "Powered On", ToolTip = "Projector is on and ready for use"),
    CoolingDown UMETA(DisplayName = "Cooling Down", ToolTip = "Projector is cooling down after being turned off"),
    WarmingUp UMETA(DisplayName = "Warming Up", ToolTip = "Projector is warming up and preparing to display"),
    Unknown UMETA(DisplayName = "Unknown State", ToolTip = "The projector's power state is unknown")
};

// PJLink 입력 소스
UENUM(BlueprintType)
enum class EPJLinkInputSource : uint8
{
    RGB UMETA(DisplayName = "RGB (1)", ToolTip = "Computer or RGB input"),
    VIDEO UMETA(DisplayName = "Video (2)", ToolTip = "Composite video input"),
    DIGITAL UMETA(DisplayName = "Digital (3)", ToolTip = "Digital input like HDMI or DVI"),
    STORAGE UMETA(DisplayName = "Storage (4)", ToolTip = "USB storage or internal memory"),
    NETWORK UMETA(DisplayName = "Network (5)", ToolTip = "Network streaming or web content"),
    Unknown UMETA(DisplayName = "Unknown Input", ToolTip = "Input source is unknown")
};

// PJLink 프로젝터 정보 구조체
USTRUCT(BlueprintType)
struct PJLINK_API FPJLinkProjectorInfo
{
    GENERATED_BODY()

    // 프로젝터 기본 정보
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink")
    FString Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink")
    FString IPAddress;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink")
    int32 Port = 4352; // PJLink 기본 포트

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink")
    EPJLinkClass DeviceClass = EPJLinkClass::Class1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink")
    bool bRequiresAuthentication = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink", meta = (EditCondition = "bRequiresAuthentication"))
    FString Password;

    // 프로젝터 상태 정보
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PJLink|Status")
    EPJLinkPowerStatus PowerStatus = EPJLinkPowerStatus::Unknown;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PJLink|Status")
    EPJLinkInputSource CurrentInputSource = EPJLinkInputSource::Unknown;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PJLink|Status")
    FString VersionInfo;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PJLink|Status")
    FString ManufacturerName;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PJLink|Status")
    FString ProductName;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PJLink|Status")
    bool bIsConnected = false;

    // 기본 생성자
    FPJLinkProjectorInfo()
    {
    }

    // 필수 정보만 받는 생성자
    FPJLinkProjectorInfo(const FString& InName, const FString& InIPAddress, int32 InPort = 4352)
        : Name(InName)
        , IPAddress(InIPAddress)
        , Port(InPort)
    {
    }
};

/**
* 프로젝터 정보 생성 (블루프린트에서 호출 가능)
*/
UFUNCTION(BlueprintPure, Category = "PJLink", meta = (DisplayName = "Create Projector Info"))
static FPJLinkProjectorInfo CreateProjectorInfo(
    const FString& Name,
    const FString& IPAddress,
    int32 Port = 4352,
    EPJLinkClass DeviceClass = EPJLinkClass::Class1,
    bool bRequiresAuthentication = false,
    const FString& Password = TEXT("")
);

// PJLink 명령 종류
UENUM(BlueprintType)
enum class EPJLinkCommand : uint8
{
    POWR UMETA(DisplayName = "Power Control"),
    INPT UMETA(DisplayName = "Input Switch"),
    AVMT UMETA(DisplayName = "AV Mute"),
    ERST UMETA(DisplayName = "Error Status"),
    LAMP UMETA(DisplayName = "Lamp Status"),
    INST UMETA(DisplayName = "Input Terminal"),
    NAME UMETA(DisplayName = "Projector Name"),
    INF1 UMETA(DisplayName = "Manufacturer Name"),
    INF2 UMETA(DisplayName = "Product Name"),
    INFO UMETA(DisplayName = "Other Information"),
    CLSS UMETA(DisplayName = "Class Information")
};

// PJLink 응답 상태
UENUM(BlueprintType)
enum class EPJLinkResponseStatus : uint8
{
    Success UMETA(DisplayName = "Success"),
    UndefinedCommand UMETA(DisplayName = "Undefined Command"),
    OutOfParameter UMETA(DisplayName = "Out of Parameter"),
    UnavailableTime UMETA(DisplayName = "Unavailable Time"),
    ProjectorFailure UMETA(DisplayName = "Projector Failure"),
    AuthenticationError UMETA(DisplayName = "Authentication Error"),
    NoResponse UMETA(DisplayName = "No Response"),
    Unknown UMETA(DisplayName = "Unknown")
};

// 전원 상태 변경 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPJLinkPowerStatusChangedDelegate,
    EPJLinkPowerStatus, OldStatus,
    EPJLinkPowerStatus, NewStatus);

// 입력 소스 변경 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPJLinkInputSourceChangedDelegate,
    EPJLinkInputSource, OldSource,
    EPJLinkInputSource, NewSource);

// 연결 상태 변경 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPJLinkConnectionChangedDelegate,
    bool, bIsConnected);

// 에러 상태 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPJLinkErrorStatusDelegate,
    const FString&, ErrorMessage);

// 프로젝터 준비 완료 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPJLinkProjectorReadyDelegate);

// 명령 완료 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPJLinkCommandCompletedDelegate,
    EPJLinkCommand, Command,
    bool, bSuccess);

// 명령 완료 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPJLinkCommandCompletedDelegate,
    EPJLinkCommand, Command,
    bool, bSuccess);

/**
 * 프로젝터 상태 정보 구조체
 */
USTRUCT(BlueprintType)
struct PJLINK_API FPJLinkProjectorStatus
{
    GENERATED_BODY()

    // 프로젝터 ID
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PJLink|Status")
    FString ProjectorID;

    // 프로젝터 이름
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PJLink|Status")
    FString ProjectorName;

    // IP 주소
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PJLink|Status")
    FString IPAddress;

    // 포트
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PJLink|Status")
    int32 Port;

    // 현재 상태
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PJLink|Status")
    FPJLinkProjectorStatusRecord CurrentStatus;

    // 이전 상태 기록 (최대 10개)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PJLink|Status")
    TArray<FPJLinkProjectorStatusRecord> StatusHistory;

    // 마지막 응답 시간
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PJLink|Status")
    FDateTime LastResponseTime;

    // 마지막 명령 시간
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PJLink|Status")
    FDateTime LastCommandTime;

    // 응답 시간 (밀리초)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PJLink|Status")
    int32 ResponseTimeMs;

    // 연결 실패 횟수
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PJLink|Status")
    int32 ConnectionFailureCount;

    // 명령 실패 횟수
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PJLink|Status")
    int32 CommandFailureCount;

    // 상태가 정상인지 확인
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PJLink|Status")
    bool bIsHealthy;

    // 기본 생성자
    FPJLinkProjectorStatus()
        : Port(4352)
        , ResponseTimeMs(0)
        , ConnectionFailureCount(0)
        , CommandFailureCount(0)
        , bIsHealthy(true)
    {
    }

    // 프로젝터 정보로 생성하는 생성자
    FPJLinkProjectorStatus(const FString& InProjectorID, const FPJLinkProjectorInfo& ProjectorInfo)
        : ProjectorID(InProjectorID)
        , ProjectorName(ProjectorInfo.Name)
        , IPAddress(ProjectorInfo.IPAddress)
        , Port(ProjectorInfo.Port)
        , ResponseTimeMs(0)
        , ConnectionFailureCount(0)
        , CommandFailureCount(0)
        , bIsHealthy(true)
    {
        // 현재 상태 초기화
        CurrentStatus = FPJLinkProjectorStatusRecord(
            ProjectorInfo.PowerStatus,
            ProjectorInfo.CurrentInputSource,
            ProjectorInfo.bIsConnected
        );
    }

    // 상태 업데이트 (기존 상태가 이전 이력으로 이동)
    void UpdateStatus(EPJLinkPowerStatus InPowerStatus, EPJLinkInputSource InInputSource, bool bInIsConnected, const FString& InErrorMessage = TEXT(""))
    {
        // 이전 상태를 이력에 추가
        StatusHistory.Insert(CurrentStatus, 0);

        // 이력 크기 제한
        if (StatusHistory.Num() > 10)
        {
            StatusHistory.RemoveAt(10, StatusHistory.Num() - 10);
        }

        // 새 상태 설정
        CurrentStatus = FPJLinkProjectorStatusRecord(
            InPowerStatus,
            InInputSource,
            bInIsConnected,
            InErrorMessage
        );

        // 마지막 응답 시간 업데이트
        LastResponseTime = FDateTime::Now();

        // 응답 시간 계산
        if (LastCommandTime != FDateTime())
        {
            FTimespan ResponseSpan = LastResponseTime - LastCommandTime;
            ResponseTimeMs = FMath::FloorToInt(ResponseSpan.GetTotalMilliseconds());
        }

        // 상태가 정상인지 확인
        bIsHealthy = bInIsConnected && InErrorMessage.IsEmpty();
    }

    // 명령 전송 기록
    void RecordCommandSent()
    {
        LastCommandTime = FDateTime::Now();
    }

    // 연결 실패 기록
    void RecordConnectionFailure()
    {
        ConnectionFailureCount++;
        bIsHealthy = false;

        // 에러 메시지 업데이트
        CurrentStatus.ErrorMessage = TEXT("Connection failed");
    }

    // 명령 실패 기록
    void RecordCommandFailure(const FString& ErrorMessage)
    {
        CommandFailureCount++;

        // 에러 메시지 업데이트
        CurrentStatus.ErrorMessage = ErrorMessage;

        // 일정 횟수 이상 실패하면 상태를 비정상으로 표시
        if (CommandFailureCount > 3)
        {
            bIsHealthy = false;
        }
    }

    // 마지막 응답 이후 경과 시간 (초)
    float GetTimeSinceLastResponse() const
    {
        if (LastResponseTime == FDateTime())
        {
            return -1.0f; // 응답 없음
        }

        FTimespan Span = FDateTime::Now() - LastResponseTime;
        return Span.GetTotalSeconds();
    }

    // 마지막 명령 이후 경과 시간 (초)
    float GetTimeSinceLastCommand() const
    {
        if (LastCommandTime == FDateTime())
        {
            return -1.0f; // 명령 없음
        }

        FTimespan Span = FDateTime::Now() - LastCommandTime;
        return Span.GetTotalSeconds();
    }

    // 카운터 리셋
    void ResetCounters()
    {
        ConnectionFailureCount = 0;
        CommandFailureCount = 0;
        bIsHealthy = true;
    }
};

/**
 * 프로젝터 상태 변경 이벤트용 델리게이트
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPJLinkProjectorStatusChangedDelegate, const FPJLinkProjectorStatus&, ProjectorStatus);

// PJLink 오류 코드
UENUM(BlueprintType)
enum class EPJLinkErrorCode : uint8
{
    None UMETA(DisplayName = "No Error"),
    ConnectionFailed UMETA(DisplayName = "Connection Failed"),
    AuthenticationFailed UMETA(DisplayName = "Authentication Failed"),
    InvalidIP UMETA(DisplayName = "Invalid IP Address"),
    SocketCreationFailed UMETA(DisplayName = "Socket Creation Failed"),
    SocketError UMETA(DisplayName = "Socket Error"),
    CommandFailed UMETA(DisplayName = "Command Failed"),
    Timeout UMETA(DisplayName = "Timeout"),
    InvalidResponse UMETA(DisplayName = "Invalid Response"),
    ProjectorError UMETA(DisplayName = "Projector Error"),
    UnknownError UMETA(DisplayName = "Unknown Error")
};

// 확장된 오류 이벤트 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPJLinkExtendedErrorDelegate,
    EPJLinkErrorCode, ErrorCode,
    const FString&, ErrorMessage,
    EPJLinkCommand, RelatedCommand);

namespace PJLinkHelpers
{
    /** 전원 상태를 문자열로 변환 */
    PJLINK_API FString PowerStatusToString(EPJLinkPowerStatus PowerStatus);

    /** 입력 소스를 문자열로 변환 */
    PJLINK_API FString InputSourceToString(EPJLinkInputSource InputSource);

    /** 명령을 문자열로 변환 */
    PJLINK_API FString CommandToString(EPJLinkCommand Command);

    /** 응답 상태를 문자열로 변환 */
    PJLINK_API FString ResponseStatusToString(EPJLinkResponseStatus Status);
}

/**
 * 프로젝터 그룹 정보를 저장하는 구조체
 */
USTRUCT(BlueprintType)
struct PJLINK_API FPJLinkProjectorGroup
{
    GENERATED_BODY()

    // 그룹 이름
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|Group")
    FString GroupName;

    // 그룹 설명
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|Group")
    FString Description;

    // 그룹 색상 (UI 표시용)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|Group")
    FLinearColor GroupColor;

    // 그룹 내 프로젝터 ID 목록
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PJLink|Group")
    TArray<FString> ProjectorIDs;

    // 기본 생성자
    FPJLinkProjectorGroup()
        : GroupName(TEXT("New Group"))
        , Description(TEXT(""))
        , GroupColor(FLinearColor(0.0f, 0.4f, 0.7f, 1.0f))
    {
    }

    // 이름으로 생성하는 생성자
    FPJLinkProjectorGroup(const FString& InGroupName)
        : GroupName(InGroupName)
        , Description(TEXT(""))
        , GroupColor(FLinearColor(0.0f, 0.4f, 0.7f, 1.0f))
    {
    }

    // 전체 정보로 생성하는 생성자
    FPJLinkProjectorGroup(const FString& InGroupName, const FString& InDescription, const FLinearColor& InColor)
        : GroupName(InGroupName)
        , Description(InDescription)
        , GroupColor(InColor)
    {
    }

    // 프로젝터 ID 추가
    void AddProjectorID(const FString& ProjectorID)
    {
        if (!ProjectorIDs.Contains(ProjectorID))
        {
            ProjectorIDs.Add(ProjectorID);
        }
    }

    // 프로젝터 ID 제거
    bool RemoveProjectorID(const FString& ProjectorID)
    {
        return ProjectorIDs.Remove(ProjectorID) > 0;
    }

    // 프로젝터 ID 포함 여부 확인
    bool ContainsProjectorID(const FString& ProjectorID) const
    {
        return ProjectorIDs.Contains(ProjectorID);
    }

    // 그룹에 있는 프로젝터 수
    int32 GetProjectorCount() const
    {
        return ProjectorIDs.Num();
    }
};

/**
 * 그룹 관련 이벤트용 델리게이트
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPJLinkGroupChangedDelegate, const FString&, GroupName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPJLinkProjectorGroupAssignmentChangedDelegate, const FString&, ProjectorID, const FString&, GroupName);

bool UPJLinkManagerComponent::IsProjectorInGroup(UPJLinkComponent* ProjectorComponent, const FString& GroupName) const
{
    if (!ProjectorComponent || !GroupMap.Contains(GroupName))
    {
        return false;
    }

    FPJLinkProjectorInfo ProjectorInfo = ProjectorComponent->GetProjectorInfo();
    FString ProjectorID = GenerateProjectorID(ProjectorInfo);

    return GroupMap[GroupName].ContainsProjectorID(ProjectorID);
}

/**
 * 프로젝터 상태 기록을 저장하는 구조체
 */
USTRUCT(BlueprintType)
struct PJLINK_API FPJLinkProjectorStatusRecord
{
    GENERATED_BODY()

    // 타임스탬프
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PJLink|Status")
    FDateTime Timestamp;

    // 전원 상태
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PJLink|Status")
    EPJLinkPowerStatus PowerStatus;

    // 입력 소스
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PJLink|Status")
    EPJLinkInputSource InputSource;

    // 연결 상태
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PJLink|Status")
    bool bIsConnected;

    // 오류 메시지 (있는 경우)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PJLink|Status")
    FString ErrorMessage;

    // 기본 생성자
    FPJLinkProjectorStatusRecord()
        : Timestamp(FDateTime::Now())
        , PowerStatus(EPJLinkPowerStatus::Unknown)
        , InputSource(EPJLinkInputSource::Unknown)
        , bIsConnected(false)
        , ErrorMessage(TEXT(""))
    {
    }

    // 정보로 생성하는 생성자
    FPJLinkProjectorStatusRecord(EPJLinkPowerStatus InPowerStatus, EPJLinkInputSource InInputSource, bool bInIsConnected, const FString& InErrorMessage = TEXT(""))
        : Timestamp(FDateTime::Now())
        , PowerStatus(InPowerStatus)
        , InputSource(InInputSource)
        , bIsConnected(bInIsConnected)
        , ErrorMessage(InErrorMessage)
    {
    }
};

void UPJLinkManagerComponent::HandleConnectionChanged(UPJLinkComponent* ProjectorComponent, bool bIsConnected)
{
    if (!ProjectorComponent)
    {
        return;
    }

    FPJLinkProjectorInfo ProjectorInfo = ProjectorComponent->GetProjectorInfo();
    FString ProjectorID = GenerateProjectorID(ProjectorInfo);

    if (ProjectorStatusMap.Contains(ProjectorID))
    {
        FPJLinkProjectorStatus& Status = ProjectorStatusMap[ProjectorID];

        // 연결 실패 시 카운터 증가
        if (!bIsConnected)
        {
            Status.RecordConnectionFailure();
        }
        else
        {
            // 연결 성공 시 카운터 리셋
            Status.ResetCounters();
        }

        // 상태 업데이트
        Status.UpdateStatus(
            ProjectorComponent->GetPowerStatus(),
            ProjectorComponent->GetInputSource(),
            bIsConnected
        );

        // 상태 변경 이벤트 발생
        OnProjectorStatusChanged.Broadcast(Status);

        PJLINK_LOG_INFO(TEXT("Connection status changed for projector %s: %s"),
            *ProjectorInfo.Name,
            bIsConnected ? TEXT("Connected") : TEXT("Disconnected"));
    }
}

void UPJLinkManagerComponent::HandleErrorStatus(UPJLinkComponent* ProjectorComponent, const FString& ErrorMessage)
{
    if (!ProjectorComponent)
    {
        return;
    }

    FPJLinkProjectorInfo ProjectorInfo = ProjectorComponent->GetProjectorInfo();
    FString ProjectorID = GenerateProjectorID(ProjectorInfo);

    if (ProjectorStatusMap.Contains(ProjectorID))
    {
        FPJLinkProjectorStatus& Status = ProjectorStatusMap[ProjectorID];

        // 명령 실패 기록
        Status.RecordCommandFailure(ErrorMessage);

        // 상태 업데이트
        Status.UpdateStatus(
            ProjectorComponent->GetPowerStatus(),
            ProjectorComponent->GetInputSource(),
            ProjectorComponent->IsConnected(),
            ErrorMessage
        );

        // 상태 변경 이벤트 발생
        OnProjectorStatusChanged.Broadcast(Status);

        PJLINK_LOG_WARNING(TEXT("Error status for projector %s: %s"),
            *ProjectorInfo.Name,
            *ErrorMessage);
    }
}

