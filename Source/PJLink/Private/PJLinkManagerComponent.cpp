// PJLinkManagerComponent.cpp
#include "PJLinkManagerComponent.h"
#include "PJLinkLog.h"
#include "Kismet/GameplayStatics.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"

UPJLinkManagerComponent::UPJLinkManagerComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.1f; // 10Hz 틱
    bAutoActivate = true;
}

// BeginPlay 함수에 상태 업데이트 타이머 설정 추가
void UPJLinkManagerComponent::BeginPlay()
{
    Super::BeginPlay();

    // 기본 그룹 생성
    CreateGroup(TEXT("Default"));

    // 상태 업데이트 타이머 시작
    if (bStatusUpdateEnabled && StatusUpdateInterval > 0.0f)
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(
                StatusUpdateTimerHandle,
                this,
                &UPJLinkManagerComponent::PeriodicStatusUpdate,
                StatusUpdateInterval,
                true);
        }
    }

    PJLINK_LOG_INFO(TEXT("PJLink Manager Component initialized"));
}

// EndPlay 함수에 타이머 정리 코드 추가
void UPJLinkManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // 모든 프로젝터 연결 해제
    DisconnectAll();

    // 타이머 정리
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(StatusUpdateTimerHandle);

        for (const auto& TimerPair : CommandTimeoutHandles)
        {
            World->GetTimerManager().ClearTimer(TimerPair.Value);
        }
    }

    // 맵 초기화
    ProjectorMap.Empty();
    GroupMap.Empty();
    CommandResults.Empty();
    CommandTimeoutHandles.Empty();
    ProjectorStatusMap.Empty();

    Super::EndPlay(EndPlayReason);
}

void UPJLinkManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // 진행 중인 명령 타임아웃 처리 등 필요한 경우 여기에 구현
}

// AddProjector 함수에 상태 초기화 코드 추가
bool UPJLinkManagerComponent::AddProjector(UPJLinkComponent* ProjectorComponent)
{
    if (!ProjectorComponent)
    {
        PJLINK_LOG_ERROR(TEXT("Failed to add projector: Invalid component reference"));
        return false;
    }

    FPJLinkProjectorInfo ProjectorInfo = ProjectorComponent->GetProjectorInfo();
    FString ProjectorID = GenerateProjectorID(ProjectorInfo);

    if (ProjectorMap.Contains(ProjectorID))
    {
        PJLINK_LOG_WARNING(TEXT("Projector already exists: %s (%s)"),
            *ProjectorInfo.Name, *ProjectorInfo.IPAddress);
        return false;
    }

    // 프로젝터 컴포넌트 추가
    ProjectorMap.Add(ProjectorID, ProjectorComponent);

    // 기본 그룹에 추가
    AddProjectorToGroup(ProjectorComponent, TEXT("Default"));

    // 상태 이벤트 연결
    ProjectorComponent->OnPowerStatusChanged.AddDynamic(this, &UPJLinkManagerComponent::HandlePowerStatusChanged);
    ProjectorComponent->OnInputSourceChanged.AddDynamic(this, &UPJLinkManagerComponent::HandleInputSourceChanged);
    ProjectorComponent->OnConnectionChanged.AddDynamic(this, &UPJLinkManagerComponent::HandleConnectionChanged);
    ProjectorComponent->OnErrorStatus.AddDynamic(this, &UPJLinkManagerComponent::HandleErrorStatus);

    // 상태 맵에 초기 상태 추가
    FPJLinkProjectorStatus NewStatus(ProjectorID, ProjectorInfo);
    ProjectorStatusMap.Add(ProjectorID, NewStatus);

    PJLINK_LOG_INFO(TEXT("Added projector: %s (%s)"),
        *ProjectorInfo.Name, *ProjectorInfo.IPAddress);

    return true;
}

UPJLinkComponent* UPJLinkManagerComponent::AddProjectorByInfo(const FPJLinkProjectorInfo& ProjectorInfo, const FString& GroupName)
{
    // 프로젝터 ID 생성
    FString ProjectorID = GenerateProjectorID(ProjectorInfo);

    // 이미 존재하는지 확인
    if (ProjectorMap.Contains(ProjectorID))
    {
        PJLINK_LOG_WARNING(TEXT("Projector already exists: %s (%s)"),
            *ProjectorInfo.Name, *ProjectorInfo.IPAddress);
        return ProjectorMap[ProjectorID];
    }

    // 새로운 UPJLinkComponent 생성
    UPJLinkComponent* NewComponent = NewObject<UPJLinkComponent>(GetOwner());
    if (!NewComponent)
    {
        PJLINK_LOG_ERROR(TEXT("Failed to create new PJLink component"));
        return nullptr;
    }

    // 컴포넌트 등록
    NewComponent->RegisterComponent();

    // 프로젝터 정보 설정
    NewComponent->SetProjectorInfo(ProjectorInfo);

    // 맵에 추가
    ProjectorMap.Add(ProjectorID, NewComponent);

    // 지정된 그룹에 추가
    if (!GroupName.IsEmpty())
    {
        // 그룹이 없으면 생성
        if (!GroupMap.Contains(GroupName))
        {
            CreateGroup(GroupName);
        }

        AddProjectorToGroup(NewComponent, GroupName);
    }

    PJLINK_LOG_INFO(TEXT("Created and added new projector: %s (%s)"),
        *ProjectorInfo.Name, *ProjectorInfo.IPAddress);

    return NewComponent;
}

bool UPJLinkManagerComponent::RemoveProjector(UPJLinkComponent* ProjectorComponent)
{
    if (!ProjectorComponent)
    {
        PJLINK_LOG_ERROR(TEXT("Failed to remove projector: Invalid component reference"));
        return false;
    }

    FPJLinkProjectorInfo ProjectorInfo = ProjectorComponent->GetProjectorInfo();
    FString ProjectorID = GenerateProjectorID(ProjectorInfo);

    return RemoveProjectorById(ProjectorID);
}

// AddProjector 함수에 상태 초기화 코드 추가
bool UPJLinkManagerComponent::AddProjector(UPJLinkComponent* ProjectorComponent)
{
    if (!ProjectorComponent)
    {
        PJLINK_LOG_ERROR(TEXT("Failed to add projector: Invalid component reference"));
        return false;
    }

    FPJLinkProjectorInfo ProjectorInfo = ProjectorComponent->GetProjectorInfo();
    FString ProjectorID = GenerateProjectorID(ProjectorInfo);

    if (ProjectorMap.Contains(ProjectorID))
    {
        PJLINK_LOG_WARNING(TEXT("Projector already exists: %s (%s)"),
            *ProjectorInfo.Name, *ProjectorInfo.IPAddress);
        return false;
    }

    // 프로젝터 컴포넌트 추가
    ProjectorMap.Add(ProjectorID, ProjectorComponent);

    // 기본 그룹에 추가
    AddProjectorToGroup(ProjectorComponent, TEXT("Default"));

    // 상태 이벤트 연결
    ProjectorComponent->OnPowerStatusChanged.AddDynamic(this, &UPJLinkManagerComponent::HandlePowerStatusChanged);
    ProjectorComponent->OnInputSourceChanged.AddDynamic(this, &UPJLinkManagerComponent::HandleInputSourceChanged);
    ProjectorComponent->OnConnectionChanged.AddDynamic(this, &UPJLinkManagerComponent::HandleConnectionChanged);
    ProjectorComponent->OnErrorStatus.AddDynamic(this, &UPJLinkManagerComponent::HandleErrorStatus);

    // 상태 맵에 초기 상태 추가
    FPJLinkProjectorStatus NewStatus(ProjectorID, ProjectorInfo);
    ProjectorStatusMap.Add(ProjectorID, NewStatus);

    PJLINK_LOG_INFO(TEXT("Added projector: %s (%s)"),
        *ProjectorInfo.Name, *ProjectorInfo.IPAddress);

    return true;
}

