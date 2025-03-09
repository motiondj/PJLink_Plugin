// PJLinkDiscoveryResultItemWidget.cpp
#include "PJLinkDiscoveryResultItemWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"

UPJLinkDiscoveryResultItemWidget::UPJLinkDiscoveryResultItemWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UPJLinkDiscoveryResultItemWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 선택 버튼 이벤트 바인딩
    if (SelectButton)
    {
        SelectButton->OnClicked.AddDynamic(this, &UPJLinkDiscoveryResultItemWidget::OnSelectButtonClicked);
    }
}

void UPJLinkDiscoveryResultItemWidget::SetResultData_Implementation(const FPJLinkDiscoveryResult& Result, int32 Index)
{
    ResultData = Result;
    ItemIndex = Index;

    // IP 주소 텍스트 설정
    if (IPAddressText)
    {
        IPAddressText->SetText(FText::FromString(Result.IPAddress));
    }

    // 이름 텍스트 설정
    if (NameText)
    {
        FString DisplayName = Result.Name.IsEmpty() ? TEXT("(이름 없음)") : Result.Name;
        NameText->SetText(FText::FromString(DisplayName));
    }

    // 모델명 텍스트 설정
    if (ModelNameText)
    {
        FString DisplayModel = Result.ModelName.IsEmpty() ? TEXT("(모델명 없음)") : Result.ModelName;
        ModelNameText->SetText(FText::FromString(DisplayModel));
    }

    // 상태 텍스트 설정
    if (StatusText)
    {
        FString ClassText = (Result.DeviceClass == EPJLinkClass::Class1) ? TEXT("Class 1") : TEXT("Class 2");
        FString AuthText = Result.bRequiresAuthentication ? TEXT("인증 필요") : TEXT("인증 불필요");

        StatusText->SetText(FText::FromString(FString::Printf(TEXT("%s, %s"), *ClassText, *AuthText)));
    }
}

void UPJLinkDiscoveryResultItemWidget::OnSelectButtonClicked()
{
    if (OnItemSelected.IsBound())
    {
        OnItemSelected.Execute(ItemIndex);
    }
}