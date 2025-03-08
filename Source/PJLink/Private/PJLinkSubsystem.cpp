// PJLinkSubsystem.cpp
#include "PJLinkSubsystem.h"
#include "PJLinkNetworkManager.h"

UPJLinkSubsystem::UPJLinkSubsystem()
    : NetworkManager(nullptr)
    , PreviousPowerStatus(EPJLinkPowerStatus::Unknown)
    , PreviousInputSource(EPJLinkInputSource::Unknown)
    , bPreviousConnectionState(false)
    , bPeriodicStatusCheck(true)
    , StatusCheckInterval(5.0f)
{
}

UPJLinkSubsystem::~UPJLinkSubsystem()
{
}

void UPJLinkSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // 네트워크 매니저 생성
    NetworkManager = NewObject<UPJLinkNetworkManager>(this);

    // 응답 이벤트 연결
    if (NetworkManager)
    {
        NetworkManager->OnResponseReceived.AddDynamic(this, &UPJLinkSubsystem::HandleResponseReceived);
    }

    // 주기적 상태 확인 타이머 설정
    if (bPeriodicStatusCheck && StatusCheckInterval > 0.0f)
    {
        GetWorld()->GetTimerManager().SetTimer(
            StatusCheckTimerHandle,
            this,
            &UPJLinkSubsystem::CheckStatus,
            StatusCheckInterval,
            true);
    }
}

void UPJLinkSubsystem::Deinitialize()
{
    // 타이머 정지
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(StatusCheckTimerHandle);
    }

    // 연결 해제
    DisconnectFromProjector();

    // 이벤트 연결 해제
    if (NetworkManager)
    {
        NetworkManager->OnResponseReceived.RemoveDynamic(this, &UPJLinkSubsystem::HandleResponseReceived);
    }

    Super::Deinitialize();
}

bool UPJLinkSubsystem::ConnectToProjector(const FPJLinkProjectorInfo& ProjectorInfo)
{
    if (NetworkManager)
    {
        bool bResult = NetworkManager->ConnectToProjector(ProjectorInfo);
        bool bNewConnectionState = bResult;

        // 연결 상태 변경 확인
        if (bPreviousConnectionState != bNewConnectionState)
        {
            OnConnectionChanged.Broadcast(bNewConnectionState);
            bPreviousConnectionState = bNewConnectionState;
        }

        return bResult;
    }

    return false;
}

void UPJLinkSubsystem::DisconnectFromProjector()
{
    if (NetworkManager)
    {
        bool bWasConnected = NetworkManager->IsConnected();

        NetworkManager->DisconnectFromProjector();

        // 연결 상태 변경 감지
        if (bWasConnected && bPreviousConnectionState != false)
        {
            OnConnectionChanged.Broadcast(false);
            bPreviousConnectionState = false;
        }
    }
}

FPJLinkProjectorInfo UPJLinkSubsystem::GetProjectorInfo() const
{
    if (NetworkManager)
    {
        return NetworkManager->GetProjectorInfo();
    }

    return FPJLinkProjectorInfo();
}

bool UPJLinkSubsystem::IsConnected() const
{
    if (NetworkManager)
    {
        return NetworkManager->IsConnected();
    }

    return false;
}

bool UPJLinkSubsystem::PowerOn()
{
    if (NetworkManager)
    {
        return NetworkManager->PowerOn();
    }

    return false;
}

bool UPJLinkSubsystem::PowerOff()
{
    if (NetworkManager)
    {
        return NetworkManager->PowerOff();
    }

    return false;
}

bool UPJLinkSubsystem::SwitchInputSource(EPJLinkInputSource InputSource)
{
    if (NetworkManager)
    {
        return NetworkManager->SwitchInputSource(InputSource);
    }

    return false;
}

bool UPJLinkSubsystem::RequestStatus()
{
    if (NetworkManager)
    {
        return NetworkManager->RequestStatus();
    }

    return false;
}

UPJLinkNetworkManager* UPJLinkSubsystem::GetNetworkManager() const
{
    return NetworkManager;
}

void UPJLinkSubsystem::HandleResponseReceived(EPJLinkCommand Command, EPJLinkResponseStatus Status, const FString& Response)
{
    // 로그 출력
    UE_LOG(LogTemp, Log, TEXT("PJLink Response - Command: %d, Status: %d, Response: %s"),
        static_cast<int32>(Command), static_cast<int32>(Status), *Response);

    // 오류 상태 확인
    if (Status != EPJLinkResponseStatus::Success)
    {
        FString ErrorMessage = FString::Printf(TEXT("Error in command %d: Status %d - %s"),
            static_cast<int32>(Command), static_cast<int32>(Status), *Response);

        UE_LOG(LogTemp, Warning, TEXT("%s"), *ErrorMessage);
        OnErrorStatus.Broadcast(ErrorMessage);
    }

    // 현재 프로젝터 정보 가져오기 (이 정보는 NetworkManager에서 업데이트됨)
    FPJLinkProjectorInfo CurrentInfo = NetworkManager->GetProjectorInfo();

    // 전원 상태 변경 확인
    if (Command == EPJLinkCommand::POWR && CurrentInfo.PowerStatus != PreviousPowerStatus)
    {
        UE_LOG(LogTemp, Log, TEXT("Power status changed from %d to %d"),
            static_cast<int32>(PreviousPowerStatus), static_cast<int32>(CurrentInfo.PowerStatus));

        OnPowerStatusChanged.Broadcast(PreviousPowerStatus, CurrentInfo.PowerStatus);
        PreviousPowerStatus = CurrentInfo.PowerStatus;
    }

    // 입력 소스 변경 확인
    if (Command == EPJLinkCommand::INPT && CurrentInfo.CurrentInputSource != PreviousInputSource)
    {
        UE_LOG(LogTemp, Log, TEXT("Input source changed from %d to %d"),
            static_cast<int32>(PreviousInputSource), static_cast<int32>(CurrentInfo.CurrentInputSource));

        OnInputSourceChanged.Broadcast(PreviousInputSource, CurrentInfo.CurrentInputSource);
        PreviousInputSource = CurrentInfo.CurrentInputSource;
    }

    // 기존 응답 이벤트 발생
    OnResponseReceived.Broadcast(Command, Status, Response);
}

void UPJLinkSubsystem::CheckStatus()
{
    bool bCurrentConnectionState = IsConnected();

    // 연결 상태 변경 감지
    if (bPreviousConnectionState != bCurrentConnectionState)
    {
        UE_LOG(LogTemp, Log, TEXT("Connection state changed to: %s"),
            bCurrentConnectionState ? TEXT("Connected") : TEXT("Disconnected"));

        OnConnectionChanged.Broadcast(bCurrentConnectionState);
        bPreviousConnectionState = bCurrentConnectionState;
    }

    if (bCurrentConnectionState)
    {
        RequestStatus();
    }
}