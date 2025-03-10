// PJLinkDiscoveryDefaultWidget.h
#pragma once

#include "CoreMinimal.h"
#include "PJLinkDiscoveryWidget.h"
#include "PJLinkDiscoveryDefaultWidget.generated.h"

/**
 * 기본 PJLink 검색 위젯 클래스
 * 이 클래스는 블루프린트에서 확장하여 사용할 수 있는 템플릿 구현을 제공합니다.
 */
UCLASS()
class PJLINK_API UPJLinkDiscoveryDefaultWidget : public UPJLinkDiscoveryWidget
{
    GENERATED_BODY()

public:
    UPJLinkDiscoveryDefaultWidget(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;

    // UI 요소들
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "PJLink|UI")
    class UButton* BroadcastSearchButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "PJLink|UI")
    class UButton* RangeSearchButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "PJLink|UI")
    class UButton* SubnetSearchButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "PJLink|UI")
    class UButton* CancelSearchButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "PJLink|UI")
    class UTextBlock* CurrentIPTextBlock;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "PJLink|UI")
    class UTextBlock* ElapsedTimeTextBlock;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "PJLink|UI")
    class UImage* ScanningAnimationImage;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "PJLink|UI")
    class UComboBox* SortOptionsComboBox;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "PJLink|UI")
    class UEditableTextBox* FilterTextBox;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "PJLink|UI")
    class UComboBox* FilterOptionsComboBox;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "PJLink|UI")
    class UVerticalBox* NoResultsBox;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "PJLink|UI")
    class UBorder* DetailPanel;

    // 검색 옵션 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|Settings")
    float DefaultSearchTimeout = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|Settings")
    FString DefaultStartIP = TEXT("192.168.1.1");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|Settings")
    FString DefaultEndIP = TEXT("192.168.1.254");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|Settings")
    FString DefaultSubnetAddress = TEXT("192.168.1.0");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|Settings")
    FString DefaultSubnetMask = TEXT("255.255.255.0");

    // 결과 항목 위젯 클래스
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|UI")
    TSubclassOf<UUserWidget> ResultItemWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|UI|Timer")
    FLinearColor TimerNormalColor = FLinearColor(0.2f, 0.6f, 1.0f, 1.0f); // 파란색

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|UI|Timer")
    FLinearColor TimerWarningColor = FLinearColor(1.0f, 0.8f, 0.2f, 1.0f); // 노란색

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|UI|Timer")
    FLinearColor TimerCriticalColor = FLinearColor(1.0f, 0.3f, 0.3f, 1.0f); // 빨간색

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|UI|Timer")
    float TimerWarningThreshold = 60.0f; // 60초(1분) 이상이면 경고색으로 변경

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|UI|Timer")
    float TimerCriticalThreshold = 300.0f; // 300초(5분) 이상이면 위험색으로 변경

    // ScannedAddressesText 변수 추가
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "PJLink|UI")
    class UTextBlock* ScannedAddressesText;

    // 애니메이션 속도 제어 변수 추가
    UPROPERTY(BlueprintReadWrite, Category = "PJLink|UI|Animation")
    float AnimationSpeedMultiplier = 1.0f;

protected:
    // 부모 클래스의 블루프린트 이벤트 구현
    virtual void UpdateProgressBar_Implementation(float ProgressPercentage, int32 DiscoveredDevices, int32 ScannedAddresses) override;
    virtual void UpdateStatusText_Implementation(const FString& StatusText) override;
    virtual void UpdateResultsList_Implementation(const TArray<FPJLinkDiscoveryResult>& Results) override;
    virtual UUserWidget* CreateResultItemWidget_Implementation(const FPJLinkDiscoveryResult& Result, int32 Index) override;
    virtual void UpdateDetailPanel_Implementation(const FPJLinkDiscoveryResult& SelectedResult) override;
    virtual void ShowNoResultsMessage_Implementation(bool bIsSearching, bool bHasFilters) override;
    virtual void StartProgressAnimation_Implementation() override;
    virtual void StopProgressAnimation_Implementation() override;
    virtual void UpdateProgressAnimation_Implementation(float ProgressPercentage) override;
    virtual void OnDiscoveryStateChanged_Implementation(EPJLinkDiscoveryState NewState) override;
    virtual void OnShowMessage_Implementation(const FString& Message, const FLinearColor& Color) override;
    virtual void OnHideMessage_Implementation() override;

    // 버튼 이벤트 핸들러
    UFUNCTION()
    void OnBroadcastSearchClicked();

    UFUNCTION()
    void OnRangeSearchClicked();

    UFUNCTION()
    void OnSubnetSearchClicked();

    UFUNCTION()
    void OnCancelSearchClicked();

    // 정렬 및 필터 이벤트 핸들러
    UFUNCTION()
    void OnSortOptionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    UFUNCTION()
    void OnFilterOptionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    UFUNCTION()
    void OnFilterTextChanged(const FText& Text);

    // 내부 도우미 함수들
    void InitializeSortOptions();
    void InitializeFilterOptions();
    void UpdateButtonStates();
    void ClearResultsPanel();
};
