// PJLinkManagerComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PJLinkTypes.h"
#include "UPJLinkComponent.h"
#include "PJLinkManagerComponent.generated.h"

/**
 * 여러 PJLink 프로젝터를 관리하는 컴포넌트
 * 그룹 관리, 동시 명령 전송, 상태 추적 기능 제공
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent), DisplayName = "PJLink Manager Component")
class PJLINK_API UPJLinkManagerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPJLinkManagerComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    //---------- 프로젝터 관리 함수 ----------//

    /**
     * 프로젝터 추가 (인스턴스 기반)
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager")
    bool AddProjector(UPJLinkComponent* ProjectorComponent);

    /**
     * 프로젝터 정보로 새 프로젝터 추가
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager")
    UPJLinkComponent* AddProjectorByInfo(const FPJLinkProjectorInfo& ProjectorInfo, const FString& GroupName = TEXT("Default"));

    /**
     * 프로젝터 제거
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager")
    bool RemoveProjector(UPJLinkComponent* ProjectorComponent);

    /**
     * ID로 프로젝터 제거
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager")
    bool RemoveProjectorById(const FString& ProjectorID);

    /**
     * 모든 프로젝터 제거
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager")
    void RemoveAllProjectors();

    /**
     * ID로 프로젝터 컴포넌트 가져오기
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager")
    UPJLinkComponent* GetProjectorById(const FString& ProjectorID) const;

    /**
     * 모든 프로젝터 컴포넌트 가져오기
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager")
    TArray<UPJLinkComponent*> GetAllProjectors() const;

    /**
     * 관리 중인 프로젝터 수 가져오기
     */
    UFUNCTION(BlueprintPure, Category = "PJLink|Manager")
    int32 GetProjectorCount() const;

    //---------- 그룹 관리 함수 ----------//

    /**
 * 새 그룹 생성 (이름만)
 */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager|Groups")
    bool CreateGroup(const FString& GroupName);

    /**
     * 새 그룹 생성 (모든 정보 포함)
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager|Groups")
    bool CreateGroupWithInfo(const FString& GroupName, const FString& Description, const FLinearColor& GroupColor);

    /**
     * 그룹 삭제
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager|Groups")
    bool DeleteGroup(const FString& GroupName);

    /**
     * 그룹 정보 가져오기
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager|Groups")
    bool GetGroupInfo(const FString& GroupName, FPJLinkProjectorGroup& OutGroupInfo) const;

    /**
     * 그룹 정보 설정
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager|Groups")
    bool SetGroupInfo(const FString& GroupName, const FPJLinkProjectorGroup& GroupInfo);

    /**
     * 그룹 색상 설정
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager|Groups")
    bool SetGroupColor(const FString& GroupName, const FLinearColor& GroupColor);

    /**
     * 그룹 설명 설정
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager|Groups")
    bool SetGroupDescription(const FString& GroupName, const FString& Description);

    /**
     * 프로젝터를 그룹에 추가
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager|Groups")
    bool AddProjectorToGroup(UPJLinkComponent* ProjectorComponent, const FString& GroupName);

    /**
     * 프로젝터를 그룹에서 제거
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager|Groups")
    bool RemoveProjectorFromGroup(UPJLinkComponent* ProjectorComponent, const FString& GroupName);

    /**
     * 프로젝터를 모든 그룹에서 제거
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager|Groups")
    bool RemoveProjectorFromAllGroups(UPJLinkComponent* ProjectorComponent);

    /**
     * 그룹의 모든 프로젝터 가져오기
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager|Groups")
    TArray<UPJLinkComponent*> GetProjectorsInGroup(const FString& GroupName) const;

    /**
     * 모든 그룹 정보 가져오기
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager|Groups")
    TArray<FPJLinkProjectorGroup> GetAllGroupInfos() const;

    /**
     * 모든 그룹 이름 가져오기
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager|Groups")
    TArray<FString> GetAllGroupNames() const;

    /**
     * 프로젝터가 속한 모든 그룹 가져오기
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager|Groups")
    TArray<FString> GetGroupsForProjector(UPJLinkComponent* ProjectorComponent) const;

    /**
     * 그룹 존재 여부 확인
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager|Groups")
    bool DoesGroupExist(const FString& GroupName) const;

    /**
     * 프로젝터가 그룹에 포함되어 있는지 확인
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager|Groups")
    bool IsProjectorInGroup(UPJLinkComponent* ProjectorComponent, const FString& GroupName) const;

    // 그룹 관련 이벤트를 추가합니다 (private 섹션 위 적절한 위치)

    /**
     * 그룹 생성 이벤트
     */
    UPROPERTY(BlueprintAssignable, Category = "PJLink|Manager|Events")
    FPJLinkGroupChangedDelegate OnGroupCreated;

    /**
     * 그룹 삭제 이벤트
     */
    UPROPERTY(BlueprintAssignable, Category = "PJLink|Manager|Events")
    FPJLinkGroupChangedDelegate OnGroupDeleted;

    /**
     * 그룹 변경 이벤트
     */
    UPROPERTY(BlueprintAssignable, Category = "PJLink|Manager|Events")
    FPJLinkGroupChangedDelegate OnGroupModified;

    /**
     * 프로젝터 그룹 할당 변경 이벤트
     */
    UPROPERTY(BlueprintAssignable, Category = "PJLink|Manager|Events")
    FPJLinkProjectorGroupAssignmentChangedDelegate OnProjectorGroupAssignmentChanged;

    *프로젝터 상태 변경 이벤트
        * /
        UPROPERTY(BlueprintAssignable, Category = "PJLink|Manager|Events")
    FPJLinkProjectorStatusChangedDelegate OnProjectorStatusChanged;


    //---------- 명령 실행 함수 ----------//

    /**
     * 모든 프로젝터 연결
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager|Commands")
    FString ConnectAll();

    /**
     * 그룹의 모든 프로젝터 연결
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager|Commands")
    FString ConnectGroup(const FString& GroupName);

    /**
     * 모든 프로젝터 연결 해제
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager|Commands")
    void DisconnectAll();

    /**
     * 그룹의 모든 프로젝터 연결 해제
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager|Commands")
    void DisconnectGroup(const FString& GroupName);

    /**
     * 모든 프로젝터 전원 켜기
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager|Commands")
    FString PowerOnAll();

    /**
     * 그룹의 모든 프로젝터 전원 켜기
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager|Commands")
    FString PowerOnGroup(const FString& GroupName);

    /**
     * 모든 프로젝터 전원 끄기
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager|Commands")
    FString PowerOffAll();

    /**
     * 그룹의 모든 프로젝터 전원 끄기
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager|Commands")
    FString PowerOffGroup(const FString& GroupName);

    /**
     * 모든 프로젝터 입력 소스 변경
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager|Commands")
    FString SwitchInputSourceAll(EPJLinkInputSource InputSource);

    /**
     * 그룹의 모든 프로젝터 입력 소스 변경
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager|Commands")
    FString SwitchInputSourceGroup(const FString& GroupName, EPJLinkInputSource InputSource);

    /**
     * 모든 프로젝터 상태 요청
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager|Commands")
    FString RequestStatusAll();

    /**
     * 그룹의 모든 프로젝터 상태 요청
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager|Commands")
    FString RequestStatusGroup(const FString& GroupName);

    /**
     * 그룹 명령 결과 가져오기
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager|Commands")
    bool GetGroupCommandResult(const FString& CommandID, FPJLinkGroupCommandResult& OutResult) const;

    /**
     * 최근 그룹 명령 결과 가져오기
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager|Commands")
    bool GetLatestGroupCommandResult(FPJLinkGroupCommandResult& OutResult) const;

    /**
     * 명령이 완료되었는지 확인
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager|Commands")
    bool IsCommandCompleted(const FString& CommandID) const;

    /**
     * 명령 타임아웃 시간 설정
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager|Commands")
    void SetCommandTimeout(float TimeoutSeconds);

    /**
     * 명령 타임아웃 시간 가져오기
     */
    UFUNCTION(BlueprintPure, Category = "PJLink|Manager|Commands")
    float GetCommandTimeout() const;

    /**
     * 현재 진행 중인 명령 취소
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager|Commands")
    bool CancelCommand(const FString& CommandID);

    // 결과 취합 이벤트 (기존 이벤트를 교체합니다)
    UPROPERTY(BlueprintAssignable, Category = "PJLink|Manager|Events")
    FPJLinkGroupCommandCompletedDelegate OnGroupCommandCompleted;


    //---------- 상태 추적 함수 ----------//

/**
 * 프로젝터 상태 가져오기
 */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager|Status")
    bool GetProjectorStatus(const FString& ProjectorID, FPJLinkProjectorStatus& OutStatus) const;

    /**
     * 프로젝터 상태 가져오기 (컴포넌트 기반)
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager|Status")
    bool GetProjectorStatusByComponent(UPJLinkComponent* ProjectorComponent, FPJLinkProjectorStatus& OutStatus) const;

    /**
     * 모든 프로젝터 상태 가져오기
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager|Status")
    TArray<FPJLinkProjectorStatus> GetAllProjectorStatuses() const;

    /**
     * 그룹 내 모든 프로젝터 상태 가져오기
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager|Status")
    TArray<FPJLinkProjectorStatus> GetGroupProjectorStatuses(const FString& GroupName) const;

    /**
     * 문제가 있는 프로젝터 상태 가져오기
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager|Status")
    TArray<FPJLinkProjectorStatus> GetUnhealthyProjectorStatuses() const;

    /**
     * 상태 업데이트 간격 설정 (초)
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager|Status")
    void SetStatusUpdateInterval(float IntervalSeconds);

    /**
     * 상태 업데이트 활성화/비활성화
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager|Status")
    void SetStatusUpdateEnabled(bool bEnabled);

    /**
     * 모든 프로젝터 상태 즉시 업데이트
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager|Status")
    void UpdateAllProjectorStatuses();

    /**
     * 그룹 내 모든 프로젝터 상태 즉시 업데이트
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager|Status")
    void UpdateGroupProjectorStatuses(const FString& GroupName);

    /**
     * 특정 프로젝터 상태 즉시 업데이트
     */
    UFUNCTION(BlueprintCallable, Category = "PJLink|Manager|Status")
    void UpdateProjectorStatus(const FString& ProjectorID);

    

