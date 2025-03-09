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