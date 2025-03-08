# PJLink 플러그인 for Unreal Engine 5.5

![PJLink Logo](Resources/Icon128.png)

## 개요

PJLink는 언리얼 엔진 5.5에서 PJLink 프로토콜을 통해 프로젝터를 제어할 수 있는 플러그인입니다. 이 플러그인을 사용하면 게임이나 애플리케이션에서 직접 프로젝터의 전원을 켜고 끄거나, 입력 소스를 변경하는 등의 작업을 수행할 수 있습니다.

## 주요 기능

- **간편한 프로젝터 제어**: 블루프린트 또는 C++에서 프로젝터 제어 가능
- **다양한 명령 지원**: 전원 켜기/끄기, 입력 소스 변경, 상태 확인 등
- **이벤트 기반 시스템**: 프로젝터 상태 변화에 대응하는 이벤트 제공
- **안정적인 오류 처리**: 네트워크 오류 발생 시 자동 재연결 및 복구
- **상태 머신 구현**: 프로젝터 상태에 따른 명령 실행 관리
- **프리셋 시스템**: 자주 사용하는 프로젝터 설정 저장 및 불러오기
- **진단 시스템**: 문제 해결을 위한 상세한 진단 정보 제공

## 시스템 요구사항

- Unreal Engine 5.5 이상
- Windows, macOS, 또는 Linux
- 네트워크로 연결된 PJLink 호환 프로젝터

## 설치 방법

### 마켓플레이스에서 설치
1. 언리얼 엔진 마켓플레이스에서 "PJLink" 검색
2. 플러그인 구매 및 다운로드
3. 프로젝트에 플러그인 추가

### 수동 설치
1. 이 저장소를 클론 또는 다운로드
2. `PJLink` 폴더를 프로젝트의 `Plugins` 폴더로 복사 (없는 경우 생성)
3. 프로젝트를 열고 플러그인 활성화

## 빠른 시작

### 블루프린트에서 사용

1. 액터 블루프린트 생성
2. 컴포넌트 패널에서 "PJLink Projector Component" 추가
3. 컴포넌트 세부 정보에서 프로젝터 IP 주소 및 포트 설정
4. 원하는 이벤트에 프로젝터 제어 명령 연결

```
BeginPlay 이벤트 → Connect → Power On
```

자세한 사용 방법은 `Documentation/PJLinkBlueprintGuide.md` 참조

### C++에서 사용

```cpp
#include "UPJLinkComponent.h"

// 컴포넌트 생성
UPJLinkComponent* PJLinkComponent = CreateDefaultSubobject<UPJLinkComponent>(TEXT("PJLinkComponent"));

// 프로젝터 설정
PJLinkComponent->ProjectorInfo.IPAddress = TEXT("192.168.1.100");
PJLinkComponent->ProjectorInfo.Port = 4352;

// 프로젝터 연결 및 제어
PJLinkComponent->Connect();
PJLinkComponent->PowerOn();
```

자세한 사용 방법은 `Documentation/PJLinkDeveloperGuide.md` 참조

## 지원되는 PJLink 명령

- **POWR**: 전원 제어 (켜기/끄기)
- **INPT**: 입력 소스 전환
- **AVMT**: AV 음소거
- **ERST**: 오류 상태 요청
- **LAMP**: 램프 상태 요청
- **INST**: 입력 단자 정보 요청
- **NAME**: 프로젝터 이름 요청
- **INF1**: 제조사 정보 요청
- **INF2**: 제품 정보 요청
- **INFO**: 기타 정보 요청
- **CLSS**: PJLink 클래스 정보 요청

## 문서

- [개발자 가이드](Documentation/PJLinkDeveloperGuide.md)
- [블루프린트 가이드](Documentation/PJLinkBlueprintGuide.md)
- [테스트 체크리스트](Documentation/TestChecklist.md)

## 로드맵

- 다중 프로젝터 동시 제어
- 네트워크 상의 PJLink 장치 자동 검색
- 명령 스케줄링 기능
- 웹 인터페이스 지원
- 모바일 플랫폼 지원

## 라이선스

이 플러그인은 MIT 라이선스 하에 배포됩니다. [LICENSE](LICENSE.md) 파일을 참조하세요.

## 연락처 및 지원

- 이슈 트래커: [GitHub Issues](https://github.com/yourusername/pjlink-ue5/issues)
- 이메일: support@yourdomain.com
- 웹사이트: [www.yourdomain.com/pjlink](https://www.yourdomain.com/pjlink)

## 기여하기

기여는 언제나 환영합니다. [CONTRIBUTING.md](CONTRIBUTING.md) 파일을 참조하여 기여 방법을 확인하세요.
