// PJLinkDiscoveryWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PJLinkDiscoveryManager.h"
#include "PJLinkTypes.h"

UENUM(BlueprintType)
enum class EPJLinkDiscoverySortOption : uint8
{
    ByIPAddress UMETA(DisplayName = "IP 주소순"),
    ByName UMETA(DisplayName = "이름순"),
    ByResponseTime UMETA(DisplayName = "응답시간순")
};

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

    UFUNCTION(BlueprintCallable, Category = "PJLink|Discovery")
    void SetSortOption(EPJLinkDiscoverySortOption SortOption);

    // 필터 텍스트 설정
    UFUNCTION(BlueprintCallable, Category = "PJLink|Discovery")
    void SetFilterText(const FString& InFilterText);

    // 정렬 옵션 설정
    UFUNCTION(BlueprintCallable, Category = "PJLink|Discovery")
    void SetSortOption(EPJLinkDiscoverySortOption SortOption);

    // 필터 텍스트 설정
    UFUNCTION(BlueprintCallable, Category = "PJLink|Discovery")
    void SetFilterText(const FString& InFilterText);

    // 정렬된/필터링된 결과 가져오기
    UFUNCTION(BlueprintCallable, Category = "PJLink|Discovery")
    TArray<FPJLinkDiscoveryResult> GetFilteredAndSortedResults() const;

    // 유효한 검색 결과인지 확인
    UFUNCTION(BlueprintPure, Category = "PJLink|Discovery")
    bool HasValidResults() const;

    /**
 * 캐싱된 검색 결과 불러오기
 */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Discovery")
    bool LoadCachedResults();

    /**
     * 현재 검색 결과 캐싱
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Discovery")
    bool SaveResultsToCache();

    /**
     * 캐시 파일 경로 가져오기
     */
    UFUNCTION(BlueprintPure, Category = "PJLink|Discovery")
    FString GetCacheFilePath() const;

    /**
     * 마지막 캐시 날짜/시간 가져오기
     */
    UFUNCTION(BlueprintPure, Category = "PJLink|Discovery")
    FDateTime GetLastCacheTime() const;

    /**
 * 오류 메시지 표시
 */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Discovery")
    void ShowErrorMessage(const FString& ErrorMessage);

    /**
     * 성공 메시지 표시
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Discovery")
    void ShowSuccessMessage(const FString& SuccessMessage);

    /**
     * 정보 메시지 표시
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Discovery")
    void ShowInfoMessage(const FString& InfoMessage);

    /**
     * 메시지 숨기기
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Discovery")
    void HideMessage();

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

    UFUNCTION(BlueprintImplementableEvent, Category = "PJLink|Discovery")
    void OnShowMessage(const FString& Message, const FLinearColor& Color);

    UFUNCTION(BlueprintImplementableEvent, Category = "PJLink|Discovery")
    void OnHideMessage();

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

    UPROPERTY()
    EPJLinkDiscoverySortOption CurrentSortOption = EPJLinkDiscoverySortOption::ByIPAddress;

    // 필터 텍스트
    UPROPERTY()
    FString FilterText;

    // 마지막 캐시 시간
    UPROPERTY()
    FDateTime LastCacheTime;

    // 캐시 파일명
    UPROPERTY()
    FString CacheFileName = TEXT("PJLinkDiscoveryCache.json");

    // 메시지 색상
    UPROPERTY()
    FLinearColor ErrorColor = FLinearColor(1.0f, 0.2f, 0.2f, 1.0f); // 빨간색

    UPROPERTY()
    FLinearColor SuccessColor = FLinearColor(0.2f, 0.8f, 0.2f, 1.0f); // 녹색

    UPROPERTY()
    FLinearColor InfoColor = FLinearColor(0.2f, 0.6f, 1.0f, 1.0f); // 파란색

    // 마지막 메시지가 표시된 시간
    UPROPERTY()
    float LastMessageTime = 0.0f;

    // 메시지 자동 숨김 시간 (초)
    UPROPERTY(EditAnywhere, Category = "PJLink|Discovery")
    float MessageAutoHideTime = 5.0f;

    // 메시지 자동 숨김 타이머 핸들
    FTimerHandle MessageTimerHandle;

    // private 섹션에 다음 함수 선언 추가:
    void SetupMessageTimer();
};
