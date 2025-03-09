// PJLinkDiscoveryManager.cpp
#include "PJLinkDiscoveryManager.h"
#include "PJLinkLog.h"
#include "PJLinkPresetManager.h"
#include "PJLinkManagerComponent.h"
#include "SocketSubsystem.h"
#include "Sockets.h"
#include "IPAddress.h"
#include "HAL/RunnableThread.h"
#include "Async/AsyncWork.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"

// 스캔 작업을 위한 비동기 작업 클래스
class FScanWorker : public FNonAbandonableTask
{
public:
    FScanWorker(UPJLinkDiscoveryManager* InManager, const FString& InDiscoveryID,
        uint32 InStartIP, uint32 InEndIP, float InTimeoutSeconds)
        : Manager(InManager)
        , DiscoveryID(InDiscoveryID)
        , StartIP(InStartIP)
        , EndIP(InEndIP)
        , TimeoutSeconds(InTimeoutSeconds)
    {
    }

    void DoWork()
    {
        if (Manager)
        {
            Manager->PerformRangeScan(DiscoveryID, StartIP, EndIP, TimeoutSeconds);
        }
    }

    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(FScanWorker, STATGROUP_ThreadPoolAsyncTasks);
    }

private:
    UPJLinkDiscoveryManager* Manager;
    FString DiscoveryID;
    uint32 StartIP;
    uint32 EndIP;
    float TimeoutSeconds;
};

UPJLinkDiscoveryManager::UPJLinkDiscoveryManager()
    : BroadcastSocket(nullptr)
    , BroadcastPort(4352)
    , DefaultTimeoutSeconds(5.0f)
    , MaxConcurrentThreads(4)
    , PerAddressWaitTimeMs(200)
{
}

UPJLinkDiscoveryManager::~UPJLinkDiscoveryManager()
{
    CancelAllDiscoveries();

    // 소켓 정리
    if (BroadcastSocket)
    {
        ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
        if (SocketSubsystem)
        {
            BroadcastSocket->Close();
            SocketSubsystem->DestroySocket(BroadcastSocket);
        }
        BroadcastSocket = nullptr;
    }
}

void UPJLinkDiscoveryManager::BeginDestroy()
{
    CancelAllDiscoveries();
    Super::BeginDestroy();
}

FString UPJLinkDiscoveryManager::StartBroadcastDiscovery(float TimeoutSeconds)
{
    // 검색 ID 생성
    FString DiscoveryID = GenerateDiscoveryID();

    // 타임아웃 값 검증
    float ActualTimeout = TimeoutSeconds > 0.0f ? TimeoutSeconds : DefaultTimeoutSeconds;

    // 검색 상태 초기화
    FPJLinkDiscoveryStatus NewStatus;
    NewStatus.DiscoveryID = DiscoveryID;
    NewStatus.TotalAddresses = 1; // 브로드캐스트는 단일 작업으로 간주
    NewStatus.StartTime = FDateTime::Now();

    {
        FScopeLock Lock(&DiscoveryLock);
        DiscoveryStatuses.Add(DiscoveryID, NewStatus);
        DiscoveryResults.Add(DiscoveryID, TArray<FPJLinkDiscoveryResult>());
    }

    // 브로드캐스트 소켓 설정
    if (!SetupBroadcastSocket())
    {
        PJLINK_LOG_ERROR(TEXT("Failed to setup broadcast socket for discovery"));
        CompleteDiscovery(DiscoveryID, false);
        return DiscoveryID;
    }

    // 타이머 설정 (타임아웃 처리)
    if (UWorld* World = GEngine->GetWorldFromContextObject(GetOuter(), EGetWorldErrorMode::LogAndReturnNull))
    {
        FTimerHandle& TimerHandle = DiscoveryTimerHandles.FindOrAdd(DiscoveryID);
        FTimerDelegate TimerDelegate;
        TimerDelegate.BindUObject(this, &UPJLinkDiscoveryManager::HandleDiscoveryTimeout, DiscoveryID);
        World->GetTimerManager().SetTimer(TimerHandle, TimerDelegate, ActualTimeout, false);
    }

    // 브로드캐스트 수행
    PerformBroadcastDiscovery(DiscoveryID, ActualTimeout);

    PJLINK_LOG_INFO(TEXT("Started broadcast discovery with ID: %s"), *DiscoveryID);
    return DiscoveryID;
}

