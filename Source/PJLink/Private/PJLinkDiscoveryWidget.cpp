﻿// PJLinkDiscoveryWidget.cpp
#include "PJLinkDiscoveryWidget.h"
#include "PJLinkLog.h"
#include "PJLinkPresetManager.h"
#include "PJLinkManagerComponent.h"
#include "UPJLinkComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"

UPJLinkDiscoveryWidget::UPJLinkDiscoveryWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer), DiscoveryManager(nullptr)
{
}

void UPJLinkDiscoveryWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 타이머 초기값 설정
    LastUIUpdateTime = GetWorld()->GetTimeSeconds();

    // 초기 상태 설정
    SetDiscoveryState(EPJLinkDiscoveryState::Idle);

    CreateDiscoveryManager();

    // 애니메이션 관련 초기화
    AnimationTime = 0.0f;
    DeviceFoundEffectTime = 0.0f;
    LastDiscoveredDeviceIndex = -1;

    // UI 업데이트 주기 설정 (애니메이션을 위해 적절한 주기 필요)
    UIUpdateInterval = FMath::Min(UIUpdateInterval, 0.25f); // 최대 4fps로 UI 업데이트 제한

    // Tick 활성화 (애니메이션을 위해 필요)
    SetIsEnabled(true);
}

void UPJLinkDiscoveryWidget::NativeDestruct()
{
    if (DiscoveryManager)
    {
        DiscoveryManager->CancelAllDiscoveries();
    }

    // UI 업데이트 타이머 정리
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(UIUpdateTimerHandle);
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
    PrepareResultsUpdate(GetFilteredAndSortedResults());

    // 상태 업데이트
    ShowInfoMessage(TEXT("브로드캐스트 검색을 시작합니다..."));
    UpdateProgressBar(0.0f, 0, 0);

    // 검색 상태 설정
    SetDiscoveryState(EPJLinkDiscoveryState::Searching);

    // 검색 시작
    CurrentDiscoveryID = DiscoveryManager->StartBroadcastDiscovery(TimeoutSeconds);

    if (CurrentDiscoveryID.IsEmpty())
    {
        ShowErrorMessage(TEXT("브로드캐스트 검색을 시작하지 못했습니다"));
        // 검색 실패 상태 설정
        SetDiscoveryState(EPJLinkDiscoveryState::Failed);
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
    PrepareResultsUpdate(GetFilteredAndSortedResults());

    // 상태 업데이트
    ShowInfoMessage(FString::Printf(TEXT("IP 범위 검색을 시작합니다 (%s - %s)..."), *StartIP, *EndIP));
    UpdateProgressBar(0.0f, 0, 0);

    // 검색 상태 설정
    SetDiscoveryState(EPJLinkDiscoveryState::Searching);

    // 검색 시작
    CurrentDiscoveryID = DiscoveryManager->StartRangeScan(StartIP, EndIP, TimeoutSeconds);

    if (CurrentDiscoveryID.IsEmpty())
    {
        ShowErrorMessage(TEXT("IP 범위 검색을 시작하지 못했습니다"));
        // 검색 실패 상태 설정
        SetDiscoveryState(EPJLinkDiscoveryState::Failed);
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
    PrepareResultsUpdate(GetFilteredAndSortedResults());

    // 상태 업데이트
    ShowInfoMessage(FString::Printf(TEXT("서브넷 검색을 시작합니다 (%s/%s)..."), *SubnetAddress, *SubnetMask));
    UpdateProgressBar(0.0f, 0, 0);

    // 검색 상태 설정
    SetDiscoveryState(EPJLinkDiscoveryState::Searching);

    // 검색 시작
    CurrentDiscoveryID = DiscoveryManager->StartSubnetScan(SubnetAddress, SubnetMask, TimeoutSeconds);

    if (CurrentDiscoveryID.IsEmpty())
    {
        ShowErrorMessage(TEXT("서브넷 검색을 시작하지 못했습니다"));
        // 검색 실패 상태 설정
        SetDiscoveryState(EPJLinkDiscoveryState::Failed);
    }
}

void UPJLinkDiscoveryWidget::CancelSearch()
{
    if (DiscoveryManager && !CurrentDiscoveryID.IsEmpty())
    {
        DiscoveryManager->CancelDiscovery(CurrentDiscoveryID);
        ShowInfoMessage(TEXT("검색이 취소되었습니다"));

        // 검색 취소 상태 설정
        SetDiscoveryState(EPJLinkDiscoveryState::Cancelled);
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
    PrepareResultsUpdate(GetFilteredAndSortedResults());

    if (bSuccess)
    {
        if (DiscoveryResults.Num() > 0)
        {
            ShowSuccessMessage(FString::Printf(TEXT("검색 완료, %d개 장치를 찾았습니다"), DiscoveryResults.Num()));
        }
        else
        {
            ShowInfoMessage(TEXT("검색 완료, 장치를 찾지 못했습니다"));
        }

        // 검색 완료 상태 설정
        SetDiscoveryState(EPJLinkDiscoveryState::Completed);
    }
    else
    {
        ShowErrorMessage(TEXT("검색이 실패했거나 취소되었습니다"));

        // 검색 실패 상태 설정 - 이미 취소 상태라면 그대로 유지
        if (CurrentDiscoveryState != EPJLinkDiscoveryState::Cancelled)
        {
            SetDiscoveryState(EPJLinkDiscoveryState::Failed);
        }
    }

    UpdateProgressBar(100.0f, DiscoveryResults.Num(), 0);

    // 캐시에 자동 저장
    if (bSuccess && DiscoveryResults.Num() > 0)
    {
        SaveResultsToCache();
    }
}

// PJLinkDiscoveryWidget.cpp 파일에서 OnDeviceDiscovered 함수를 찾아 수정
void UPJLinkDiscoveryWidget::OnDeviceDiscovered(const FPJLinkDiscoveryResult& DiscoveredDevice)
{
    // 이미 같은 장치가 있는지 확인
    bool bAlreadyExists = false;
    for (const FPJLinkDiscoveryResult& ExistingDevice : DiscoveryResults)
    {
        if (ExistingDevice.IPAddress == DiscoveredDevice.IPAddress)
        {
            bAlreadyExists = true;
            break;
        }
    }

    // 새 장치인 경우에만 추가
    if (!bAlreadyExists)
    {
        // 결과 추가
        DiscoveryResults.Add(DiscoveredDevice);

        // 마지막 발견 장치 인덱스 업데이트
        LastDiscoveredDeviceIndex = DiscoveryResults.Num() - 1;

        // 장치 발견 효과 재생
        DeviceFoundEffectTime = DeviceFoundEffectDuration;
        ShowDeviceFoundEffect(LastDiscoveredDeviceIndex, FoundColor);

        // 애니메이션 스피드 조정 (발견 장치 수에 따라 증가)
        AnimationSpeedMultiplier = FMath::Min(3.0f, 1.0f + (DiscoveryResults.Num() * 0.1f));

        // 장치 발견 시 애니메이션 효과 재생
        if (bEnableAdvancedAnimations)
        {
            PlayDeviceFoundEffect(LastDiscoveredDeviceIndex);
        }

        // 장치 발견 메시지 표시 - 이름이 있으면 이름과 IP, 없으면 IP만 표시
        FString DeviceName = DiscoveredDevice.Name.IsEmpty() ? DiscoveredDevice.IPAddress :
            FString::Printf(TEXT("%s (%s)"), *DiscoveredDevice.Name, *DiscoveredDevice.IPAddress);

        ShowInfoMessage(FString::Printf(TEXT("장치를 발견했습니다: %s"), *DeviceName));

        // 발견된 장치 수에 따라 애니메이션 업데이트
        if (bEnableProgressAnimation)
        {
            UpdateProgressAnimation(DiscoveryResults.Num() * 5.0f); // 장치 발견 시 애니메이션 속도 증가

            // 애니메이션 속도 계수 업데이트
            AnimationSpeedMultiplier = FMath::Min(3.0f, 1.0f + (DiscoveryResults.Num() * 0.1f));
        }

        // UI 업데이트 최적화 - 너무 자주 업데이트하지 않도록 조절
        float CurrentTime = GetWorld()->GetTimeSeconds();
        if (CurrentTime - LastUIUpdateTime >= UIUpdateInterval)
        {
            // 충분한 시간이 경과했으면 즉시 UI 업데이트
            PrepareResultsUpdate(GetFilteredAndSortedResults());
            LastUIUpdateTime = CurrentTime;
            bPendingUIUpdate = false;

            // 타이머 취소
            if (UWorld* World = GetWorld())
            {
                World->GetTimerManager().ClearTimer(UIUpdateTimerHandle);
            }
        }
        else
        {
            // 그렇지 않으면 업데이트 예약
            bPendingUIUpdate = true;

            // 이미 타이머가 활성화되어 있지 않다면 타이머 설정
            if (UWorld* World = GetWorld())
            {
                if (!World->GetTimerManager().IsTimerActive(UIUpdateTimerHandle))
                {
                    World->GetTimerManager().SetTimer(
                        UIUpdateTimerHandle,
                        this,
                        &UPJLinkDiscoveryWidget::PerformDeferredUIUpdate,
                        UIUpdateInterval - (CurrentTime - LastUIUpdateTime),
                        false);
                }
            }
        }
    }
}
void UPJLinkDiscoveryWidget::OnDiscoveryProgressUpdated(const FPJLinkDiscoveryStatus& Status)
{
    // 진행 상황 백분율 계산 - 보다 정확하고 부드러운 업데이트를 위해 수정
    float ProgressPercentage = Status.TotalAddresses > 0
        ? FMath::Clamp(static_cast<float>(Status.ScannedAddresses) / static_cast<float>(Status.TotalAddresses) * 100.0f, 0.0f, 100.0f)
        : 0.0f;

    // 진행 바 업데이트 호출
    UpdateProgressBar(ProgressPercentage, Status.DiscoveredDevices, Status.ScannedAddresses);

    // 검색 소요 시간 문자열 생성 및 포맷팅 개선
    FString ElapsedTimeText;

    // 시간 단위에 따른 형식화
    if (Status.ElapsedTime.GetHours() > 0)
    {
        // 시간 단위 표시
        ElapsedTimeText = FString::Printf(TEXT("%d시간 %d분 %d초"),
            Status.ElapsedTime.GetHours(),
            Status.ElapsedTime.GetMinutes() % 60,
            FMath::FloorToInt(Status.ElapsedTime.GetTotalSeconds()) % 60);
    }
    else if (Status.ElapsedTime.GetMinutes() > 0)
    {
        // 분 단위 표시
        ElapsedTimeText = FString::Printf(TEXT("%d분 %d초"),
            Status.ElapsedTime.GetMinutes(),
            FMath::FloorToInt(Status.ElapsedTime.GetTotalSeconds()) % 60);
    }
    else
    {
        // 초 단위 표시 (소수점 1자리까지)
        ElapsedTimeText = FString::Printf(TEXT("%.1f초"), Status.ElapsedTime.GetTotalSeconds());
    }

    // 검색 완료 시 "총 소요 시간" 형식으로 변경
    if (Status.bIsComplete)
    {
        ElapsedTimeText = FString::Printf(TEXT("총 소요 시간: %s"), *ElapsedTimeText);
    }

    // 검색 중일 때 예상 남은 시간 추가 (1분 이상 실행된 경우)
    else if (Status.ElapsedTime.GetTotalSeconds() > 60.0f && Status.ScanSpeedIPsPerSecond > 0)
    {
        // 예상 남은 시간 형식화
        FString RemainingTimeText;
        if (Status.EstimatedTimeRemaining.GetTotalMinutes() < 1)
        {
            RemainingTimeText = FString::Printf(TEXT("약 %.0f초 남음"), Status.EstimatedTimeRemaining.GetTotalSeconds());
        }
        else if (Status.EstimatedTimeRemaining.GetTotalHours() < 1)
        {
            RemainingTimeText = FString::Printf(TEXT("약 %d분 %d초 남음"),
                Status.EstimatedTimeRemaining.GetMinutes(),
                FMath::FloorToInt(Status.EstimatedTimeRemaining.GetTotalSeconds()) % 60);
        }
        else
        {
            RemainingTimeText = FString::Printf(TEXT("약 %d시간 %d분 남음"),
                Status.EstimatedTimeRemaining.GetHours(),
                Status.EstimatedTimeRemaining.GetMinutes() % 60);
        }

        ElapsedTimeText = FString::Printf(TEXT("%s (%s)"), *ElapsedTimeText, *RemainingTimeText);
    }

    // 소요 시간 문자열 저장 및 업데이트 이벤트 호출
    CurrentElapsedTimeString = ElapsedTimeText;
    UpdateElapsedTime(ElapsedTimeText);

    // 현재 스캔 중인 IP 주소 업데이트
    if (!Status.bIsComplete && Status.ScannedAddresses > 0)
    {
        FString CurrentIP;
        // Status에서 직접 현재 스캔 중인 IP 주소 사용
        if (!Status.CurrentScanningIP.IsEmpty())
        {
            // 스캔 속도 및 예상 완료 시간 정보 추가
            if (Status.ScanSpeedIPsPerSecond > 0)
            {
                // 남은 시간 형식화
                FString RemainingTimeText;
                if (Status.EstimatedTimeRemaining.GetTotalSeconds() < 60)
                {
                    RemainingTimeText = FString::Printf(TEXT("약 %.0f초 남음"), Status.EstimatedTimeRemaining.GetTotalSeconds());
                }
                else
                {
                    int32 Minutes = FMath::FloorToInt(Status.EstimatedTimeRemaining.GetTotalMinutes());
                    int32 Seconds = FMath::FloorToInt(Status.EstimatedTimeRemaining.GetTotalSeconds()) % 60;
                    RemainingTimeText = FString::Printf(TEXT("약 %d분 %d초 남음"), Minutes, Seconds);
                }

                CurrentIP = FString::Printf(TEXT("현재 스캔 중: %s (%.1f IP/초, %s)"),
                    *Status.CurrentScanningIP, Status.ScanSpeedIPsPerSecond, *RemainingTimeText);
            }
            else
            {
                CurrentIP = FString::Printf(TEXT("현재 스캔 중: %s"), *Status.CurrentScanningIP);
            }
        }
        else
        {
            // 기존 로직 유지 (IP 정보가 없는 경우를 위한 대비책)
            CurrentIP = FString::Printf(TEXT("스캔 진행 중 (%d/%d)"), Status.ScannedAddresses, Status.TotalAddresses);
        }

        // IP 주소 저장 및 업데이트 이벤트 호출
        CurrentScanAddress = CurrentIP;
        UpdateCurrentScanAddress(CurrentIP);
    }

    // 진행 상황에 따른 애니메이션 속도 조정
    if (bEnableProgressAnimation)
    {
        // 발견된 장치가 많을수록 애니메이션 속도 증가
        AnimationSpeedMultiplier = 1.0f + (Status.DiscoveredDevices * 0.1f);
        AnimationSpeedMultiplier = FMath::Clamp(AnimationSpeedMultiplier, 1.0f, 3.0f);

        // 진행 애니메이션 업데이트
        UpdateProgressAnimation(ProgressPercentage);
    }

    // 상태 텍스트 구성
    FString StatusText;
    if (Status.bIsComplete)
    {
        if (Status.bWasCancelled)
        {
            StatusText = FString::Printf(TEXT("검색 취소됨. 발견된 장치: %d개 (검색 시간: %s)"),
                Status.DiscoveredDevices, *ElapsedTimeText);
        }
        else
        {
            StatusText = FString::Printf(TEXT("검색 완료! 발견된 장치: %d개 (검색 시간: %s)"),
                Status.DiscoveredDevices, *ElapsedTimeText);
        }
    }
    else
    {
        StatusText = FString::Printf(TEXT("검색 중... %d개 장치 발견 (%d/%d 주소 검색, %.1f%%, %s 경과)"),
            Status.DiscoveredDevices, Status.ScannedAddresses, Status.TotalAddresses,
            ProgressPercentage, *ElapsedTimeText);
    }

    // 상태 텍스트 업데이트 호출
    UpdateStatusText(StatusText);
}

bool UPJLinkDiscoveryWidget::SaveResultsToCache()
{
    if (DiscoveryResults.Num() == 0)
    {
        // 저장할 결과가 없음
        return false;
    }

    // JSON 객체 생성
    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);
    TArray<TSharedPtr<FJsonValue>> ResultArray;

    // 각 검색 결과를 JSON으로 변환
    for (const FPJLinkDiscoveryResult& Result : DiscoveryResults)
    {
        TSharedPtr<FJsonObject> ResultObj = MakeShareable(new FJsonObject);
        ResultObj->SetStringField(TEXT("IPAddress"), Result.IPAddress);
        ResultObj->SetNumberField(TEXT("Port"), Result.Port);
        ResultObj->SetStringField(TEXT("Name"), Result.Name);
        ResultObj->SetStringField(TEXT("ModelName"), Result.ModelName);
        ResultObj->SetNumberField(TEXT("DeviceClass"), static_cast<int32>(Result.DeviceClass));
        ResultObj->SetBoolField(TEXT("RequiresAuthentication"), Result.bRequiresAuthentication);
        ResultObj->SetStringField(TEXT("DiscoveryTime"), Result.DiscoveryTime.ToString());
        ResultObj->SetNumberField(TEXT("ResponseTimeMs"), Result.ResponseTimeMs);

        ResultArray.Add(MakeShareable(new FJsonValueObject(ResultObj)));
    }

    // 루트 객체에 배열 추가
    RootObject->SetArrayField(TEXT("DiscoveryResults"), ResultArray);

    // 최종 업데이트 시간 추가
    LastCacheTime = FDateTime::Now();
    RootObject->SetStringField(TEXT("LastCacheTime"), LastCacheTime.ToString());

    // JSON 문자열로 변환
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);

    // 캐시 디렉토리 생성
    FString FilePath = GetCacheFilePath();
    FString DirectoryPath = FPaths::GetPath(FilePath);
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

    if (!PlatformFile.DirectoryExists(*DirectoryPath))
    {
        PlatformFile.CreateDirectoryTree(*DirectoryPath);
    }

    // 파일에 저장
    bool bSuccess = FFileHelper::SaveStringToFile(OutputString, *FilePath);

    if (bSuccess)
    {
        UpdateStatusText(FString::Printf(TEXT("검색 결과가 캐시에 저장되었습니다. (%d 장치)"), DiscoveryResults.Num()));
    }
    else
    {
        UpdateStatusText(FString::Printf(TEXT("검색 결과를 캐시에 저장하지 못했습니다.")));
    }

    return bSuccess;
}

bool UPJLinkDiscoveryWidget::LoadCachedResults()
{
    // 캐시 파일 경로
    FString FilePath = GetCacheFilePath();

    // 파일 존재 여부 확인
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    if (!PlatformFile.FileExists(*FilePath))
    {
        UpdateStatusText(TEXT("캐시된 검색 결과가 없습니다."));
        return false;
    }

    // 파일 읽기
    FString JsonString;
    if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
    {
        UpdateStatusText(TEXT("캐시 파일을 읽을 수 없습니다."));
        return false;
    }

    // JSON 파싱
    TSharedPtr<FJsonObject> RootObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
    {
        UpdateStatusText(TEXT("캐시 파일을 파싱할 수 없습니다."));
        return false;
    }

    // 마지막 캐시 시간 가져오기
    FString LastCacheTimeStr = RootObject->GetStringField(TEXT("LastCacheTime"));
    FDateTime::Parse(LastCacheTimeStr, LastCacheTime);

    // 검색 결과 배열 가져오기
    TArray<TSharedPtr<FJsonValue>> ResultArray = RootObject->GetArrayField(TEXT("DiscoveryResults"));

    // 기존 결과 초기화
    DiscoveryResults.Empty();

    // 각 결과 파싱
    for (const auto& JsonValue : ResultArray)
    {
        TSharedPtr<FJsonObject> ResultObj = JsonValue->AsObject();
        if (!ResultObj.IsValid())
        {
            continue;
        }

        FPJLinkDiscoveryResult Result;
        Result.IPAddress = ResultObj->GetStringField(TEXT("IPAddress"));
        Result.Port = ResultObj->GetNumberField(TEXT("Port"));
        Result.Name = ResultObj->GetStringField(TEXT("Name"));
        Result.ModelName = ResultObj->GetStringField(TEXT("ModelName"));
        Result.DeviceClass = static_cast<EPJLinkClass>(ResultObj->GetNumberField(TEXT("DeviceClass")));
        Result.bRequiresAuthentication = ResultObj->GetBoolField(TEXT("RequiresAuthentication"));

        FString DiscoveryTimeStr = ResultObj->GetStringField(TEXT("DiscoveryTime"));
        FDateTime::Parse(DiscoveryTimeStr, Result.DiscoveryTime);

        Result.ResponseTimeMs = ResultObj->GetNumberField(TEXT("ResponseTimeMs"));

        DiscoveryResults.Add(Result);
    }

    PrepareResultsUpdate(GetFilteredAndSortedResults());  // 변경된 코드

    FTimespan TimeSinceCache = FDateTime::Now() - LastCacheTime;
    double HoursSinceCache = TimeSinceCache.GetTotalHours();

    FString TimeMessage;
    if (HoursSinceCache < 1.0)
    {
        int32 MinutesSinceCache = FMath::FloorToInt(TimeSinceCache.GetTotalMinutes());
        TimeMessage = FString::Printf(TEXT("%d분 전"), MinutesSinceCache);
    }
    else if (HoursSinceCache < 24.0)
    {
        int32 HoursRounded = FMath::FloorToInt(HoursSinceCache);
        TimeMessage = FString::Printf(TEXT("%d시간 전"), HoursRounded);
    }
    else
    {
        int32 DaysSinceCache = FMath::FloorToInt(HoursSinceCache / 24.0);
        TimeMessage = FString::Printf(TEXT("%d일 전"), DaysSinceCache);
    }

    UpdateStatusText(FString::Printf(TEXT("캐시에서 %d개 장치를 불러왔습니다. (마지막 업데이트: %s)"),
        DiscoveryResults.Num(), *TimeMessage));

    return true;
}

FString UPJLinkDiscoveryWidget::GetCacheFilePath() const
{
    return FPaths::ProjectSavedDir() / TEXT("PJLink") / CacheFileName;
}

FDateTime UPJLinkDiscoveryWidget::GetLastCacheTime() const
{
    return LastCacheTime;
}

void UPJLinkDiscoveryWidget::ShowErrorMessage(const FString& ErrorMessage)
{
    if (OnShowMessage.IsBound())
    {
        OnShowMessage.Execute(ErrorMessage, ErrorColor);
    }

    // 콘솔에 오류 로그 출력
    UE_LOG(LogTemp, Error, TEXT("[PJLink Discovery] %s"), *ErrorMessage);

    // 자동 숨김 타이머 설정
    SetupMessageTimer();
}

void UPJLinkDiscoveryWidget::ShowSuccessMessage(const FString& SuccessMessage)
{
    if (OnShowMessage.IsBound())
    {
        OnShowMessage.Execute(SuccessMessage, SuccessColor);
    }

    // 콘솔에 로그 출력
    UE_LOG(LogTemp, Log, TEXT("[PJLink Discovery] %s"), *SuccessMessage);

    // 자동 숨김 타이머 설정
    SetupMessageTimer();
}

void UPJLinkDiscoveryWidget::ShowInfoMessage(const FString& InfoMessage)
{
    if (OnShowMessage.IsBound())
    {
        OnShowMessage.Execute(InfoMessage, InfoColor);
    }

    // 콘솔에 로그 출력
    UE_LOG(LogTemp, Log, TEXT("[PJLink Discovery] %s"), *InfoMessage);

    // 자동 숨김 타이머 설정
    SetupMessageTimer();
}

void UPJLinkDiscoveryWidget::HideMessage()
{
    if (OnHideMessage.IsBound())
    {
        OnHideMessage.Execute();
    }

    // 타이머 취소
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(MessageTimerHandle);
    }
}

