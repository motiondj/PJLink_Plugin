// PJLinkStateMachine.cpp
#include "PJLinkStateMachine.h"
#include "PJLinkLog.h"

UPJLinkStateMachine::UPJLinkStateMachine()
    : CurrentState(EPJLinkProjectorState::Disconnected)
    , CurrentErrorMessage(TEXT(""))
{
}

void UPJLinkStateMachine::SetState(EPJLinkProjectorState NewState)
{
    if (CurrentState != NewState)
    {
        EPJLinkProjectorState OldState = CurrentState;
        CurrentState = NewState;

        // 오류 상태가 아니면 오류 메시지 초기화
        if (NewState != EPJLinkProjectorState::Error)
        {
            CurrentErrorMessage = TEXT("");
        }

        PJLINK_LOG_INFO(TEXT("State changed from %s to %s"),
            *GetStateName(), *GetStateName());

        // 상태 변경 이벤트 발생
        OnStateChanged.Broadcast(OldState, NewState);
    }
}

bool UPJLinkStateMachine::CanPerformAction(EPJLinkCommand Action) const
{
    switch (CurrentState)
    {
    case EPJLinkProjectorState::Disconnected:
        // 연결되지 않은 상태에서는 어떤 명령도 실행할 수 없음
        return false;

    case EPJLinkProjectorState::Connecting:
        // 연결 중에는 어떤 명령도 실행할 수 없음
        return false;

    case EPJLinkProjectorState::Connected:
        // 연결된 상태에서는 일부 명령만 실행 가능
        return Action == EPJLinkCommand::POWR ||
            Action == EPJLinkCommand::NAME ||
            Action == EPJLinkCommand::INF1 ||
            Action == EPJLinkCommand::INF2 ||
            Action == EPJLinkCommand::CLSS;

    case EPJLinkProjectorState::PoweringOn:
        // 전원 켜는 중에는 상태 확인만 가능
        return Action == EPJLinkCommand::POWR ||
            Action == EPJLinkCommand::ERST;

    case EPJLinkProjectorState::PoweringOff:
        // 전원 끄는 중에는 상태 확인만 가능
        return Action == EPJLinkCommand::POWR ||
            Action == EPJLinkCommand::ERST;

    case EPJLinkProjectorState::ReadyForUse:
        // 사용 준비가 된 상태에서는 모든 명령 실행 가능
        return true;

    case EPJLinkProjectorState::Error:
        // 오류 상태에서는 상태 확인과 전원 끄기만 가능
        return Action == EPJLinkCommand::POWR ||
            Action == EPJLinkCommand::ERST;

    default:
        return false;
    }
}

void UPJLinkStateMachine::UpdateFromPowerStatus(EPJLinkPowerStatus PowerStatus)
{
    switch (PowerStatus)
    {
    case EPJLinkPowerStatus::PoweredOff:
        // 전원이 꺼진 상태
        if (CurrentState != EPJLinkProjectorState::Disconnected &&
            CurrentState != EPJLinkProjectorState::Error)
        {
            SetState(EPJLinkProjectorState::Connected);
        }
        break;

    case EPJLinkPowerStatus::PoweredOn:
        // 전원이 켜진 상태
        if (CurrentState != EPJLinkProjectorState::Error)
        {
            SetState(EPJLinkProjectorState::ReadyForUse);
        }
        break;

    case EPJLinkPowerStatus::CoolingDown:
        // 냉각 중
        if (CurrentState != EPJLinkProjectorState::Error)
        {
            SetState(EPJLinkProjectorState::PoweringOff);
        }
        break;

    case EPJLinkPowerStatus::WarmingUp:
        // 예열 중
        if (CurrentState != EPJLinkProjectorState::Error)
        {
            SetState(EPJLinkProjectorState::PoweringOn);
        }
        break;

    case EPJLinkPowerStatus::Unknown:
        // 상태 알 수 없음
        break;
    }
}

void UPJLinkStateMachine::UpdateFromConnectionStatus(bool bIsConnected)
{
    if (bIsConnected)
    {
        if (CurrentState == EPJLinkProjectorState::Disconnected)
        {
            SetState(EPJLinkProjectorState::Connected);
        }
    }
    else
    {
        if (CurrentState != EPJLinkProjectorState::Disconnected)
        {
            SetState(EPJLinkProjectorState::Disconnected);
        }
    }
}

void UPJLinkStateMachine::SetErrorState(const FString& ErrorMessage)
{
    CurrentErrorMessage = ErrorMessage;
    SetState(EPJLinkProjectorState::Error);

    PJLINK_LOG_ERROR(TEXT("Projector error: %s"), *ErrorMessage);
}

FString UPJLinkStateMachine::GetStateName() const
{
    switch (CurrentState)
    {
    case EPJLinkProjectorState::Disconnected:
        return TEXT("Disconnected");
    case EPJLinkProjectorState::Connecting:
        return TEXT("Connecting");
    case EPJLinkProjectorState::Connected:
        return TEXT("Connected");
    case EPJLinkProjectorState::PoweringOn:
        return TEXT("PoweringOn");
    case EPJLinkProjectorState::PoweringOff:
        return TEXT("PoweringOff");
    case EPJLinkProjectorState::ReadyForUse:
        return TEXT("ReadyForUse");
    case EPJLinkProjectorState::Error:
        return TEXT("Error");
    default:
        return TEXT("Unknown");
    }
}

FString UPJLinkStateMachine::GetStateNameFromState(EPJLinkProjectorState State) const
{
    switch (State)
    {
    case EPJLinkProjectorState::Disconnected:
        return TEXT("Disconnected");
    case EPJLinkProjectorState::Connecting:
        return TEXT("Connecting");
    case EPJLinkProjectorState::Connected:
        return TEXT("Connected");
    case EPJLinkProjectorState::PoweringOn:
        return TEXT("PoweringOn");
    case EPJLinkProjectorState::PoweringOff:
        return TEXT("PoweringOff");
    case EPJLinkProjectorState::ReadyForUse:
        return TEXT("ReadyForUse");
    case EPJLinkProjectorState::Error:
        return TEXT("Error");
    default:
        return TEXT("Unknown");
    }
}