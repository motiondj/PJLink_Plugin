// PJLinkBlueprintLibrary.h
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PJLinkTypes.h"
#include "PJLinkBlueprintLibrary.generated.h"

// 전방 선언
class UPJLinkSubsystem;

/**
 * PJLink 기능을 위한 블루프린트 함수 라이브러리
 * 블루프린트에서 쉽게 PJLink 기능을 사용할 수 있는 정적 함수들 제공
 */
UCLASS()
class PJLINK_API UPJLinkBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * PJLink 서브시스템 가져오기
     */
    UFUNCTION(BlueprintPure, Category = "PJLink", meta = (WorldContext = "WorldContextObject"))
    static UPJLinkSubsystem* GetPJLinkSubsystem(const UObject* WorldContextObject);

    /**
    * 프로젝터 정보 생성
    */
    UFUNCTION(BlueprintPure, Category = "PJLink")
    static FPJLinkProjectorInfo CreateProjectorInfo(
        const FString& Name,
        const FString& IPAddress,
        int32 Port = 4352,
        EPJLinkClass DeviceClass = EPJLinkClass::Class1,
        bool bRequiresAuthentication = false,
        const FString& Password = TEXT("")  // FString() 대신 TEXT("") 사용
    );

    /**
     * 프로젝터에 연결
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink", meta = (WorldContext = "WorldContextObject"))
    static bool ConnectToProjector(
        const UObject* WorldContextObject,
        const FPJLinkProjectorInfo& ProjectorInfo
    );

    /**
     * 프로젝터 연결 해제
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink", meta = (WorldContext = "WorldContextObject"))
    static void DisconnectFromProjector(const UObject* WorldContextObject);

    /**
     * 프로젝터 전원 켜기
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Control", meta = (WorldContext = "WorldContextObject"))
    static bool PowerOnProjector(const UObject* WorldContextObject);

    /**
     * 프로젝터 전원 끄기
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Control", meta = (WorldContext = "WorldContextObject"))
    static bool PowerOffProjector(const UObject* WorldContextObject);

    /**
     * 입력 소스 변경
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Control", meta = (WorldContext = "WorldContextObject"))
    static bool SwitchInputSource(
        const UObject* WorldContextObject,
        EPJLinkInputSource InputSource
    );

    /**
     * 상태 요청
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Control", meta = (WorldContext = "WorldContextObject"))
    static bool RequestStatus(const UObject* WorldContextObject);

    /**
     * 연결 상태 확인
     */
    UFUNCTION(BlueprintPure, Category = "PJLink", meta = (WorldContext = "WorldContextObject"))
    static bool IsConnected(const UObject* WorldContextObject);

    /**
     * 현재 프로젝터 정보 가져오기
     */
    UFUNCTION(BlueprintPure, Category = "PJLink", meta = (WorldContext = "WorldContextObject"))
    static FPJLinkProjectorInfo GetProjectorInfo(const UObject* WorldContextObject);

    /**
 * PJLink 장치 검색 매니저 생성
 */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Discovery", meta = (WorldContext = "WorldContextObject"))
    static UPJLinkDiscoveryManager* CreatePJLinkDiscoveryManager(const UObject* WorldContextObject, AActor* OwnerActor = nullptr);

    /**
     * 로컬 네트워크에서 UDP 브로드캐스트로 PJLink 장치 검색
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Discovery", meta = (WorldContext = "WorldContextObject"))
    static FString DiscoverPJLinkDevices(const UObject* WorldContextObject, UPJLinkDiscoveryManager*& OutDiscoveryManager, float TimeoutSeconds = 5.0f);

    /**
     * IP 범위 내에서 PJLink 장치 검색
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Discovery", meta = (WorldContext = "WorldContextObject"))
    static FString ScanIPRangeForPJLinkDevices(const UObject* WorldContextObject, UPJLinkDiscoveryManager*& OutDiscoveryManager,
        const FString& StartIP, const FString& EndIP, float TimeoutSeconds = 10.0f);

    /**
     * 서브넷 내에서 PJLink 장치 검색
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Discovery", meta = (WorldContext = "WorldContextObject"))
    static FString ScanSubnetForPJLinkDevices(const UObject* WorldContextObject, UPJLinkDiscoveryManager*& OutDiscoveryManager,
        const FString& SubnetAddress, const FString& SubnetMask, float TimeoutSeconds = 20.0f);
};