FString UPJLinkDiscoveryManager::StartRangeScan(const FString& StartIPAddress, const FString& EndIPAddress, float TimeoutSeconds)
{
    // IP 주소 변환
    uint32 StartIP = IPStringToUint32(StartIPAddress);
    uint32 EndIP = IPStringToUint32(EndIPAddress);

    // 유효성 검사
    if (StartIP == 0 || EndIP == 0 || StartIP > EndIP)
    {
        PJLINK_LOG_ERROR(TEXT("Invalid IP range: %s - %s"), *StartIPAddress, *EndIPAddress);
        return TEXT("");
    }

    // 검색 ID 생성
    FString DiscoveryID = GenerateDiscoveryID();

    // 타임아웃 값 검증
    float ActualTimeout = TimeoutSeconds > 0.0f ? TimeoutSeconds : DefaultTimeoutSeconds;

    // 검색 상태 초기화
    FPJLinkDiscoveryStatus NewStatus;
    NewStatus.DiscoveryID = DiscoveryID;
    NewStatus.TotalAddresses = EndIP - StartIP + 1;
    NewStatus.StartTime = FDateTime::Now();

    {
        FScopeLock Lock(&DiscoveryLock);
        DiscoveryStatuses.Add(DiscoveryID, NewStatus);
        DiscoveryResults.Add(DiscoveryID, TArray<FPJLinkDiscoveryResult>());
    }

    // 타이머 설정 (타임아웃 처리)
    if (UWorld* World = GEngine->GetWorldFromContextObject(GetOuter(), EGetWorldErrorMode::LogAndReturnNull))
    {
        FTimerHandle& TimerHandle = DiscoveryTimerHandles.FindOrAdd(DiscoveryID);
        FTimerDelegate TimerDelegate;
        TimerDelegate.BindUObject(this, &UPJLinkDiscoveryManager::HandleDiscoveryTimeout, DiscoveryID);
        World->GetTimerManager().SetTimer(TimerHandle, TimerDelegate, ActualTimeout, false);
    }

    // 범위 스캔 작업 시작
    (new FAutoDeleteAsyncTask<FScanWorker>(this, DiscoveryID, StartIP, EndIP, ActualTimeout))->StartBackgroundTask();

    PJLINK_LOG_INFO(TEXT("Started IP range scan with ID: %s, Range: %s - %s, Addresses: %d"),
        *DiscoveryID, *StartIPAddress, *EndIPAddress, NewStatus.TotalAddresses);

    return DiscoveryID;
}

FString UPJLinkDiscoveryManager::StartSubnetScan(const FString& SubnetAddress, const FString& SubnetMask, float TimeoutSeconds)
{
    // 서브넷 및 마스크 변환
    uint32 NetworkAddress = IPStringToUint32(SubnetAddress);
    uint32 Mask = IPStringToUint32(SubnetMask);

    // 유효성 검사
    if (NetworkAddress == 0 || Mask == 0)
    {
        PJLINK_LOG_ERROR(TEXT("Invalid subnet parameters: %s / %s"), *SubnetAddress, *SubnetMask);
        return TEXT("");
    }

    // 검색 ID 생성
    FString DiscoveryID = GenerateDiscoveryID();

    // 타임아웃 값 검증
    float ActualTimeout = TimeoutSeconds > 0.0f ? TimeoutSeconds : DefaultTimeoutSeconds;

    // 사용 가능한 주소 수 계산 (비트 반전 후 1 빼기: 네트워크 주소와 브로드캐스트 주소 제외)
    uint32 HostBits = ~Mask;
    uint32 AddressCount = HostBits - 1;

    // 검색 상태 초기화
    FPJLinkDiscoveryStatus NewStatus;
    NewStatus.DiscoveryID = DiscoveryID;
    NewStatus.TotalAddresses = AddressCount;
    NewStatus.StartTime = FDateTime::Now();

    {
        FScopeLock Lock(&DiscoveryLock);
        DiscoveryStatuses.Add(DiscoveryID, NewStatus);
        DiscoveryResults.Add(DiscoveryID, TArray<FPJLinkDiscoveryResult>());
    }

    // 타이머 설정 (타임아웃 처리)
    if (UWorld* World = GEngine->GetWorldFromContextObject(GetOuter(), EGetWorldErrorMode::LogAndReturnNull))
    {
        FTimerHandle& TimerHandle = DiscoveryTimerHandles.FindOrAdd(DiscoveryID);
        FTimerDelegate TimerDelegate;
        TimerDelegate.BindUObject(this, &UPJLinkDiscoveryManager::HandleDiscoveryTimeout, DiscoveryID);
        World->GetTimerManager().SetTimer(TimerHandle, TimerDelegate, ActualTimeout, false);
    }

    // 서브넷 스캔 수행
    (new FAutoDeleteAsyncTask<FScanWorker>(this, DiscoveryID, NetworkAddress + 1, NetworkAddress + AddressCount, ActualTimeout))->StartBackgroundTask();

    PJLINK_LOG_INFO(TEXT("Started subnet scan with ID: %s, Subnet: %s/%s, Addresses: %d"),
        *DiscoveryID, *SubnetAddress, *SubnetMask, AddressCount);

    return DiscoveryID;
}