void UPJLinkManagerComponent::RemoveAllProjectors()
{
    // 모든 프로젝터 연결 해제
    DisconnectAll();

    // 모든 그룹 비우기
    for (auto& GroupPair : GroupMap)
    {
        GroupPair.Value.Empty();
    }

    // 프로젝터 맵 비우기
    ProjectorMap.Empty();

    PJLINK_LOG_INFO(TEXT("Removed all projectors"));
}

UPJLinkComponent* UPJLinkManagerComponent::GetProjectorById(const FString& ProjectorID) const
{
    if (!ProjectorMap.Contains(ProjectorID))
    {
        return nullptr;
    }

    return ProjectorMap[ProjectorID];
}

TArray<UPJLinkComponent*> UPJLinkManagerComponent::GetAllProjectors() const
{
    TArray<UPJLinkComponent*> Result;
    ProjectorMap.GenerateValueArray(Result);
    return Result;
}

int32 UPJLinkManagerComponent::GetProjectorCount() const
{
    return ProjectorMap.Num();
}

bool UPJLinkManagerComponent::CreateGroup(const FString& GroupName)
{
    return CreateGroupWithInfo(GroupName, TEXT(""), FLinearColor(0.0f, 0.4f, 0.7f, 1.0f));
}

// 새 함수를 추가합니다
bool UPJLinkManagerComponent::CreateGroupWithInfo(const FString& GroupName, const FString& Description, const FLinearColor& GroupColor)
{
    if (GroupName.IsEmpty())
    {
        PJLINK_LOG_ERROR(TEXT("Failed to create group: Empty group name"));
        return false;
    }

    if (GroupMap.Contains(GroupName))
    {
        PJLINK_LOG_WARNING(TEXT("Group already exists: %s"), *GroupName);
        return false;
    }

    // 새 그룹 생성
    FPJLinkProjectorGroup NewGroup(GroupName, Description, GroupColor);
    GroupMap.Add(GroupName, NewGroup);

    PJLINK_LOG_INFO(TEXT("Created group: %s"), *GroupName);

    // 이벤트 발생
    OnGroupCreated.Broadcast(GroupName);

    return true;
}

bool UPJLinkManagerComponent::DeleteGroup(const FString& GroupName)
{
    if (!GroupMap.Contains(GroupName))
    {
        PJLINK_LOG_WARNING(TEXT("Group not found: %s"), *GroupName);
        return false;
    }

    // Default 그룹은 삭제할 수 없음
    if (GroupName == TEXT("Default"))
    {
        PJLINK_LOG_WARNING(TEXT("Cannot delete Default group"));
        return false;
    }

    // 그룹 삭제
    GroupMap.Remove(GroupName);

    PJLINK_LOG_INFO(TEXT("Deleted group: %s"), *GroupName);

    // 이벤트 발생
    OnGroupDeleted.Broadcast(GroupName);

    return true;
}

bool UPJLinkManagerComponent::GetGroupInfo(const FString& GroupName, FPJLinkProjectorGroup& OutGroupInfo) const
{
    if (!GroupMap.Contains(GroupName))
    {
        PJLINK_LOG_WARNING(TEXT("Group not found: %s"), *GroupName);
        return false;
    }

    OutGroupInfo = GroupMap[GroupName];
    return true;
}

TArray<UPJLinkComponent*> UPJLinkManagerComponent::GetProjectorsInGroup(const FString& GroupName) const
{
    TArray<UPJLinkComponent*> Result;

    if (!GroupMap.Contains(GroupName))
    {
        PJLINK_LOG_WARNING(TEXT("Group not found: %s"), *GroupName);
        return Result;
    }

    // 그룹의 모든 프로젝터 컴포넌트 수집
    for (const FString& ProjectorID : GroupMap[GroupName])
    {
        if (ProjectorMap.Contains(ProjectorID))
        {
            Result.Add(ProjectorMap[ProjectorID]);
        }
    }

    return Result;
}

TArray<FString> UPJLinkManagerComponent::GetAllGroupNames() const
{
    TArray<FString> Result;
    GroupMap.GetKeys(Result);
    return Result;
}

TArray<FPJLinkProjectorGroup> UPJLinkManagerComponent::GetAllGroupInfos() const
{
    TArray<FPJLinkProjectorGroup> Result;
    GroupMap.GenerateValueArray(Result);
    return Result;
}

FString UPJLinkManagerComponent::ConnectAll()
{
    TArray<UPJLinkComponent*> AllProjectors = GetAllProjectors();
    return StartGroupCommand(TEXT("AllGroups"), EPJLinkCommand::POWR, TEXT("CONNECT"), AllProjectors);
}

FString UPJLinkManagerComponent::ConnectGroup(const FString& GroupName)
{
    TArray<UPJLinkComponent*> GroupProjectors = GetProjectorsInGroup(GroupName);
    return StartGroupCommand(GroupName, EPJLinkCommand::POWR, TEXT("CONNECT"), GroupProjectors);
}

void UPJLinkManagerComponent::DisconnectAll()
{
    TArray<UPJLinkComponent*> AllProjectors = GetAllProjectors();

    for (UPJLinkComponent* Projector : AllProjectors)
    {
        if (Projector)
        {
            Projector->Disconnect();
        }
    }

    PJLINK_LOG_INFO(TEXT("Disconnected all projectors (%d)"), AllProjectors.Num());
}

void UPJLinkManagerComponent::DisconnectGroup(const FString& GroupName)
{
    TArray<UPJLinkComponent*> GroupProjectors = GetProjectorsInGroup(GroupName);

    for (UPJLinkComponent* Projector : GroupProjectors)
    {
        if (Projector)
        {
            Projector->Disconnect();
        }
    }

    PJLINK_LOG_INFO(TEXT("Disconnected all projectors in group '%s' (%d)"),
        *GroupName, GroupProjectors.Num());
}

FString UPJLinkManagerComponent::PowerOnAll()
{
    TArray<UPJLinkComponent*> AllProjectors = GetAllProjectors();
    return StartGroupCommand(TEXT("AllGroups"), EPJLinkCommand::POWR, TEXT("1"), AllProjectors);
}

FString UPJLinkManagerComponent::PowerOnGroup(const FString& GroupName)
{
    TArray<UPJLinkComponent*> GroupProjectors = GetProjectorsInGroup(GroupName);
    return StartGroupCommand(GroupName, EPJLinkCommand::POWR, TEXT("1"), GroupProjectors);
}

FString UPJLinkManagerComponent::PowerOffAll()
{
    TArray<UPJLinkComponent*> AllProjectors = GetAllProjectors();
    return StartGroupCommand(TEXT("AllGroups"), EPJLinkCommand::POWR, TEXT("0"), AllProjectors);
}

FString UPJLinkManagerComponent::PowerOffGroup(const FString& GroupName)
{
    TArray<UPJLinkComponent*> GroupProjectors = GetProjectorsInGroup(GroupName);
    return StartGroupCommand(GroupName, EPJLinkCommand::POWR, TEXT("0"), GroupProjectors);
}

FString UPJLinkManagerComponent::SwitchInputSourceAll(EPJLinkInputSource InputSource)
{
    TArray<UPJLinkComponent*> AllProjectors = GetAllProjectors();
    FString Parameter = FString::FromInt((int32)InputSource);
    return StartGroupCommand(TEXT("AllGroups"), EPJLinkCommand::INPT, Parameter, AllProjectors);
}


