// PJLinkSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PJLinkTypes.h"
#include "PJLinkSubsystem.generated.h"

// 전방 선언
class UPJLinkNetworkManager;

/**
 * PJLink 기능을 위한 게임 인스턴스 서브시스템
 * 언리얼 엔진 내에서 전역적으로 접근 가능한 인터페이스 제공
 */
UCLASS()
class PJLINK_API UPJLinkSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UPJLinkSubsystem();
    virtual ~UPJLinkSubsystem();

    // USubsystem 인터페이스 구현
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // 프로젝터 연결
    UFUNCTION(BlueprintCallable, Category = "PJLink")
    bool ConnectToProjector(const FPJLinkProjectorInfo& ProjectorInfo);

    // 프로젝터 연결 해제
    UFUNCTION(BlueprintCallable, Category = "PJLink")
    void DisconnectFromProjector();

    // 현재 연결된 프로젝터 정보 가져오기
    UFUNCTION(BlueprintCallable, Category = "PJLink")
    FPJLinkProjectorInfo GetProjectorInfo() const;

    // 연결 상태 확인
    UFUNCTION(BlueprintCallable, Category = "PJLink")
    bool IsConnected() const;

    // 프로젝터 전원 켜기
    UFUNCTION(BlueprintCallable, Category = "PJLink")
    bool PowerOn();

    // 프로젝터 전원 끄기
    UFUNCTION(BlueprintCallable, Category = "PJLink")
    bool PowerOff();

    // 입력 소스 변경
    UFUNCTION(BlueprintCallable, Category = "PJLink")
    bool SwitchInputSource(EPJLinkInputSource InputSource);

    // 상태 업데이트 요청
    UFUNCTION(BlueprintCallable, Category = "PJLink")
    bool RequestStatus();

    // 네트워크 매니저 직접 접근
    UFUNCTION(BlueprintCallable, Category = "PJLink|Advanced")
    UPJLinkNetworkManager* GetNetworkManager() const;

    // 응답 이벤트를 전달하기 위한 델리게이트
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

private:
    // 네트워크 관리자 인스턴스
    UPROPERTY()
    UPJLinkNetworkManager* NetworkManager;

    // 응답 이벤트 핸들러
    void HandleResponseReceived(EPJLinkCommand Command, EPJLinkResponseStatus Status, const FString& Response);

    // 이전 상태 추적을 위한 변수
    EPJLinkPowerStatus PreviousPowerStatus;
    EPJLinkInputSource PreviousInputSource;
    bool bPreviousConnectionState;

    // 주기적 상태 확인을 위한 타이머
    FTimerHandle StatusCheckTimerHandle;
    void CheckStatus();

    // 주기적 상태 확인 활성화 여부
    bool bPeriodicStatusCheck;

    // 상태 확인 주기 (초)
    float StatusCheckInterval;
};
