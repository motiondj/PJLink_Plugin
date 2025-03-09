// PJLinkDiscoveryWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PJLinkDiscoveryManager.h"
#include "PJLinkTypes.h"

// 정렬 옵션 열거형을 확장
UENUM(BlueprintType)
enum class EPJLinkDiscoverySortOption : uint8
{
    ByIPAddress UMETA(DisplayName = "IP 주소순"),
    ByName UMETA(DisplayName = "이름순"),
    ByResponseTime UMETA(DisplayName = "응답시간순"),
    ByModelName UMETA(DisplayName = "모델명순"), // 추가
    ByDiscoveryTime UMETA(DisplayName = "발견시간순") // 추가
};

// 필터 옵션 열거형 추가
UENUM(BlueprintType)
enum class EPJLinkDiscoveryFilterOption : uint8
{
    All UMETA(DisplayName = "모두 표시"),
    RequiresAuth UMETA(DisplayName = "인증 필요"),
    NoAuth UMETA(DisplayName = "인증 불필요"),
    Class1 UMETA(DisplayName = "클래스 1"),
    Class2 UMETA(DisplayName = "클래스 2")
};

// 검색 상태 열거형
UENUM(BlueprintType)
enum class EPJLinkDiscoveryState : uint8
{
    Idle UMETA(DisplayName = "대기 중"),
    Searching UMETA(DisplayName = "검색 중"),
    Completed UMETA(DisplayName = "완료됨"),
    Cancelled UMETA(DisplayName = "취소됨"),
    Failed UMETA(DisplayName = "실패")
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

    // 진행 상황 애니메이션 활성화 여부
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|Discovery|UI")
    bool bEnableProgressAnimation = true;

    // 진행 상황 애니메이션 속도 (초당 회전 수)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|Discovery|UI", meta = (EditCondition = "bEnableProgressAnimation", ClampMin = "0.1", ClampMax = "5.0"))
    float ProgressAnimationSpeed = 1.0f;

    // 현재 검색 상태
    UPROPERTY(BlueprintReadOnly, Category = "PJLink|Discovery|UI")
    EPJLinkDiscoveryState CurrentDiscoveryState = EPJLinkDiscoveryState::Idle;