// private 함수 추가 (헤더에 선언도 필요)
void UPJLinkDiscoveryWidget::SetupMessageTimer()
{
    // 마지막 메시지 시간 업데이트
    LastMessageTime = GetWorld()->GetTimeSeconds();

    // 기존 타이머 취소
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(MessageTimerHandle);

        // 새 타이머 설정
        if (MessageAutoHideTime > 0.0f)
        {
            World->GetTimerManager().SetTimer(
                MessageTimerHandle,
                this,
                &UPJLinkDiscoveryWidget::HideMessage,
                MessageAutoHideTime,
                false);
        }
    }
}

void UPJLinkDiscoveryWidget::PerformDeferredUIUpdate()
{
    // 대기 중인 UI 업데이트가 있고, 충분한 시간이 경과했는지 확인
    if (bPendingUIUpdate)
    {
        // UI 업데이트 플래그 초기화
        bPendingUIUpdate = false;

        // 정렬 및 필터링된 결과로 UI 업데이트
        PrepareResultsUpdate(GetFilteredAndSortedResults());

        // 마지막 업데이트 시간 기록
        LastUIUpdateTime = GetWorld()->GetTimeSeconds();
    }
}

FString UPJLinkDiscoveryWidget::GetDiscoveryStateText() const
{
    switch (CurrentDiscoveryState)
    {
    case EPJLinkDiscoveryState::Idle:
        return TEXT("대기 중");
    case EPJLinkDiscoveryState::Searching:
        return TEXT("검색 중...");
    case EPJLinkDiscoveryState::Completed:
        return TEXT("검색 완료");
    case EPJLinkDiscoveryState::Cancelled:
        return TEXT("검색 취소됨");
    case EPJLinkDiscoveryState::Failed:
        return TEXT("검색 실패");
    default:
        return TEXT("");
    }
}