bool UPJLinkDiscoveryManager::GetDiscoveryStatus(const FString& DiscoveryID, FPJLinkDiscoveryStatus& OutStatus)
{
    FScopeLock Lock(&DiscoveryLock);

    if (!DiscoveryStatuses.Contains(DiscoveryID))
    {
        return false;
    }

    // 경과 시간 업데이트
    FPJLinkDiscoveryStatus& Status = DiscoveryStatuses[DiscoveryID];
    Status.ElapsedTime = FDateTime::Now() - Status.StartTime;

    OutStatus = Status;
    return true;
}

TArray<FPJLinkDiscoveryStatus> UPJLinkDiscoveryManager::GetAllDiscoveryStatuses()
{
    FScopeLock Lock(&DiscoveryLock);

    TArray<FPJLinkDiscoveryStatus> Statuses;
    FDateTime Now = FDateTime::Now();

    for (auto& Pair : DiscoveryStatuses)
    {
        // 경과 시간 업데이트
        Pair.Value.ElapsedTime = Now - Pair.Value.StartTime;
        Statuses.Add(Pair.Value);
    }

    return Statuses;
}

bool UPJLinkDiscoveryManager::CancelDiscovery(const FString& DiscoveryID)
{
    FScopeLock Lock(&DiscoveryLock);

    if (!DiscoveryStatuses.Contains(DiscoveryID))
    {
        return false;
    }

    // 상태 업데이트
    FPJLinkDiscoveryStatus& Status = DiscoveryStatuses[DiscoveryID];
    Status.bWasCancelled = true;
    Status.bIsComplete = true;
    Status.ElapsedTime = FDateTime::Now() - Status.StartTime;

    // 타이머 정리
    if (DiscoveryTimerHandles.Contains(DiscoveryID))
    {
        if (UWorld* World = GEngine->GetWorldFromContextObject(GetOuter(), EGetWorldErrorMode::LogAndReturnNull))
        {
            World->GetTimerManager().ClearTimer(DiscoveryTimerHandles[DiscoveryID]);
        }
        DiscoveryTimerHandles.Remove(DiscoveryID);
    }

    PJLINK_LOG_INFO(TEXT("Cancelled discovery: %s"), *DiscoveryID);

    return true;
}

void UPJLinkDiscoveryManager::CancelAllDiscoveries()
{
    FScopeLock Lock(&DiscoveryLock);

    // 모든 검색 작업 취소
    for (auto& Pair : DiscoveryStatuses)
    {
        Pair.Value.bWasCancelled = true;
        Pair.Value.bIsComplete = true;
        Pair.Value.ElapsedTime = FDateTime::Now() - Pair.Value.StartTime;
    }

    // 모든 타이머 정리
    if (UWorld* World = GEngine->GetWorldFromContextObject(GetOuter(), EGetWorldErrorMode::LogAndReturnNull))
    {
        for (auto& Pair : DiscoveryTimerHandles)
        {
            World->GetTimerManager().ClearTimer(Pair.Value);
        }
    }
    DiscoveryTimerHandles.Empty();

    PJLINK_LOG_INFO(TEXT("Cancelled all discoveries"));
}

TArray<FPJLinkDiscoveryResult> UPJLinkDiscoveryManager::GetDiscoveryResults(const FString& DiscoveryID)
{
    FScopeLock Lock(&DiscoveryLock);

    if (!DiscoveryResults.Contains(DiscoveryID))
    {
        return TArray<FPJLinkDiscoveryResult>();
    }

    return DiscoveryResults[DiscoveryID];
}

FPJLinkProjectorInfo UPJLinkDiscoveryManager::ConvertToProjectorInfo(const FPJLinkDiscoveryResult& DiscoveryResult)
{
    FPJLinkProjectorInfo ProjectorInfo;

    // 기본 정보 설정
    ProjectorInfo.Name = DiscoveryResult.Name.IsEmpty() ? FString::Printf(TEXT("Projector_%s"), *DiscoveryResult.IPAddress) : DiscoveryResult.Name;
    ProjectorInfo.IPAddress = DiscoveryResult.IPAddress;
    ProjectorInfo.Port = DiscoveryResult.Port;
    ProjectorInfo.DeviceClass = DiscoveryResult.DeviceClass;
    ProjectorInfo.bRequiresAuthentication = DiscoveryResult.bRequiresAuthentication;

    // 추가 정보가 있으면 설정
    if (!DiscoveryResult.ModelName.IsEmpty())
    {
        ProjectorInfo.ProductName = DiscoveryResult.ModelName;
    }

    return ProjectorInfo;
}