    // 애니메이션 관련 확장 속성
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|Discovery|Animation")
    bool bEnableAdvancedAnimations = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|Discovery|Animation", meta = (EditCondition = "bEnableAdvancedAnimations"))
    float RotationAnimationSpeed = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|Discovery|Animation", meta = (EditCondition = "bEnableAdvancedAnimations"))
    float PulseAnimationSpeed = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|Discovery|Animation", meta = (EditCondition = "bEnableAdvancedAnimations"))
    FLinearColor ScanningColor = FLinearColor(0.2f, 0.5f, 1.0f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|Discovery|Animation", meta = (EditCondition = "bEnableAdvancedAnimations"))
    FLinearColor DeviceFoundColor = FLinearColor(0.2f, 0.8f, 0.2f, 1.0f);

    // 장치 발견 시 시각적 효과를 위한 속성
    UPROPERTY(BlueprintReadOnly, Category = "PJLink|Discovery|Animation")
    int32 LastDiscoveredDeviceIndex = -1;

    UPROPERTY(BlueprintReadOnly, Category = "PJLink|Discovery|Animation")
    float DeviceFoundEffectTime = 0.0f;

    // 애니메이션 상태 추적 속성
    UPROPERTY(BlueprintReadOnly, Category = "PJLink|Discovery|Animation")
    float AnimationTime = 0.0f;

    // 검색 버튼 애니메이션 관련 이벤트
    UFUNCTION(BlueprintImplementableEvent, Category = "PJLink|Discovery|Animation")
    void PlaySearchButtonAnimation(bool bIsStarting);

    // 장치 발견 시 애니메이션 효과
    UFUNCTION(BlueprintImplementableEvent, Category = "PJLink|Discovery|Animation")
    void PlayDeviceFoundEffect(int32 DeviceIndex);

    // 검색 완료 애니메이션
    UFUNCTION(BlueprintImplementableEvent, Category = "PJLink|Discovery|Animation")
    void PlayCompletionAnimation(bool bSuccessful, int32 DeviceCount);

    // 검색 상태 변경 이벤트
    UFUNCTION(BlueprintImplementableEvent, Category = "PJLink|Discovery|Events")
    void OnDiscoveryStateChanged(EPJLinkDiscoveryState NewState);

    // 검색 상태 텍스트 가져오기
    UFUNCTION(BlueprintPure, Category = "PJLink|Discovery|UI")
    FString GetDiscoveryStateText() const;

    // 현재 선택된 필터 옵션
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|Discovery|UI")
    EPJLinkDiscoveryFilterOption CurrentFilterOption = EPJLinkDiscoveryFilterOption::All;

    // 필터 옵션 설정
    UFUNCTION(BlueprintCallable, Category = "PJLink|Discovery")
    void SetFilterOption(EPJLinkDiscoveryFilterOption FilterOption);

    // 정렬 옵션과 필터 옵션을 동시에 적용한 결과 가져오기
    UFUNCTION(BlueprintCallable, Category = "PJLink|Discovery")
    TArray<FPJLinkDiscoveryResult> GetFilteredAndSortedResults() const;

    // 검색 결과 요약 텍스트 가져오기
    UFUNCTION(BlueprintPure, Category = "PJLink|Discovery|UI")
    FString GetResultSummaryText() const;

    // 검색 결과 요약 색상 가져오기 (결과 수에 따라 다른 색상)
    UFUNCTION(BlueprintPure, Category = "PJLink|Discovery|UI")
    FLinearColor GetResultSummaryColor() const;

    // 장치 상태 텍스트 가져오기
    UFUNCTION(BlueprintPure, Category = "PJLink|Discovery|UI")
    FString GetDeviceStatusText(const FPJLinkDiscoveryResult& Result) const;

    // 장치 상태 색상 가져오기
    UFUNCTION(BlueprintPure, Category = "PJLink|Discovery|UI")
    FLinearColor GetDeviceStatusColor(const FPJLinkDiscoveryResult& Result) const;

    // 애니메이션 업데이트를 위한 Tick 함수
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    // 애니메이션 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|Discovery|Animation")
    float PulseAnimationDuration = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|Discovery|Animation")
    float DeviceFoundEffectDuration = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|Discovery|Animation")
    FLinearColor SearchingColor = FLinearColor(0.1f, 0.5f, 1.0f, 1.0f);  // 검색 중 색상

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|Discovery|Animation")
    FLinearColor FoundColor = FLinearColor(0.1f, 0.8f, 0.2f, 1.0f);      // 발견 색상

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|Discovery|Animation")
    float CompletionAnimationDuration = 3.0f;

    // 애니메이션 관련 블루프린트 이벤트
    UFUNCTION(BlueprintImplementableEvent, Category = "PJLink|Discovery|Animation")
    void PlayPulseAnimation(UWidget* TargetWidget, const FLinearColor& PulseColor, float Duration);

    UFUNCTION(BlueprintImplementableEvent, Category = "PJLink|Discovery|Animation")
    void StopPulseAnimation(UWidget* TargetWidget);

    UFUNCTION(BlueprintImplementableEvent, Category = "PJLink|Discovery|Animation")
    void ShowDeviceFoundEffect(int32 DeviceIndex, const FLinearColor& EffectColor);

    UFUNCTION(BlueprintImplementableEvent, Category = "PJLink|Discovery|Animation")
    void PlayCompletionEffect(bool bSuccess, int32 DeviceCount);

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

    // 수정된 코드:
/**
 * 진행 상황 애니메이션 시작
 * 블루프린트에서 구현하여 회전 또는 깜박임 애니메이션을 시작
 */
    UFUNCTION(BlueprintImplementableEvent, Category = "PJLink|Discovery|UI")
    void StartProgressAnimation();

    /**
     * 진행 상황 애니메이션 중지
     * 블루프린트에서 구현하여 애니메이션을 중지
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "PJLink|Discovery|UI")
    void StopProgressAnimation();

    /**
     * 진행 상황 애니메이션 업데이트
     * 진행률에 따라 애니메이션 속도나 색상 등을 조절
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "PJLink|Discovery|UI")
    void UpdateProgressAnimation(float ProgressPercentage);

    /**
     * 현재 스캔 중인 IP 주소 업데이트
     * 블루프린트에서 구현하여 현재 스캔 중인 IP 주소 정보 표시
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "PJLink|Discovery|UI")
    void UpdateCurrentScanAddress(const FString& CurrentAddress);

    /**
     * 검색 소요 시간 업데이트
     * 블루프린트에서 구현하여 현재 검색 소요 시간 표시
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "PJLink|Discovery|UI")
    void UpdateElapsedTime(const FString& ElapsedTimeString);

    // 다음 속성 추가 (위젯의 private 영역 위에 추가)
    UPROPERTY(BlueprintReadOnly, Category = "PJLink|Discovery|UI")
    FString CurrentScanAddress;

    UPROPERTY(BlueprintReadOnly, Category = "PJLink|Discovery|UI")
    FString CurrentElapsedTimeString;

    UPROPERTY(BlueprintReadOnly, Category = "PJLink|Discovery|UI")
    float AnimationSpeedMultiplier = 1.0f;

    // 검색 상태 설정
    void SetDiscoveryState(EPJLinkDiscoveryState NewState);

    // 검색 결과 목록 항목 위젯 생성 이벤트
    UFUNCTION(BlueprintImplementableEvent, Category = "PJLink|Discovery|UI")
    UUserWidget* CreateResultItemWidget(const FPJLinkDiscoveryResult& Result, int32 Index);

    // 검색 결과 세부 정보 패널 업데이트 이벤트
    UFUNCTION(BlueprintImplementableEvent, Category = "PJLink|Discovery|UI")
    void UpdateDetailPanel(const FPJLinkDiscoveryResult& SelectedResult);

    // 장치 선택 이벤트
    UFUNCTION(BlueprintCallable, Category = "PJLink|Discovery|UI")
    void SelectDevice(int32 DeviceIndex);

    // 현재 선택된 장치 인덱스
    UPROPERTY(BlueprintReadOnly, Category = "PJLink|Discovery|UI")
    int32 SelectedDeviceIndex = -1;

    // 검색 결과 없음 UI 업데이트 이벤트
    UFUNCTION(BlueprintImplementableEvent, Category = "PJLink|Discovery|UI")
    void ShowNoResultsMessage(bool bIsSearching, bool bHasFilters);

    void PrepareResultsUpdate(const TArray<FPJLinkDiscoveryResult>& Results);

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

    // UI 업데이트 제한을 위한 타이머
    FTimerHandle UIUpdateTimerHandle;

    // UI 업데이트 주기 (초)
    UPROPERTY(EditAnywhere, Category = "PJLink|Discovery|Performance")
    float UIUpdateInterval = 0.25f; // 0.25초마다 UI 업데이트

    // 마지막 UI 업데이트 시간
    float LastUIUpdateTime = 0.0f;

    // 업데이트 대기 중인 UI 변경 사항 플래그
    bool bPendingUIUpdate = false;

    // 지연된 UI 업데이트 수행
    void PerformDeferredUIUpdate();

    // 애니메이션 상태 변수
    float PulseAnimationTime = 0.0f;
    bool bIsPulseAnimationActive = false;
    TArray<UWidget*> AnimatedWidgets;
};
