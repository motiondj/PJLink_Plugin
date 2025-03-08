// PJLinkStateMachine.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "PJLinkTypes.h"
#include "PJLinkStateMachine.generated.h"

/**
 * 프로젝터의 상태를 정의하는 열거형
 */
UENUM(BlueprintType)
enum class EPJLinkProjectorState : uint8
{
    Disconnected UMETA(DisplayName = "Disconnected"),
    Connecting UMETA(DisplayName = "Connecting"),
    Connected UMETA(DisplayName = "Connected"),
    PoweringOn UMETA(DisplayName = "Powering On"),
    PoweringOff UMETA(DisplayName = "Powering Off"),
    ReadyForUse UMETA(DisplayName = "Ready For Use"),
    Error UMETA(DisplayName = "Error")
};

/**
 * 상태 변경 이벤트 델리게이트
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPJLinkStateChangedDelegate,
    EPJLinkProjectorState, OldState,
    EPJLinkProjectorState, NewState);

/**
 * PJLink 프로젝터 상태 머신 클래스
 * 프로젝터의 상태 전환과 가능한 작업을 관리합니다.
 */
UCLASS(BlueprintType)
class PJLINK_API UPJLinkStateMachine : public UObject
{
    GENERATED_BODY()

public:
    UPJLinkStateMachine();

    // 현재 상태 가져오기
    UFUNCTION(BlueprintPure, Category = "PJLink|StateMachine")
    EPJLinkProjectorState GetCurrentState() const { return CurrentState; }

    // 상태 설정
    UFUNCTION(BlueprintCallable, Category = "PJLink|StateMachine")
    void SetState(EPJLinkProjectorState NewState);

    // 현재 상태에서 특정 작업이 가능한지 확인
    UFUNCTION(BlueprintPure, Category = "PJLink|StateMachine")
    bool CanPerformAction(EPJLinkCommand Action) const;

    // 전원 상태에 따른 상태 업데이트
    UFUNCTION(BlueprintCallable, Category = "PJLink|StateMachine")
    void UpdateFromPowerStatus(EPJLinkPowerStatus PowerStatus);

    // 연결 상태에 따른 상태 업데이트
    UFUNCTION(BlueprintCallable, Category = "PJLink|StateMachine")
    void UpdateFromConnectionStatus(bool bIsConnected);

    // 오류 상태 설정
    UFUNCTION(BlueprintCallable, Category = "PJLink|StateMachine")
    void SetErrorState(const FString& ErrorMessage);

    // 상태 이름 가져오기
    UFUNCTION(BlueprintPure, Category = "PJLink|StateMachine")
    FString GetStateName() const;

    // 현재 오류 메시지 가져오기
    UFUNCTION(BlueprintPure, Category = "PJLink|StateMachine")
    FString GetErrorMessage() const { return CurrentErrorMessage; }

    // 상태 변경 이벤트
    UPROPERTY(BlueprintAssignable, Category = "PJLink|Events")
    FPJLinkStateChangedDelegate OnStateChanged;

    // 상태 이름 가져오기 (상태 열거형에서)
    UFUNCTION(BlueprintPure, Category = "PJLink|StateMachine")
    FString GetStateNameFromState(EPJLinkProjectorState State) const;

private:
    // 현재 상태
    EPJLinkProjectorState CurrentState;

    // 현재 오류 메시지
    FString CurrentErrorMessage;
};