bool UPJLinkDiscoveryManager::SaveDiscoveryResultAsPreset(const FPJLinkDiscoveryResult& DiscoveryResult, const FString& PresetName)
{
    // 프로젝터 정보로 변환
    FPJLinkProjectorInfo ProjectorInfo = ConvertToProjectorInfo(DiscoveryResult);

    // 게임 인스턴스 가져오기
    UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(GetOuter());
    if (!GameInstance)
    {
        PJLINK_LOG_ERROR(TEXT("Failed to get GameInstance"));
        return false;
    }

    // 프리셋 매니저 가져오기
    UPJLinkPresetManager* PresetManager = GameInstance->GetSubsystem<UPJLinkPresetManager>();
    if (!PresetManager)
    {
        PJLINK_LOG_ERROR(TEXT("Failed to get PresetManager"));
        return false;
    }

    // 프리셋 저장
    bool bResult = PresetManager->SavePreset(PresetName, ProjectorInfo);

    if (bResult)
    {
        PJLINK_LOG_INFO(TEXT("Saved discovery result as preset: %s"), *PresetName);
    }
    else
    {
        PJLINK_LOG_ERROR(TEXT("Failed to save discovery result as preset: %s"), *PresetName);
    }

    return bResult;
}

int32 UPJLinkDiscoveryManager::SaveDiscoveryResultsAsGroup(const FString& DiscoveryID, const FString& GroupName)
{
    TArray<FPJLinkDiscoveryResult> Results;

    {
        FScopeLock Lock(&DiscoveryLock);
        if (!DiscoveryResults.Contains(DiscoveryID))
        {
            PJLINK_LOG_ERROR(TEXT("Discovery ID not found: %s"), *DiscoveryID);
            return 0;
        }

        Results = DiscoveryResults[DiscoveryID];
    }

    if (Results.Num() == 0)
    {
        PJLINK_LOG_WARNING(TEXT("No discovery results to save for ID: %s"), *DiscoveryID);
        return 0;
    }

    // 액터 찾기
    AActor* OwnerActor = nullptr;
    if (GetOuter() && GetOuter()->IsA<AActor>())
    {
        OwnerActor = Cast<AActor>(GetOuter());
    }
    else
    {
        TArray<AActor*> Actors;
        UGameplayStatics::GetAllActorsOfClass(GetOuter(), AActor::StaticClass(), Actors);

        for (AActor* Actor : Actors)
        {
            UPJLinkManagerComponent* ManagerComp = Actor->FindComponentByClass<UPJLinkManagerComponent>();
            if (ManagerComp)
            {
                OwnerActor = Actor;
                break;
            }
        }
    }

    if (!OwnerActor)
    {
        PJLINK_LOG_ERROR(TEXT("No actor with PJLinkManagerComponent found"));
        return 0;
    }

    // 매니저 컴포넌트 찾기
    UPJLinkManagerComponent* ManagerComp = OwnerActor->FindComponentByClass<UPJLinkManagerComponent>();
    if (!ManagerComp)
    {
        PJLINK_LOG_ERROR(TEXT("PJLinkManagerComponent not found on actor"));
        return 0;
    }

    // 그룹 생성 (또는 이미 존재하는 경우 사용)
    if (!ManagerComp->DoesGroupExist(GroupName))
    {
        ManagerComp->CreateGroup(GroupName);
    }

    // 각 결과를 그룹에 추가
    int32 AddedCount = 0;
    for (const FPJLinkDiscoveryResult& Result : Results)
    {
        FPJLinkProjectorInfo ProjectorInfo = ConvertToProjectorInfo(Result);
        UPJLinkComponent* ProjectorComp = ManagerComp->AddProjectorByInfo(ProjectorInfo, GroupName);

        if (ProjectorComp)
        {
            AddedCount++;
        }
    }

    PJLINK_LOG_INFO(TEXT("Added %d projectors to group '%s' from discovery results"),
        AddedCount, *GroupName);

    return AddedCount;
}