void UPJLinkDiscoveryWidget::SetDiscoveryState(EPJLinkDiscoveryState NewState)
{
    if (CurrentDiscoveryState != NewState)
    {
        CurrentDiscoveryState = NewState;

        // 상태에 따라 회전 애니메이션 시작/중지
        switch (NewState)
        {
        case EPJLinkDiscoveryState::Searching:
            StartProgressAnimation();
            break;

        case EPJLinkDiscoveryState::Completed:
        case EPJLinkDiscoveryState::Cancelled:
        case EPJLinkDiscoveryState::Failed:
        case EPJLinkDiscoveryState::Idle:
            // 서서히 중지되도록 플래그만 변경
            if (bRotationAnimationActive)
            {
                // 회전 속도 감속 시작 - 애니메이션 자체는 UpdateRotationAnimation에서 서서히 중지됨
                SetRotationAnimationSpeed(RotationAnimationSpeed * 0.5f);
            }
            break;
        }

        // 이벤트 호출
        OnDiscoveryStateChanged(NewState);

        // 상태에 따른 애니메이션 처리
        switch (NewState)
        {
        case EPJLinkDiscoveryState::Searching:
            // 검색 시작 애니메이션
            if (bEnableAdvancedAnimations)
            {
                PlaySearchButtonAnimation(true);
                AnimationTime = 0.0f;
                bIsPulseAnimationActive = true;
            }

            // 프로그레스 애니메이션 시작
            if (bEnableProgressAnimation)
            {
                StartProgressAnimation();
            }
            break;

        case EPJLinkDiscoveryState::Completed:
            // 검색 완료 애니메이션
            if (bEnableAdvancedAnimations)
            {
                PlaySearchButtonAnimation(false);
                PlayCompletionEffect(true, DiscoveryResults.Num());
                bIsPulseAnimationActive = false;
            }

            // 프로그레스 애니메이션 중지
            if (bEnableProgressAnimation)
            {
                StopProgressAnimation();
            }
            break;

        case EPJLinkDiscoveryState::Cancelled:
        case EPJLinkDiscoveryState::Failed:
            // 취소/실패 애니메이션
            if (bEnableAdvancedAnimations)
            {
                PlaySearchButtonAnimation(false);
                PlayCompletionEffect(false, DiscoveryResults.Num());
                bIsPulseAnimationActive = false;
            }

            // 프로그레스 애니메이션 중지
            if (bEnableProgressAnimation)
            {
                StopProgressAnimation();
            }
            break;

        default:
            break;
        }
    }
}

