# PJLink 플러그인 개발자 가이드

## 개요
이 문서는 언리얼 엔진 5.5용 PJLink 플러그인의 개발자 가이드입니다. PJLink 프로토콜을 사용하여 프로젝터를 제어하는 방법에 대해 설명합니다.

## 주요 클래스 및 개념

### UPJLinkComponent
액터에 부착하여 사용하는 기본 컴포넌트입니다. 블루프린트와 C++에서 모두 사용할 수 있습니다.

```cpp
// C++ 예제: 컴포넌트 생성 및 사용
UPJLinkComponent* PJLinkComponent = CreateDefaultSubobject<UPJLinkComponent>(TEXT("PJLinkComponent"));
PJLinkComponent->ProjectorInfo.IPAddress = TEXT("192.168.1.100");
PJLinkComponent->ProjectorInfo.Port = 4352;
PJLinkComponent->bAutoConnect = true;
```

### UPJLinkSubsystem
게임 인스턴스 서브시스템으로, 전역적으로 접근 가능한 PJLink 인터페이스를 제공합니다.

```cpp
// C++ 예제: 서브시스템 사용
UGameInstance* GameInstance = GetWorld()->GetGameInstance();
UPJLinkSubsystem* PJLinkSystem = GameInstance->GetSubsystem<UPJLinkSubsystem>();
PJLinkSystem->ConnectToProjector(ProjectorInfo);
```

### UPJLinkNetworkManager
실제 네트워크 통신을 담당하는 클래스입니다. 대부분의 경우 직접 사용할 필요는 없으며, UPJLinkComponent나 UPJLinkSubsystem을 통해 간접적으로 사용합니다.

### UPJLinkStateMachine
프로젝터의 상태를 관리하는 상태 머신 클래스입니다. 상태 전환 로직과 가능한 작업을 관리합니다.

## 이벤트 시스템
플러그인은 다음과 같은 이벤트를 제공합니다:

- **OnResponseReceived**: PJLink 명령에 대한 응답을 받았을 때 발생
- **OnPowerStatusChanged**: 전원 상태가 변경되었을 때 발생
- **OnInputSourceChanged**: 입력 소스가 변경되었을 때 발생
- **OnConnectionChanged**: 연결 상태가 변경되었을 때 발생
- **OnErrorStatus**: 오류가 발생했을 때 발생
- **OnExtendedError**: 상세한 오류 정보가 있을 때 발생

## 오류 처리
플러그인은 EPJLinkErrorCode 열거형을 통해 다양한 오류 상태를 정의합니다. 오류가 발생하면 OnErrorStatus 또는 OnExtendedError 이벤트가 발생합니다.

## 진단 및 디버깅
플러그인은 상세한 진단 정보를 수집하고 보고서를 생성하는 기능을 제공합니다.

```cpp
// C++ 예제: 진단 보고서 생성
UPJLinkNetworkManager* NetworkManager = PJLinkComponent->GetNetworkManager();
FString DiagnosticReport = NetworkManager->GenerateDiagnosticReport();
UE_LOG(LogTemp, Log, TEXT("%s"), *DiagnosticReport);
```

## 확장 및 커스터마이징
자체 PJLink 명령을 추가하거나 기존 기능을 확장하려면 다음 클래스를 상속하는 것이 좋습니다:

- **UPJLinkComponent**: 사용자 정의 컴포넌트를 만들 때
- **UPJLinkNetworkManager**: 사용자 정의 네트워크 로직을 구현할 때