# PJLink API 참조

## 클래스

### UPJLinkComponent
액터에 부착하여 PJLink 프로젝터 제어 기능을 제공하는 컴포넌트입니다.

#### 속성
- **ProjectorInfo**: 프로젝터 연결 정보(IP, 포트 등)
- **bAutoConnect**: 게임 시작 시 자동 연결 여부
- **bAutoReconnect**: 연결 끊김 시 자동 재연결 여부
- **ReconnectInterval**: 재연결 시도 간격(초)
- **MaxReconnectAttempts**: 최대 재연결 시도 횟수
- **ConnectionTimeout**: 연결 시도 제한 시간(초)
- **bPeriodicStatusCheck**: 주기적 상태 확인 여부
- **StatusCheckInterval**: 상태 확인 주기(초)
- **bVerboseLogging**: 자세한 로그 메시지 활성화 여부

#### 함수
- **Connect()**: 프로젝터에 연결
- **Disconnect()**: 프로젝터 연결 해제
- **IsConnected()**: 연결 상태 확인
- **PowerOn()**: 프로젝터 전원 켜기
- **PowerOff()**: 프로젝터 전원 끄기
- **SwitchInputSource(EPJLinkInputSource)**: 입력 소스 변경
- **SwitchInputSourceByIndex(int32)**: 인덱스로 입력 소스 변경 (0: RGB, 1: VIDEO, 2: DIGITAL, 3: STORAGE, 4: NETWORK)
- **RequestStatus()**: 프로젝터 상태 요청
- **GetProjectorInfo()**: 프로젝터 정보 가져오기
- **GetPowerStatus()**: 전원 상태 가져오기
- **GetInputSource()**: 입력 소스 가져오기
- **GetProjectorName()**: 프로젝터 이름 가져오기
- **IsProjectorReady()**: 프로젝터가 사용 가능한 상태인지 확인
- **IsPoweredOn()**: 전원이 켜져 있는지 확인
- **IsPoweredOff()**: 전원이 꺼져 있는지 확인
- **IsWarmingUp()**: 예열 중인지 확인
- **IsCoolingDown()**: 냉각 중인지 확인
- **GetConnectionStatus()**: 연결 상태 정보를 한 번에 가져오기
- **SaveCurrentSettingsAsPreset(FString)**: 현재 설정을 프리셋으로 저장
- **QuickSavePreset()**: 현재 설정을 빠르게 프리셋으로 저장
- **LoadPreset(FString)**: 프리셋 로드
- **LoadPresetByIndex(int32)**: 인덱스로 프리셋 로드
- **GetAvailablePresets()**: 사용 가능한 프리셋 목록 가져오기
- **GetPresetDisplayNames()**: 사용 가능한 프리셋 이름 목록 가져오기
- **GenerateDiagnosticReport()**: 진단 보고서 생성
- **GenerateDiagnosticDigest()**: 간략한 진단 보고서 생성

#### 이벤트
- **OnResponseReceived**: 명령 응답 수신
- **OnPowerStatusChanged**: 전원 상태 변경
- **OnInputSourceChanged**: 입력 소스 변경
- **OnConnectionChanged**: 연결 상태 변경
- **OnErrorStatus**: 오류 상태
- **OnExtendedError**: 확장 오류 정보
- **OnProjectorReady**: 프로젝터 준비 완료
- **OnCommandCompleted**: 명령 실행 완료
- **OnDebugMessage**: 디버그 메시지
- **OnCommunicationLog**: 통신 로그

### UPJLinkNetworkManager
프로젝터와의 네트워크 통신을 담당하는 클래스입니다.

#### 함수
- **ConnectToProjector(FPJLinkProjectorInfo)**: 프로젝터에 연결
- **DisconnectFromProjector()**: 프로젝터 연결 해제
- **SendCommand(EPJLinkCommand, FString)**: 명령 전송
- **GetProjectorInfo()**: 프로젝터 정보 가져오기
- **IsConnected()**: 연결 상태 확인
- **PowerOn()**: 프로젝터 전원 켜기
- **PowerOff()**: 프로젝터 전원 끄기
- **SwitchInputSource(EPJLinkInputSource)**: 입력 소스 변경
- **RequestStatus()**: 프로젝터 상태 요청
- **GenerateDiagnosticReport()**: 진단 보고서 생성