FString UPJLinkManagerComponent::SwitchInputSourceGroup(const FString& GroupName, EPJLinkInputSource InputSource)
{
    TArray<UPJLinkComponent*> GroupProjectors = GetProjectorsInGroup(GroupName);
    FString Parameter = FString::FromInt((int32)InputSource);
    return StartGroupCommand(GroupName, EPJLinkCommand::INPT, Parameter, GroupProjectors);
}

FString UPJLinkManagerComponent::RequestStatusAll()
{
    TArray<UPJLinkComponent*> AllProjectors = GetAllProjectors();
    return StartGroupCommand(TEXT("AllGroups"), EPJLinkCommand::POWR, TEXT("?"), AllProjectors);
}

FString UPJLinkManagerComponent::RequestStatusGroup(const FString& GroupName)
{
    TArray<UPJLinkComponent*> GroupProjectors = GetProjectorsInGroup(GroupName);
    return StartGroupCommand(GroupName, EPJLinkCommand::POWR, TEXT("?"), GroupProjectors);
}

bool UPJLinkManagerComponent::GetGroupCommandResult(const FString& CommandID, FPJLinkGroupCommandResult& OutResult) const
{
    if (!CommandResults.Contains(CommandID))
    {
        return false;
    }

    OutResult = CommandResults[CommandID];
    return true;
}

bool UPJLinkManagerComponent::GetLatestGroupCommandResult(FPJLinkGroupCommandResult& OutResult) const
{
    return GetGroupCommandResult(LatestCommandID, OutResult);
}

bool UPJLinkManagerComponent::IsCommandCompleted(const FString& CommandID) const
{
    if (!CommandResults.Contains(CommandID))
    {
        return false;
    }

    return CommandResults[CommandID].HasAllResponded();
}

void UPJLinkManagerComponent::SetCommandTimeout(float TimeoutSeconds)
{
    CommandTimeoutSeconds = FMath::Max(1.0f, TimeoutSeconds);
}

float UPJLinkManagerComponent::GetCommandTimeout() const
{
    return CommandTimeoutSeconds;
}

FString UPJLinkManagerComponent::GenerateCommandID() const
{
    return FGuid::NewGuid().ToString();
}

FString UPJLinkManagerComponent::StartGroupCommand(const FString& GroupName, EPJLinkCommand Command, const FString& Parameter, const TArray<UPJLinkComponent*>& Projectors)
{
    // 명령 ID 생성
    FString CommandID = GenerateCommandID();
    LatestCommandID = CommandID;

    // 명령 결과 객체 생성
    FPJLinkGroupCommandResult CommandResult(CommandID, GroupName, Command, Parameter, Projectors.Num());

    // 결과 맵에 추가
    CommandResults.Add(CommandID, CommandResult);

    // 프로젝터 ID 목록 준비 (타임아웃 처리용)
    TArray<FString> ProjectorIDs;

    int32 SuccessCount = 0;

    // 각 프로젝터에 명령 전송
    for (UPJLinkComponent* Projector : Projectors)
    {
        if (Projector)
        {
            FPJLinkProjectorInfo ProjectorInfo = Projector->GetProjectorInfo();
            FString ProjectorID = GenerateProjectorID(ProjectorInfo);
            ProjectorIDs.Add(ProjectorID);

            // 특별한 CONNECT 명령 처리
            if (Parameter == TEXT("CONNECT"))
            {
                if (Projector->Connect())
                {
                    CommandResult.AddSuccess(ProjectorID);
                    SuccessCount++;
                }
                else
                {
                    CommandResult.AddFailure(ProjectorID);
                }
            }
            else if (ExecuteCommandOnProjector(Projector, Command, Parameter, CommandID))
            {
                SuccessCount++;
            }
            else
            {
                CommandResult.AddFailure(ProjectorID);
            }
        }
    }

    // 타임아웃 타이머 설정
    if (SuccessCount > 0 && !CommandResult.HasAllResponded())
    {
        if (UWorld* World = GetWorld())
        {
            FTimerDelegate TimerDelegate;
            TimerDelegate.BindUObject(this, &UPJLinkManagerComponent::HandleCommandTimeout, CommandID);

            FTimerHandle TimerHandle;
            World->GetTimerManager().SetTimer(TimerHandle, TimerDelegate, CommandTimeoutSeconds, false);
            CommandTimeoutHandles.Add(CommandID, TimerHandle);
        }
    }
    else
    {
        // 모든 명령이 즉시 성공 또는 실패한 경우
        OnGroupCommandCompleted.Broadcast(CommandResult);
    }

    PJLINK_LOG_INFO(TEXT("Started group command: %s - Group: %s, Command: %s, Parameter: %s, Success: %d/%d"),
        *CommandID, *GroupName, *PJLinkHelpers::CommandToString(Command), *Parameter, SuccessCount, Projectors.Num());

    return CommandID;
}

bool UPJLinkManagerComponent::ExecuteCommandOnProjector(UPJLinkComponent* ProjectorComponent, EPJLinkCommand Command, const FString& Parameter, const FString& CommandID)
{
    if (!ProjectorComponent)
    {
        return false;
    }

    // 프로젝터가 연결되어 있는지 확인
    if (!ProjectorComponent->IsConnected())
    {
        PJLINK_LOG_VERBOSE(TEXT("Projector not connected: %s"), *ProjectorComponent->GetProjectorInfo().Name);
        return false;
    }

    // 명령 실행
    bool bSuccess = false;

    // 명령에 따라 적절한 함수 호출
    switch (Command)
    {
    case EPJLinkCommand::POWR:
        if (Parameter == TEXT("1"))
        {
            bSuccess = ProjectorComponent->PowerOn();
        }
        else if (Parameter == TEXT("0"))
        {
            bSuccess = ProjectorComponent->PowerOff();
        }
        else if (Parameter == TEXT("?"))
        {
            bSuccess = ProjectorComponent->RequestStatus();
        }
        break;

    case EPJLinkCommand::INPT:
    {
        int32 SourceIndex = FCString::Atoi(*Parameter);
        EPJLinkInputSource InputSource = static_cast<EPJLinkInputSource>(SourceIndex);
        bSuccess = ProjectorComponent->SwitchInputSource(InputSource);
    }
    break;

    default:
        // 기타 명령 추가 가능
        PJLINK_LOG_WARNING(TEXT("Unsupported command: %s"), *PJLinkHelpers::CommandToString(Command));
        break;
    }

    if (bSuccess)
    {
        // 명령 완료 핸들러 연결
        ProjectorComponent->OnCommandCompleted.AddDynamic(this, &UPJLinkManagerComponent::HandleCommandCompleted);
    }

    return bSuccess;
}