void UPJLinkDiscoveryWidget::SetFilterOption(EPJLinkDiscoveryFilterOption FilterOption)
{
    if (CurrentFilterOption != FilterOption)
    {
        CurrentFilterOption = FilterOption;

        // UI 즉시 업데이트
        PrepareResultsUpdate(GetFilteredAndSortedResults());
    }
}

TArray<FPJLinkDiscoveryResult> UPJLinkDiscoveryWidget::GetFilteredAndSortedResults() const
{
    // 원본 결과 복사
    TArray<FPJLinkDiscoveryResult> FilteredResults = DiscoveryResults;

    // 필터 적용
    if (CurrentFilterOption != EPJLinkDiscoveryFilterOption::All)
    {
        TArray<FPJLinkDiscoveryResult> TempResults;

        for (const FPJLinkDiscoveryResult& Result : FilteredResults)
        {
            bool bShouldInclude = false;

            switch (CurrentFilterOption)
            {
            case EPJLinkDiscoveryFilterOption::RequiresAuth:
                bShouldInclude = Result.bRequiresAuthentication;
                break;
            case EPJLinkDiscoveryFilterOption::NoAuth:
                bShouldInclude = !Result.bRequiresAuthentication;
                break;
            case EPJLinkDiscoveryFilterOption::Class1:
                bShouldInclude = (Result.DeviceClass == EPJLinkClass::Class1);
                break;
            case EPJLinkDiscoveryFilterOption::Class2:
                bShouldInclude = (Result.DeviceClass == EPJLinkClass::Class2);
                break;
            default:
                bShouldInclude = true;
                break;
            }

            if (bShouldInclude)
            {
                TempResults.Add(Result);
            }
        }

        FilteredResults = TempResults;
    }

    // 텍스트 필터 적용 (이미 구현된 경우 유지)
    if (!FilterText.IsEmpty())
    {
        TArray<FPJLinkDiscoveryResult> TempResults;

        for (const FPJLinkDiscoveryResult& Result : FilteredResults)
        {
            // IP 주소, 이름, 모델명 중 하나라도 필터 텍스트를 포함하면 표시
            if (Result.IPAddress.Contains(FilterText) ||
                Result.Name.Contains(FilterText) ||
                Result.ModelName.Contains(FilterText))
            {
                TempResults.Add(Result);
            }
        }

        FilteredResults = TempResults;
    }

    // 정렬 적용
    switch (CurrentSortOption)
    {
    case EPJLinkDiscoverySortOption::ByIPAddress:
        // IP 주소 기준 정렬
        FilteredResults.Sort([](const FPJLinkDiscoveryResult& A, const FPJLinkDiscoveryResult& B) {
            return A.IPAddress < B.IPAddress;
            });
        break;

    case EPJLinkDiscoverySortOption::ByName:
        // 이름 기준 정렬 (이름이 없으면 IP 주소 사용)
        FilteredResults.Sort([](const FPJLinkDiscoveryResult& A, const FPJLinkDiscoveryResult& B) {
            const FString& NameA = A.Name.IsEmpty() ? A.IPAddress : A.Name;
            const FString& NameB = B.Name.IsEmpty() ? B.IPAddress : B.Name;
            return NameA < NameB;
            });
        break;

    case EPJLinkDiscoverySortOption::ByResponseTime:
        // 응답 시간 기준 정렬 (빠른 순)
        FilteredResults.Sort([](const FPJLinkDiscoveryResult& A, const FPJLinkDiscoveryResult& B) {
            return A.ResponseTimeMs < B.ResponseTimeMs;
            });
        break;

    case EPJLinkDiscoverySortOption::ByModelName:
        // 모델명 기준 정렬
        FilteredResults.Sort([](const FPJLinkDiscoveryResult& A, const FPJLinkDiscoveryResult& B) {
            return A.ModelName < B.ModelName;
            });
        break;

    case EPJLinkDiscoverySortOption::ByDiscoveryTime:
        // 발견 시간 기준 정렬 (최신 순)
        FilteredResults.Sort([](const FPJLinkDiscoveryResult& A, const FPJLinkDiscoveryResult& B) {
            return A.DiscoveryTime > B.DiscoveryTime;
            });
        break;

    default:
        break;
    }

    return FilteredResults;
}