### UPJLinkStateMachine
프로젝터의 상태 전환을 관리하는 클래스입니다.

#### 함수
- **SetState(EPJLinkProjectorState)**: 상태 설정
- **GetCurrentState()**: 현재 상태 가져오기
- **CanPerformAction(EPJLinkCommand)**: 특정 명령 실행 가능 여부 확인
- **UpdateFromPowerStatus(EPJLinkPowerStatus)**: 전원 상태에 따른 상태 업데이트
- **UpdateFromConnectionStatus(bool)**: 연결 상태에 따른 상태 업데이트
- **SetErrorState(FString)**: 오류 상태 설정
- **GetStateName()**: 상태 이름 가져오기
- **GetErrorMessage()**: 오류 메시지 가져오기

### UPJLinkPresetManager
프로젝터 설정 프리셋을 관리하는 클래스입니다.

#### 함수
- **SavePreset(FString, FPJLinkProjectorInfo)**: 프리셋 저장
- **LoadPreset(FString, FPJLinkProjectorInfo&)**: 프리셋 로드
- **DeletePreset(FString)**: 프리셋 삭제
- **GetAllPresets()**: 모든 프리셋 가져오기
- **GetAllPresetNames()**: 모든 프리셋 이름 가져오기
- **DoesPresetExist(FString)**: 프리셋 존재 여부 확인

## 열거형

### EPJLinkPowerStatus
프로젝터 전원 상태를 나타내는 열거형입니다.
- **PoweredOff**: 전원 꺼짐
- **PoweredOn**: 전원 켜짐
- **CoolingDown**: 냉각 중
- **WarmingUp**: 예열 중
- **Unknown**: 알 수 없음

### EPJLinkInputSource
프로젝터 입력 소스를 나타내는 열거형입니다.
- **RGB**: RGB 입력 (1)
- **VIDEO**: 비디오 입력 (2)
- **DIGITAL**: 디지털 입력 (3)
- **STORAGE**: 저장장치 입력 (4)
- **NETWORK**: 네트워크 입력 (5)
- **Unknown**: 알 수 없음

### EPJLinkCommand
PJLink 명령을 나타내는 열거형입니다.
- **POWR**: 전원 제어
- **INPT**: 입력 소스 전환
- **AVMT**: AV 음소거
- **ERST**: 오류 상태
- **LAMP**: 램프 상태
- **INST**: 입력 터미널 정보
- **NAME**: 프로젝터 이름
- **INF1**: 제조사 정보
- **INF2**: 제품명 정보
- **INFO**: 기타 정보
- **CLSS**: 클래스 정보

### EPJLinkProjectorState
프로젝터 상태를 나타내는 열거형입니다.
- **Disconnected**: 연결 끊김
- **Connecting**: 연결 중
- **Connected**: 연결됨
- **PoweringOn**: 전원 켜는 중
- **PoweringOff**: 전원 끄는 중
- **ReadyForUse**: 사용 준비 완료
- **Error**: 오류 상태

### EPJLinkErrorCode
PJLink 오류 코드를 나타내는 열거형입니다.
- **None**: 오류 없음
- **ConnectionFailed**: 연결 실패
- **AuthenticationFailed**: 인증 실패
- **InvalidIP**: 잘못된 IP 주소
- **SocketCreationFailed**: 소켓 생성 실패
- **SocketError**: 소켓 오류
- **CommandFailed**: 명령 실패
- **Timeout**: 타임아웃
- **InvalidResponse**: 잘못된 응답
- **ProjectorError**: 프로젝터 오류
- **UnknownError**: 알 수 없는 오류

## 구조체

### FPJLinkProjectorInfo
프로젝터 정보를 저장하는 구조체입니다.
- **Name**: 프로젝터 이름
- **IPAddress**: IP 주소
- **Port**: 포트 번호
- **DeviceClass**: 장치 클래스
- **bRequiresAuthentication**: 인증 필요 여부
- **Password**: 암호
- **PowerStatus**: 전원 상태
- **CurrentInputSource**: 현재 입력 소스
- **bIsConnected**: 연결 여부

### FPJLinkProjectorPreset
프로젝터 프리셋 정보를 저장하는 구조체입니다.
- **PresetName**: 프리셋 이름
- **ProjectorInfo**: 프로젝터 정보