bool UPJLinkDiscoveryManager::SetupBroadcastSocket()
{
    // 이미 소켓이 있으면 재사용
    if (BroadcastSocket)
    {
        return true;
    }

    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    if (!SocketSubsystem)
    {
        PJLINK_LOG_ERROR(TEXT("Failed to get socket subsystem"));
        return false;
    }

    // UDP 소켓 생성
    BroadcastSocket = SocketSubsystem->CreateSocket(NAME_DGram, TEXT("PJLinkDiscovery"), true);
    if (!BroadcastSocket)
    {
        PJLINK_LOG_ERROR(TEXT("Failed to create UDP socket"));
        return false;
    }

    // 브로드캐스트 허용
    BroadcastSocket->SetBroadcast(true);

    // 재사용 가능하도록 설정
    BroadcastSocket->SetReuseAddr(true);

    // 바인딩
    FIPv4Address LocalAddr;
    FIPv4Address::Parse(TEXT("0.0.0.0"), LocalAddr);
    TSharedRef<FInternetAddr> LocalAddress = SocketSubsystem->CreateInternetAddr();
    LocalAddress->SetIp(LocalAddr.Value);
    LocalAddress->SetPort(0); // 시스템이 포트 할당

    if (!BroadcastSocket->Bind(*LocalAddress))
    {
        PJLINK_LOG_ERROR(TEXT("Failed to bind broadcast socket"));
        SocketSubsystem->DestroySocket(BroadcastSocket);
        BroadcastSocket = nullptr;
        return false;
    }

    return true;
}

void UPJLinkDiscoveryManager::PerformBroadcastDiscovery(const FString& DiscoveryID, float TimeoutSeconds)
{
    if (!BroadcastSocket)
    {
        PJLINK_LOG_ERROR(TEXT("Broadcast socket is null"));
        CompleteDiscovery(DiscoveryID, false);
        return;
    }

    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    if (!SocketSubsystem)
    {
        PJLINK_LOG_ERROR(TEXT("Failed to get socket subsystem"));
        CompleteDiscovery(DiscoveryID, false);
        return;
    }

    // 브로드캐스트 주소 설정
    TSharedRef<FInternetAddr> BroadcastAddr = SocketSubsystem->CreateInternetAddr();
    BroadcastAddr->SetBroadcast();
    BroadcastAddr->SetPort(BroadcastPort);

    // PJLink 검색 명령 (예: "%1CLSS ?\r")
    FString DiscoveryCommand = TEXT("%1CLSS ?\r");
    FTCHARToUTF8 Utf8Command(*DiscoveryCommand);

    // 브로드캐스트 전송
    int32 BytesSent = 0;
    bool bSendSuccess = BroadcastSocket->SendTo((uint8*)Utf8Command.Get(), Utf8Command.Length(), BytesSent, *BroadcastAddr);

    if (!bSendSuccess || BytesSent != Utf8Command.Length())
    {
        PJLINK_LOG_ERROR(TEXT("Failed to send broadcast: %s"), *SocketSubsystem->GetSocketError().ToString());
        CompleteDiscovery(DiscoveryID, false);
        return;
    }

    PJLINK_LOG_INFO(TEXT("Broadcast sent, waiting for responses..."));

    // 브로드캐스트 응답 대기는 비동기로 처리됨
    // 타임아웃 후 완료 처리
    if (UWorld* World = GEngine->GetWorldFromContextObject(GetOuter(), EGetWorldErrorMode::LogAndReturnNull))
    {
        FTimerDelegate CompletionDelegate;
        CompletionDelegate.BindUObject(this, &UPJLinkDiscoveryManager::CompleteDiscovery, DiscoveryID, true);

        FTimerHandle CompletionTimerHandle;
        World->GetTimerManager().SetTimer(CompletionTimerHandle, CompletionDelegate, TimeoutSeconds, false);
    }
}

void UPJLinkDiscoveryManager::PerformRangeScan(const FString& DiscoveryID, uint32 StartIP, uint32 EndIP, float TimeoutSeconds)
{
    // 최대 IP 숫자 제한 (너무 큰 범위는 분할 처리)
    const uint32 MaxIPsPerBatch = 255;

    // 범위가 너무 크면 여러 작업으로 분할
    if (EndIP - StartIP + 1 > MaxIPsPerBatch)
    {
        uint32 BatchCount = ((EndIP - StartIP) / MaxIPsPerBatch) + 1;
        uint32 CurrentStartIP = StartIP;

        for (uint32 i = 0; i < BatchCount; i++)
        {
            uint32 CurrentEndIP = FMath::Min(CurrentStartIP + MaxIPsPerBatch - 1, EndIP);

            // 부분 범위 스캔 작업 생성
            TArray<FString> IPsToScan;
            for (uint32 IP = CurrentStartIP; IP <= CurrentEndIP; IP++)
            {
                IPsToScan.Add(Uint32ToIPString(IP));
            }

            // 각 IP 주소 스캔
            for (const FString& IPAddress : IPsToScan)
            {
                ScanIPAddress(DiscoveryID, IPAddress, TimeoutSeconds);

                // 작업 중단 여부 확인
                {
                    FScopeLock Lock(&DiscoveryLock);
                    if (DiscoveryStatuses.Contains(DiscoveryID) &&
                        (DiscoveryStatuses[DiscoveryID].bIsComplete || DiscoveryStatuses[DiscoveryID].bWasCancelled))
                    {
                        return;
                    }
                }

                // IP 스캔 간 대기 (네트워크 부하 방지)
                FPlatformProcess::Sleep(PerAddressWaitTimeMs / 1000.0f);
            }

            CurrentStartIP = CurrentEndIP + 1;
        }
    }
    else
    {
        // 작은 범위는 직접 처리
        TArray<FString> IPsToScan;
        for (uint32 IP = StartIP; IP <= EndIP; IP++)
        {
            IPsToScan.Add(Uint32ToIPString(IP));
        }

        // 각 IP 주소 스캔
        for (const FString& IPAddress : IPsToScan)
        {
            ScanIPAddress(DiscoveryID, IPAddress, TimeoutSeconds);

            // 작업 중단 여부 확인
            {
                FScopeLock Lock(&DiscoveryLock);
                if (DiscoveryStatuses.Contains(DiscoveryID) &&
                    (DiscoveryStatuses[DiscoveryID].bIsComplete || DiscoveryStatuses[DiscoveryID].bWasCancelled))
                {
                    return;
                }
            }

            // IP 스캔 간 대기 (네트워크 부하 방지)
            FPlatformProcess::Sleep(PerAddressWaitTimeMs / 1000.0f);
        }
    }

    // 스캔 완료 처리
    CompleteDiscovery(DiscoveryID, true);
}

