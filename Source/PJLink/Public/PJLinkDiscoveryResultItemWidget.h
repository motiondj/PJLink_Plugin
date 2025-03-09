// PJLinkDiscoveryResultItemWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PJLinkTypes.h"
#include "PJLinkDiscoveryInterfaces.h"
#include "PJLinkDiscoveryResultItemWidget.generated.h"

/**
 * PJLink 검색 결과 항목 위젯 템플릿
 * 검색 결과 항목을 표시하기 위한 기본 위젯 클래스
 */
UCLASS()
class PJLINK_API UPJLinkDiscoveryResultItemWidget : public UUserWidget, public IResultItemInterface
{
    GENERATED_BODY()

public:
    UPJLinkDiscoveryResultItemWidget(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;

    // ResultItemInterface 구현
    virtual void SetResultData_Implementation(const FPJLinkDiscoveryResult& Result, int32 Index) override;

    // UI 요소들
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "PJLink|UI")
    class UTextBlock* IPAddressText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "PJLink|UI")
    class UTextBlock* NameText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "PJLink|UI")
    class UTextBlock* ModelNameText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "PJLink|UI")
    class UTextBlock* StatusText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "PJLink|UI")
    class UButton* SelectButton;

    // 결과 데이터
    UPROPERTY(BlueprintReadWrite, Category = "PJLink|Discovery")
    FPJLinkDiscoveryResult ResultData;

    // 항목 인덱스
    UPROPERTY(BlueprintReadWrite, Category = "PJLink|Discovery")
    int32 ItemIndex;

protected:
    // 선택 버튼 클릭 이벤트
    UFUNCTION()
    void OnSelectButtonClicked();
};