void UPJLinkManagerComponent::HandleCommandCompleted(UPJLinkComponent* ProjectorComponent, EPJLinkCommand Command, bool bSuccess)
{
    if (!ProjectorComponent)
    {
        return;
    }

    // 프로젝터 ID 가져오기
    FPJLinkProjectorInfo ProjectorInfo = ProjectorComponent->GetProjectorInfo();
    FString ProjectorID = GenerateProjectorID(ProjectorInfo);

    // 관련 명령 ID 찾기
    FString MatchingCommandID;

    for (const auto& Pair : CommandResults)
    {
        if (Pair.Value.Command == Command && !Pair.Value.HasAllResponded())
        {
            if (!Pair.Value.ProjectorResults.Contains(ProjectorID))
            {
                MatchingCommandID = Pair.Key;
                break;
            }
        }
    }

    if (MatchingCommandID.IsEmpty())
    {
        // 명령 핸들러 연결 해제
        ProjectorComponent->OnCommandCompleted.RemoveDynamic(this, &UPJLinkManagerComponent::HandleCommandCompleted);
        return;
    }

    // 결과 업데이트
    FPJLinkGroupCommandResult& Result = CommandResults[MatchingCommandID];

    if (bSuccess)
    {
        Result.AddSuccess(ProjectorID);
        PJLINK_LOG_VERBOSE(TEXT("Command succeeded on projector: %s - Command: %s"),
            *ProjectorInfo.Name, *PJLinkHelpers::CommandToString(Command));
    }
    else
    {
        Result.AddFailure(ProjectorID);
        PJLINK_LOG_WARNING(TEXT("Command failed on projector: %s - Command: %s"),
            *ProjectorInfo.Name, *PJLinkHelpers::CommandToString(Command));
    }

    // 명령 핸들러 연결 해제
    ProjectorComponent->OnCommandCompleted.RemoveDynamic(this, &UPJLinkManagerComponent::HandleCommandCompleted);

    // 모든 프로젝터가 응답했는지 확인
    if (Result.HasAllResponded())
    {
        // 타이머 정리
        if (UWorld* World = GetWorld())
        {
            if (CommandTimeoutHandles.Contains(MatchingCommandID))
            {
                World->GetTimerManager().ClearTimer(CommandTimeoutHandles[MatchingCommandID]);
                CommandTimeoutHandles.Remove(MatchingCommandID);
            }
        }

        // 명령 완료 이벤트 발생
        OnGroupCommandCompleted.Broadcast(Result);

        PJLINK_LOG_INFO(TEXT("Group command completed: %s - Success: %d/%d, Time: %.2f seconds"),
            *MatchingCommandID, Result.SuccessCount, Result.TotalCount, Result.GetElapsedTimeSeconds());
    }
}

void UPJLinkManagerComponent::HandleCommandTimeout(const FString& CommandID)
{
    if (!CommandResults.Contains(CommandID))
    {
        return;
    }

    FPJLinkGroupCommandResult& Result = CommandResults[CommandID];

    // 타임아웃 핸들 제거
    CommandTimeoutHandles.Remove(CommandID);

    // 아직 응답 없는 프로젝터들을 타임아웃으로 처리
    TArray<FString> NoResponseProjectorIDs;
    for (const auto& Pair : ProjectorMap)
    {
        const FString& ProjectorID = Pair.Key;
        if (!Result.ProjectorResults.Contains(ProjectorID))
        {
            NoResponseProjectorIDs.Add(ProjectorID);
        }
    }

    Result.HandleTimeout(NoResponseProjectorIDs);

    // 명령 완료 이벤트 발생
    OnGroupCommandCompleted.Broadcast(Result);

    PJLINK_LOG_WARNING(TEXT("Command timed out: %s - Success: %d/%d, No Response: %d/%d"),
        *CommandID, Result.SuccessCount, Result.TotalCount, Result.NoResponseCount, Result.TotalCount);

    // 명령 핸들러 연결 해제
    for (UPJLinkComponent* Projector : GetAllProjectors())
    {
        if (Projector)
        {
            Projector->OnCommandCompleted.RemoveDynamic(this, &UPJLinkManagerComponent::HandleCommandCompleted);
        }
    }
}

void UPJLinkManagerComponent::CleanupCommand(const FString& CommandID)
{
    // 타이머 정리
    if (UWorld* World = GetWorld())
    {
        if (CommandTimeoutHandles.Contains(CommandID))
        {
            World->GetTimerManager().ClearTimer(CommandTimeoutHandles[CommandID]);
            CommandTimeoutHandles.Remove(CommandID);
        }
    }

    // 최근 명령 결과는 유지하되 오래된 결과는 제거
    if (CommandResults.Num() > 10 && CommandID != LatestCommandID)
    {
        CommandResults.Remove(CommandID);
    }

    // 명령 핸들러 연결 해제
    for (UPJLinkComponent* Projector : GetAllProjectors())
    {
        if (Projector)
        {
            Projector->OnCommandCompleted.RemoveDynamic(this, &UPJLinkManagerComponent::HandleCommandCompleted);
        }
    }
}

void UPJLinkManagerComponent::HandleCommandCompleted(UPJLinkComponent* ProjectorComponent, EPJLinkCommand Command, bool bSuccess,
    const FString& GroupName, int32& SuccessCount, int32& TotalCount)
{
    if (!ProjectorComponent)
    {
        return;
    }

    // 명령 ID 생성
    FString CommandID = GenerateCommandID(GroupName, Command);

    // 진행 중인 명령 정보 확인
    if (!PendingCommands.Contains(CommandID))
    {
        return;
    }

    FPendingGroupCommand& PendingCommand = PendingCommands[CommandID];

    // 성공 여부에 따라 카운트 증가
    if (bSuccess)
    {
        PendingCommand.SuccessCount++;
    }

    // 응답 카운트 증가
    PendingCommand.ResponseCount++;

    // 모든 프로젝터가 응답했는지 확인
    if (PendingCommand.ResponseCount >= PendingCommand.TotalCount)
    {
        // 명령 완료 이벤트 발생
        OnGroupCommandCompleted.Broadcast(
            PendingCommand.GroupName,
            PendingCommand.Command,
            PendingCommand.SuccessCount);

        // 명령 핸들러 연결 해제
        for (UPJLinkComponent* Projector : GetProjectorsInGroup(PendingCommand.GroupName))
        {
            if (Projector)
            {
                Projector->OnCommandCompleted.RemoveDynamic(this, &UPJLinkManagerComponent::HandleCommandCompleted);
            }
        }

        // 추적 맵에서 제거
        PendingCommands.Remove(CommandID);

        PJLINK_LOG_INFO(TEXT("Group command completed: %s - Success: %d/%d"),
            *PendingCommand.GroupName, PendingCommand.SuccessCount, PendingCommand.TotalCount);
    }
}

FString UPJLinkManagerComponent::GenerateProjectorID(const FPJLinkProjectorInfo& ProjectorInfo) const
{
    // IP 주소와 포트를 사용하여 고유한 ID 생성
    return FString::Printf(TEXT("%s:%d"), *ProjectorInfo.IPAddress, ProjectorInfo.Port);
}

FString UPJLinkManagerComponent::GenerateCommandID(const FString& GroupName, EPJLinkCommand Command) const
{
    // 그룹 이름과 명령을 조합하여 고유한 ID 생성
    return FString::Printf(TEXT("%s_%d_%s"), *GroupName, static_cast<int32>(Command), *FGuid::NewGuid().ToString());
}

bool UPJLinkManagerComponent::GetGroupInfo(const FString& GroupName, FPJLinkProjectorGroup& OutGroupInfo) const
{
    if (!GroupMap.Contains(GroupName))
    {
        PJLINK_LOG_WARNING(TEXT("Group not found: %s"), *GroupName);
        return false;
    }

    OutGroupInfo = GroupMap[GroupName];
    return true;
}

