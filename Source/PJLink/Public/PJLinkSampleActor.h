// PJLinkSampleActor.h - 블루프린트에서 파생될 클래스
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UPJLinkComponent.h"
#include "PJLinkSampleActor.generated.h"

UCLASS(BlueprintType, Blueprintable)
class PJLINK_API APJLinkSampleActor : public AActor
{
    GENERATED_BODY()

public:
    APJLinkSampleActor();

    virtual void BeginPlay() override;

    // PJLink 컴포넌트
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PJLink")
    UPJLinkComponent* PJLinkComponent;
};