void UPJLinkDiscoveryWidget::SelectDevice(int32 DeviceIndex)
{
    // 유효한 인덱스인지 확인
    TArray<FPJLinkDiscoveryResult> Results = GetFilteredAndSortedResults();
    if (Results.IsValidIndex(DeviceIndex))
    {
        SelectedDeviceIndex = DeviceIndex;

        // 세부 정보 패널 업데이트
        UpdateDetailPanel(Results[DeviceIndex]);
    }
    else
    {
        SelectedDeviceIndex = -1;
    }
}

FString UPJLinkDiscoveryWidget::GetResultSummaryText() const
{
    TArray<FPJLinkDiscoveryResult> FilteredResults = GetFilteredAndSortedResults();
    int32 TotalCount = DiscoveryResults.Num();
    int32 FilteredCount = FilteredResults.Num();

    if (CurrentDiscoveryState == EPJLinkDiscoveryState::Searching)
    {
        return FString::Printf(TEXT("검색 중... %d개 장치 발견"), TotalCount);
    }
    else if (TotalCount == 0)
    {
        return TEXT("발견된 장치 없음");
    }
    else if (TotalCount == FilteredCount)
    {
        return FString::Printf(TEXT("장치 %d개 발견"), TotalCount);
    }
    else
    {
        return FString::Printf(TEXT("장치 %d개 중 %d개 표시 중"), TotalCount, FilteredCount);
    }
}

FLinearColor UPJLinkDiscoveryWidget::GetResultSummaryColor() const
{
    int32 ResultCount = GetFilteredAndSortedResults().Num();

    if (CurrentDiscoveryState == EPJLinkDiscoveryState::Searching)
    {
        // 검색 중 - 파란색
        return FLinearColor(0.2f, 0.4f, 1.0f, 1.0f);
    }
    else if (ResultCount == 0)
    {
        // 결과 없음 - 회색
        return FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);
    }
    else
    {
        // 결과 있음 - 녹색
        return FLinearColor(0.2f, 0.8f, 0.2f, 1.0f);
    }
}

FString UPJLinkDiscoveryWidget::GetDeviceStatusText(const FPJLinkDiscoveryResult& Result) const
{
    // 장치 클래스 표시
    FString ClassText = (Result.DeviceClass == EPJLinkClass::Class1) ? TEXT("Class 1") : TEXT("Class 2");

    // 인증 요구 여부 표시
    FString AuthText = Result.bRequiresAuthentication ? TEXT("인증 필요") : TEXT("인증 불필요");

    // 응답 시간 표시
    FString ResponseText = FString::Printf(TEXT("%dms"), Result.ResponseTimeMs);

    return FString::Printf(TEXT("%s, %s, %s"), *ClassText, *AuthText, *ResponseText);
}

FLinearColor UPJLinkDiscoveryWidget::GetDeviceStatusColor(const FPJLinkDiscoveryResult& Result) const
{
    // 인증 여부에 따른 색상 변경
    if (Result.bRequiresAuthentication)
    {
        // 인증 필요 - 노란색
        return FLinearColor(1.0f, 0.8f, 0.2f, 1.0f);
    }
    else
    {
        // 인증 불필요 - 녹색
        return FLinearColor(0.2f, 0.8f, 0.2f, 1.0f);
    }
}