private:
    // 진행 중인 명령 결과 맵
    TMap<FString, FPJLinkGroupCommandResult> CommandResults;

    // 가장 최근 명령 ID
    FString LatestCommandID;

    // 명령 타임아웃 시간 (초)
    float CommandTimeoutSeconds;

    // 타임아웃 타이머 핸들 맵
    TMap<FString, FTimerHandle> CommandTimeoutHandles;

    // 명령 ID 생성
    FString GenerateCommandID() const;

    // 명령 시작
    FString StartGroupCommand(const FString& GroupName, EPJLinkCommand Command, const FString& Parameter, const TArray<UPJLinkComponent*>& Projectors);

    // 프로젝터 명령 실행
    bool ExecuteCommandOnProjector(UPJLinkComponent* ProjectorComponent, EPJLinkCommand Command, const FString& Parameter, const FString& CommandID);

    // 명령 완료 이벤트 핸들러
    UFUNCTION()
    void HandleCommandCompleted(UPJLinkComponent* ProjectorComponent, EPJLinkCommand Command, bool bSuccess);

    // 명령 타임아웃 핸들러
    void HandleCommandTimeout(const FString& CommandID);

    // 진행 중인 명령 정리
    void CleanupCommand(const FString& CommandID);
    // 프로젝터 상태 맵
    TMap<FString, FPJLinkProjectorStatus> ProjectorStatusMap;

    // 상태 업데이트 타이머 핸들
    FTimerHandle StatusUpdateTimerHandle;

    // 상태 업데이트 간격 (초)
    float StatusUpdateInterval;

    // 상태 업데이트 활성화 여부
    bool bStatusUpdateEnabled;

    // 상태 업데이트 함수
    void PeriodicStatusUpdate();

    // 프로젝터 상태 업데이트 핸들러
    UFUNCTION()
    void HandlePowerStatusChanged(UPJLinkComponent* ProjectorComponent, EPJLinkPowerStatus OldStatus, EPJLinkPowerStatus NewStatus);

    UFUNCTION()
    void HandleInputSourceChanged(UPJLinkComponent* ProjectorComponent, EPJLinkInputSource OldSource, EPJLinkInputSource NewSource);

    UFUNCTION()
    void HandleConnectionChanged(UPJLinkComponent* ProjectorComponent, bool bIsConnected);

    UFUNCTION()
    void HandleErrorStatus(UPJLinkComponent* ProjectorComponent, const FString& ErrorMessage);

    // 그룹 프리셋 맵
    TMap<FString, FPJLinkGroupPreset> GroupPresetMap;

    // 그룹 프리셋 파일 경로 가져오기
    FString GetGroupPresetFilePath() const;

    // 프리셋 데이터 직렬화 (JSON)
    TSharedPtr<FJsonObject> SerializeGroupPreset(const FPJLinkGroupPreset& Preset) const;

    // 프리셋 데이터 역직렬화 (JSON)
    bool DeserializeGroupPreset(const TSharedPtr<FJsonObject>& JsonObject, FPJLinkGroupPreset& OutPreset) const;
};
