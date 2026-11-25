# 개발 환경 기록

- 확인한 날짜: 2026-11-20
- macOS 버전: macOS 27.2.1 (Build 26B131)
- CPU 종류: arm64
- 컴파일러 버전: Apple clang version 21.0.0 (clang-2100.4.1.1)
- make 버전: GNU Make 3.81
- 열 수 있는 fd 수: 1048575
- 연결 대기 큐 한도: 확인 못 함. `sysctl kern.ipc.somaxconn` 실행이 Operation not permitted로 실패
- MSL 값: 확인 못 함. `sysctl net.inet.tcp.msl` 실행이 Operation not permitted로 실패
- 없는 도구: wrk, hey. 부하 테스트 단계에서 필요
- 선택 도구 brew: /opt/homebrew/bin/brew
- 8080 포트: 연결 거부 확인. `nc -z 127.0.0.1 8080` 종료 코드 1, `netstat -an -p tcp | grep LISTEN | grep '\.8080 '`에서 일치 항목 없음. `scripts/check_env.sh` 출력은 `비어 있음 (연결 거부 + LISTEN 줄 없음)`.
- netstat 조회 주의: 개별 명령 실행 때 `sysctl: net.inet.tcp.pcblist_n: Operation not permitted` 메시지가 출력됨. 점검 스크립트 실행에서는 8080 일치 줄이 없어 비어 있음으로 보고됨.

## IDE — Visual Studio Code

### 튜토리얼 설명 규칙

코드 안내에서 축약어가 처음 나오면 영어 원문과 풀어 쓴 뜻을 함께 적는다. 예를 들어 파일 설명자(fd, file descriptor)는 프로세스가 열린 파일이나 소켓을 가리킬 때 쓰는 번호다. 이후 같은 설명에서 다시 나오는 축약어는 문맥에 맞게 사용한다.

매 답변의 첫머리 선언에 생략 금지 원칙과 축약어의 영어 원문 병기 원칙을 함께 밝힌다.

VS Code 1.143.1, arm64. 영어 화면. 작업 영역에서 끈 확장 없음.

승인 후 실행한 설치 명령
```sh
code --install-extension ms-vscode.cpptools
code --install-extension vadimcn.vscode-lldb
code --install-extension ms-vscode.makefile-tools
code --install-extension EditorConfig.EditorConfig
```

설치 후 확장 목록
```text
alexdauenhauer.catppuccin-noctis@3.1.9
atomiks.moonlight@0.11.1
avetis.tokyo-night@1.0.2
chaseadamsio.vscode-theme-neon-night@0.35.0
dracula-theme.theme-dracula@2.25.1
dsoloha.native-macos@0.5.2
echevarriandre.theme-dracula-clean@4.0.6
editorconfig.editorconfig@0.18.2
esthorace.python-theme@3.0.0
gokturksm.nebula-pandas@0.0.4
llacoste2000.unofficial-gitlab-dark-theme@1.2.3
ms-python.debugpy@2026.6.0
ms-python.python@2026.10.0
ms-python.vscode-pylance@2026.4.1
ms-python.vscode-python-envs@1.44.0
ms-vscode.cpptools@1.35.6
ms-vscode.makefile-tools@0.12.17
mskelton.one-dark-theme@1.14.3
nguyenhoanglam.beautiful-dracula@0.1.4
rafaelmardojai.vscode-gnome-theme@0.4.1
vadimcn.vscode-lldb@1.12.3
```

| 검사 | 결과와 근거 | 확인자 |
|---|---|---|
| 1 프로젝트 열기 | 통과. Explorer에 프로젝트 표시 | 사용자와 Codex |
| 2 신뢰 | 빌드 작업과 디버깅 실행 확인. 신뢰 대화상자 응답 자체는 미기록 | 사용자와 Codex |
| 3 헤더 | 통과. 여섯 줄에 오류 밑줄 없음 | 사용자 |
| 4 자동 완성 | 통과. sock 입력 시 socket 표시 | 사용자 |
| 5 정의 이동 | 통과. F12로 sys/socket.h 이동 | 사용자 |
| 6 빌드 작업 | 통과. 컴파일과 링크 성공 | 사용자 로그와 Codex |
| 7 터미널 빌드 | 통과. env ok, fd = 3 | Codex와 사용자 |
| 8 오류 위치 | 통과. Problems에서 Makefile Tools 10행 36열과 IntelliSense 11행 5열 확인 | 사용자 로그와 Codex |
| 9 디버깅 | 통과. printf 중단점, fd = 3, 종료 코드 0 | 사용자 화면과 Codex |
| 10 통합 터미널 | 통과. 영어 입력에서 터미널 열림. 프로젝트에서 make 실행 | 사용자 |
| 11 탭 유지 | 통과. 사용자가 Makefile을 열고 닫은 뒤 탭 시작 줄 5개 | 사용자와 Codex |