// 결과 목록을 업데이트하기 전에 호출하는 함수 추가
void UPJLinkDiscoveryWidget::PrepareResultsUpdate(const TArray<FPJLinkDiscoveryResult>& Results)
{
    // 결과가 없는 경우 처리
    bool bIsSearching = CurrentDiscoveryState == EPJLinkDiscoveryState::Searching;
    bool bHasFilters = !FilterText.IsEmpty() || CurrentFilterOption != EPJLinkDiscoveryFilterOption::All;

    if (Results.Num() == 0)
    {
        // 결과 없음 메시지 표시
        ShowNoResultsMessage(bIsSearching, bHasFilters);
    }

    // 선택된 장치가 더 이상 결과에 없는 경우, 선택 해제
    if (!Results.IsValidIndex(SelectedDeviceIndex))
    {
        SelectedDeviceIndex = -1;
    }

    // 블루프린트에 구현된 UpdateResultsList 호출
    UpdateResultsList(Results);
}

void UPJLinkDiscoveryWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // 애니메이션 시간 업데이트
    AnimationTime += InDeltaTime;

    // 검색 중 상태일 때 애니메이션 업데이트
    if (CurrentDiscoveryState == EPJLinkDiscoveryState::Searching && bEnableAdvancedAnimations)
    {
        // 회전 애니메이션 업데이트
        UpdateRotationAnimation(InDeltaTime);

        // 펄스 애니메이션 시간 관리
        PulseAnimationTime += InDeltaTime;
        if (PulseAnimationTime >= PulseAnimationDuration)
        {
            PulseAnimationTime = 0.0f;
        }

        // 펄스 애니메이션 강도 (0.0 ~ 1.0)
        float PulseIntensity = FMath::Sin(PulseAnimationTime / PulseAnimationDuration * PI * 2.0f) * 0.5f + 0.5f;

        // 커스텀 애니메이션 업데이트 함수 호출
        UpdateProgressAnimation(PulseIntensity * 100.0f);
    }

    // 장치 발견 효과 타이머 업데이트
    if (DeviceFoundEffectTime > 0.0f)
    {
        DeviceFoundEffectTime -= InDeltaTime;
    }
}

void UPJLinkDiscoveryWidget::UpdateCurrentScanAddress_Implementation(const FString& CurrentAddress)
{
    // 기본 구현 (여기서는 현재 스캔 중인 IP 주소를 저장하고 화면에 표시)
    CurrentScanAddress = CurrentAddress;

    // 추가 애니메이션 효과를 위한 블루프린트 구현이 있을 수 있음
    // 블루프린트에서 이 함수를 오버라이드하여 추가 시각적 효과 구현 가능
}

// 회전 애니메이션 함수 개선
void UPJLinkDiscoveryWidget::UpdateRotationAnimation(float DeltaTime)
{
    if (!bRotationAnimationActive)
        return;

    // 검색 상태에 따른 속도 조정
    if (CurrentDiscoveryState == EPJLinkDiscoveryState::Searching)
    {
        // 가속
        RotationAnimationSpeed = FMath::Min(
            RotationAnimationSpeed + (RotationAnimationAcceleration * DeltaTime),
            RotationAnimationMaxSpeed
        );
    }
    else
    {
        // 감속
        RotationAnimationSpeed = FMath::Max(
            RotationAnimationSpeed - (RotationAnimationDeceleration * DeltaTime),
            0.0f
        );

        // 속도가 0에 가까워지면 애니메이션 중지
        if (RotationAnimationSpeed < 0.1f)
        {
            bRotationAnimationActive = false;
            return;
        }
    }

    // 애니메이션 속도 계산 (발견된 장치 수와 진행 상황에 따라 동적 조정)
    float TargetSpeed = RotationAnimationSpeed * AnimationSpeedMultiplier;

    // 회전 각도 업데이트
    CurrentRotationAngle += TargetSpeed * DeltaTime * 360.0f; // 초당 회전 각도

    // 360도 초과 시 정규화
    if (CurrentRotationAngle >= 360.0f)
    {
        CurrentRotationAngle -= 360.0f;
    }

    // 시각적 회전 적용 (블루프린트에서 구현)
    ApplyRotationToImage(CurrentRotationAngle);
}

// 장치 발견 효과 함수 개선
void UPJLinkDiscoveryWidget::ShowDeviceFoundEffect(int32 DeviceIndex, const FLinearColor& EffectColor)
{
    // 발견된 장치에 시각적 효과 적용
    if (DeviceIndex >= 0)
    {
        // 효과 시간 설정
        DeviceFoundEffectTime = DeviceFoundEffectDuration;

        // 효과 색상 설정 - 기본 녹색에서 장치 발견 수에 따라 더 밝은 색상으로 변경
        FLinearColor EnhancedColor = EffectColor;
        int32 DeviceCount = DiscoveryResults.Num();

        if (DeviceCount > 5)
        {
            // 발견된 장치가 많을수록 더 밝은 색상 사용
            float BrightnessBoost = FMath::Min(0.3f, DeviceCount * 0.02f);
            EnhancedColor.R += BrightnessBoost;
            EnhancedColor.G += BrightnessBoost;
            EnhancedColor.B += BrightnessBoost;
            EnhancedColor = EnhancedColor.GetClamped(); // 값 제한 (0-1)
        }

        // 블루프린트 이벤트 호출
        PlayDeviceFoundEffect(DeviceIndex);
    }
}

// 완료 효과 함수 개선
void UPJLinkDiscoveryWidget::PlayCompletionEffect(bool bSuccess, int32 DeviceCount)
{
    // 검색 완료 효과 설정
    FLinearColor CompletionColor = bSuccess ?
        FLinearColor(0.2f, 0.8f, 0.2f, 1.0f) : // 성공 색상 (녹색)
        FLinearColor(0.8f, 0.2f, 0.2f, 1.0f);  // 실패 색상 (빨간색)

    // 발견된 장치 수에 따라 효과 강도 조정
    float EffectIntensity = FMath::Min(1.0f, 0.2f + (DeviceCount * 0.05f));

    // 블루프린트 이벤트 호출을 위한 준비
    CompletionAnimationDuration = bSuccess ?
        FMath::Max(2.0f, FMath::Min(5.0f, DeviceCount * 0.2f)) : // 성공 시 장치 수에 따라 지속 시간 조정
        2.0f; // 실패 시 짧은 지속 시간

    // 블루프린트 이벤트 호출
    PlayCompletionAnimation(bSuccess);
}

void UPJLinkDiscoveryWidget::SetRotationAnimationSpeed(float Speed)
{
    RotationAnimationSpeed = FMath::Clamp(Speed, 0.0f, RotationAnimationMaxSpeed);
}

void UPJLinkDiscoveryWidget::StartProgressAnimation_Implementation()
{
    // 회전 애니메이션 활성화
    bRotationAnimationActive = true;
    RotationAnimationSpeed = 1.0f; // 초기 속도
    CurrentRotationAngle = 0.0f;   // 초기 각도

    // ScanningAnimationImage 표시
    if (ScanningAnimationImage)
    {
        ScanningAnimationImage->SetVisibility(ESlateVisibility::Visible);
    }

    // 블루프린트에서 추가 구현할 수 있도록 이벤트 호출
    if (bEnableAdvancedAnimations)
    {
        PlaySearchButtonAnimation(true);
    }
}

void UPJLinkDiscoveryWidget::StopProgressAnimation_Implementation()
{
    // 애니메이션 중지는 UpdateRotationAnimation에서 서서히 처리
    // 여기서는 플래그만 해제

    // ScanningAnimationImage 숨기기
    if (ScanningAnimationImage)
    {
        ScanningAnimationImage->SetVisibility(ESlateVisibility::Hidden);
    }

    // 블루프린트에서 추가 구현할 수 있도록 이벤트 호출
    if (bEnableAdvancedAnimations)
    {
        PlaySearchButtonAnimation(false);
    }
}

