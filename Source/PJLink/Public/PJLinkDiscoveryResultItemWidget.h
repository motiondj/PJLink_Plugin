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

    // 선택 상태 설정
    UFUNCTION(BlueprintCallable, Category = "PJLink|Discovery|Selection")
    void SetSelected(bool bInSelected);

    // 선택 상태 가져오기
    UFUNCTION(BlueprintPure, Category = "PJLink|Discovery|Selection")
    bool IsSelected() const;

    // 항목 배경 및 테두리
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "PJLink|UI")
    class UBorder* SelectionBorder;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "PJLink|UI")
    class UButton* ItemBackground;

    // 선택 색상
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|UI|Appearance")
    FLinearColor SelectedColor = FLinearColor(0.2f, 0.6f, 1.0f, 0.3f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|UI|Appearance")
    FLinearColor NormalColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);

    // 선택 상태
    UPROPERTY(BlueprintReadOnly, Category = "PJLink|Discovery|Selection")
    bool bIsSelected = false;

protected:
    // 선택 버튼 클릭 이벤트
    UFUNCTION()
    void OnSelectButtonClicked();

    // 항목 클릭 이벤트
    UFUNCTION()
    FReply OnItemClicked(FGeometry MyGeometry, const FPointerEvent& MouseEvent);
};
