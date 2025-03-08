// PJLinkPresetManager.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PJLinkTypes.h"
#include "PJLinkPresetManager.generated.h"

/**
 * 프로젝터 프리셋 정보를 저장하는 구조체
 */
USTRUCT(BlueprintType)
struct PJLINK_API FPJLinkProjectorPreset
{
    GENERATED_BODY()

    // 프리셋 이름
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|Preset")
    FString PresetName;

    // 프로젝터 정보
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PJLink|Preset")
    FPJLinkProjectorInfo ProjectorInfo;

    // 기본 생성자
    FPJLinkProjectorPreset() : PresetName(TEXT("New Preset")) {}

    // 생성자
    FPJLinkProjectorPreset(const FString& InPresetName, const FPJLinkProjectorInfo& InProjectorInfo)
        : PresetName(InPresetName), ProjectorInfo(InProjectorInfo) {
    }
};

/**
 * 설정 프리셋을 관리하는 서브시스템
 */
UCLASS()
class PJLINK_API UPJLinkPresetManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UPJLinkPresetManager();
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // 프리셋 저장
    UFUNCTION(BlueprintCallable, Category = "PJLink|Presets")
    bool SavePreset(const FString& PresetName, const FPJLinkProjectorInfo& ProjectorInfo);

    // 프리셋 로드
    UFUNCTION(BlueprintCallable, Category = "PJLink|Presets")
    bool LoadPreset(const FString& PresetName, FPJLinkProjectorInfo& OutProjectorInfo);

    // 프리셋 삭제
    UFUNCTION(BlueprintCallable, Category = "PJLink|Presets")
    bool DeletePreset(const FString& PresetName);

    // 모든 프리셋 가져오기
    UFUNCTION(BlueprintCallable, Category = "PJLink|Presets")
    TArray<FPJLinkProjectorPreset> GetAllPresets();

    // 모든 프리셋 이름 가져오기
    UFUNCTION(BlueprintCallable, Category = "PJLink|Presets")
    TArray<FString> GetAllPresetNames();

    // 프리셋 존재 여부 확인
    UFUNCTION(BlueprintCallable, Category = "PJLink|Presets")
    bool DoesPresetExist(const FString& PresetName);

    // 프리셋 파일 저장
    UFUNCTION(BlueprintCallable, Category = "PJLink|Presets")
    bool SavePresetsToFile();

    // 프리셋 파일 로드
    UFUNCTION(BlueprintCallable, Category = "PJLink|Presets")
    bool LoadPresetsFromFile();

private:
    // 프리셋 목록
    TMap<FString, FPJLinkProjectorPreset> Presets;

    // 파일에서 경로 가져오기
    FString GetPresetFilePath() const;
};
