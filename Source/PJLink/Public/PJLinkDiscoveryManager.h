// PJLinkDiscoveryManager.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "PJLinkTypes.h"
#include "Networking/Public/Interfaces/IPv4/IPv4SubnetInfo.h"
#include "PJLinkDiscoveryManager.generated.h"

/**
 * PJLink 장치 검색 결과를 나타내는 구조체
 */
USTRUCT(BlueprintType)
struct PJLINK_API FPJLinkDiscoveryResult
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|Discovery")
    FString IPAddress;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|Discovery")
    int32 Port = 4352;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|Discovery")
    FString Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|Discovery")
    FString ModelName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|Discovery")
    EPJLinkClass DeviceClass = EPJLinkClass::Class1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|Discovery")
    bool bRequiresAuthentication = false;

    // 발견 시간
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|Discovery")
    FDateTime DiscoveryTime;

    // 응답 시간 (밀리초)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|Discovery")
    int32 ResponseTimeMs = 0;

    // 기본 생성자
    FPJLinkDiscoveryResult() : DiscoveryTime(FDateTime::Now()) {}
};

/**
 * 검색 작업 상태를 나타내는 구조체
 */
USTRUCT(BlueprintType)
struct PJLINK_API FPJLinkDiscoveryStatus
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PJLink|Discovery")
    FString DiscoveryID;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PJLink|Discovery")
    int32 TotalAddresses = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PJLink|Discovery")
    int32 ScannedAddresses = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PJLink|Discovery")
    int32 DiscoveredDevices = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PJLink|Discovery")
    float ProgressPercentage = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PJLink|Discovery")
    bool bIsComplete = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PJLink|Discovery")
    bool bWasCancelled = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PJLink|Discovery")
    FDateTime StartTime;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PJLink|Discovery")
    FTimespan ElapsedTime;

    // 기본 생성자
    FPJLinkDiscoveryStatus() : StartTime(FDateTime::Now()) {}
};

// 검색 완료 이벤트 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPJLinkDiscoveryCompletedDelegate,
    const TArray<FPJLinkDiscoveryResult>&, DiscoveredDevices,
    bool, bSuccess);

// 장치 발견 이벤트 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPJLinkDeviceDiscoveredDelegate,
    const FPJLinkDiscoveryResult&, DiscoveredDevice);

// 검색 진행 상황 업데이트 이벤트 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPJLinkDiscoveryProgressDelegate,
    const FPJLinkDiscoveryStatus&, Status);

/**
 * PJLink 장치 검색을 담당하는 클래스
 */
UCLASS(BlueprintType, Blueprintable)
class PJLINK_API UPJLinkDiscoveryManager : public UObject
{
    GENERATED_BODY()

public:
    UPJLinkDiscoveryManager();
    virtual ~UPJLinkDiscoveryManager();

    virtual void BeginDestroy() override;