bool UPJLinkManagerComponent::RemoveProjectorFromAllGroups(UPJLinkComponent* ProjectorComponent)
{
    if (!ProjectorComponent)
    {
        PJLINK_LOG_ERROR(TEXT("Failed to remove projector from all groups: Invalid component reference"));
        return false;
    }

    FPJLinkProjectorInfo ProjectorInfo = ProjectorComponent->GetProjectorInfo();
    FString ProjectorID = GenerateProjectorID(ProjectorInfo);

    bool bRemovedFromAny = false;

    // 모든 그룹에서 프로젝터 제거
    for (auto& GroupPair : GroupMap)
    {
        FString GroupName = GroupPair.Key;
        if (GroupPair.Value.RemoveProjectorID(ProjectorID))
        {
            bRemovedFromAny = true;

            PJLINK_LOG_INFO(TEXT("Removed projector from group: %s - %s"),
                *ProjectorInfo.Name, *GroupName);

            // 각 그룹에 대해 이벤트 발생
            OnProjectorGroupAssignmentChanged.Broadcast(ProjectorID, TEXT(""));
        }
    }

    if (!bRemovedFromAny)
    {
        PJLINK_LOG_VERBOSE(TEXT("Projector not in any group: %s"), *ProjectorInfo.Name);
    }

    return bRemovedFromAny;
}

TArray<FPJLinkProjectorGroup> UPJLinkManagerComponent::GetAllGroupInfos() const
{
    TArray<FPJLinkProjectorGroup> Result;
    GroupMap.GenerateValueArray(Result);
    return Result;
}

bool UPJLinkManagerComponent::DoesGroupExist(const FString& GroupName) const
{
    return GroupMap.Contains(GroupName);
}

bool UPJLinkManagerComponent::IsProjectorInGroup(UPJLinkComponent* ProjectorComponent, const FString& GroupName) const
{
    if (!ProjectorComponent || !GroupMap.Contains(GroupName))
    {
        return false;
    }

    FPJLinkProjectorInfo ProjectorInfo = ProjectorComponent->GetProjectorInfo();
    FString ProjectorID = GenerateProjectorID(ProjectorInfo);

    return GroupMap[GroupName].ContainsProjectorID(ProjectorID);
}

// 생성자에 초기화 코드 추가
UPJLinkManagerComponent::UPJLinkManagerComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.1f; // 10Hz 틱
    bAutoActivate = true;

    // 명령 타임아웃 기본값 설정
    CommandTimeoutSeconds = 10.0f;

    // 상태 업데이트 설정
    StatusUpdateInterval = 30.0f; // 기본 30초마다 업데이트
    bStatusUpdateEnabled = true;
}

// 다음 함수들을 파일 마지막에 추가합니다

bool UPJLinkManagerComponent::GetProjectorStatus(const FString& ProjectorID, FPJLinkProjectorStatus& OutStatus) const
{
    if (!ProjectorStatusMap.Contains(ProjectorID))
    {
        return false;
    }

    OutStatus = ProjectorStatusMap[ProjectorID];
    return true;
}

bool UPJLinkManagerComponent::GetProjectorStatusByComponent(UPJLinkComponent* ProjectorComponent, FPJLinkProjectorStatus& OutStatus) const
{
    if (!ProjectorComponent)
    {
        return false;
    }

    FPJLinkProjectorInfo ProjectorInfo = ProjectorComponent->GetProjectorInfo();
    FString ProjectorID = GenerateProjectorID(ProjectorInfo);

    return GetProjectorStatus(ProjectorID, OutStatus);
}

TArray<FPJLinkProjectorStatus> UPJLinkManagerComponent::GetAllProjectorStatuses() const
{
    TArray<FPJLinkProjectorStatus> Result;
    ProjectorStatusMap.GenerateValueArray(Result);
    return Result;
}

TArray<FPJLinkProjectorStatus> UPJLinkManagerComponent::GetGroupProjectorStatuses(const FString& GroupName) const
{
    TArray<FPJLinkProjectorStatus> Result;

    if (!GroupMap.Contains(GroupName))
    {
        return Result;
    }

    const TArray<FString>& GroupProjectorIDs = GroupMap[GroupName].ProjectorIDs;

    for (const FString& ProjectorID : GroupProjectorIDs)
    {
        if (ProjectorStatusMap.Contains(ProjectorID))
        {
            Result.Add(ProjectorStatusMap[ProjectorID]);
        }
    }

    return Result;
}

TArray<FPJLinkProjectorStatus> UPJLinkManagerComponent::GetUnhealthyProjectorStatuses() const
{
    TArray<FPJLinkProjectorStatus> Result;

    for (const auto& Pair : ProjectorStatusMap)
    {
        if (!Pair.Value.bIsHealthy)
        {
            Result.Add(Pair.Value);
        }
    }

    return Result;
}

void UPJLinkManagerComponent::SetStatusUpdateInterval(float IntervalSeconds)
{
    StatusUpdateInterval = FMath::Max(1.0f, IntervalSeconds);

    // 타이머 갱신
    if (bStatusUpdateEnabled&& UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(StatusUpdateTimerHandle);

        if (StatusUpdateInterval > 0.0f)
        {
            World->GetTimerManager().SetTimer(
                StatusUpdateTimerHandle,
                this,
                &UPJLinkManagerComponent::PeriodicStatusUpdate,
                StatusUpdateInterval,
                true);
        }
    }

    PJLINK_LOG_INFO(TEXT("Status update interval set to %.1f seconds"), StatusUpdateInterval);
}

void UPJLinkManagerComponent::SetStatusUpdateEnabled(bool bEnabled)
{
    bStatusUpdateEnabled = bEnabled;

    if (UWorld* World = GetWorld())
    {
        if (bStatusUpdateEnabled && StatusUpdateInterval > 0.0f)
        {
            // 활성화되면 타이머 시작
            World->GetTimerManager().SetTimer(
                StatusUpdateTimerHandle,
                this,
                &UPJLinkManagerComponent::PeriodicStatusUpdate,
                StatusUpdateInterval,
                true);

            PJLINK_LOG_INFO(TEXT("Status updates enabled"));
        }
        else
        {
            // 비활성화되면 타이머 중지
            World->GetTimerManager().ClearTimer(StatusUpdateTimerHandle);

            PJLINK_LOG_INFO(TEXT("Status updates disabled"));
        }
    }
}

void UPJLinkManagerComponent::UpdateAllProjectorStatuses()
{
    // 상태 요청 명령 전송
    RequestStatusAll();

    PJLINK_LOG_INFO(TEXT("Updating all projector statuses"));
}

void UPJLinkManagerComponent::UpdateGroupProjectorStatuses(const FString& GroupName)
{
    // 그룹 상태 요청 명령 전송
    RequestStatusGroup(GroupName);

    PJLINK_LOG_INFO(TEXT("Updating projector statuses in group: %s"), *GroupName);
}

void UPJLinkManagerComponent::UpdateProjectorStatus(const FString& ProjectorID)
{
    if (!ProjectorMap.Contains(ProjectorID))
    {
        PJLINK_LOG_WARNING(TEXT("Projector not found: %s"), *ProjectorID);
        return;
    }

    UPJLinkComponent* ProjectorComponent = ProjectorMap[ProjectorID];
    if (ProjectorComponent && ProjectorComponent->IsConnected())
    {
        ProjectorComponent->RequestStatus();

        // 명령 전송 기록
        if (ProjectorStatusMap.Contains(ProjectorID))
        {
            ProjectorStatusMap[ProjectorID].RecordCommandSent();
        }

        PJLINK_LOG_INFO(TEXT("Updating projector status: %s"), *ProjectorID);
    }
    else
    {
        PJLINK_LOG_WARNING(TEXT("Cannot update status - projector not connected: %s"), *ProjectorID);

        // 연결 실패 기록
        if (ProjectorStatusMap.Contains(ProjectorID))
        {
            ProjectorStatusMap[ProjectorID].RecordConnectionFailure();

            // 상태 변경 이벤트 발생
            OnProjectorStatusChanged.Broadcast(ProjectorStatusMap[ProjectorID]);
        }
    }
}

