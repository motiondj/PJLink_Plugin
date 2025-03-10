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

// PJLinkDiscoveryDefaultWidget.cpp 파일의 UpdateProgressBar_Implementation 함수 수정
void UPJLinkDiscoveryDefaultWidget::UpdateProgressBar_Implementation(float ProgressPercentage, int32 DiscoveredDevices, int32 ScannedAddresses)
{
    if (SearchProgressBar)
    {
        // 진행 바 값 업데이트
        SearchProgressBar->SetPercent(ProgressPercentage / 100.0f);

        // 진행 바 색상 변경 (진행도에 따라)
        FLinearColor ProgressColor;
        if (ProgressPercentage < 25.0f)
        {
            // 시작 단계: 파란색
            ProgressColor = FLinearColor(0.1f, 0.3f, 0.8f, 1.0f);
        }
        else if (ProgressPercentage < 75.0f)
        {
            // 중간 단계: 보라색~파란색 그라데이션
            float Alpha = (ProgressPercentage - 25.0f) / 50.0f;
            ProgressColor = FLinearColor::LerpUsingHSV(
                FLinearColor(0.4f, 0.2f, 0.8f, 1.0f), // 보라색
                FLinearColor(0.2f, 0.7f, 0.9f, 1.0f), // 밝은 파란색
                Alpha);
        }
        else
        {
            // 완료 단계: 밝은 파란색~녹색 그라데이션
            float Alpha = (ProgressPercentage - 75.0f) / 25.0f;
            ProgressColor = FLinearColor::LerpUsingHSV(
                FLinearColor(0.2f, 0.7f, 0.9f, 1.0f), // 밝은 파란색
                FLinearColor(0.2f, 0.8f, 0.4f, 1.0f), // 녹색
                Alpha);
        }

        // 프로그레스 바 색상 적용
        SearchProgressBar->SetFillColorAndOpacity(ProgressColor);
    }

    // 애니메이션 이미지 회전 속도 업데이트
    if (ScanningAnimationImage && DiscoveredDevices > 0)
    {
        // 발견된 장치 수에 따라 회전 속도 증가
        float RotationRate = FMath::Clamp(1.0f + (DiscoveredDevices * 0.1f), 1.0f, 3.0f);

        // 회전 애니메이션을 위한 초기화 코드
        ScanningAnimationImage->SetVisibility(ESlateVisibility::Visible);

        // 회전 애니메이션 속도 정보 저장 (블루프린트에서 사용)
        AnimationSpeedMultiplier = RotationRate;
    }
    else if (ScanningAnimationImage && ProgressPercentage >= 100.0f)
    {
        // 완료 시 애니메이션 중지
        ScanningAnimationImage->SetVisibility(ESlateVisibility::Hidden);
    }

    // 스캔된 주소 수 표시 업데이트
    if (ScannedAddressesText)
    {
        FString AddressText = FString::Printf(TEXT("스캔된 주소: %d"), ScannedAddresses);
        ScannedAddressesText->SetText(FText::FromString(AddressText));
    }
}