    /**
     * UDP 브로드캐스트를 통한 PJLink 장치 검색 시작
     * @param TimeoutSeconds 검색 제한 시간 (초)
     * @return 검색 작업 식별자
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Discovery")
    FString StartBroadcastDiscovery(float TimeoutSeconds = 5.0f);

    /**
     * IP 주소 범위를 스캔하여 PJLink 장치 검색
     * @param StartIPAddress 시작 IP 주소
     * @param EndIPAddress 종료 IP 주소
     * @param TimeoutSeconds 검색 제한 시간 (초)
     * @return 검색 작업 식별자
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Discovery")
    FString StartRangeScan(const FString& StartIPAddress, const FString& EndIPAddress, float TimeoutSeconds = 10.0f);

    /**
     * 서브넷 내 모든 IP를 스캔
     * @param SubnetAddress 서브넷 주소 (예: 192.168.1.0)
     * @param SubnetMask 서브넷 마스크 (예: 255.255.255.0)
     * @param TimeoutSeconds 검색 제한 시간 (초)
     * @return 검색 작업 식별자
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Discovery")
    FString StartSubnetScan(const FString& SubnetAddress, const FString& SubnetMask, float TimeoutSeconds = 20.0f);

    /**
     * 현재 진행 중인 검색 상태 가져오기
     * @param DiscoveryID 검색 작업 ID
     * @param OutStatus 출력 상태 구조체
     * @return 검색 작업이 존재하는지 여부
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Discovery")
    bool GetDiscoveryStatus(const FString& DiscoveryID, FPJLinkDiscoveryStatus& OutStatus);

    /**
     * 모든 진행 중인 검색 상태 가져오기
     * @return 모든 검색 상태 목록
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Discovery")
    TArray<FPJLinkDiscoveryStatus> GetAllDiscoveryStatuses();

    /**
     * 진행 중인 검색 작업 취소
     * @param DiscoveryID 취소할 검색 작업의 ID
     * @return 취소 성공 여부
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Discovery")
    bool CancelDiscovery(const FString& DiscoveryID);

    /**
     * 모든 진행 중인 검색 작업 취소
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Discovery")
    void CancelAllDiscoveries();

    /**
     * 검색 결과 가져오기
     * @param DiscoveryID 검색 작업 ID
     * @return 발견된 장치 목록
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Discovery")
    TArray<FPJLinkDiscoveryResult> GetDiscoveryResults(const FString& DiscoveryID);

    /**
     * 검색된 장치를 프로젝터 정보로 변환
     * @param DiscoveryResult 검색 결과
     * @return 프로젝터 정보
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Discovery")
    FPJLinkProjectorInfo ConvertToProjectorInfo(const FPJLinkDiscoveryResult& DiscoveryResult);

    /**
     * 검색 결과를 프리셋으로 저장
     * @param DiscoveryResult 검색 결과
     * @param PresetName 저장할 프리셋 이름
     * @return 저장 성공 여부
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Discovery")
    bool SaveDiscoveryResultAsPreset(const FPJLinkDiscoveryResult& DiscoveryResult, const FString& PresetName);

    /**
     * 모든 검색 결과를 그룹으로 저장
     * @param DiscoveryID 검색 작업 ID
     * @param GroupName 저장할 그룹 이름
     * @return 저장된 프로젝터 수
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Discovery")
    int32 SaveDiscoveryResultsAsGroup(const FString& DiscoveryID, const FString& GroupName);

    /**
     * 브로드캐스트 검색에 사용할 포트 설정
     * @param Port UDP 브로드캐스트 포트
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Discovery")
    void SetBroadcastPort(int32 Port) { BroadcastPort = Port; }

    /**
     * 검색 제한 시간 설정
     * @param TimeoutSeconds 검색 제한 시간 (초)
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Discovery")
    void SetDefaultTimeout(float TimeoutSeconds) { DefaultTimeoutSeconds = FMath::Max(1.0f, TimeoutSeconds); }

    /**
     * 최대 동시 검색 스레드 수 설정
     * @param MaxThreads 최대 스레드 수
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Discovery")
    void SetMaxConcurrentThreads(int32 MaxThreads) { MaxConcurrentThreads = FMath::Clamp(MaxThreads, 1, 16); }

    /**
     * 검색 대기 시간 설정 (각 IP당)
     * @param WaitTimeMs 대기 시간 (밀리초)
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Discovery")
    void SetPerAddressWaitTime(int32 WaitTimeMs) { PerAddressWaitTimeMs = FMath::Clamp(WaitTimeMs, 50, 5000); }

    // 이벤트
    UPROPERTY(BlueprintAssignable, Category = "PJLink|Discovery|Events")
    FPJLinkDiscoveryCompletedDelegate OnDiscoveryCompleted;

    UPROPERTY(BlueprintAssignable, Category = "PJLink|Discovery|Events")
    FPJLinkDeviceDiscoveredDelegate OnDeviceDiscovered;

    UPROPERTY(BlueprintAssignable, Category = "PJLink|Discovery|Events")
    FPJLinkDiscoveryProgressDelegate OnDiscoveryProgress;

private:
    // 브로드캐스트 준비
    bool SetupBroadcastSocket();

    // UDP 브로드캐스트 수행
    void PerformBroadcastDiscovery(const FString& DiscoveryID, float TimeoutSeconds);

    // IP 범위 스캔 수행
    void PerformRangeScan(const FString& DiscoveryID, uint32 StartIP, uint32 EndIP, float TimeoutSeconds);

    // 서브넷 스캔 수행
    void PerformSubnetScan(const FString& DiscoveryID, uint32 NetworkAddress, uint32 SubnetMask, float TimeoutSeconds);

    // 단일 IP 주소 스캔
    void ScanIPAddress(const FString& DiscoveryID, const FString& IPAddress, float TimeoutSeconds);

    // 검색 결과 처리
    void ProcessDiscoveryResponse(const FString& DiscoveryID, const FString& IPAddress,
        const FString& Response, int32 ResponseTimeMs);

    // 검색 완료 처리
    void CompleteDiscovery(const FString& DiscoveryID, bool bSuccess);

    // 검색 상태 업데이트
    void UpdateDiscoveryProgress(const FString& DiscoveryID, int32 ScannedAddresses, int32 DiscoveredDevices);

    // 검색 타임아웃 처리
    void HandleDiscoveryTimeout(const FString& DiscoveryID);

    // 고유 ID 생성
    FString GenerateDiscoveryID() const;

    // IP 문자열을 uint32로 변환
    static uint32 IPStringToUint32(const FString& IPString);

    // uint32를 IP 문자열로 변환
    static FString Uint32ToIPString(uint32 IPAddress);

    // UDP 소켓
    class FSocket* BroadcastSocket;

    // 브로드캐스트 포트
    int32 BroadcastPort = 4352;

    // 기본 타임아웃 시간 (초)
    float DefaultTimeoutSeconds = 5.0f;

    // 최대 동시 스레드 수
    int32 MaxConcurrentThreads = 4;

    // 각 IP당 대기 시간 (밀리초)
    int32 PerAddressWaitTimeMs = 200;

    // 진행 중인 검색 작업 상태
    TMap<FString, FPJLinkDiscoveryStatus> DiscoveryStatuses;

    // 검색 결과 저장
    TMap<FString, TArray<FPJLinkDiscoveryResult>> DiscoveryResults;

    // 타이머 핸들 저장
    TMap<FString, FTimerHandle> DiscoveryTimerHandles;

    // 검색 작업 동기화를 위한 임계 영역
    FCriticalSection DiscoveryLock;

    // 검색 스레드 관리
    TArray<class FRunnableThread*> ScanThreads;
};