void UPJLinkDiscoveryWidget::UpdateProgressAnimation_Implementation(float ProgressPercentage)
{
    // 진행 상황에 따른 애니메이션 속도 및 색상 조정
    if (bEnableAdvancedAnimations)
    {
        // 발견된 장치 수에 따라 애니메이션 스피드 조정
        AnimationSpeedMultiplier = FMath::Min(3.0f, 1.0f + (ProgressPercentage / 100.0f * 2.0f));

        // 진행률에 따른 색상 그라데이션 계산 (0% = 파란색, 100% = 녹색)
        FLinearColor ProgressColor = FLinearColor::LerpUsingHSV(
            SearchingColor,   // 시작 색상 (파란색)
            FoundColor,       // 종료 색상 (녹색)
            ProgressPercentage / 100.0f  // 진행률 비율
        );

        // 회전 애니메이션 업데이트 - 속도 조정만 수행하고 실제 회전은 UpdateRotationAnimation에서 처리
        SetRotationAnimationSpeed(RotationAnimationSpeed * AnimationSpeedMultiplier);
    }
}

// PJLinkDiscoveryWidget.cpp 파일에 추가할 함수들

// 선택된 장치 관리 함수
void UPJLinkDiscoveryWidget::ManageSelectedDevice()
{
    // 현재 선택된 장치가 있는지 확인
    TArray<FPJLinkDiscoveryResult> FilteredResults = GetFilteredAndSortedResults();
    if (!FilteredResults.IsValidIndex(SelectedDeviceIndex))
    {
        ShowInfoMessage(TEXT("먼저 장치를 선택해주세요."));
        return;
    }

    // 선택된 장치 정보
    const FPJLinkDiscoveryResult& SelectedDevice = FilteredResults[SelectedDeviceIndex];

    // 세부 정보 패널 업데이트
    UpdateDetailPanel(SelectedDevice);

    // 선택된 장치 강조 표시
    HighlightSelectedDevice(SelectedDeviceIndex);
}

// 장치에 연결하는 함수 업데이트
bool UPJLinkDiscoveryWidget::ConnectToSelectedDevice()
{
    // 현재 선택된 장치가 있는지 확인
    TArray<FPJLinkDiscoveryResult> FilteredResults = GetFilteredAndSortedResults();
    if (!FilteredResults.IsValidIndex(SelectedDeviceIndex))
    {
        ShowInfoMessage(TEXT("먼저 장치를 선택해주세요."));
        return false;
    }

    // 선택된 장치 정보
    const FPJLinkDiscoveryResult& SelectedDevice = FilteredResults[SelectedDeviceIndex];

    // 연결 시작
    ShowInfoMessage(FString::Printf(TEXT("%s (%s)에 연결 중..."),
        *SelectedDevice.Name, *SelectedDevice.IPAddress));

    // 실제 연결 시도
    ConnectToDevice(SelectedDevice);

    return true;
}

// 선택된 장치 강조 표시 함수
void UPJLinkDiscoveryWidget::HighlightSelectedDevice_Implementation(int32 DeviceIndex)
{
    // 이 함수는 블루프린트에서 구현될 수 있습니다
    // 기본 구현에서는 아무것도 안함
}

// 프리셋으로 저장
bool UPJLinkDiscoveryWidget::SaveSelectedDeviceAsPreset(const FString& PresetName)
{
    // 현재 선택된 장치가 있는지 확인
    TArray<FPJLinkDiscoveryResult> FilteredResults = GetFilteredAndSortedResults();
    if (!FilteredResults.IsValidIndex(SelectedDeviceIndex))
    {
        ShowInfoMessage(TEXT("먼저 장치를 선택해주세요."));
        return false;
    }

    // 선택된 장치 정보
    const FPJLinkDiscoveryResult& SelectedDevice = FilteredResults[SelectedDeviceIndex];

    // 프리셋 이름 검증
    FString ActualPresetName = PresetName;
    if (ActualPresetName.IsEmpty())
    {
        ActualPresetName = FString::Printf(TEXT("%s_%s"),
            *SelectedDevice.Name.IsEmpty() ? TEXT("Projector") : *SelectedDevice.Name,
            *SelectedDevice.IPAddress);
    }

    // 저장
    SaveDeviceAsPreset(SelectedDevice, ActualPresetName);

    ShowSuccessMessage(FString::Printf(TEXT("장치를 프리셋 '%s'으로 저장했습니다."), *ActualPresetName));

    return true;
}

// 그룹에 추가
bool UPJLinkDiscoveryWidget::AddSelectedDeviceToGroup(const FString& GroupName)
{
    // 현재 선택된 장치가 있는지 확인
    TArray<FPJLinkDiscoveryResult> FilteredResults = GetFilteredAndSortedResults();
    if (!FilteredResults.IsValidIndex(SelectedDeviceIndex))
    {
        ShowInfoMessage(TEXT("먼저 장치를 선택해주세요."));
        return false;
    }

    // 선택된 장치 정보
    const FPJLinkDiscoveryResult& SelectedDevice = FilteredResults[SelectedDeviceIndex];

    // 그룹 이름 검증
    FString ActualGroupName = GroupName;
    if (ActualGroupName.IsEmpty())
    {
        ActualGroupName = TEXT("Default");
    }

    // 그룹에 추가
    AddDeviceToGroup(SelectedDevice, ActualGroupName);

    ShowSuccessMessage(FString::Printf(TEXT("장치를 그룹 '%s'에 추가했습니다."), *ActualGroupName));

    return true;
}

// 선택된 장치 상세 정보 보기
void UPJLinkDiscoveryWidget::ShowSelectedDeviceDetails()
{
    // 현재 선택된 장치가 있는지 확인
    TArray<FPJLinkDiscoveryResult> FilteredResults = GetFilteredAndSortedResults();
    if (!FilteredResults.IsValidIndex(SelectedDeviceIndex))
    {
        ShowInfoMessage(TEXT("먼저 장치를 선택해주세요."));
        return;
    }

    // 선택된 장치 정보
    const FPJLinkDiscoveryResult& SelectedDevice = FilteredResults[SelectedDeviceIndex];

    // 세부 정보 업데이트 호출
    UpdateDetailPanel(SelectedDevice);
}

// 여러 장치 일괄 선택 처리
TArray<int32> UPJLinkDiscoveryWidget::GetSelectedDeviceIndices() const
{
    return SelectedDeviceIndices;
}

// 장치 선택 토글
void UPJLinkDiscoveryWidget::ToggleDeviceSelection(int32 DeviceIndex)
{
    if (SelectedDeviceIndices.Contains(DeviceIndex))
    {
        SelectedDeviceIndices.Remove(DeviceIndex);
    }
    else
    {
        SelectedDeviceIndices.Add(DeviceIndex);
    }

    // 단일 선택 모드인 경우 현재 선택된 인덱스도 업데이트
    if (bSingleSelectionMode)
    {
        SelectedDeviceIndex = SelectedDeviceIndices.Num() > 0 ? SelectedDeviceIndices.Last() : -1;
    }

    // 선택 변경 이벤트 발생
    OnDeviceSelectionChanged.Broadcast(SelectedDeviceIndices);
}