// PJLinkDiscoveryDefaultWidget.cpp 파일에 추가
void UPJLinkDiscoveryDefaultWidget::UpdateElapsedTime_Implementation(const FString& ElapsedTimeString)
{
    if (ElapsedTimeTextBlock)
    {
        // 소요 시간 텍스트 설정
        ElapsedTimeTextBlock->SetText(FText::FromString(ElapsedTimeString));

        // 시간 문자열에서 초 단위로 변환하여 색상 결정
        float TotalSeconds = 0.0f;

        // "X분 Y초", "X.Y초" 형식의 문자열에서 시간 추출
        if (ElapsedTimeString.Contains(TEXT("분")))
        {
            // "X분 Y초" 형식 파싱
            FString MinutesStr, SecondsStr;
            ElapsedTimeString.Split(TEXT("분 "), &MinutesStr, &SecondsStr);

            int32 Minutes = FCString::Atoi(*MinutesStr);
            // "Y초" 에서 숫자만 추출
            SecondsStr.Split(TEXT("초"), &SecondsStr, nullptr);
            int32 Seconds = FCString::Atoi(*SecondsStr.TrimEnd());

            TotalSeconds = Minutes * 60.0f + Seconds;
        }
        else if (ElapsedTimeString.EndsWith(TEXT("초")))
        {
            // "X.Y초" 형식 파싱
            FString SecondsStr;
            ElapsedTimeString.Split(TEXT("초"), &SecondsStr, nullptr);
            TotalSeconds = FCString::Atof(*SecondsStr);
        }

        // 시간에 따른 색상 결정
        FLinearColor TimeColor = TimerNormalColor;

        if (TotalSeconds >= TimerCriticalThreshold)
        {
            // 위험 시간 초과
            TimeColor = TimerCriticalColor;
        }
        else if (TotalSeconds >= TimerWarningThreshold)
        {
            // 경고 시간 초과
            TimeColor = TimerWarningColor;
        }

        // 색상 적용
        ElapsedTimeTextBlock->SetColorAndOpacity(FSlateColor(TimeColor));

        // 긴 시간이 걸리는 경우 시각적 표시 추가 (선택적)
        if (TotalSeconds >= TimerWarningThreshold)
        {
            // 글꼴 크기를 약간 키우거나 글꼴 스타일 변경
            FSlateFontInfo CurrentFont = ElapsedTimeTextBlock->Font;
            CurrentFont.Size += 1; // 폰트 크기 약간 증가
            CurrentFont.TypefaceFontName = TEXT("Bold"); // 볼드체로 변경
            ElapsedTimeTextBlock->SetFont(CurrentFont);
        }
        else
        {
            // 일반 폰트 스타일로 복원
            FSlateFontInfo DefaultFont = ElapsedTimeTextBlock->Font;
            DefaultFont.TypefaceFontName = TEXT("Regular"); // 일반 폰트로 변경
            ElapsedTimeTextBlock->SetFont(DefaultFont);
        }
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

// 장치 발견 효과 함수 추가
void UPJLinkDiscoveryDefaultWidget::ShowDeviceFoundEffect_Implementation(int32 DeviceIndex, const FLinearColor& EffectColor)
{
    // ResultsScrollBox의 자식 위젯 중에서 해당 인덱스의 항목 찾기
    if (ResultsScrollBox && DeviceIndex >= 0 && DeviceIndex < ResultsScrollBox->GetChildrenCount())
    {
        UWidget* ItemWidget = ResultsScrollBox->GetChildAt(DeviceIndex);
        if (ItemWidget)
        {
            // 강조 효과 적용 - 배경색 변경 또는 애니메이션 적용
            // 블루프린트에서 구현을 위해 이벤트 발생
            PlayDeviceFoundAnimation(ItemWidget, EffectColor);
        }
    }
}

// 펄스 애니메이션 효과 함수 추가
void UPJLinkDiscoveryDefaultWidget::PlayDeviceFoundAnimation_Implementation(UWidget* TargetWidget, const FLinearColor& EffectColor)
{
    // 블루프린트에서 구현될 함수의 스텁
    // C++에서는 기본 동작 제공 - 블루프린트에서 더 복잡한 애니메이션 구현 가능
    if (TargetWidget)
    {
        // 위젯이 Border인 경우 배경색 변경 시도
        UBorder* Border = Cast<UBorder>(TargetWidget);
        if (Border)
        {
            Border->SetBrushColor(EffectColor);

            // 원래 색상으로 돌아가는 타이머 설정
            FTimerHandle ResetColorTimerHandle;
            FTimerDelegate ResetColorDelegate;

            ResetColorDelegate.BindLambda([Border]()
                {
                    Border->SetBrushColor(FLinearColor(0.1f, 0.1f, 0.1f, 0.6f)); // 기본 색상으로 복원
                });

            GetWorld()->GetTimerManager().SetTimer(
                ResetColorTimerHandle,
                ResetColorDelegate,
                0.5f, // 0.5초 후 원래 색상으로 복원
                false
            );
        }
    }
}

// 회전 애니메이션 관련 함수 추가
void UPJLinkDiscoveryDefaultWidget::ApplyRotationToImage(float Angle)
{
    if (ScanningAnimationImage)
    {
        // 이미지 회전 설정
        // 언리얼 엔진에서는 회전을 래디안 단위로 설정하므로 변환 필요
        float RadianAngle = FMath::DegreesToRadians(Angle);

        // RenderTransform을 사용하여 회전 적용
        FWidgetTransform Transform = ScanningAnimationImage->RenderTransform;
        Transform.Angle = RadianAngle;
        ScanningAnimationImage->SetRenderTransform(Transform);
    }
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

    // 상태에 따른 UI 업데이트
    switch (NewState)
    {
    case EPJLinkDiscoveryState::Searching:
        // 검색 진행 중 상태 UI
        if (StatusTextBlock)
        {
            // 텍스트 색상을 파란색으로 변경
            StatusTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.2f, 0.6f, 1.0f, 1.0f)));
        }

        if (SearchProgressBar)
        {
            // 진행 바 표시
            SearchProgressBar->SetVisibility(ESlateVisibility::Visible);
        }

        if (ElapsedTimeTextBlock)
        {
            // 타이머 표시
            ElapsedTimeTextBlock->SetVisibility(ESlateVisibility::Visible);
            // 기본 색상으로 초기화
            ElapsedTimeTextBlock->SetColorAndOpacity(FSlateColor(TimerNormalColor));
        }

        if (CurrentIPTextBlock)
        {
            // 현재 스캔 중인 IP 표시
            CurrentIPTextBlock->SetVisibility(ESlateVisibility::Visible);
        }

        if (ScanningAnimationImage)
        {
            // 스캔 애니메이션 표시
            ScanningAnimationImage->SetVisibility(ESlateVisibility::Visible);
        }
        break;

    case EPJLinkDiscoveryState::Completed:
        // 검색 완료 상태 UI
        if (StatusTextBlock)
        {
            // 텍스트 색상을 녹색으로 변경
            StatusTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.2f, 0.8f, 0.2f, 1.0f)));
        }

        if (SearchProgressBar)
        {
            // 진행 바 100%로 설정
            SearchProgressBar->SetPercent(1.0f);
            // 진행 바 색상을 녹색으로 변경
            SearchProgressBar->SetFillColorAndOpacity(FLinearColor(0.2f, 0.8f, 0.4f, 1.0f));
        }

        if (CurrentIPTextBlock)
        {
            // 현재 스캔 중인 IP 숨기기
            CurrentIPTextBlock->SetVisibility(ESlateVisibility::Collapsed);
        }

        if (ScanningAnimationImage)
        {
            // 스캔 애니메이션 숨기기
            ScanningAnimationImage->SetVisibility(ESlateVisibility::Collapsed);
        }
        break;

    case EPJLinkDiscoveryState::Cancelled:
        // 검색 취소 상태 UI
        if (StatusTextBlock)
        {
            // 텍스트 색상을 노란색으로 변경
            StatusTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.8f, 0.2f, 1.0f)));
        }

        if (SearchProgressBar)
        {
            // 진행 바 숨기기
            SearchProgressBar->SetVisibility(ESlateVisibility::Hidden);
        }

        if (CurrentIPTextBlock)
        {
            // 현재 스캔 중인 IP 숨기기
            CurrentIPTextBlock->SetVisibility(ESlateVisibility::Collapsed);
        }

        if (ScanningAnimationImage)
        {
            // 스캔 애니메이션 숨기기
            ScanningAnimationImage->SetVisibility(ESlateVisibility::Collapsed);
        }
        break;

    case EPJLinkDiscoveryState::Failed:
        // 검색 실패 상태 UI
        if (StatusTextBlock)
        {
            // 텍스트 색상을 빨간색으로 변경
            StatusTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.3f, 0.3f, 1.0f)));
        }

        if (SearchProgressBar)
        {
            // 진행 바 숨기기
            SearchProgressBar->SetVisibility(ESlateVisibility::Hidden);
        }

        if (CurrentIPTextBlock)
        {
            // 현재 스캔 중인 IP 숨기기
            CurrentIPTextBlock->SetVisibility(ESlateVisibility::Collapsed);
        }

        if (ScanningAnimationImage)
        {
            // 스캔 애니메이션 숨기기
            ScanningAnimationImage->SetVisibility(ESlateVisibility::Collapsed);
        }
        break;

    case EPJLinkDiscoveryState::Idle:
    default:
        // 대기 상태 UI
        if (StatusTextBlock)
        {
            // 텍스트 색상을 기본색으로 변경
            StatusTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        }

        if (SearchProgressBar)
        {
            // 진행 바 0%로 설정
            SearchProgressBar->SetPercent(0.0f);
            // 진행 바 표시 (선택적)
            SearchProgressBar->SetVisibility(ESlateVisibility::Visible);
        }

        if (ElapsedTimeTextBlock)
        {
            // 타이머 숨기기
            ElapsedTimeTextBlock->SetVisibility(ESlateVisibility::Hidden);
        }

        if (CurrentIPTextBlock)
        {
            // 현재 스캔 중인 IP 숨기기
            CurrentIPTextBlock->SetVisibility(ESlateVisibility::Collapsed);
        }

        if (ScanningAnimationImage)
        {
            // 스캔 애니메이션 숨기기
            ScanningAnimationImage->SetVisibility(ESlateVisibility::Collapsed);
        }
        break;
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

