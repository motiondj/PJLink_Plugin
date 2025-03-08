// PJLinkLog.h
#pragma once

#include "CoreMinimal.h"

// 전역 PJLink 로그 카테고리 정의
DECLARE_LOG_CATEGORY_EXTERN(LogPJLink, Log, All);

// 모듈 전체에서 사용할 유틸리티 매크로
#define PJLINK_LOG(Verbosity, Format, ...) \
    UE_LOG(LogPJLink, Verbosity, TEXT("%s - %s"), *FString(__FUNCTION__), *FString::Printf(Format, ##__VA_ARGS__))

// 조건부 로깅 추가
#define PJLINK_LOG_VERBOSE_COND(Condition, Format, ...) \
    if(Condition) { PJLINK_LOG(Verbose, Format, ##__VA_ARGS__); }

#define PJLINK_LOG_INFO(Format, ...) PJLINK_LOG(Log, Format, ##__VA_ARGS__)
#define PJLINK_LOG_WARNING(Format, ...) PJLINK_LOG(Warning, Format, ##__VA_ARGS__)
#define PJLINK_LOG_ERROR(Format, ...) PJLINK_LOG(Error, Format, ##__VA_ARGS__)
#define PJLINK_LOG_VERBOSE(Format, ...) PJLINK_LOG(Verbose, Format, ##__VA_ARGS__)

// 현재 함수 및 라인 번호를 포함한 고급 로깅 매크로
#define PJLINK_LOG_LOCATION(Verbosity, Format, ...) \
    UE_LOG(LogPJLink, Verbosity, TEXT("%s(%d) - %s: %s"), *FString(__FILE__), __LINE__, *FString(__FUNCTION__), *FString::Printf(Format, ##__VA_ARGS__))

// 진단 데이터 수집 매크로
#define PJLINK_CAPTURE_DIAGNOSTIC(DiagnosticStruct, Format, ...) \
    DiagnosticStruct.AddEntry(FString::Printf(Format, ##__VA_ARGS__), __FILE__, __LINE__, __FUNCTION__)

// 진단 데이터 구조체
USTRUCT(BlueprintType)
struct PJLINK_API FPJLinkDiagnosticData
{
    GENERATED_BODY()

    // 진단 정보 엔트리
    USTRUCT(BlueprintType)
        struct FDiagnosticEntry
    {
        GENERATED_BODY()

        // 메시지
        UPROPERTY(BlueprintReadOnly, Category = "PJLink|Diagnostic")
        FString Message;

        // 파일
        UPROPERTY(BlueprintReadOnly, Category = "PJLink|Diagnostic")
        FString File;

        // 라인 번호
        UPROPERTY(BlueprintReadOnly, Category = "PJLink|Diagnostic")
        int32 Line;

        // 함수 이름
        UPROPERTY(BlueprintReadOnly, Category = "PJLink|Diagnostic")
        FString Function;

        // 타임스탬프
        UPROPERTY(BlueprintReadOnly, Category = "PJLink|Diagnostic")
        FDateTime Timestamp;

        FDiagnosticEntry() : Line(0) {}

        FDiagnosticEntry(const FString& InMessage, const FString& InFile, int32 InLine, const FString& InFunction)
            : Message(InMessage)
            , File(FPaths::GetCleanFilename(InFile))
            , Line(InLine)
            , Function(InFunction)
            , Timestamp(FDateTime::Now())
        {
        }
    };

    // 엔트리 배열
    UPROPERTY(BlueprintReadOnly, Category = "PJLink|Diagnostic")
    TArray<FDiagnosticEntry> Entries;

    // 진단 ID
    UPROPERTY(BlueprintReadOnly, Category = "PJLink|Diagnostic")
    FGuid DiagnosticId;

    // 시작 시간
    UPROPERTY(BlueprintReadOnly, Category = "PJLink|Diagnostic")
    FDateTime StartTime;

    // 마지막 업데이트 시간
    UPROPERTY(BlueprintReadOnly, Category = "PJLink|Diagnostic")
    FDateTime LastUpdateTime;

    FPJLinkDiagnosticData()
        : DiagnosticId(FGuid::NewGuid())
        , StartTime(FDateTime::Now())
        , LastUpdateTime(FDateTime::Now())
    {
    }

    // 항목 추가
    void AddEntry(const FString& Message, const FString& File, int32 Line, const FString& Function)
    {
        Entries.Add(FDiagnosticEntry(Message, File, Line, Function));
        LastUpdateTime = FDateTime::Now();
    }

    // 보고서 생성
    FString GenerateReport() const
    {
        FString Report = FString::Printf(TEXT("PJLink Diagnostic Report\n"));
        Report += FString::Printf(TEXT("ID: %s\n"), *DiagnosticId.ToString());
        Report += FString::Printf(TEXT("Start Time: %s\n"), *StartTime.ToString());
        Report += FString::Printf(TEXT("Last Update: %s\n"), *LastUpdateTime.ToString());
        Report += FString::Printf(TEXT("Entry Count: %d\n\n"), Entries.Num());

        for (int32 i = 0; i < Entries.Num(); ++i)
        {
            const FDiagnosticEntry& Entry = Entries[i];
            Report += FString::Printf(TEXT("[%3d] %s - %s(%d) - %s: %s\n"),
                i + 1,
                *Entry.Timestamp.ToString(TEXT("%H:%M:%S.%s")),
                *Entry.File,
                Entry.Line,
                *Entry.Function,
                *Entry.Message);
        }

        return Report;
    }

    // 로그에 출력
    void LogReport(ELogVerbosity::Type Verbosity = ELogVerbosity::Log) const
    {
        UE_LOG(LogPJLink, Verbosity, TEXT("\n%s"), *GenerateReport());
    }
};

// 진단 데이터 구조체
USTRUCT(BlueprintType)
struct PJLINK_API FPJLinkDiagnosticData
{
    GENERATED_BODY()

    // 진단 정보 엔트리
    USTRUCT(BlueprintType)
        struct FDiagnosticEntry
    {
        GENERATED_BODY()

        // 메시지
        UPROPERTY(BlueprintReadOnly, Category = "PJLink|Diagnostic")
        FString Message;

        // 파일
        UPROPERTY(BlueprintReadOnly, Category = "PJLink|Diagnostic")
        FString File;

        // 라인 번호
        UPROPERTY(BlueprintReadOnly, Category = "PJLink|Diagnostic")
        int32 Line;

        // 함수 이름
        UPROPERTY(BlueprintReadOnly, Category = "PJLink|Diagnostic")
        FString Function;

        // 타임스탬프
        UPROPERTY(BlueprintReadOnly, Category = "PJLink|Diagnostic")
        FDateTime Timestamp;

        FDiagnosticEntry() : Line(0) {}

        FDiagnosticEntry(const FString& InMessage, const FString& InFile, int32 InLine, const FString& InFunction)
            : Message(InMessage)
            , File(FPaths::GetCleanFilename(InFile))
            , Line(InLine)
            , Function(InFunction)
            , Timestamp(FDateTime::Now())
        {
        }
    };

    // 엔트리 배열
    UPROPERTY(BlueprintReadOnly, Category = "PJLink|Diagnostic")
    TArray<FDiagnosticEntry> Entries;

    // 진단 ID
    UPROPERTY(BlueprintReadOnly, Category = "PJLink|Diagnostic")
    FGuid DiagnosticId;

    // 시작 시간
    UPROPERTY(BlueprintReadOnly, Category = "PJLink|Diagnostic")
    FDateTime StartTime;

    // 마지막 업데이트 시간
    UPROPERTY(BlueprintReadOnly, Category = "PJLink|Diagnostic")
    FDateTime LastUpdateTime;

    FPJLinkDiagnosticData()
        : DiagnosticId(FGuid::NewGuid())
        , StartTime(FDateTime::Now())
        , LastUpdateTime(FDateTime::Now())
    {
    }

    // 항목 추가
    void AddEntry(const FString& Message, const FString& File, int32 Line, const FString& Function)
    {
        Entries.Add(FDiagnosticEntry(Message, File, Line, Function));
        LastUpdateTime = FDateTime::Now();
    }

    // 보고서 생성
    FString GenerateReport() const
    {
        FString Report = FString::Printf(TEXT("PJLink Diagnostic Report\n"));
        Report += FString::Printf(TEXT("ID: %s\n"), *DiagnosticId.ToString());
        Report += FString::Printf(TEXT("Start Time: %s\n"), *StartTime.ToString());
        Report += FString::Printf(TEXT("Last Update: %s\n"), *LastUpdateTime.ToString());
        Report += FString::Printf(TEXT("Entry Count: %d\n\n"), Entries.Num());

        for (int32 i = 0; i < Entries.Num(); ++i)
        {
            const FDiagnosticEntry& Entry = Entries[i];
            Report += FString::Printf(TEXT("[%3d] %s - %s(%d) - %s: %s\n"),
                i + 1,
                *Entry.Timestamp.ToString(TEXT("%H:%M:%S.%s")),
                *Entry.File,
                Entry.Line,
                *Entry.Function,
                *Entry.Message);
        }

        return Report;
    }

    // 로그에 출력
    void LogReport(ELogVerbosity::Type Verbosity = ELogVerbosity::Log) const
    {
        UE_LOG(LogPJLink, Verbosity, TEXT("\n%s"), *GenerateReport());
    }
};
