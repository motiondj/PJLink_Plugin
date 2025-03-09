// PJLinkDiscoveryWidget.cpp
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

        // 장치 발견 메시지 표시 - 이름이 있으면 이름과 IP, 없으면 IP만 표시
        FString DeviceName = DiscoveredDevice.Name.IsEmpty() ? DiscoveredDevice.IPAddress :
            FString::Printf(TEXT("%s (%s)"), *DiscoveredDevice.Name, *DiscoveredDevice.IPAddress);

        ShowInfoMessage(FString::Printf(TEXT("장치를 발견했습니다: %s"), *DeviceName));

        // 발견된 장치 수에 따라 애니메이션 업데이트
        if (bEnableProgressAnimation)
        {
            UpdateProgressAnimation(DiscoveryResults.Num() * 5.0f); // 장치 발견 시 애니메이션 속도 증가
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

// 수정된 함수:
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

        // 이벤트 호출
        OnDiscoveryStateChanged(NewState);

        // 상태에 따라 UI 요소 제어
        switch (NewState)
        {
        case EPJLinkDiscoveryState::Searching:
            // 검색 시작 - 애니메이션 시작
            if (bEnableProgressAnimation)
            {
                StartProgressAnimation();
            }
            break;

        case EPJLinkDiscoveryState::Completed:
        case EPJLinkDiscoveryState::Cancelled:
        case EPJLinkDiscoveryState::Failed:
            // 검색 종료 - 애니메이션 중지
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