void UPJLinkDiscoveryManager::PerformSubnetScan(const FString& DiscoveryID, uint32 NetworkAddress, uint32 SubnetMask, float TimeoutSeconds)
{
    // 호스트 부분 비트 구하기
    uint32 HostBits = ~SubnetMask;

    // 첫 번째 호스트 주소 (네트워크 주소 + 1)
    uint32 FirstHostIP = NetworkAddress + 1;

    // 마지막 호스트 주소 (브로드캐스트 주소 - 1)
    uint32 LastHostIP = NetworkAddress + HostBits - 1;

    // 범위 스캔 수행
    PerformRangeScan(DiscoveryID, FirstHostIP, LastHostIP, TimeoutSeconds);
}

void UPJLinkDiscoveryManager::ScanIPAddress(const FString& DiscoveryID, const FString& IPAddress, float TimeoutSeconds)
{
    // 스캔 진행 상황 업데이트
    {
        FScopeLock Lock(&DiscoveryLock);
        if (DiscoveryStatuses.Contains(DiscoveryID))
        {
            FPJLinkDiscoveryStatus& Status = DiscoveryStatuses[DiscoveryID];
            Status.ScannedAddresses++;
            UpdateDiscoveryProgress(DiscoveryID, Status.ScannedAddresses, Status.DiscoveredDevices);
        }
    }

    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    if (!SocketSubsystem)
    {
        PJLINK_LOG_ERROR(TEXT("Failed to get socket subsystem"));
        return;
    }

    // TCP 소켓 생성
    FSocket* Socket = SocketSubsystem->CreateSocket(NAME_Stream, TEXT("PJLinkScan"), true);
    if (!Socket)
    {
        PJLINK_LOG_ERROR(TEXT("Failed to create TCP socket for scanning"));
        return;
    }

    // 연결 타임아웃 설정
    Socket->SetNonBlocking(true);

    // 연결 시도
    FIPv4Address IP;
    if (!FIPv4Address::Parse(IPAddress, IP))
    {
        PJLINK_LOG_ERROR(TEXT("Invalid IP address: %s"), *IPAddress);
        SocketSubsystem->DestroySocket(Socket);
        return;
    }

    TSharedRef<FInternetAddr> Addr = SocketSubsystem->CreateInternetAddr();
    Addr->SetIp(IP.Value);
    Addr->SetPort(BroadcastPort);

    // 연결 시도 (비동기)
    bool bConnected = Socket->Connect(*Addr);

    if (!bConnected)
    {
        ESocketErrors LastError = SocketSubsystem->GetLastErrorCode();

        // EWOULDBLOCK은 비동기 연결 진행 중임을 의미
        if (LastError != SE_EWOULDBLOCK)
        {
            // 연결 실패
            SocketSubsystem->DestroySocket(Socket);
            return;
        }
    }

    // 연결 대기
    double StartTime = FPlatformTime::Seconds();
    double EndTime = StartTime + TimeoutSeconds;

    while (FPlatformTime::Seconds() < EndTime)
    {
        // 작업 중단 여부 확인
        {
            FScopeLock Lock(&DiscoveryLock);
            if (DiscoveryStatuses.Contains(DiscoveryID) &&
                (DiscoveryStatuses[DiscoveryID].bIsComplete || DiscoveryStatuses[DiscoveryID].bWasCancelled))
            {
                SocketSubsystem->DestroySocket(Socket);
                return;
            }
        }

        // 연결 상태 확인
        bool bConnectionComplete = false;
        Socket->HasPendingConnection(bConnectionComplete);

        if (bConnectionComplete)
        {
            // 연결 성공, PJLink 명령 보내기
            FString PJLinkCommand = TEXT("%1CLSS ?\r");
            FTCHARToUTF8 Utf8Command(*PJLinkCommand);

            int32 BytesSent = 0;
            bool bSendSuccess = Socket->Send((uint8*)Utf8Command.Get(), Utf8Command.Length(), BytesSent);

            if (bSendSuccess && BytesSent == Utf8Command.Length())
            {
                // 응답 대기
                uint8 RecvBuffer[1024] = { 0 };
                int32 BytesRead = 0;
                bool bReadSuccess = Socket->Wait(ESocketWaitConditions::WaitForRead, FTimespan::FromSeconds(2.0));

                if (bReadSuccess)
                {
                    // 응답 읽기
                    bReadSuccess = Socket->Recv(RecvBuffer, sizeof(RecvBuffer) - 1, BytesRead);

                    if (bReadSuccess && BytesRead > 0)
                    {
                        // 널 종료 추가
                        RecvBuffer[BytesRead] = 0;

                        // UTF-8 문자열로 변환
                        FString Response = UTF8_TO_TCHAR((const char*)RecvBuffer);

                        // 응답 시간 계산
                        int32 ResponseTimeMs = FMath::FloorToInt((FPlatformTime::Seconds() - StartTime) * 1000.0);

                        // 응답 처리
                        ProcessDiscoveryResponse(DiscoveryID, IPAddress, Response, ResponseTimeMs);
                    }
                }
            }

            break; // 데이터 송수신 완료 후 종료
        }

        FPlatformProcess::Sleep(0.01f); // 10ms 대기
    }

    // 소켓 정리
    Socket->Close();
    SocketSubsystem->DestroySocket(Socket);
}

