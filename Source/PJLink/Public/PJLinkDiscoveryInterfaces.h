// PJLinkDiscoveryInterfaces.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PJLinkTypes.h"
#include "PJLinkDiscoveryInterfaces.generated.h"

// 결과 항목 선택 델리게이트
DECLARE_DELEGATE_OneParam(FOnResultItemSelected, int32);

UINTERFACE(MinimalAPI, Blueprintable)
class UResultItemInterface : public UInterface
{
    GENERATED_BODY()
};

/**
 * 검색 결과 항목 위젯 인터페이스
 * 검색 결과 항목 위젯이 구현해야 하는 인터페이스
 */
class PJLINK_API IResultItemInterface
{
    GENERATED_BODY()

public:
    // 결과 데이터 설정
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PJLink|Discovery")
    void SetResultData(const FPJLinkDiscoveryResult& Result, int32 Index);

    // 항목 선택 이벤트
    FOnResultItemSelected OnItemSelected;
};

UINTERFACE(MinimalAPI, Blueprintable)
class UDetailPanelInterface : public UInterface
{
    GENERATED_BODY()
};

/**
 * 세부 정보 패널 인터페이스
 * 세부 정보 패널 위젯이 구현해야 하는 인터페이스
 */
class PJLINK_API IDetailPanelInterface
{
    GENERATED_BODY()

public:
    // 선택된 결과 데이터 설정
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PJLink|Discovery")
    void SetSelectedResult(const FPJLinkDiscoveryResult& Result);

    // 패널 초기화
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PJLink|Discovery")
    void ClearPanel();
};
