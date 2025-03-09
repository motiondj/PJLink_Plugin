// PJLinkBlueprintLibrary.cpp
#include "PJLinkBlueprintLibrary.h"
#include "PJLinkSubsystem.h" // 여기서 완전한 정의 포함
#include "Kismet/GameplayStatics.h"
#include "PJLinkDiscoveryManager.h"

UPJLinkSubsystem* UPJLinkBlueprintLibrary::GetPJLinkSubsystem(const UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return nullptr;
    }

    UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(WorldContextObject);
    if (!GameInstance)
    {
        return nullptr;
    }

    // 타입 캐스팅 방식으로 변경
    return Cast<UPJLinkSubsystem>(GameInstance->GetSubsystemBase(UPJLinkSubsystem::StaticClass()));
}

FPJLinkProjectorInfo UPJLinkBlueprintLibrary::CreateProjectorInfo(
    const FString& Name,
    const FString& IPAddress,
    int32 Port,
    EPJLinkClass DeviceClass,
    bool bRequiresAuthentication,
    const FString& Password)
{
    FPJLinkProjectorInfo ProjectorInfo;
    ProjectorInfo.Name = Name;
    ProjectorInfo.IPAddress = IPAddress;
    ProjectorInfo.Port = Port;
    ProjectorInfo.DeviceClass = DeviceClass;
    ProjectorInfo.bRequiresAuthentication = bRequiresAuthentication;
    ProjectorInfo.Password = Password;

    return ProjectorInfo;
}

bool UPJLinkBlueprintLibrary::ConnectToProjector(
    const UObject* WorldContextObject,
    const FPJLinkProjectorInfo& ProjectorInfo)
{
    UPJLinkSubsystem* PJLinkSystem = GetPJLinkSubsystem(WorldContextObject);
    if (PJLinkSystem)
    {
        return PJLinkSystem->ConnectToProjector(ProjectorInfo);
    }

    return false;
}

void UPJLinkBlueprintLibrary::DisconnectFromProjector(const UObject* WorldContextObject)
{
    UPJLinkSubsystem* PJLinkSystem = GetPJLinkSubsystem(WorldContextObject);
    if (PJLinkSystem)
    {
        PJLinkSystem->DisconnectFromProjector();
    }
}

bool UPJLinkBlueprintLibrary::PowerOnProjector(const UObject* WorldContextObject)
{
    UPJLinkSubsystem* PJLinkSystem = GetPJLinkSubsystem(WorldContextObject);
    if (PJLinkSystem)
    {
        return PJLinkSystem->PowerOn();
    }

    return false;
}

bool UPJLinkBlueprintLibrary::PowerOffProjector(const UObject* WorldContextObject)
{
    UPJLinkSubsystem* PJLinkSystem = GetPJLinkSubsystem(WorldContextObject);
    if (PJLinkSystem)
    {
        return PJLinkSystem->PowerOff();
    }

    return false;
}

bool UPJLinkBlueprintLibrary::SwitchInputSource(
    const UObject* WorldContextObject,
    EPJLinkInputSource InputSource)
{
    UPJLinkSubsystem* PJLinkSystem = GetPJLinkSubsystem(WorldContextObject);
    if (PJLinkSystem)
    {
        return PJLinkSystem->SwitchInputSource(InputSource);
    }

    return false;
}

bool UPJLinkBlueprintLibrary::RequestStatus(const UObject* WorldContextObject)
{
    UPJLinkSubsystem* PJLinkSystem = GetPJLinkSubsystem(WorldContextObject);
    if (PJLinkSystem)
    {
        return PJLinkSystem->RequestStatus();
    }

    return false;
}

bool UPJLinkBlueprintLibrary::IsConnected(const UObject* WorldContextObject)
{
    UPJLinkSubsystem* PJLinkSystem = GetPJLinkSubsystem(WorldContextObject);
    if (PJLinkSystem)
    {
        return PJLinkSystem->IsConnected();
    }

    return false;
}

FPJLinkProjectorInfo UPJLinkBlueprintLibrary::GetProjectorInfo(const UObject* WorldContextObject)
{
    UPJLinkSubsystem* PJLinkSystem = GetPJLinkSubsystem(WorldContextObject);
    if (PJLinkSystem)
    {
        return PJLinkSystem->GetProjectorInfo();
    }

    return FPJLinkProjectorInfo();
}

UPJLinkDiscoveryManager* UPJLinkBlueprintLibrary::CreatePJLinkDiscoveryManager(const UObject* WorldContextObject, AActor* OwnerActor)
{
    UObject* Outer = OwnerActor ? OwnerActor : const_cast<UObject*>(WorldContextObject);
    if (!Outer)
    {
        return nullptr;
    }

    return NewObject<UPJLinkDiscoveryManager>(Outer);
}

FString UPJLinkBlueprintLibrary::DiscoverPJLinkDevices(const UObject* WorldContextObject, UPJLinkDiscoveryManager*& OutDiscoveryManager, float TimeoutSeconds)
{
    // 매니저 생성
    if (!OutDiscoveryManager)
    {
        OutDiscoveryManager = CreatePJLinkDiscoveryManager(WorldContextObject);
    }

    if (!OutDiscoveryManager)
    {
        return TEXT("");
    }

    // 브로드캐스트 검색 시작
    return OutDiscoveryManager->StartBroadcastDiscovery(TimeoutSeconds);
}

FString UPJLinkBlueprintLibrary::ScanIPRangeForPJLinkDevices(const UObject* WorldContextObject, UPJLinkDiscoveryManager*& OutDiscoveryManager,
    const FString& StartIP, const FString& EndIP, float TimeoutSeconds)
{
    // 매니저 생성
    if (!OutDiscoveryManager)
    {
        OutDiscoveryManager = CreatePJLinkDiscoveryManager(WorldContextObject);
    }

    if (!OutDiscoveryManager)
    {
        return TEXT("");
    }

    // IP 범위 스캔 시작
    return OutDiscoveryManager->StartRangeScan(StartIP, EndIP, TimeoutSeconds);
}

FString UPJLinkBlueprintLibrary::ScanSubnetForPJLinkDevices(const UObject* WorldContextObject, UPJLinkDiscoveryManager*& OutDiscoveryManager,
    const FString& SubnetAddress, const FString& SubnetMask, float TimeoutSeconds)
{
    // 매니저 생성
    if (!OutDiscoveryManager)
    {
        OutDiscoveryManager = CreatePJLinkDiscoveryManager(WorldContextObject);
    }

    if (!OutDiscoveryManager)
    {
        return TEXT("");
    }

    // 서브넷 스캔 시작
    return OutDiscoveryManager->StartSubnetScan(SubnetAddress, SubnetMask, TimeoutSeconds);
}