void UPJLinkDiscoveryDefaultWidget::UpdateProgressAnimation_Implementation(float ProgressPercentage)
{
    // 진행 상황에 따른 애니메이션 속도 및 색상 조정
    if (ScanningAnimationImage)
    {
        // 애니메이션 속도를 진행률 및 발견된 장치 수에 따라 조정
        float SpeedMultiplier = FMath::Min(3.0f, 1.0f + (ProgressPercentage / 100.0f));
        AnimationSpeedMultiplier = SpeedMultiplier;

        // 진행률에 따른 색상 그라데이션 계산 (0% = 파란색, 100% = 녹색)
        FLinearColor AnimColor = FLinearColor::LerpUsingHSV(
            FLinearColor(0.2f, 0.5f, 1.0f, 1.0f), // 시작 색상 (파란색)
            FLinearColor(0.2f, 0.8f, 0.4f, 1.0f), // 종료 색상 (녹색)
            ProgressPercentage / 100.0f           // 진행률 비율
        );

        // 이미지에 색상 적용
        ScanningAnimationImage->SetColorAndOpacity(AnimColor);

        // 이미지 크기도 진행 상황에 따라 약간 변화 (펄스 효과)
        FVector2D BaseSize(32.0f, 32.0f); // 기본 크기 
        float PulseValue = FMath::Sin(GetWorld()->GetTimeSeconds() * 4.0f) * 0.1f + 1.0f;
        ScanningAnimationImage->SetDesiredSizeOverride(BaseSize * PulseValue * SpeedMultiplier);

        // 회전 속도 업데이트 (회전은 AnimationSpeedMultiplier에 따라 NativeTick에서 처리)
    }
}

