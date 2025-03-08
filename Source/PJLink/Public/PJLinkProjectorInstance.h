// PJLinkProjectorInstance.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "PJLinkTypes.h"
#include "PJLinkProjectorInstance.generated.h"

// 전방 선언
class UPJLinkNetworkManager;

/**
 * 블루프린트에서 직접 조작할 수 있는 프로젝터 인스턴스 객체
 */
UCLASS(BlueprintType, Blueprintable)
class PJLINK_API UPJLinkProjectorInstance : public UObject
{
    GENERATED_BODY()

public:
    UPJLinkProjectorInstance();

    /**
     * 프로젝터 설정 초기화
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink")
    void Initialize(
        const FString& InName,
        const FString& InIPAddress,
        int32 InPort = 4352,
        EPJLinkClass InDeviceClass = EPJLinkClass::Class1,
        bool bInRequiresAuthentication = false,
        const FString& InPassword = TEXT("")  // FString() 대신 TEXT("") 사용
    );

    /**
     * 프로젝터 연결
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink")
    bool Connect();

    /**
     * 프로젝터 연결 해제
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink")
    void Disconnect();

    /**
     * 프로젝터 전원 켜기
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Control")
    bool PowerOn();

    /**
     * 프로젝터 전원 끄기
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Control")
    bool PowerOff();

    /**
     * 입력 소스 변경
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Control")
    bool SwitchInputSource(EPJLinkInputSource InputSource);

    /**
     * 상태 업데이트 요청
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Control")
    bool RequestStatus();

    /**
     * 연결 상태 확인
     */
    UFUNCTION(BlueprintPure, Category = "PJLink")
    bool IsConnected() const;

    /**
     * 프로젝터 정보 가져오기
     */
    UFUNCTION(BlueprintPure, Category = "PJLink")
    FPJLinkProjectorInfo GetProjectorInfo() const;

    /**
     * 응답 이벤트
     */
    UPROPERTY(BlueprintAssignable, Category = "PJLink|Events")
    FPJLinkResponseDelegate OnResponseReceived;

private:
    UPROPERTY()
    UPJLinkNetworkManager* NetworkManager;

    UPROPERTY()
    FPJLinkProjectorInfo ProjectorInfo;

    void HandleResponseReceived(EPJLinkCommand Command, EPJLinkResponseStatus Status, const FString& Response);
};