void UPJLinkDiscoveryManager::ProcessDiscoveryResponse(const FString& DiscoveryID, const FString& IPAddress,
    const FString& Response, int32 ResponseTimeMs)
{
    // PJLink 응답인지 확인
    if (!Response.StartsWith(TEXT("%")) || Response.Len() < 7)
    {
        return;
    }

    // 검색 결과 생성
    FPJLinkDiscoveryResult Result;
    Result.IPAddress = IPAddress;
    Result.Port = BroadcastPort;
    Result.DiscoveryTime = FDateTime::Now();
    Result.ResponseTimeMs = ResponseTimeMs;

    // 클래스 정보 추출
    if (Response.Contains(TEXT("=1")))
    {
        Result.DeviceClass = EPJLinkClass::Class1;
    }
    else if (Response.Contains(TEXT("=2")))
    {
        Result.DeviceClass = EPJLinkClass::Class2;
    }

    // 인증 필요 여부 확인 (인증 챌린지가 있는지)
    Result.bRequiresAuthentication = Response.Contains(TEXT("PJLINK 1"));

    // 추가 정보 요청 (NAME 명령 등)을 위해서는 별도의 연결 필요
    // 여기서는 기본 검색 결과만 반환

    // 결과 저장
    bool bNewDevice = false;
    {
        FScopeLock Lock(&DiscoveryLock);
        if (DiscoveryStatuses.Contains(DiscoveryID) && DiscoveryResults.Contains(DiscoveryID))
        {
            // 중복 체크
            bool bDuplicate = false;
            for (const FPJLinkDiscoveryResult& ExistingResult : DiscoveryResults[DiscoveryID])
            {
                if (ExistingResult.IPAddress == IPAddress)
                {
                    bDuplicate = true;
                    break;
                }
            }

            if (!bDuplicate)
            {
                DiscoveryResults[DiscoveryID].Add(Result);
                DiscoveryStatuses[DiscoveryID].DiscoveredDevices++;
                bNewDevice = true;

                // 진행 상황 업데이트
                UpdateDiscoveryProgress(DiscoveryID, DiscoveryStatuses[DiscoveryID].ScannedAddresses,
                    DiscoveryStatuses[DiscoveryID].DiscoveredDevices);
            }
        }
    }

    // 새 장치 발견 이벤트 발생
    if (bNewDevice && OnDeviceDiscovered.IsBound())
    {
        OnDeviceDiscovered.Broadcast(Result);
    }

    PJLINK_LOG_INFO(TEXT("Discovered PJLink device at %s (Response time: %dms)"), *IPAddress, ResponseTimeMs);
}