// 스캐닝 시작 함수
void UPJLinkDiscoveryDefaultWidget::StartProgressAnimation_Implementation()
{
    // 애니메이션 이미지 초기화 및 표시
    if (ScanningAnimationImage)
    {
        // 이미지 표시
        ScanningAnimationImage->SetVisibility(ESlateVisibility::Visible);

        // 초기 색상 설정
        ScanningAnimationImage->SetColorAndOpacity(FLinearColor(0.2f, 0.5f, 1.0f, 1.0f));

        // 애니메이션 속도 초기화
        AnimationSpeedMultiplier = 1.0f;
    }

    if (SearchProgressBar)
    {
        // 프로그레스 바 초기화
        SearchProgressBar->SetPercent(0.0f);
        SearchProgressBar->SetVisibility(ESlateVisibility::Visible);
    }

    // 상태 텍스트 업데이트
    if (StatusTextBlock)
    {
        StatusTextBlock->SetText(FText::FromString(TEXT("검색 중...")));
        StatusTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.2f, 0.6f, 1.0f, 1.0f)));
    }

    // 검색 버튼 상태 업데이트
    UpdateButtonStates();
}

// 스캐닝 중지 함수
void UPJLinkDiscoveryDefaultWidget::StopProgressAnimation_Implementation()
{
    // 애니메이션 이미지 천천히 사라지게 설정 (서서히 투명해지는 효과)
    if (ScanningAnimationImage)
    {
        // 즉시 숨기지 않고 서서히 투명해지는 효과를 위해 바로 숨기지 않음
        // 실제 숨김 처리는 FadeOutAnimation 블루프린트에서 처리
        FadeOutScanningAnimation();
    }

    // 버튼 상태 업데이트
    UpdateButtonStates();
}

// 장치 발견 효과 함수 (블루프린트에서 호출할 함수 템플릿)
void UPJLinkDiscoveryDefaultWidget::ShowDeviceFoundEffect_Implementation(int32 DeviceIndex, const FLinearColor& EffectColor)
{
    // 블루프린트에서 구현: 발견된 장치에 시각적 효과 적용
}

