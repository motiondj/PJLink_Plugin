// PJLinkProjectorInstance.cpp
#include "PJLinkProjectorInstance.h"
#include "PJLinkNetworkManager.h"

UPJLinkProjectorInstance::UPJLinkProjectorInstance()
    : NetworkManager(nullptr)  // 초기화 목록으로 명시적 초기화
{
    // 생성자 내에서 NetworkManager 생성 코드를 지연 초기화로 변경
    // BeginPlay나 Initialize 시점으로 이동
}

void UPJLinkProjectorInstance::Initialize(
    const FString& InName,
    const FString& InIPAddress,
    int32 InPort,
    EPJLinkClass InDeviceClass,
    bool bInRequiresAuthentication,
    const FString& InPassword)
{
    ProjectorInfo.Name = InName;
    ProjectorInfo.IPAddress = InIPAddress;
    ProjectorInfo.Port = InPort;
    ProjectorInfo.DeviceClass = InDeviceClass;
    ProjectorInfo.bRequiresAuthentication = bInRequiresAuthentication;
    ProjectorInfo.Password = InPassword;

    // NetworkManager를 여기서 초기화
    if (!NetworkManager)
    {
        NetworkManager = NewObject<UPJLinkNetworkManager>(this);
        if (NetworkManager)
        {
            NetworkManager->OnResponseReceived.AddDynamic(this, &UPJLinkProjectorInstance::HandleResponseReceived);
        }
    }
}

bool UPJLinkProjectorInstance::Connect()
{
    if (NetworkManager)
    {
        return NetworkManager->ConnectToProjector(ProjectorInfo);
    }

    return false;
}

void UPJLinkProjectorInstance::Disconnect()
{
    if (NetworkManager)
    {
        NetworkManager->DisconnectFromProjector();
    }
}

bool UPJLinkProjectorInstance::PowerOn()
{
    if (NetworkManager)
    {
        return NetworkManager->PowerOn();
    }

    return false;
}

bool UPJLinkProjectorInstance::PowerOff()
{
    if (NetworkManager)
    {
        return NetworkManager->PowerOff();
    }

    return false;
}

bool UPJLinkProjectorInstance::SwitchInputSource(EPJLinkInputSource InputSource)
{
    if (NetworkManager)
    {
        return NetworkManager->SwitchInputSource(InputSource);
    }

    return false;
}

bool UPJLinkProjectorInstance::RequestStatus()
{
    if (NetworkManager)
    {
        return NetworkManager->RequestStatus();
    }

    return false;
}

bool UPJLinkProjectorInstance::IsConnected() const
{
    if (NetworkManager)
    {
        return NetworkManager->IsConnected();
    }

    return false;
}

FPJLinkProjectorInfo UPJLinkProjectorInstance::GetProjectorInfo() const
{
    if (NetworkManager)
    {
        return NetworkManager->GetProjectorInfo();
    }

    return ProjectorInfo;
}

void UPJLinkProjectorInstance::HandleResponseReceived(EPJLinkCommand Command, EPJLinkResponseStatus Status, const FString& Response)
{
    // 이벤트 전달
    OnResponseReceived.Broadcast(Command, Status, Response);
}