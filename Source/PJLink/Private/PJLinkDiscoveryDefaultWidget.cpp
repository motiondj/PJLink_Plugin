// PJLinkDiscoveryDefaultWidget.cpp
#include "PJLinkDiscoveryDefaultWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/ScrollBox.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/VerticalBox.h"
#include "Components/Border.h"
#include "Components/Image.h"

UPJLinkDiscoveryDefaultWidget::UPJLinkDiscoveryDefaultWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UPJLinkDiscoveryDefaultWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 버튼 이벤트 바인딩
    if (BroadcastSearchButton)
    {
        BroadcastSearchButton->OnClicked.AddDynamic(this, &UPJLinkDiscoveryDefaultWidget::OnBroadcastSearchClicked);
    }

    if (RangeSearchButton)
    {
        RangeSearchButton->OnClicked.AddDynamic(this, &UPJLinkDiscoveryDefaultWidget::OnRangeSearchClicked);
    }

    if (SubnetSearchButton)
    {
        SubnetSearchButton->OnClicked.AddDynamic(this, &UPJLinkDiscoveryDefaultWidget::OnSubnetSearchClicked);
    }

    if (CancelSearchButton)
    {
        CancelSearchButton->OnClicked.AddDynamic(this, &UPJLinkDiscoveryDefaultWidget::OnCancelSearchClicked);
    }

    // 정렬 및 필터 옵션 초기화
    InitializeSortOptions();
    InitializeFilterOptions();

    // 초기 버튼 상태 설정
    UpdateButtonStates();

    // 초기 진행 상태 설정
    if (SearchProgressBar)
    {
        SearchProgressBar->SetPercent(0.0f);
    }

    // 세부 정보 패널 초기화
    if (DetailPanel)
    {
        DetailPanel->SetVisibility(ESlateVisibility::Collapsed);
    }

    // 결과 없음 패널 초기화
    if (NoResultsBox)
    {
        NoResultsBox->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UPJLinkDiscoveryDefaultWidget::UpdateProgressBar_Implementation(float ProgressPercentage, int32 DiscoveredDevices, int32 ScannedAddresses)
{
    if (SearchProgressBar)
    {
        SearchProgressBar->SetPercent(ProgressPercentage / 100.0f);
    }
}

void UPJLinkDiscoveryDefaultWidget::UpdateStatusText_Implementation(const FString& StatusText)
{
    if (StatusTextBlock)
    {
        StatusTextBlock->SetText(FText::FromString(StatusText));
    }
}

void UPJLinkDiscoveryDefaultWidget::UpdateResultsList_Implementation(const TArray<FPJLinkDiscoveryResult>& Results)
{
    if (!ResultsScrollBox)
    {
        return;
    }

    // 결과 목록 초기화
    ClearResultsPanel();

    // 결과가 없는 경우 처리는 ShowNoResultsMessage_Implementation에서 수행
    if (Results.Num() == 0)
    {
        return;
    }

    // 결과가 있으면 NoResultsBox 숨기기
    if (NoResultsBox)
    {
        NoResultsBox->SetVisibility(ESlateVisibility::Collapsed);
    }

    // 각 결과에 대한 항목 위젯 생성 및 추가
    for (int32 i = 0; i < Results.Num(); ++i)
    {
        UUserWidget* ItemWidget = CreateResultItemWidget(Results[i], i);
        if (ItemWidget)
        {
            ResultsScrollBox->AddChild(ItemWidget);
        }
    }
}

UUserWidget* UPJLinkDiscoveryDefaultWidget::CreateResultItemWidget_Implementation(const FPJLinkDiscoveryResult& Result, int32 Index)
{
    if (!ResultItemWidgetClass)
    {
        return nullptr;
    }

    // 항목 위젯 생성
    UUserWidget* ItemWidget = CreateWidget<UUserWidget>(this, ResultItemWidgetClass);
    if (!ItemWidget)
    {
        return nullptr;
    }

    // 결과 데이터 설정을 위한 함수 호출 (결과 항목 위젯 클래스에 구현 필요)
    IResultItemInterface* ResultItem = Cast<IResultItemInterface>(ItemWidget);
    if (ResultItem)
    {
        ResultItem->SetResultData(Result, Index);

        // 항목 선택 이벤트 바인딩 (필요한 경우)
        ResultItem->OnItemSelected.BindUObject(this, &UPJLinkDiscoveryDefaultWidget::SelectDevice);
    }

    return ItemWidget;
}

void UPJLinkDiscoveryDefaultWidget::UpdateDetailPanel_Implementation(const FPJLinkDiscoveryResult& SelectedResult)
{
    if (!DetailPanel)
    {
        return;
    }

    // 선택된 장치가 있으면 세부 정보 패널 표시
    DetailPanel->SetVisibility(ESlateVisibility::Visible);

    // 세부 정보 패널의 내용 업데이트
    // 이 부분은 블루프린트에서 구현하거나 여기서 직접 구현할 수 있음
}

void UPJLinkDiscoveryDefaultWidget::ShowNoResultsMessage_Implementation(bool bIsSearching, bool bHasFilters)
{
    if (!NoResultsBox)
    {
        return;
    }

    // 결과 없음 패널 표시
    NoResultsBox->SetVisibility(ESlateVisibility::Visible);

    // 메시지 설정 (NoResultsBox에 TextBlock이 있다고 가정)
    UTextBlock* MessageTextBlock = Cast<UTextBlock>(NoResultsBox->GetChildAt(0));
    if (MessageTextBlock)
    {
        FString Message;

        if (bIsSearching)
        {
            Message = TEXT("검색 중...");
        }
        else if (bHasFilters)
        {
            Message = TEXT("검색 조건에 맞는 장치가 없습니다.");
        }
        else
        {
            Message = TEXT("발견된 장치가 없습니다.");
        }

        MessageTextBlock->SetText(FText::FromString(Message));
    }
}

void UPJLinkDiscoveryDefaultWidget::StartProgressAnimation_Implementation()
{
    // 진행 상태 애니메이션 시작
    // 이 부분은 블루프린트에서 구현하거나 여기서 직접 구현할 수 있음
}

void UPJLinkDiscoveryDefaultWidget::StopProgressAnimation_Implementation()
{
    // 진행 상태 애니메이션 중지
    // 이 부분은 블루프린트에서 구현하거나 여기서 직접 구현할 수 있음
}

void UPJLinkDiscoveryDefaultWidget::UpdateProgressAnimation_Implementation(float ProgressPercentage)
{
    // 진행 상태에 따른 애니메이션 업데이트
    // 이 부분은 블루프린트에서 구현하거나 여기서 직접 구현할 수 있음
}

void UPJLinkDiscoveryDefaultWidget::OnDiscoveryStateChanged_Implementation(EPJLinkDiscoveryState NewState)
{
    // 검색 상태 변경에 따른 UI 업데이트
    UpdateButtonStates();

    // 진행 바 업데이트
    if (NewState == EPJLinkDiscoveryState::Idle ||
        NewState == EPJLinkDiscoveryState::Completed ||
        NewState == EPJLinkDiscoveryState::Cancelled ||
        NewState == EPJLinkDiscoveryState::Failed)
    {
        if (SearchProgressBar)
        {
            SearchProgressBar->SetPercent(0.0f);
        }
    }
}

void UPJLinkDiscoveryDefaultWidget::OnShowMessage_Implementation(const FString& Message, const FLinearColor& Color)
{
    // 메시지 표시
    if (StatusTextBlock)
    {
        StatusTextBlock->SetText(FText::FromString(Message));
        StatusTextBlock->SetColorAndOpacity(FSlateColor(Color));
    }
}

void UPJLinkDiscoveryDefaultWidget::OnHideMessage_Implementation()
{
    // 메시지 숨기기
    if (StatusTextBlock)
    {
        StatusTextBlock->SetText(FText::GetEmpty());
    }
}

void UPJLinkDiscoveryDefaultWidget::OnBroadcastSearchClicked()
{
    StartBroadcastSearch(DefaultSearchTimeout);
}

void UPJLinkDiscoveryDefaultWidget::OnRangeSearchClicked()
{
    StartRangeSearch(DefaultStartIP, DefaultEndIP, DefaultSearchTimeout);
}

void UPJLinkDiscoveryDefaultWidget::OnSubnetSearchClicked()
{
    StartSubnetSearch(DefaultSubnetAddress, DefaultSubnetMask, DefaultSearchTimeout);
}

void UPJLinkDiscoveryDefaultWidget::OnCancelSearchClicked()
{
    CancelSearch();
}

void UPJLinkDiscoveryDefaultWidget::OnSortOptionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    // 선택된 정렬 옵션에 따라 CurrentSortOption 업데이트
    if (SelectedItem.Equals(TEXT("IP 주소순")))
    {
        SetSortOption(EPJLinkDiscoverySortOption::ByIPAddress);
    }
    else if (SelectedItem.Equals(TEXT("이름순")))
    {
        SetSortOption(EPJLinkDiscoverySortOption::ByName);
    }
    else if (SelectedItem.Equals(TEXT("응답시간순")))
    {
        SetSortOption(EPJLinkDiscoverySortOption::ByResponseTime);
    }
    else if (SelectedItem.Equals(TEXT("모델명순")))
    {
        SetSortOption(EPJLinkDiscoverySortOption::ByModelName);
    }
    else if (SelectedItem.Equals(TEXT("발견시간순")))
    {
        SetSortOption(EPJLinkDiscoverySortOption::ByDiscoveryTime);
    }
}