// FadeOutScanningAnimation 함수 - 블루프린트에서 구현할 기능을 위한 템플릿
UFUNCTION(BlueprintImplementableEvent, Category = "PJLink|Discovery|Animation")
void UPJLinkDiscoveryDefaultWidget::FadeOutScanningAnimation();

// PlayDeviceFoundAnimation 함수 - 블루프린트에서 구현할 기능을 위한 템플릿
UFUNCTION(BlueprintImplementableEvent, Category = "PJLink|Discovery|Animation")
void UPJLinkDiscoveryDefaultWidget::PlayDeviceFoundAnimation(UWidget* TargetWidget, const FLinearColor& EffectColor);

// 결과 항목 선택 이벤트 처리
void UPJLinkDiscoveryDefaultWidget::OnResultItemClicked(int32 ItemIndex)
{
    // 단일 선택 모드인 경우 기존 선택 해제
    if (bSingleSelectionMode)
    {
        ClearSelection();
    }

    // 선택 상태 토글
    ToggleDeviceSelection(ItemIndex);

    // 선택된 장치가 있으면 세부 정보 표시
    if (SelectedDeviceIndex >= 0)
    {
        ShowSelectedDeviceDetails();
    }
    else if (DetailPanel)
    {
        // 선택 해제된 경우 세부 정보 패널 숨기기
        DetailPanel->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UPJLinkDiscoveryDefaultWidget::OnManageButtonClicked()
{
    ManageSelectedDevice();
}

void UPJLinkDiscoveryDefaultWidget::OnConnectButtonClicked()
{
    ConnectToSelectedDevice();
}

void UPJLinkDiscoveryDefaultWidget::OnAddToGroupButtonClicked()
{
    // 그룹 선택 다이얼로그 표시 (블루프린트에서 구현)
    ShowGroupSelectionDialog();
}

void UPJLinkDiscoveryDefaultWidget::OnSaveAsPresetButtonClicked()
{
    // 프리셋 이름 입력 다이얼로그 표시 (블루프린트에서 구현)
    ShowPresetNameInputDialog();
}

// 블루프린트로 구현할 다이얼로그 함수
void UPJLinkDiscoveryDefaultWidget::ShowGroupSelectionDialog_Implementation()
{
    // 기본 구현은 비어있음 - 블루프린트에서 구현
}

void UPJLinkDiscoveryDefaultWidget::ShowPresetNameInputDialog_Implementation()
{
    // 기본 구현은 비어있음 - 블루프린트에서 구현
}

// 선택 상태 변경 처리
void UPJLinkDiscoveryDefaultWidget::OnSelectionChanged(const TArray<int32>& SelectedIndices)
{
    // 선택된 항목이 있으면 관리 버튼 활성화
    if (ManageButton)
    {
        ManageButton->SetIsEnabled(SelectedIndices.Num() > 0);
    }

    if (ConnectButton)
    {
        ConnectButton->SetIsEnabled(SelectedIndices.Num() > 0);
    }

    if (AddToGroupButton)
    {
        AddToGroupButton->SetIsEnabled(SelectedIndices.Num() > 0);
    }

    if (SaveAsPresetButton)
    {
        SaveAsPresetButton->SetIsEnabled(SelectedIndices.Num() > 0);
    }
}

// 검색 결과 확장 업데이트
void UPJLinkDiscoveryDefaultWidget::UpdateResultsList_Implementation(const TArray<FPJLinkDiscoveryResult>& Results)
{
    // 상위 클래스 구현 호출
    Super::UpdateResultsList_Implementation(Results);

    // 선택 상태 초기화
    ClearSelection();

    // 결과 없음 메시지 처리
    if (Results.Num() == 0)
    {
        if (DetailPanel)
        {
            DetailPanel->SetVisibility(ESlateVisibility::Collapsed);
        }

        if (NoResultsBox)
        {
            NoResultsBox->SetVisibility(ESlateVisibility::Visible);
        }
    }
    else
    {
        if (NoResultsBox)
        {
            NoResultsBox->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
}

// 선택된 장치 강조 표시 구현
void UPJLinkDiscoveryDefaultWidget::HighlightSelectedDevice_Implementation(int32 DeviceIndex)
{
    // 결과 아이템 위젯들을 조회하여 선택 상태 시각적 표시
    if (ResultsScrollBox)
    {
        for (int32 i = 0; i < ResultsScrollBox->GetChildrenCount(); i++)
        {
            UPJLinkDiscoveryResultItemWidget* ItemWidget =
                Cast<UPJLinkDiscoveryResultItemWidget>(ResultsScrollBox->GetChildAt(i));

            if (ItemWidget)
            {
                // 선택 상태 설정
                ItemWidget->SetSelected(ItemWidget->ItemIndex == DeviceIndex);
            }
        }
    }
}

void UPJLinkDiscoveryDefaultWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 기존 버튼 이벤트 바인딩

    // 확장 컨트롤 관련 이벤트 바인딩
    if (ExtendedControlsToggleButton)
    {
        ExtendedControlsToggleButton->OnClicked.AddDynamic(this, &UPJLinkDiscoveryDefaultWidget::OnToggleExtendedControlsClicked);
    }

    if (BatchConnectButton)
    {
        BatchConnectButton->OnClicked.AddDynamic(this, &UPJLinkDiscoveryDefaultWidget::OnBatchConnectClicked);
    }

    if (BatchSaveAsPresetsButton)
    {
        BatchSaveAsPresetsButton->OnClicked.AddDynamic(this, &UPJLinkDiscoveryDefaultWidget::OnBatchSaveAsPresetsClicked);
    }

    if (BatchAddToGroupButton)
    {
        BatchAddToGroupButton->OnClicked.AddDynamic(this, &UPJLinkDiscoveryDefaultWidget::OnBatchAddToGroupClicked);
    }

    // 초기 확장 컨트롤 패널 상태 설정
    UpdateExtendedControlPanelVisibility();

    // 장치 선택 이벤트 연결
    OnDeviceSelectionChanged.AddUniqueDynamic(this, &UPJLinkDiscoveryDefaultWidget::OnSelectionChanged);
}

// 확장 컨트롤 토글 버튼 클릭 이벤트
void UPJLinkDiscoveryDefaultWidget::OnToggleExtendedControlsClicked()
{
    ToggleExtendedControlPanel();
}

// 일괄 연결 버튼 클릭 이벤트
void UPJLinkDiscoveryDefaultWidget::OnBatchConnectClicked()
{
    ShowBatchConnectUI();
}

// 일괄 프리셋 저장 버튼 클릭 이벤트
void UPJLinkDiscoveryDefaultWidget::OnBatchSaveAsPresetsClicked()
{
    ShowBatchSaveAsPresetsUI();
}

// 일괄 그룹 추가 버튼 클릭 이벤트
void UPJLinkDiscoveryDefaultWidget::OnBatchAddToGroupClicked()
{
    ShowBatchAddToGroupUI();
}

// 확장 컨트롤 패널 가시성 업데이트 구현
void UPJLinkDiscoveryDefaultWidget::UpdateExtendedControlPanelVisibility_Implementation()
{
    if (ExtendedControlPanel)
    {
        ExtendedControlPanel->SetVisibility(
            bShowExtendedControlPanel ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    // 확장 컨트롤 토글 버튼 텍스트 업데이트
    if (ExtendedControlsToggleText)
    {
        ExtendedControlsToggleText->SetText(FText::FromString(
            bShowExtendedControlPanel ? TEXT("간단히 보기") : TEXT("확장 제어판")));
    }
}

// 세부 정보 패널 크기 업데이트 구현
void UPJLinkDiscoveryDefaultWidget::UpdateDetailPanelSize_Implementation()
{
    if (DetailPanel)
    {
        // 확장된 상태에 따라 패널 크기 변경
        if (bExpandedDeviceDetails)
        {
            // 확장된 크기로 변경 (UE4 위젯 크기 조정 방식에 따라 구현)
            UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(DetailPanel->Slot);
            if (OverlaySlot)
            {
                OverlaySlot->SetSize(FVector2D(1.0f, 0.4f)); // 높이를 40%로 확장
            }
        }
        else
        {
            // 기본 크기로 변경
            UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(DetailPanel->Slot);
            if (OverlaySlot)
            {
                OverlaySlot->SetSize(FVector2D(1.0f, 0.25f)); // 높이를 25%로 설정
            }
        }
    }
}

// 작업 진행 중 표시 가시성 설정 구현
void UPJLinkDiscoveryDefaultWidget::SetWorkingIndicatorVisible_Implementation(bool bVisible)
{
    if (WorkingIndicator)
    {
        WorkingIndicator->SetVisibility(
            bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
}