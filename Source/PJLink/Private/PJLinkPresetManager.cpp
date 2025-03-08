// PJLinkPresetManager.cpp
#include "PJLinkPresetManager.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"
#include "PJLinkLog.h"

UPJLinkPresetManager::UPJLinkPresetManager()
{
}

void UPJLinkPresetManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // 파일에서 프리셋 로드
    LoadPresetsFromFile();
}

void UPJLinkPresetManager::Deinitialize()
{
    // 프리셋 파일 저장
    SavePresetsToFile();

    Super::Deinitialize();
}

bool UPJLinkPresetManager::SavePreset(const FString& PresetName, const FPJLinkProjectorInfo& ProjectorInfo)
{
    if (PresetName.IsEmpty())
    {
        PJLINK_LOG_ERROR(TEXT("Cannot save preset with empty name"));
        return false;
    }

    FPJLinkProjectorPreset Preset(PresetName, ProjectorInfo);
    Presets.Add(PresetName, Preset);

    PJLINK_LOG_INFO(TEXT("Saved preset: %s"), *PresetName);
    return true;
}

bool UPJLinkPresetManager::LoadPreset(const FString& PresetName, FPJLinkProjectorInfo& OutProjectorInfo)
{
    if (!DoesPresetExist(PresetName))
    {
        PJLINK_LOG_ERROR(TEXT("Preset not found: %s"), *PresetName);
        return false;
    }

    OutProjectorInfo = Presets[PresetName].ProjectorInfo;
    PJLINK_LOG_INFO(TEXT("Loaded preset: %s"), *PresetName);
    return true;
}

bool UPJLinkPresetManager::DeletePreset(const FString& PresetName)
{
    if (!DoesPresetExist(PresetName))
    {
        PJLINK_LOG_ERROR(TEXT("Cannot delete preset - not found: %s"), *PresetName);
        return false;
    }

    Presets.Remove(PresetName);
    PJLINK_LOG_INFO(TEXT("Deleted preset: %s"), *PresetName);
    return true;
}

TArray<FPJLinkProjectorPreset> UPJLinkPresetManager::GetAllPresets()
{
    TArray<FPJLinkProjectorPreset> PresetArray;
    Presets.GenerateValueArray(PresetArray);
    return PresetArray;
}

TArray<FString> UPJLinkPresetManager::GetAllPresetNames()
{
    TArray<FString> PresetNames;
    Presets.GetKeys(PresetNames);
    return PresetNames;
}

bool UPJLinkPresetManager::DoesPresetExist(const FString& PresetName)
{
    return Presets.Contains(PresetName);
}

bool UPJLinkPresetManager::SavePresetsToFile()
{
    // JSON 객체 생성
    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);
    TArray<TSharedPtr<FJsonValue>> PresetArray;

    // 각 프리셋을 JSON으로 변환
    for (const auto& Pair : Presets)
    {
        const FPJLinkProjectorPreset& Preset = Pair.Value;
        TSharedPtr<FJsonObject> PresetObj = MakeShareable(new FJsonObject);

        // 프리셋 이름
        PresetObj->SetStringField(TEXT("PresetName"), Preset.PresetName);

        // 프로젝터 정보
        TSharedPtr<FJsonObject> ProjectorObj = MakeShareable(new FJsonObject);
        ProjectorObj->SetStringField(TEXT("Name"), Preset.ProjectorInfo.Name);
        ProjectorObj->SetStringField(TEXT("IPAddress"), Preset.ProjectorInfo.IPAddress);
        ProjectorObj->SetNumberField(TEXT("Port"), Preset.ProjectorInfo.Port);
        ProjectorObj->SetNumberField(TEXT("DeviceClass"), static_cast<int32>(Preset.ProjectorInfo.DeviceClass));
        ProjectorObj->SetBoolField(TEXT("RequiresAuthentication"), Preset.ProjectorInfo.bRequiresAuthentication);
        ProjectorObj->SetStringField(TEXT("Password"), Preset.ProjectorInfo.Password);

        PresetObj->SetObjectField(TEXT("ProjectorInfo"), ProjectorObj);

        // 배열에 추가
        PresetArray.Add(MakeShareable(new FJsonValueObject(PresetObj)));
    }

    // 루트 객체에 배열 추가
    RootObject->SetArrayField(TEXT("Presets"), PresetArray);

    // JSON 문자열로 변환
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);

    // 파일에 저장
    FString FilePath = GetPresetFilePath();
    bool bSuccess = FFileHelper::SaveStringToFile(OutputString, *FilePath);

    if (bSuccess)
    {
        PJLINK_LOG_INFO(TEXT("Saved presets to file: %s"), *FilePath);
    }
    else
    {
        PJLINK_LOG_ERROR(TEXT("Failed to save presets to file: %s"), *FilePath);
    }

    return bSuccess;
}

bool UPJLinkPresetManager::LoadPresetsFromFile()
{
    // 프리셋 초기화
    Presets.Empty();

    // 파일 경로
    FString FilePath = GetPresetFilePath();

    // 파일 존재 여부 확인
    if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*FilePath))
    {
        PJLINK_LOG_INFO(TEXT("Preset file does not exist: %s"), *FilePath);
        return false;
    }

    // 파일 읽기
    FString JsonString;
    if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
    {
        PJLINK_LOG_ERROR(TEXT("Failed to load preset file: %s"), *FilePath);
        return false;
    }

    // JSON 파싱
    TSharedPtr<FJsonObject> RootObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
    {
        PJLINK_LOG_ERROR(TEXT("Failed to parse preset JSON file: %s"), *FilePath);
        return false;
    }

    // 프리셋 배열 가져오기
    TArray<TSharedPtr<FJsonValue>> PresetArray = RootObject->GetArrayField(TEXT("Presets"));

    // 각 프리셋 처리
    for (const auto& JsonValue : PresetArray)
    {
        TSharedPtr<FJsonObject> PresetObj = JsonValue->AsObject();
        if (!PresetObj.IsValid())
        {
            continue;
        }

        // 프리셋 이름
        FString PresetName = PresetObj->GetStringField(TEXT("PresetName"));

        // 프로젝터 정보
        TSharedPtr<FJsonObject> ProjectorObj = PresetObj->GetObjectField(TEXT("ProjectorInfo"));
        if (!ProjectorObj.IsValid())
        {
            continue;
        }

        FPJLinkProjectorInfo ProjectorInfo;
        ProjectorInfo.Name = ProjectorObj->GetStringField(TEXT("Name"));
        ProjectorInfo.IPAddress = ProjectorObj->GetStringField(TEXT("IPAddress"));
        ProjectorInfo.Port = ProjectorObj->GetIntegerField(TEXT("Port"));
        ProjectorInfo.DeviceClass = static_cast<EPJLinkClass>(ProjectorObj->GetIntegerField(TEXT("DeviceClass")));
        ProjectorInfo.bRequiresAuthentication = ProjectorObj->GetBoolField(TEXT("RequiresAuthentication"));
        ProjectorInfo.Password = ProjectorObj->GetStringField(TEXT("Password"));

        // 프리셋 추가
        FPJLinkProjectorPreset Preset(PresetName, ProjectorInfo);
        Presets.Add(PresetName, Preset);
    }

    PJLINK_LOG_INFO(TEXT("Loaded %d presets from file: %s"), Presets.Num(), *FilePath);
    return true;
}

FString UPJLinkPresetManager::GetPresetFilePath() const
{
    return FPaths::ProjectSavedDir() / TEXT("PJLink") / TEXT("Presets.json");
}