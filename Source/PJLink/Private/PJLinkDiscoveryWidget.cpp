// PJLinkDiscoveryWidget.cpp
#include "PJLinkDiscoveryWidget.h"
#include "PJLinkLog.h"
#include "PJLinkPresetManager.h"
#include "PJLinkManagerComponent.h"
#include "UPJLinkComponent.h"
#include "Kismet/GameplayStatics.h"

UPJLinkDiscoveryWidget::UPJLinkDiscoveryWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer), DiscoveryManager(nullptr)
{
}

void UPJLinkDiscoveryWidget::NativeConstruct()
{
    Super::NativeConstruct();

    CreateDiscoveryManager();
}

void UPJLinkDiscoveryWidget::NativeDestruct()
{
    if (DiscoveryManager)
    {
        DiscoveryManager->CancelAllDiscoveries();
    }

    Super::NativeDestruct();
}

void UPJLinkDiscoveryWidget::CreateDiscoveryManager()
{
    if (!DiscoveryManager)
    {
        DiscoveryManager = NewObject<UPJLinkDiscoveryManager>(this);
        SetupDiscovery();
    }
}

void UPJLinkDiscoveryWidget::SetupDiscovery()
{
    if (!DiscoveryManager)
    {
        return;
    }

    // 이벤트 바인딩
    DiscoveryManager->OnDiscoveryCompleted.AddDynamic(this, &UPJLinkDiscoveryWidget::OnDiscoveryCompleted);
    DiscoveryManager->OnDeviceDiscovered.AddDynamic(this, &UPJLinkDiscoveryWidget::OnDeviceDiscovered);
    DiscoveryManager->OnDiscoveryProgress.AddDynamic(this, &UPJLinkDiscoveryWidget::OnDiscoveryProgressUpdated);
}

void UPJLinkDiscoveryWidget::StartBroadcastSearch(float TimeoutSeconds)
{
    if (!DiscoveryManager)
    {
        CreateDiscoveryManager();
    }

    // 기존 결과 초기화
    DiscoveryResults.Empty();
    UpdateResultsList(DiscoveryResults);

    // 상태 업데이트
    UpdateStatusText(TEXT("Starting broadcast search..."));
    UpdateProgressBar(0.0f, 0, 0);

    // 검색 시작
    CurrentDiscoveryID = DiscoveryManager->StartBroadcastDiscovery(TimeoutSeconds);

    if (CurrentDiscoveryID.IsEmpty())
    {
        UpdateStatusText(TEXT("Failed to start broadcast search"));
    }
}

void UPJLinkDiscoveryWidget::StartRangeSearch(const FString& StartIP, const FString& EndIP, float TimeoutSeconds)
{
    if (!DiscoveryManager)
    {
        CreateDiscoveryManager();
    }

    // 기존 결과 초기화
    DiscoveryResults.Empty();
    UpdateResultsList(DiscoveryResults);

    // 상태 업데이트
    UpdateStatusText(FString::Printf(TEXT("Starting IP range search (%s - %s)..."), *StartIP, *EndIP));
    UpdateProgressBar(0.0f, 0, 0);

    // 검색 시작
    CurrentDiscoveryID = DiscoveryManager->StartRangeScan(StartIP, EndIP, TimeoutSeconds);

    if (CurrentDiscoveryID.IsEmpty())
    {
        UpdateStatusText(TEXT("Failed to start IP range search"));
    }
}

void UPJLinkDiscoveryWidget::StartSubnetSearch(const FString& SubnetAddress, const FString& SubnetMask, float TimeoutSeconds)
{
    if (!DiscoveryManager)
    {
        CreateDiscoveryManager();
    }

    // 기존 결과 초기화
    DiscoveryResults.Empty();
    UpdateResultsList(DiscoveryResults);

    // 상태 업데이트
    UpdateStatusText(FString::Printf(TEXT("Starting subnet search (%s/%s)..."), *SubnetAddress, *SubnetMask));
    UpdateProgressBar(0.0f, 0, 0);

    // 검색 시작
    CurrentDiscoveryID = DiscoveryManager->StartSubnetScan(SubnetAddress, SubnetMask, TimeoutSeconds);

    if (CurrentDiscoveryID.IsEmpty())
    {
        UpdateStatusText(TEXT("Failed to start subnet search"));
    }
}

void UPJLinkDiscoveryWidget::CancelSearch()
{
    if (DiscoveryManager && !CurrentDiscoveryID.IsEmpty())
    {
        DiscoveryManager->CancelDiscovery(CurrentDiscoveryID);
        UpdateStatusText(TEXT("Search cancelled"));
    }
}