void UPJLinkDiscoveryDefaultWidget::OnFilterOptionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    // 선택된 필터 옵션에 따라 CurrentFilterOption 업데이트
    if (SelectedItem.Equals(TEXT("모두 표시")))
    {
        SetFilterOption(EPJLinkDiscoveryFilterOption::All);
    }
    else if (SelectedItem.Equals(TEXT("인증 필요")))
    {
        SetFilterOption(EPJLinkDiscoveryFilterOption::RequiresAuth);
    }
    else if (SelectedItem.Equals(TEXT("인증 불필요")))
    {
        SetFilterOption(EPJLinkDiscoveryFilterOption::NoAuth);
    }
    else if (SelectedItem.Equals(TEXT("클래스 1")))
    {
        SetFilterOption(EPJLinkDiscoveryFilterOption::Class1);
    }
    else if (SelectedItem.Equals(TEXT("클래스 2")))
    {
        SetFilterOption(EPJLinkDiscoveryFilterOption::Class2);
    }
}

void UPJLinkDiscoveryDefaultWidget::OnFilterTextChanged(const FText& Text)
{
    SetFilterText(Text.ToString());
}

void UPJLinkDiscoveryDefaultWidget::InitializeSortOptions()
{
    if (!SortOptionsComboBox)
    {
        return;
    }

    // ComboBox 항목 추가
    SortOptionsComboBox->AddOption(TEXT("IP 주소순"));
    SortOptionsComboBox->AddOption(TEXT("이름순"));
    SortOptionsComboBox->AddOption(TEXT("응답시간순"));
    SortOptionsComboBox->AddOption(TEXT("모델명순"));
    SortOptionsComboBox->AddOption(TEXT("발견시간순"));

    // 기본 선택 항목 설정
    SortOptionsComboBox->SetSelectedIndex(0);

    // 이벤트 바인딩
    SortOptionsComboBox->OnSelectionChanged.AddDynamic(this, &UPJLinkDiscoveryDefaultWidget::OnSortOptionChanged);
}