void UPJLinkManagerComponent::PeriodicStatusUpdate()
{
    // 모든 프로젝터 상태 업데이트
    TArray<FString> ProjectorIDs;
    ProjectorMap.GetKeys(ProjectorIDs);

    for (const FString& ProjectorID : ProjectorIDs)
    {
        UpdateProjectorStatus(ProjectorID);
    }
}

void UPJLinkManagerComponent::HandlePowerStatusChanged(UPJLinkComponent* ProjectorComponent, EPJLinkPowerStatus OldStatus, EPJLinkPowerStatus NewStatus)
{
    if (!ProjectorComponent)
    {
        return;
    }

    FPJLinkProjectorInfo ProjectorInfo = ProjectorComponent->GetProjectorInfo();
    FString ProjectorID = GenerateProjectorID(ProjectorInfo);

    if (ProjectorStatusMap.Contains(ProjectorID))
    {
        FPJLinkProjectorStatus& Status = ProjectorStatusMap[ProjectorID];

        // 상태 업데이트
        Status.UpdateStatus(
            NewStatus,
            ProjectorComponent->GetInputSource(),
            ProjectorComponent->IsConnected()
        );

        // 상태 변경 이벤트 발생
        OnProjectorStatusChanged.Broadcast(Status);

        PJLINK_LOG_VERBOSE(TEXT("Power status changed for projector %s: %s -> %s"),
            *ProjectorInfo.Name,
            *PJLinkHelpers::PowerStatusToString(OldStatus),
            *PJLinkHelpers::PowerStatusToString(NewStatus));
    }
}

void UPJLinkManagerComponent::HandleInputSourceChanged(UPJLinkComponent* ProjectorComponent, EPJLinkInputSource OldSource, EPJLinkInputSource NewSource)
{
    if (!ProjectorComponent)
    {
        return;
    }

    FPJLinkProjectorInfo ProjectorInfo = ProjectorComponent->GetProjectorInfo();
    FString ProjectorID = GenerateProjectorID(ProjectorInfo);

    if (ProjectorStatusMap.Contains(ProjectorID))
    {
        FPJLinkProjectorStatus& Status = ProjectorStatusMap[ProjectorID];

        // 상태 업데이트
        Status.UpdateStatus(
            ProjectorComponent->GetPowerStatus(),
            NewSource,
            ProjectorComponent->IsConnected()
        );

        // 상태 변경 이벤트 발생
        OnProjectorStatusChanged.Broadcast(Status);

        PJLINK_LOG_VERBOSE(TEXT("Input source changed for projector %s: %s -> %s"),
            *ProjectorInfo.Name,
            *PJLinkHelpers::InputSourceToString(OldSource),
            *PJLinkHelpers::InputSourceToString(NewSource));
    }
}

void UPJLinkManagerComponent::HandleConnectionChanged(UPJLinkComponent* ProjectorComponent, bool bIsConnected)
{
    if (!ProjectorComponent)
    {
        return;
    }

    FPJLinkProjectorInfo ProjectorInfo = ProjectorComponent->GetProjectorInfo();
    FString ProjectorID = GenerateProjectorID(ProjectorInfo);

    if (ProjectorStatusMap.Contains(ProjectorID))
    {
        FPJLinkProjectorStatus& Status = ProjectorStatusMap[ProjectorID];

        // 연결 실패 시 카운터 증가
        if (!bIsConnected)
        {
            Status.RecordConnectionFailure();
        }
        else
        {
            // 연결 성공 시 카운터 리셋
            Status.ResetCounters();
        }

        // 상태 업데이트
        Status.UpdateStatus(
            ProjectorComponent->GetPowerStatus(),
            ProjectorComponent->GetInputSource(),
            bIsConnected
        );

        // 상태 변경 이벤트 발생
        OnProjectorStatusChanged.Broadcast(Status);

        PJLINK_LOG_INFO(TEXT("Connection status changed for projector %s: %s"),
            *ProjectorInfo.Name,
            bIsConnected ? TEXT("Connected") : TEXT("Disconnected"));
    }
}

void UPJLinkManagerComponent::HandleErrorStatus(UPJLinkComponent* ProjectorComponent, const FString& ErrorMessage)
{
    if (!ProjectorComponent)
    {
        return;
    }

    FPJLinkProjectorInfo ProjectorInfo = ProjectorComponent->GetProjectorInfo();
    FString ProjectorID = GenerateProjectorID(ProjectorInfo);

    if (ProjectorStatusMap.Contains(ProjectorID))
    {
        FPJLinkProjectorStatus& Status = ProjectorStatusMap[ProjectorID];

        // 명령 실패 기록
        Status.RecordCommandFailure(ErrorMessage);

        // 상태 업데이트
        Status.UpdateStatus(
            ProjectorComponent->GetPowerStatus(),
            ProjectorComponent->GetInputSource(),
            ProjectorComponent->IsConnected(),
            ErrorMessage
        );

        // 상태 변경 이벤트 발생
        OnProjectorStatusChanged.Broadcast(Status);

        PJLINK_LOG_WARNING(TEXT("Error status for projector %s: %s"),
            *ProjectorInfo.Name,
            *ErrorMessage);
    }
}


bool UPJLinkManagerComponent::SaveGroupPreset(const FString& PresetName, const FString& GroupName, const FString& Description)
{
    if (PresetName.IsEmpty())
    {
        PJLINK_LOG_ERROR(TEXT("Cannot save preset with empty name"));
        return false;
    }

    if (!GroupMap.Contains(GroupName))
    {
        PJLINK_LOG_WARNING(TEXT("Group not found: %s"), *GroupName);
        return false;
    }

    // 프리셋 생성 또는 업데이트
    FPJLinkGroupPreset Preset;

    if (GroupPresetMap.Contains(PresetName))
    {
        // 기존 프리셋 업데이트
        Preset = GroupPresetMap[PresetName];
        Preset.LastModifiedTime = FDateTime::Now();
    }
    else
    {
        // 새 프리셋 생성
        Preset.PresetName = PresetName;
        Preset.CreationTime = FDateTime::Now();
        Preset.LastModifiedTime = FDateTime::Now();
    }

    // 그룹 정보 설정
    Preset.GroupName = GroupName;
    Preset.Description = Description;

    // 프로젝터 설정 가져오기
    const FPJLinkProjectorGroup& Group = GroupMap[GroupName];
    Preset.ProjectorSettings.Empty();

    for (const FString& ProjectorID : Group.ProjectorIDs)
    {
        if (ProjectorMap.Contains(ProjectorID))
        {
            UPJLinkComponent* ProjectorComponent = ProjectorMap[ProjectorID];
            if (ProjectorComponent)
            {
                FPJLinkProjectorInfo ProjectorInfo = ProjectorComponent->GetProjectorInfo();
                Preset.AddProjectorSetting(ProjectorID, ProjectorInfo);

                // 현재 상태 기록
                if (ProjectorComponent->IsConnected())
                {
                    Preset.PowerStatus = ProjectorComponent->GetPowerStatus();
                    Preset.InputSource = ProjectorComponent->GetInputSource();
                }
            }
        }
    }

    // 프리셋 맵에 저장
    GroupPresetMap.Add(PresetName, Preset);

    // 프리셋 파일 저장
    SaveGroupPresetsToFile();

    // 이벤트 발생
    if (GroupPresetMap.Contains(PresetName))
    {
        OnGroupPresetCreated.Broadcast(PresetName);
    }
    else
    {
        OnGroupPresetModified.Broadcast(PresetName);
    }

    PJLINK_LOG_INFO(TEXT("Saved group preset: %s (Group: %s, Projectors: %d)"),
        *PresetName, *GroupName, Preset.GetProjectorCount());

    return true;
}