void UPJLinkDiscoveryWidget::ConnectToDevice(const FPJLinkDiscoveryResult& Device)
{
    if (!DiscoveryManager)
    {
        return;
    }

    // 프로젝터 정보로 변환
    FPJLinkProjectorInfo ProjectorInfo = DiscoveryManager->ConvertToProjectorInfo(Device);

    // 액터 찾기
    AActor* OwnerActor = nullptr;
    TArray<AActor*> Actors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), Actors);

    for (AActor* Actor : Actors)
    {
        UPJLinkComponent* PJLinkComp = Actor->FindComponentByClass<UPJLinkComponent>();
        if (PJLinkComp)
        {
            // 기존 컴포넌트 재사용
            PJLinkComp->SetProjectorInfo(ProjectorInfo);
            PJLinkComp->Connect();

            UpdateStatusText(FString::Printf(TEXT("Connected to %s (%s)"), *ProjectorInfo.Name, *ProjectorInfo.IPAddress));
            return;
        }
    }

    // 적합한 컴포넌트가 없으면 매니저 컴포넌트 검색
    for (AActor* Actor : Actors)
    {
        UPJLinkManagerComponent* ManagerComp = Actor->FindComponentByClass<UPJLinkManagerComponent>();
        if (ManagerComp)
        {
            UPJLinkComponent* NewComp = ManagerComp->AddProjectorByInfo(ProjectorInfo);
            if (NewComp)
            {
                NewComp->Connect();
                UpdateStatusText(FString::Printf(TEXT("Added and connected to %s (%s)"), *ProjectorInfo.Name, *ProjectorInfo.IPAddress));
                return;
            }
        }
    }

    UpdateStatusText(TEXT("No suitable actor found to connect to the device"));
}

void UPJLinkDiscoveryWidget::SaveDeviceAsPreset(const FPJLinkDiscoveryResult& Device, const FString& PresetName)
{
    if (!DiscoveryManager)
    {
        return;
    }

    bool bSuccess = DiscoveryManager->SaveDiscoveryResultAsPreset(Device, PresetName);

    if (bSuccess)
    {
        UpdateStatusText(FString::Printf(TEXT("Saved device %s as preset '%s'"), *Device.IPAddress, *PresetName));
    }
    else
    {
        UpdateStatusText(FString::Printf(TEXT("Failed to save device %s as preset"), *Device.IPAddress));
    }
}

void UPJLinkDiscoveryWidget::AddDeviceToGroup(const FPJLinkDiscoveryResult& Device, const FString& GroupName)
{
    if (!DiscoveryManager)
    {
        return;
    }

    // 프로젝터 정보로 변환
    FPJLinkProjectorInfo ProjectorInfo = DiscoveryManager->ConvertToProjectorInfo(Device);

    // 매니저 컴포넌트 검색
    TArray<AActor*> Actors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), Actors);

    for (AActor* Actor : Actors)
    {
        UPJLinkManagerComponent* ManagerComp = Actor->FindComponentByClass<UPJLinkManagerComponent>();
        if (ManagerComp)
        {
            // 그룹이 없으면 생성
            if (!ManagerComp->DoesGroupExist(GroupName))
            {
                ManagerComp->CreateGroup(GroupName);
            }

            UPJLinkComponent* NewComp = ManagerComp->AddProjectorByInfo(ProjectorInfo, GroupName);
            if (NewComp)
            {
                UpdateStatusText(FString::Printf(TEXT("Added device %s to group '%s'"), *ProjectorInfo.Name, *GroupName));
                return;
            }
        }
    }

    UpdateStatusText(TEXT("No manager component found to add device to group"));
}

void UPJLinkDiscoveryWidget::SaveAllResultsAsGroup(const FString& GroupName)
{
    if (!DiscoveryManager || CurrentDiscoveryID.IsEmpty())
    {
        return;
    }

    int32 AddedCount = DiscoveryManager->SaveDiscoveryResultsAsGroup(CurrentDiscoveryID, GroupName);

    if (AddedCount > 0)
    {
        UpdateStatusText(FString::Printf(TEXT("Added %d devices to group '%s'"), AddedCount, *GroupName));
    }
    else
    {
        UpdateStatusText(TEXT("Failed to add devices to group"));
    }
}

void UPJLinkDiscoveryWidget::OnDiscoveryCompleted(const TArray<FPJLinkDiscoveryResult>& DiscoveredDevices, bool bSuccess)
{
    // 결과 저장
    DiscoveryResults = DiscoveredDevices;

    // UI 업데이트
    UpdateResultsList(DiscoveryResults);

    if (bSuccess)
    {
        UpdateStatusText(FString::Printf(TEXT("Search completed, found %d devices"), DiscoveryResults.Num()));
    }
    else
    {
        UpdateStatusText(TEXT("Search failed or was cancelled"));
    }

    UpdateProgressBar(100.0f, DiscoveryResults.Num(), 0);
}

void UPJLinkDiscoveryWidget::OnDeviceDiscovered(const FPJLinkDiscoveryResult& DiscoveredDevice)
{
    // 결과 추가
    DiscoveryResults.Add(DiscoveredDevice);

    // UI 업데이트
    UpdateResultsList(DiscoveryResults);
    UpdateStatusText(FString::Printf(TEXT("Found device at %s"), *DiscoveredDevice.IPAddress));
}

void UPJLinkDiscoveryWidget::OnDiscoveryProgressUpdated(const FPJLinkDiscoveryStatus& Status)
{
    // 진행 상황 업데이트
    UpdateProgressBar(Status.ProgressPercentage, Status.DiscoveredDevices, Status.ScannedAddresses);

    if (Status.bIsComplete)
    {
        UpdateStatusText(FString::Printf(TEXT("Search completed, found %d devices"), Status.DiscoveredDevices));
    }
    else
    {
        UpdateStatusText(FString::Printf(TEXT("Scanning... Found %d devices (%.1f%% complete)"),
            Status.DiscoveredDevices, Status.ProgressPercentage));
    }
}