# 작업 지시서 — macOS에서 Visual Studio Code로 C 개발 환경 구성

## 0. 목적과 경계

목적: 제가 C 코드를 직접 한 줄씩 치기 전에, 이 Mac의 Visual Studio Code(이하 VS Code)를 C 개발에 쓸 수 있는 상태로 만듭니다. IDE는 VS Code로 정했습니다. 튜토리얼 1-1구간은 이 작업이 모두 통과된 뒤에 VS Code 안에서 시작합니다.

이미 확인된 것(다시 하지 않아도 됨): cc(Apple clang 21.0.0), make(GNU Make 3.81), git, lldb, leaks, curl, nc, netstat, lsof, python3, 프로젝트 폴더 c-webserver-proxy의 뼈대, Makefile, scripts/check_env.sh, docs/ENVIRONMENT.md.

해야 하는 일: VS Code 설치 여부 점검, 터미널 명령 code 점검, 확장 기능 점검, 프로젝트용 설정 파일 작성, 동작 검사, 기록, 보고.

하면 안 되는 일:
- src/*.c 와 include/*.h 를 작성하거나 고치지 않습니다. 검사용 임시 파일 src/env_check.c 하나만 예외이고, 검사 뒤 지웁니다.
- VS Code의 사용자 전역 설정(~/Library/Application Support/Code/User/settings.json)과 전역 단축키를 고치지 않습니다. 설정은 프로젝트 폴더의 .vscode/ 안에만 둡니다.
- ~/.zshrc 같은 셸 설정 파일을 고치지 않습니다.
- 다른 IDE(Xcode 앱, CLion, Cursor, Zed)를 설치하거나 권하지 않습니다.

먼저 물어야 하는 일: 무엇이든 설치하는 일(VS Code, 확장 기능, brew 패키지), sudo가 필요한 일, 프로젝트 폴더 밖의 파일을 고치는 일.

답하는 말: 한국어. 처음 나오는 용어는 쉬운 말로 풀고 영어 원문을 함께 적습니다.

## 1단계 — VS Code 설치 여부 점검

아래 명령으로 확인하고 결과를 표로 보여 주세요. 이 단계에서는 아무것도 설치하거나 고치지 않습니다.

  ls /Applications | grep -i "visual studio code"
  ls ~/Applications 2>/dev/null | grep -i "visual studio code"
  command -v code && code --version
  xcode-select -p

표의 열: 항목 / 결과 / 버전.
- VS Code 앱이 있는가
- 터미널 명령 code 를 쓸 수 있는가
- VS Code가 Apple Silicon(arm64)용인가 (code --version 의 셋째 줄이 arm64 인지 확인)
- Command Line Tools 경로

## 2단계 — VS Code가 없거나 부족할 때

2-1. VS Code 앱이 없으면 설치 방법 두 가지를 제시하고 제 승인을 기다립니다. 제가 답하기 전에는 설치하지 않습니다.
  (가) 공식 사이트에서 Apple Silicon용을 내려받아 /Applications 에 넣기 — 제가 직접 합니다.
  (나) brew install --cask visual-studio-code — 제가 승인하면 당신이 실행합니다.

2-2. 앱은 있는데 code 명령이 없으면, VS Code에서 Cmd+Shift+P 를 누르고 "Shell Command: Install 'code' command in PATH"를 실행해 달라고 저에게 안내합니다. 제가 직접 합니다. 끝나면 새 터미널에서 command -v code 로 다시 확인합니다.

2-3. VS Code가 Intel(x64)용이면 그 사실을 알리고, Apple Silicon용으로 바꿀지 저에게 묻습니다. 바꾸지 않아도 작업은 계속합니다.

## 3단계 — 확장 기능 점검과 설치

3-1. 지금 설치된 확장 기능을 확인합니다.
  code --list-extensions --show-versions

3-2. 아래 표와 대조해 있는 것과 없는 것을 보여 주고, 없는 것은 설치해도 되는지 묻습니다.

| 확장 기능 ID | 무엇에 쓰나 | 필수 여부 |
|---|---|---|
| ms-vscode.cpptools | C 자동 완성, 오류 밑줄, 정의로 이동 (IntelliSense) | 필수 |
| vadimcn.vscode-lldb | lldb로 중단점 디버깅 (CodeLLDB) | 필수 |
| ms-vscode.makefile-tools | Makefile의 대상 목록 보기 | 선택 |
| EditorConfig.EditorConfig | 들여쓰기 규칙을 파일로 고정 | 선택 |

3-3. 승인받은 것만 아래 형식으로 설치합니다.
  code --install-extension <확장 기능 ID>

3-4. clangd 확장(llvm-vs-code-extensions.vscode-clangd)이 이미 설치되어 있으면 ms-vscode.cpptools 와 기능이 겹칩니다. 둘을 함께 켜지 말고 어느 쪽을 쓸지 저에게 묻습니다. 제가 정하지 않으면 ms-vscode.cpptools 를 쓰고, clangd 확장은 지우지 않고 이 작업 영역(workspace)에서만 끄도록 안내합니다.

3-5. 한국어 화면을 원하는지 저에게 한 번 묻습니다. 원하면 MS-CEINTL.vscode-language-pack-ko 설치를 제안합니다. 이 지시서의 메뉴 이름은 영어 화면 기준이고, 한국어 화면에서의 이름을 함께 알려 줍니다.

## 4단계 — 프로젝트용 설정 파일 작성

모두 c-webserver-proxy 폴더 안에 만듭니다. 같은 파일이 이미 있으면 덮어쓰지 말고 내용을 보여 준 뒤 어떻게 할지 묻습니다.

4-1. .vscode/c_cpp_properties.json — 자동 완성과 오류 표시가 쓰는 컴파일러와 헤더 위치
{
  "version": 4,
  "configurations": [
    {
      "name": "Mac",
      "compilerPath": "/usr/bin/clang",
      "cStandard": "c11",
      "intelliSenseMode": "macos-clang-arm64",
      "includePath": ["${workspaceFolder}/include"],
      "defines": []
    }
  ]
}

4-2. .vscode/tasks.json — Cmd+Shift+B 로 실행하는 빌드 작업
{
  "version": "2.0.0",
  "tasks": [
    {
      "label": "make",
      "type": "shell",
      "command": "make",
      "group": { "kind": "build", "isDefault": true },
      "problemMatcher": ["$gcc"]
    },
    {
      "label": "make asan",
      "type": "shell",
      "command": "make asan",
      "problemMatcher": ["$gcc"]
    },
    {
      "label": "make clean",
      "type": "shell",
      "command": "make clean",
      "problemMatcher": []
    }
  ]
}

4-3. .vscode/launch.json — F5 로 실행하는 디버깅
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "server 디버그 (lldb)",
      "type": "lldb",
      "request": "launch",
      "program": "${workspaceFolder}/build/server",
      "args": [],
      "cwd": "${workspaceFolder}",
      "preLaunchTask": "make"
    }
  ]
}

4-4. .vscode/settings.json — 이 프로젝트에만 적용되는 편집 설정
{
  "files.associations": { "*.h": "c" },
  "editor.tabSize": 4,
  "editor.insertSpaces": true,
  "files.insertFinalNewline": true,
  "files.trimTrailingWhitespace": true,
  "[makefile]": { "editor.insertSpaces": false, "editor.detectIndentation": false },
  "C_Cpp.errorSquiggles": "enabled",
  "editor.inlineSuggest.enabled": false
}

마지막 줄은 AI 자동 완성 제안을 이 프로젝트에서 끄는 설정입니다. 제가 코드를 직접 치는 것이 목적이기 때문입니다. GitHub Copilot 같은 확장 기능이 설치되어 있으면 그 사실을 알리고, 이 작업 영역에서 끌지 저에게 묻습니다.

4-5. .vscode/extensions.json — 팀원이 폴더를 열 때 권장 확장 기능을 안내
{
  "recommendations": [
    "ms-vscode.cpptools",
    "vadimcn.vscode-lldb",
    "ms-vscode.makefile-tools",
    "EditorConfig.EditorConfig"
  ]
}

4-6. .editorconfig
root = true

[*]
charset = utf-8
end_of_line = lf
insert_final_newline = true

[*.{c,h}]
indent_style = space
indent_size = 4

[Makefile]
indent_style = tab

4-7. 작성한 뒤 Makefile의 명령 줄이 여전히 탭 문자로 시작하는지 확인합니다. 결과가 5여야 합니다.
  grep -c "$(printf '^\t')" Makefile

4-8. JSON 파일 4개가 문법에 맞는지 확인합니다. 파일마다 실행합니다.
  python3 -m json.tool .vscode/tasks.json > /dev/null && echo OK

## 5단계 — 동작 검사

검사에는 임시 파일 src/env_check.c 만 씁니다. 당신이 아래 내용으로 만듭니다.

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>

int main(void) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    printf("env ok, fd = %d\n", fd);
    close(fd);
    return 0;
}

당신이 터미널에서 확인할 수 있는 것과, 화면을 봐야 해서 제가 확인해야 하는 것을 나눕니다. 화면 확인은 무엇을 눌러 무엇을 보면 되는지 한 번에 하나씩 안내하고, 제 답을 받아 적은 뒤 다음으로 넘어갑니다.

| 번호 | 검사 | 누가 확인 | 통과 기준 |
|---|---|---|---|
| 1 | 프로젝트 폴더 열기 | 당신이 code <프로젝트 폴더 경로> 실행, 저는 화면 확인 | VS Code 창이 열리고 왼쪽 탐색기에 src, include, Makefile이 보인다 |
| 2 | 작업 영역 신뢰 | 저 (화면) | "Do you trust the authors" 창이 뜨면 제가 "Yes"를 누른다. 신뢰하지 않으면 작업과 디버깅이 막힌다 |
| 3 | 헤더 인식 | 저 (화면) | src/env_check.c 를 열었을 때 #include 6줄에 빨간 밑줄이 없다 |
| 4 | 자동 완성 | 저 (화면) | 빈 줄에 sock 까지 치면 socket 이 목록에 나온다. 확인 뒤 친 글자를 지운다 |
| 5 | 정의로 이동 | 저 (화면) | socket 위에서 F12 를 누르면 sys/socket.h 의 선언으로 간다 |
| 6 | 빌드 작업 | 저 (화면) | Cmd+Shift+B 를 누르면 make 가 실행되고 경고와 오류가 0개다 |
| 7 | 터미널 빌드 | 당신 | make 뒤 ./build/server 가 env ok, fd = 3 을 출력한다 |
| 8 | 오류 위치 표시 | 저 (화면) | 일부러 세미콜론 하나를 지우고 Cmd+Shift+B 를 누르면 Problems 패널(Cmd+Shift+M)에 줄 번호가 뜬다. 확인 뒤 세미콜론을 되돌린다 |
| 9 | 중단점 디버깅 | 저 (화면) | printf 줄 번호 왼쪽을 눌러 빨간 점을 찍고 F5 를 누르면 그 줄에서 멈추고, 왼쪽 Variables 창에 fd 값이 보인다. F5 로 끝까지 실행한다 |
| 10 | 통합 터미널 | 저 (화면) | Ctrl+` 로 터미널을 열면 현재 폴더가 프로젝트 폴더이고, make 가 실행된다 |
| 11 | Makefile 탭 유지 | 당신 | VS Code에서 Makefile을 열었다 닫은 뒤에도 4-7의 결과가 5다 |

9번에서 "개발자 도구 접근" 암호 창이 뜨면 제가 직접 입력합니다. 당신은 암호를 묻거나 입력하지 않습니다. 디버거가 붙지 못하면 아래 명령으로 상태만 확인해 보고하고, 설정을 바꾸는 명령(sudo 필요)은 실행하지 말고 저에게 알립니다.
  DevToolsSecurity -status

검사가 끝나면 아래를 실행하고, src/ 안에 .gitkeep 만 남았는지 확인합니다.
  make clean
  rm src/env_check.c

## 6단계 — VS Code에서 튜토리얼을 진행하는 방법을 저에게 안내

검사가 끝나면 아래 내용을 짧게 정리해 저에게 알려 줍니다. 이것이 이후 모든 구간의 진행 방식입니다.

- 파일 만들기: 왼쪽 탐색기에서 src 폴더를 고르고 New File 을 눌러 server.c 를 만든다.
- 코드 치기: 제가 직접 칩니다. 저장은 Cmd+S.
- 빌드: Cmd+Shift+B (또는 통합 터미널에서 make).
- 실행: 통합 터미널(Ctrl+`)에서 ./build/server.
- 오류 보기: Problems 패널(Cmd+Shift+M)에서 줄 번호를 누르면 그 줄로 간다.
- 디버깅: 줄 번호 왼쪽에 중단점을 찍고 F5. 한 줄씩 실행은 F10, 함수 안으로 들어가기는 F11, 계속 실행은 F5, 끝내기는 Shift+F5.
- 터미널 두 개 쓰기: 통합 터미널 오른쪽 위의 + 를 누르면 둘째 터미널이 열린다. 하나에서 서버를 띄우고 다른 하나에서 curl 이나 nc 를 실행한다.

## 7단계 — 기록

docs/ENVIRONMENT.md 에 "IDE — Visual Studio Code" 절을 더해 아래를 적습니다. 실제로 확인한 값만 적습니다.
- VS Code 버전과 아키텍처 (code --version)
- 설치되어 있는 확장 기능 ID와 버전 (code --list-extensions --show-versions)
- 이번 작업에서 새로 설치한 것과 실행한 명령
- 이 작업 영역에서 끈 확장 기능 (있다면)
- 5단계 검사 11개의 결과와 누가 확인했는지
- 확인하지 못한 항목과 그 이유

## 8단계 — 완료 조건

아래가 모두 참이면 끝입니다.
1. VS Code가 프로젝트 폴더를 열고, 작업 영역이 신뢰된 상태다.
2. 터미널 명령 code 를 쓸 수 있다.
3. 필수 확장 기능 2개(ms-vscode.cpptools, vadimcn.vscode-lldb)가 설치되어 있다.
4. .vscode/ 의 파일 5개(c_cpp_properties.json, tasks.json, launch.json, settings.json, extensions.json)와 .editorconfig 가 있고, JSON 문법 검사를 통과했다.
5. 5단계 검사 11개가 모두 통과다.
6. Makefile의 탭 줄 수가 5다.
7. src/env_check.c 와 build/ 가 지워졌다.
8. docs/ENVIRONMENT.md 에 IDE 절이 있다.
9. 6단계의 진행 방법을 저에게 안내했다.

## 9단계 — 보고 형식

1. 1단계 점검 표
2. 설치한 것 — 승인받아 실행한 명령 그대로. 없으면 "없음"
3. 확장 기능 표 — 원래 있던 것, 새로 설치한 것, 이 작업 영역에서 끈 것
4. 만든 파일 목록 — 프로젝트 폴더 기준 경로
5. 5단계 검사 표 — 검사마다 "통과", "실패", "확인 못 함"과 확인한 사람(당신 또는 저)
6. 8단계 완료 조건 9개의 결과
7. 남은 문제 — 무엇을 시도했고 어디서 막혔는지

하지 않은 검사를 했다고 쓰지 않습니다. 제가 화면으로 확인해 주지 않은 항목은 "확인 못 함"으로 적습니다.

## 10단계 — 이 작업이 통과된 뒤

모두 통과되면 튜토리얼 1-1구간을 VS Code 안에서 시작합니다. 제가 src/server.c 를 직접 치고, 당신은 한 번에 한 줄씩 안내하고 확인만 합니다. 1-1구간의 목표는 "socket()으로 소켓을 하나 만들고 fd 번호를 출력한 뒤 close()로 닫기"이고, 예상 출력은 listen_fd = 3 입니다. 1-1구간의 자세한 지시는 제가 따로 전달합니다.