// 모든 장치 선택
void UPJLinkDiscoveryWidget::SelectAllDevices()
{
    SelectedDeviceIndices.Empty();

    TArray<FPJLinkDiscoveryResult> FilteredResults = GetFilteredAndSortedResults();
    for (int32 i = 0; i < FilteredResults.Num(); i++)
    {
        SelectedDeviceIndices.Add(i);
    }

    // 선택 변경 이벤트 발생
    OnDeviceSelectionChanged.Broadcast(SelectedDeviceIndices);
}

// 선택 장치 추가 함수
void UPJLinkDiscoveryWidget::AddToSelection(int32 DeviceIndex)
{
    if (!SelectedDeviceIndices.Contains(DeviceIndex))
    {
        SelectedDeviceIndices.Add(DeviceIndex);

        // 단일 선택 모드인 경우 현재 선택된 인덱스도 업데이트
        if (bSingleSelectionMode)
        {
            SelectedDeviceIndex = DeviceIndex;
        }

        // 선택 변경 이벤트 발생
        OnDeviceSelectionChanged.Broadcast(SelectedDeviceIndices);
    }
}

// 선택 해제
void UPJLinkDiscoveryWidget::ClearSelection()
{
    SelectedDeviceIndices.Empty();
    SelectedDeviceIndex = -1;

    // 선택 변경 이벤트 발생
    OnDeviceSelectionChanged.Broadcast(SelectedDeviceIndices);
}

// 선택된 장치들에 대한 일괄 작업
bool UPJLinkDiscoveryWidget::ProcessSelectedDevices(EPJLinkBatchOperation Operation, const FString& Parameter)
{
    if (SelectedDeviceIndices.Num() == 0)
    {
        ShowInfoMessage(TEXT("먼저 장치를 선택해주세요."));
        return false;
    }

    TArray<FPJLinkDiscoveryResult> FilteredResults = GetFilteredAndSortedResults();
    TArray<FPJLinkDiscoveryResult> SelectedDevices;

    // 선택된 인덱스들에 해당하는 장치들 수집
    for (int32 Index : SelectedDeviceIndices)
    {
        if (FilteredResults.IsValidIndex(Index))
        {
            SelectedDevices.Add(FilteredResults[Index]);
        }
    }

    if (SelectedDevices.Num() == 0)
    {
        ShowInfoMessage(TEXT("유효한 선택된 장치가 없습니다."));
        return false;
    }

    // 작업 유형에 따른 처리
    switch (Operation)
    {
    case EPJLinkBatchOperation::Connect:
        return BatchConnectDevices(SelectedDevices);

    case EPJLinkBatchOperation::SaveAsPresets:
        return BatchSaveAsPresets(SelectedDevices, Parameter);

    case EPJLinkBatchOperation::AddToGroup:
        return BatchAddToGroup(SelectedDevices, Parameter);

    default:
        ShowErrorMessage(TEXT("지원되지 않는 일괄 작업입니다."));
        return false;
    }
}

// 일괄 연결
bool UPJLinkDiscoveryWidget::BatchConnectDevices(const TArray<FPJLinkDiscoveryResult>& Devices)
{
    if (Devices.Num() == 0)
    {
        return false;
    }

    int32 SuccessCount = 0;

    // 현재는 첫 번째 장치에만 연결 (다중 연결은 관리자 컴포넌트를 통해 구현 필요)
    if (Devices.Num() > 0)
    {
        ConnectToDevice(Devices[0]);
        SuccessCount++;

        if (Devices.Num() > 1)
        {
            ShowInfoMessage(FString::Printf(TEXT("첫 번째 장치에 연결했습니다. 여러 장치 연결은 PJLinkManagerComponent를 사용하세요.")));
        }
        else
        {
            ShowInfoMessage(FString::Printf(TEXT("'%s' (%s)에 연결했습니다."),
                *Devices[0].Name, *Devices[0].IPAddress));
        }
    }

    return SuccessCount > 0;
}

// 일괄 프리셋 저장
bool UPJLinkDiscoveryWidget::BatchSaveAsPresets(const TArray<FPJLinkDiscoveryResult>& Devices, const FString& BasePresetName)
{
    if (Devices.Num() == 0)
    {
        return false;
    }

    int32 SuccessCount = 0;

    for (int32 i = 0; i < Devices.Num(); i++)
    {
        // 프리셋 이름 생성 (여러 개일 경우 번호 추가)
        FString PresetName = BasePresetName;
        if (PresetName.IsEmpty())
        {
            PresetName = Devices[i].Name.IsEmpty() ?
                FString::Printf(TEXT("Projector_%s"), *Devices[i].IPAddress) :
                Devices[i].Name;
        }

        if (Devices.Num() > 1)
        {
            PresetName = FString::Printf(TEXT("%s_%d"), *PresetName, i + 1);
        }

        // 저장
        SaveDeviceAsPreset(Devices[i], PresetName);
        SuccessCount++;
    }

    if (SuccessCount > 0)
    {
        ShowSuccessMessage(FString::Printf(TEXT("%d개 장치를 프리셋으로 저장했습니다."), SuccessCount));
    }

    return SuccessCount > 0;
}

// 일괄 그룹 추가
bool UPJLinkDiscoveryWidget::BatchAddToGroup(const TArray<FPJLinkDiscoveryResult>& Devices, const FString& GroupName)
{
    if (Devices.Num() == 0)
    {
        return false;
    }

    FString ActualGroupName = GroupName.IsEmpty() ? TEXT("Default") : GroupName;
    int32 SuccessCount = 0;

    for (const FPJLinkDiscoveryResult& Device : Devices)
    {
        AddDeviceToGroup(Device, ActualGroupName);
        SuccessCount++;
    }

    if (SuccessCount > 0)
    {
        ShowSuccessMessage(FString::Printf(TEXT("%d개 장치를 그룹 '%s'에 추가했습니다."),
            SuccessCount, *ActualGroupName));
    }

    return SuccessCount > 0;
}

// 작업 시작 메시지 개선
void UPJLinkDiscoveryWidget::ShowWorkingMessage(const FString& Operation, const FString& Details)
{
    FString Message = FString::Printf(TEXT("%s 중... %s"), *Operation, *Details);
    ShowInfoMessage(Message);

    // 작업 진행 중 표시 활성화
    SetWorkingIndicatorVisible(true);
}

// 작업 성공 메시지 개선
void UPJLinkDiscoveryWidget::ShowSuccessMessage(const FString& Message)
{
    // 작업 진행 중 표시 비활성화
    SetWorkingIndicatorVisible(false);

    // 원래 구현 호출
    Super::ShowSuccessMessage(Message);
}

// 작업 실패 메시지 개선
void UPJLinkDiscoveryWidget::ShowErrorMessage(const FString& ErrorMessage)
{
    // 작업 진행 중 표시 비활성화
    SetWorkingIndicatorVisible(false);

    // 원래 구현 호출
    Super::ShowErrorMessage(ErrorMessage);
}

// 작업 진행 중 표시 가시성 설정
void UPJLinkDiscoveryWidget::SetWorkingIndicatorVisible_Implementation(bool bVisible)
{
    // 블루프린트에서 구현할 수 있도록 함
    // 기본 구현은 비어 있음
}