bool UPJLinkManagerComponent::LoadGroupPreset(const FString& PresetName)
{
    if (!GroupPresetMap.Contains(PresetName))
    {
        PJLINK_LOG_WARNING(TEXT("Preset not found: %s"), *PresetName);
        return false;
    }

    const FPJLinkGroupPreset& Preset = GroupPresetMap[PresetName];

    // 그룹이 없으면 생성
    if (!GroupMap.Contains(Preset.GroupName))
    {
        CreateGroup(Preset.GroupName);
    }

    // 프리셋의 각 프로젝터 설정 적용
    for (const auto& Pair : Preset.ProjectorSettings)
    {
        const FString& ProjectorID = Pair.Key;
        const FPJLinkProjectorInfo& ProjectorInfo = Pair.Value;

        // 이미 존재하는 프로젝터인지 확인
        if (ProjectorMap.Contains(ProjectorID))
        {
            UPJLinkComponent* ProjectorComponent = ProjectorMap[ProjectorID];
            if (ProjectorComponent)
            {
                // 프로젝터 정보 업데이트
                ProjectorComponent->SetProjectorInfo(ProjectorInfo);

                // 그룹에 추가 (이미 있어도 상관없음)
                AddProjectorToGroup(ProjectorComponent, Preset.GroupName);

                // 연결되어 있으면 설정 적용
                if (ProjectorComponent->IsConnected())
                {
                    // 전원 상태 설정
                    if (Preset.PowerStatus == EPJLinkPowerStatus::PoweredOn)
                    {
                        ProjectorComponent->PowerOn();
                    }
                    else if (Preset.PowerStatus == EPJLinkPowerStatus::PoweredOff)
                    {
                        ProjectorComponent->PowerOff();
                    }

                    // 입력 소스 설정
                    ProjectorComponent->SwitchInputSource(Preset.InputSource);
                }
            }
        }
        else
        {
            // 새 프로젝터 추가
            UPJLinkComponent* NewComponent = AddProjectorByInfo(ProjectorInfo, Preset.GroupName);
            if (NewComponent)
            {
                // 연결 시도
                NewComponent->Connect();
            }
        }
    }

    // 이벤트 발생
    OnGroupPresetLoaded.Broadcast(PresetName);

    PJLINK_LOG_INFO(TEXT("Loaded group preset: %s (Group: %s, Projectors: %d)"),
        *PresetName, *Preset.GroupName, Preset.GetProjectorCount());

    return true;
}

bool UPJLinkManagerComponent::DeleteGroupPreset(const FString& PresetName)
{
    if (!GroupPresetMap.Contains(PresetName))
    {
        PJLINK_LOG_WARNING(TEXT("Preset not found: %s"), *PresetName);
        return false;
    }

    // 프리셋 삭제
    GroupPresetMap.Remove(PresetName);

    // 파일 저장
    SaveGroupPresetsToFile();

    // 이벤트 발생
    OnGroupPresetDeleted.Broadcast(PresetName);

    PJLINK_LOG_INFO(TEXT("Deleted group preset: %s"), *PresetName);

    return true;
}

bool UPJLinkManagerComponent::GetGroupPresetInfo(const FString& PresetName, FPJLinkGroupPreset& OutPreset) const
{
    if (!GroupPresetMap.Contains(PresetName))
    {
        return false;
    }

    OutPreset = GroupPresetMap[PresetName];
    return true;
}

bool UPJLinkManagerComponent::RenameGroupPreset(const FString& OldPresetName, const FString& NewPresetName)
{
    if (!GroupPresetMap.Contains(OldPresetName))
    {
        PJLINK_LOG_WARNING(TEXT("Preset not found: %s"), *OldPresetName);
        return false;
    }

    if (NewPresetName.IsEmpty())
    {
        PJLINK_LOG_ERROR(TEXT("New preset name cannot be empty"));
        return false;
    }

    if (GroupPresetMap.Contains(NewPresetName))
    {
        PJLINK_LOG_WARNING(TEXT("Preset with name %s already exists"), *NewPresetName);
        return false;
    }

    // 프리셋 복사 및 이름 변경
    FPJLinkGroupPreset Preset = GroupPresetMap[OldPresetName];
    Preset.PresetName = NewPresetName;
    Preset.LastModifiedTime = FDateTime::Now();

    // 새 이름으로 추가
    GroupPresetMap.Add(NewPresetName, Preset);

    // 기존 프리셋 삭제
    GroupPresetMap.Remove(OldPresetName);

    // 파일 저장
    SaveGroupPresetsToFile();

    // 이벤트 발생
    OnGroupPresetModified.Broadcast(NewPresetName);

    PJLINK_LOG_INFO(TEXT("Renamed group preset: %s -> %s"), *OldPresetName, *NewPresetName);

    return true;
}

bool UPJLinkManagerComponent::SetGroupPresetDescription(const FString& PresetName, const FString& Description)
{
    if (!GroupPresetMap.Contains(PresetName))
    {
        PJLINK_LOG_WARNING(TEXT("Preset not found: %s"), *PresetName);
        return false;
    }

    // 설명 업데이트
    FPJLinkGroupPreset& Preset = GroupPresetMap[PresetName];
    Preset.Description = Description;
    Preset.LastModifiedTime = FDateTime::Now();

    // 파일 저장
    SaveGroupPresetsToFile();

    // 이벤트 발생
    OnGroupPresetModified.Broadcast(PresetName);

    PJLINK_LOG_INFO(TEXT("Updated description for group preset: %s"), *PresetName);

    return true;
}

TArray<FPJLinkGroupPreset> UPJLinkManagerComponent::GetAllGroupPresets() const
{
    TArray<FPJLinkGroupPreset> Result;
    GroupPresetMap.GenerateValueArray(Result);
    return Result;
}

TArray<FPJLinkGroupPreset> UPJLinkManagerComponent::GetGroupPresets(const FString& GroupName) const
{
    TArray<FPJLinkGroupPreset> Result;

    for (const auto& Pair : GroupPresetMap)
    {
        if (Pair.Value.GroupName == GroupName)
        {
            Result.Add(Pair.Value);
        }
    }

    return Result;
}

TArray<FString> UPJLinkManagerComponent::GetAllGroupPresetNames() const
{
    TArray<FString> Result;
    GroupPresetMap.GetKeys(Result);
    return Result;
}

TArray<FString> UPJLinkManagerComponent::GetGroupPresetNames(const FString& GroupName) const
{
    TArray<FString> Result;

    for (const auto& Pair : GroupPresetMap)
    {
        if (Pair.Value.GroupName == GroupName)
        {
            Result.Add(Pair.Key);
        }
    }

    return Result;
}

bool UPJLinkManagerComponent::DoesGroupPresetExist(const FString& PresetName) const
{
    return GroupPresetMap.Contains(PresetName);
}

