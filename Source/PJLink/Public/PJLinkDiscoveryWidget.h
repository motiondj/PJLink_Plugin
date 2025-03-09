// PJLinkDiscoveryWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PJLinkDiscoveryManager.h"
#include "PJLinkTypes.h"
#include "PJLinkDiscoveryWidget.generated.h"

/**
 * PJLink 자동 검색 기능을 위한 위젯
 */
UCLASS()
class PJLINK_API UPJLinkDiscoveryWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPJLinkDiscoveryWidget(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    /**
     * 브로드캐스트 검색 시작
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Discovery")
    void StartBroadcastSearch(float TimeoutSeconds = 5.0f);

    /**
     * IP 범위 검색 시작
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Discovery")
    void StartRangeSearch(const FString& StartIP, const FString& EndIP, float TimeoutSeconds = 10.0f);

    /**
     * 서브넷 검색 시작
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Discovery")
    void StartSubnetSearch(const FString& SubnetAddress, const FString& SubnetMask, float TimeoutSeconds = 20.0f);

    /**
     * 검색 취소
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Discovery")
    void CancelSearch();

    /**
     * 발견된 장치에 연결
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Discovery")
    void ConnectToDevice(const FPJLinkDiscoveryResult& Device);

    /**
     * 발견된 장치를 프리셋으로 저장
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Discovery")
    void SaveDeviceAsPreset(const FPJLinkDiscoveryResult& Device, const FString& PresetName);

    /**
     * 발견된 장치를 그룹에 추가
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Discovery")
    void AddDeviceToGroup(const FPJLinkDiscoveryResult& Device, const FString& GroupName);

    /**
     * 모든 검색 결과를 그룹으로 저장
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Discovery")
    void SaveAllResultsAsGroup(const FString& GroupName);

protected:
    /**
     * 검색 완료 이벤트 처리
     */
    UFUNCTION()
    void OnDiscoveryCompleted(const TArray<FPJLinkDiscoveryResult>& DiscoveredDevices, bool bSuccess);

    /**
     * 장치 발견 이벤트 처리
     */
    UFUNCTION()
    void OnDeviceDiscovered(const FPJLinkDiscoveryResult& DiscoveredDevice);

    /**
     * 검색 진행 상황 업데이트 처리
     */
    UFUNCTION()
    void OnDiscoveryProgressUpdated(const FPJLinkDiscoveryStatus& Status);

    /**
     * 결과 목록 업데이트
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "PJLink|Discovery")
    void UpdateResultsList(const TArray<FPJLinkDiscoveryResult>& Results);

    /**
     * 진행 상황 업데이트
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "PJLink|Discovery")
    void UpdateProgressBar(float ProgressPercentage, int32 DiscoveredDevices, int32 ScannedAddresses);

    /**
     * 상태 텍스트 업데이트
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "PJLink|Discovery")
    void UpdateStatusText(const FString& StatusText);

private:
    /**
     * 검색 매니저 생성
     */
    void CreateDiscoveryManager();

    /**
     * 검색 초기화 및 이벤트 설정
     */
    void SetupDiscovery();

    // 검색 매니저
    UPROPERTY()
    UPJLinkDiscoveryManager* DiscoveryManager;

    // 현재 검색 ID
    FString CurrentDiscoveryID;

    // 검색 결과
    UPROPERTY()
    TArray<FPJLinkDiscoveryResult> DiscoveryResults;
};
