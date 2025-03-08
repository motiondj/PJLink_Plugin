// PJLinkSampleActor.cpp
#include "PJLinkSampleActor.h"

APJLinkSampleActor::APJLinkSampleActor()
{
    PrimaryActorTick.bCanEverTick = false;

    // PJLink 컴포넌트 생성
    PJLinkComponent = CreateDefaultSubobject<UPJLinkComponent>(TEXT("PJLinkComponent"));
}

void APJLinkSampleActor::BeginPlay()
{
    Super::BeginPlay();

    // 필요한 초기화 코드
}