bool UPJLinkManagerComponent::SaveGroupPresetsToFile()
{
    // JSON 객체 생성
    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);
    TArray<TSharedPtr<FJsonValue>> PresetArray;

    // 각 프리셋을 JSON으로 변환
    for (const auto& Pair : GroupPresetMap)
    {
        const FPJLinkGroupPreset& Preset = Pair.Value;
        TSharedPtr<FJsonObject> JsonPreset = SerializeGroupPreset(Preset);
        PresetArray.Add(MakeShareable(new FJsonValueObject(JsonPreset)));
    }

    // 루트 객체에 배열 추가
    RootObject->SetArrayField(TEXT("GroupPresets"), PresetArray);

    // JSON 문자열로 변환
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);

    // 디렉토리 확인 및 생성
    FString FilePath = GetGroupPresetFilePath();
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
        PJLINK_LOG_INFO(TEXT("Saved group presets to file: %s"), *FilePath);
    }
    else
    {
        PJLINK_LOG_ERROR(TEXT("Failed to save group presets to file: %s"), *FilePath);
    }

    return bSuccess;
}

bool UPJLinkManagerComponent::LoadGroupPresetsFromFile()
{
    // 파일 경로
    FString FilePath = GetGroupPresetFilePath();

    // 파일 존재 여부 확인
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    if (!PlatformFile.FileExists(*FilePath))
    {
        PJLINK_LOG_INFO(TEXT("Group preset file does not exist: %s"), *FilePath);
        return false;
    }

    // 파일 읽기
    FString JsonString;
    if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
    {
        PJLINK_LOG_ERROR(TEXT("Failed to load group preset file: %s"), *FilePath);
        return false;
    }

    // JSON 파싱
    TSharedPtr<FJsonObject> RootObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
    {
        PJLINK_LOG_ERROR(TEXT("Failed to parse group preset JSON file: %s"), *FilePath);
        return false;
    }

    // 프리셋 배열 가져오기
    TArray<TSharedPtr<FJsonValue>> PresetArray = RootObject->GetArrayField(TEXT("GroupPresets"));

    // 기존 프리셋 맵 초기화
    GroupPresetMap.Empty();

    // 각 프리셋 처리
    for (const auto& JsonValue : PresetArray)
    {
        TSharedPtr<FJsonObject> JsonPreset = JsonValue->AsObject();
        if (!JsonPreset.IsValid())
        {
            continue;
        }

        FPJLinkGroupPreset Preset;
        if (DeserializeGroupPreset(JsonPreset, Preset))
        {
            GroupPresetMap.Add(Preset.PresetName, Preset);
        }
    }

    PJLINK_LOG_INFO(TEXT("Loaded %d group presets from file: %s"), GroupPresetMap.Num(), *FilePath);

    return true;
}

FString UPJLinkManagerComponent::GetGroupPresetFilePath() const
{
    return FPaths::ProjectSavedDir() / TEXT("PJLink") / TEXT("GroupPresets.json");
}

TSharedPtr<FJsonObject> UPJLinkManagerComponent::SerializeGroupPreset(const FPJLinkGroupPreset& Preset) const
{
    TSharedPtr<FJsonObject> JsonPreset = MakeShareable(new FJsonObject);

    // 기본 정보
    JsonPreset->SetStringField(TEXT("PresetName"), Preset.PresetName);
    JsonPreset->SetStringField(TEXT("Description"), Preset.Description);
    JsonPreset->SetStringField(TEXT("GroupName"), Preset.GroupName);
    JsonPreset->SetStringField(TEXT("CreationTime"), Preset.CreationTime.ToString());
    JsonPreset->SetStringField(TEXT("LastModifiedTime"), Preset.LastModifiedTime.ToString());
    JsonPreset->SetNumberField(TEXT("PowerStatus"), static_cast<int32>(Preset.PowerStatus));
    JsonPreset->SetNumberField(TEXT("InputSource"), static_cast<int32>(Preset.InputSource));

    // 프로젝터 설정
    TSharedPtr<FJsonObject> JsonProjectorSettings = MakeShareable(new FJsonObject);

    for (const auto& Pair : Preset.ProjectorSettings)
    {
        TSharedPtr<FJsonObject> JsonProjectorInfo = MakeShareable(new FJsonObject);
        const FPJLinkProjectorInfo& ProjectorInfo = Pair.Value;

        JsonProjectorInfo->SetStringField(TEXT("Name"), ProjectorInfo.Name);
        JsonProjectorInfo->SetStringField(TEXT("IPAddress"), ProjectorInfo.IPAddress);
        JsonProjectorInfo->SetNumberField(TEXT("Port"), ProjectorInfo.Port);
        JsonProjectorInfo->SetNumberField(TEXT("DeviceClass"), static_cast<int32>(ProjectorInfo.DeviceClass));
        JsonProjectorInfo->SetBoolField(TEXT("RequiresAuthentication"), ProjectorInfo.bRequiresAuthentication);
        JsonProjectorInfo->SetStringField(TEXT("Password"), ProjectorInfo.Password);

        JsonProjectorSettings->SetObjectField(Pair.Key, JsonProjectorInfo);
    }

    JsonPreset->SetObjectField(TEXT("ProjectorSettings"), JsonProjectorSettings);

    return JsonPreset;
}

bool UPJLinkManagerComponent::DeserializeGroupPreset(const TSharedPtr<FJsonObject>& JsonObject, FPJLinkGroupPreset& OutPreset) const
{
    // 기본 정보
    OutPreset.PresetName = JsonObject->GetStringField(TEXT("PresetName"));
    OutPreset.Description = JsonObject->GetStringField(TEXT("Description"));
    OutPreset.GroupName = JsonObject->GetStringField(TEXT("GroupName"));

    FDateTime::Parse(JsonObject->GetStringField(TEXT("CreationTime")), OutPreset.CreationTime);
    FDateTime::Parse(JsonObject->GetStringField(TEXT("LastModifiedTime")), OutPreset.LastModifiedTime);

    OutPreset.PowerStatus = static_cast<EPJLinkPowerStatus>(JsonObject->GetIntegerField(TEXT("PowerStatus")));
    OutPreset.InputSource = static_cast<EPJLinkInputSource>(JsonObject->GetIntegerField(TEXT("InputSource")));

    // 프로젝터 설정
    TSharedPtr<FJsonObject> JsonProjectorSettings = JsonObject->GetObjectField(TEXT("ProjectorSettings"));
    OutPreset.ProjectorSettings.Empty();

    for (const auto& Pair : JsonProjectorSettings->Values)
    {
        const FString& ProjectorID = Pair.Key;
        TSharedPtr<FJsonObject> JsonProjectorInfo = Pair.Value->AsObject();

        FPJLinkProjectorInfo ProjectorInfo;
        ProjectorInfo.Name = JsonProjectorInfo->GetStringField(TEXT("Name"));
        ProjectorInfo.IPAddress = JsonProjectorInfo->GetStringField(TEXT("IPAddress"));
        ProjectorInfo.Port = JsonProjectorInfo->GetIntegerField(TEXT("Port"));
        ProjectorInfo.DeviceClass = static_cast<EPJLinkClass>(JsonProjectorInfo->GetIntegerField(TEXT("DeviceClass")));
        ProjectorInfo.bRequiresAuthentication = JsonProjectorInfo->GetBoolField(TEXT("RequiresAuthentication"));
        ProjectorInfo.Password = JsonProjectorInfo->GetStringField(TEXT("Password"));

        OutPreset.ProjectorSettings.Add(ProjectorID, ProjectorInfo);
    }

    return true;
}

bool UPJLinkManagerComponent::IsProjectorInGroup(UPJLinkComponent* ProjectorComponent, const FString& GroupName) const
{
    if (!ProjectorComponent || !GroupMap.Contains(GroupName))
    {
        return false;
    }

    FPJLinkProjectorInfo ProjectorInfo = ProjectorComponent->GetProjectorInfo();
    FString ProjectorID = GenerateProjectorID(ProjectorInfo);

    return GroupMap[GroupName].ContainsProjectorID(ProjectorID);
}