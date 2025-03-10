# PJLink 플러그인 for Unreal Engine 5.5

## 개요
PJLink 플러그인은 언리얼 엔진 5.5에서 PJLink 프로토콜을 사용하여 프로젝터를 제어할 수 있는 기능을 제공합니다. 블루프린트와 C++에서 모두 사용 가능하며, 네트워크를 통해 PJLink 호환 프로젝터와 통신합니다.

## 주요 기능
- 프로젝터 전원 제어 (켜기/끄기)
- 입력 소스 전환
- 상태 모니터링 및 이벤트 처리
- 프리셋 관리 (설정 저장 및 불러오기)
- 오류 처리 및 자동 재연결
- 블루프린트 친화적인 인터페이스
- 확장 가능한 아키텍처

## 설치 방법
1. 플러그인 파일을 프로젝트의 Plugins 폴더에 복사합니다.
2. 언리얼 에디터를 실행하고 플러그인을 활성화합니다.
3. 프로젝트를 다시 시작합니다.

## 빠른 시작
1. 액터에 PJLink 컴포넌트를 추가합니다.
2. 프로젝터 정보(IP 주소, 포트 등)를 설정합니다.
3. 자동 연결 옵션을 활성화하거나 수동으로 Connect 함수를 호출합니다.
4. PowerOn, PowerOff, SwitchInputSource 등의 함수를 사용하여 프로젝터를 제어합니다.

## 예제
플러그인에는 다음과 같은 예제가 포함되어 있습니다:
- 기본 연결 및 제어 예제
- 상태 모니터링 UI 예제
- 프리셋 관리 예제
- 오류 처리 예제

## 문서
자세한 사용법은 다음 문서를 참조하세요:
- [개발자 가이드](./Docs/PJLinkDeveloperGuide.md)
- [블루프린트 가이드](./Docs/PJLinkBlueprintGuide.md)
- [API 참조](./Docs/API_Reference.md)

## 문제 해결
일반적인 문제 해결 방법:
- 프로젝터와의 네트워크 연결을 확인합니다.
- 올바른 IP 주소와 포트를 설정했는지 확인합니다.
- 인증이 필요한 경우 올바른 암호를 설정했는지 확인합니다.
- 진단 보고서 기능을 사용하여 상세한 문제 분석이 가능합니다.

## 라이센스
이 플러그인은 MIT 라이센스에 따라 배포됩니다. 자세한 내용은 LICENSE 파일을 참조하세요.