void UPJLinkDiscoveryManager::CompleteDiscovery(const FString& DiscoveryID, bool bSuccess)
{
    TArray<FPJLinkDiscoveryResult> Results;
    bool bAlreadyComplete = false;

    {
        FScopeLock Lock(&DiscoveryLock);
        if (!DiscoveryStatuses.Contains(DiscoveryID))
        {
            return;
        }

        FPJLinkDiscoveryStatus& Status = DiscoveryStatuses[DiscoveryID];

        // 이미 완료된 작업인지 확인
        bAlreadyComplete = Status.bIsComplete;

        if (!bAlreadyComplete)
        {
            // 상태 업데이트
            Status.bIsComplete = true;
            Status.ElapsedTime = FDateTime::Now() - Status.StartTime;

            // 타이머 정리
            if (DiscoveryTimerHandles.Contains(DiscoveryID))
            {
                if (UWorld* World = GEngine->GetWorldFromContextObject(GetOuter(), EGetWorldErrorMode::LogAndReturnNull))
                {
                    World->GetTimerManager().ClearTimer(DiscoveryTimerHandles[DiscoveryID]);
                }
                DiscoveryTimerHandles.Remove(DiscoveryID);
            }
        }

        // 결과 가져오기
        if (DiscoveryResults.Contains(DiscoveryID))
        {
            Results = DiscoveryResults[DiscoveryID];
        }
    }

    // 완료 이벤트 발생 (이미 완료된 작업이 아닌 경우에만)
    if (!bAlreadyComplete && OnDiscoveryCompleted.IsBound())
    {
        OnDiscoveryCompleted.Broadcast(Results, bSuccess);
    }

    PJLINK_LOG_INFO(TEXT("Discovery completed: %s, Success: %s, Devices found: %d"),
        *DiscoveryID, bSuccess ? TEXT("True") : TEXT("False"), Results.Num());
}

void UPJLinkDiscoveryManager::UpdateDiscoveryProgress(const FString& DiscoveryID, int32 ScannedAddresses, int32 DiscoveredDevices)
{
    bool bShouldBroadcast = false;
    FPJLinkDiscoveryStatus StatusCopy;

    {
        FScopeLock Lock(&DiscoveryLock);
        if (!DiscoveryStatuses.Contains(DiscoveryID))
        {
            return;
        }

        FPJLinkDiscoveryStatus& Status = DiscoveryStatuses[DiscoveryID];

        // 진행률 계산
        if (Status.TotalAddresses > 0)
        {
            Status.ProgressPercentage = static_cast<float>(ScannedAddresses) / static_cast<float>(Status.TotalAddresses) * 100.0f;
        }

        // 필요할 때만 업데이트 이벤트 발생 (예: 5% 단위)
        static const float ProgressStep = 5.0f;
        static TMap<FString, float> LastReportedProgress;

        float LastProgress = LastReportedProgress.Contains(DiscoveryID) ? LastReportedProgress[DiscoveryID] : -1.0f;
        float CurrentProgress = Status.ProgressPercentage;

        if (LastProgress < 0.0f || CurrentProgress - LastProgress >= ProgressStep || CurrentProgress >= 100.0f)
        {
            LastReportedProgress.Add(DiscoveryID, CurrentProgress);
            bShouldBroadcast = true;
            StatusCopy = Status;
        }
    }

    // 이벤트 발생 (임계 영역 밖에서)
    if (bShouldBroadcast && OnDiscoveryProgress.IsBound())
    {
        OnDiscoveryProgress.Broadcast(StatusCopy);
    }
}

void UPJLinkDiscoveryManager::HandleDiscoveryTimeout(const FString& DiscoveryID)
{
    // 타임아웃 처리 - 검색 종료
    CompleteDiscovery(DiscoveryID, true);
}

FString UPJLinkDiscoveryManager::GenerateDiscoveryID() const
{
    return FGuid::NewGuid().ToString();
}

uint32 UPJLinkDiscoveryManager::IPStringToUint32(const FString& IPString)
{
    uint32 Result = 0;

    TArray<FString> Octets;
    if (IPString.ParseIntoArray(Octets, TEXT("."), true) != 4)
    {
        return 0;
    }

    for (int32 i = 0; i < 4; i++)
    {
        int32 Octet = FCString::Atoi(*Octets[i]);
        if (Octet < 0 || Octet > 255)
        {
            return 0;
        }

        Result = (Result << 8) | static_cast<uint32>(Octet);
    }

    return Result;
}

FString UPJLinkDiscoveryManager::Uint32ToIPString(uint32 IPAddress)
{
    return FString::Printf(TEXT("%d.%d.%d.%d"),
        (IPAddress >> 24) & 0xFF,
        (IPAddress >> 16) & 0xFF,
        (IPAddress >> 8) & 0xFF,
        IPAddress & 0xFF);
}