void UPJLinkDiscoveryDefaultWidget::InitializeFilterOptions()
{
    if (!FilterOptionsComboBox)
    {
        return;
    }

    // ComboBox 항목 추가
    FilterOptionsComboBox->AddOption(TEXT("모두 표시"));
    FilterOptionsComboBox->AddOption(TEXT("인증 필요"));
    FilterOptionsComboBox->AddOption(TEXT("인증 불필요"));
    FilterOptionsComboBox->AddOption(TEXT("클래스 1"));
    FilterOptionsComboBox->AddOption(TEXT("클래스 2"));

    // 기본 선택 항목 설정
    FilterOptionsComboBox->SetSelectedIndex(0);

    // 이벤트 바인딩
    FilterOptionsComboBox->OnSelectionChanged.AddDynamic(this, &UPJLinkDiscoveryDefaultWidget::OnFilterOptionChanged);

    // 필터 텍스트 이벤트 바인딩
    if (FilterTextBox)
    {
        FilterTextBox->OnTextChanged.AddDynamic(this, &UPJLinkDiscoveryDefaultWidget::OnFilterTextChanged);
    }
}

void UPJLinkDiscoveryDefaultWidget::UpdateButtonStates()
{
    // 검색 상태에 따라 버튼 활성화/비활성화
    bool bIsSearching = (CurrentDiscoveryState == EPJLinkDiscoveryState::Searching);

    if (BroadcastSearchButton)
    {
        BroadcastSearchButton->SetIsEnabled(!bIsSearching);
    }

    if (RangeSearchButton)
    {
        RangeSearchButton->SetIsEnabled(!bIsSearching);
    }

    if (SubnetSearchButton)
    {
        SubnetSearchButton->SetIsEnabled(!bIsSearching);
    }

    if (CancelSearchButton)
    {
        CancelSearchButton->SetIsEnabled(bIsSearching);
    }
}

void UPJLinkDiscoveryDefaultWidget::ClearResultsPanel()
{
    if (ResultsScrollBox)
    {
        ResultsScrollBox->ClearChildren();
    }
}