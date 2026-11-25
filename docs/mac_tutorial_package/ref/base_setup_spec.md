# C 웹서버 · 프록시 과제 — macOS 기반 세팅 명세서

작성일: 2026-11-20
받는 쪽: macOS에서 이 과제를 돕는 다른 AI
용도: 학습자가 C 코드를 **직접 한 줄씩** 치기 전에, macOS에 개발 환경과 프로젝트 뼈대를 갖추는 작업의 명세입니다. 이 문서만 읽고도 작업을 끝낼 수 있게 썼습니다.

---

## 0. 먼저 읽을 것 — 이 작업의 경계

| 구분 | 내용 |
|---|---|
| 해야 하는 일 | 도구 설치 여부 확인, 빠진 도구 설치 안내, 프로젝트 폴더 뼈대 · `Makefile` · 점검 스크립트 · 환경 기록 파일 만들기, 완료 조건 검사 |
| 하면 안 되는 일 | **서버 소스 코드(`src/*.c`, `include/*.h`)를 대신 작성하는 일.** 서버 코드는 학습자가 튜토리얼을 따라 직접 칩니다. |
| 사용자에게 먼저 물어야 하는 일 | 무엇이든 설치하는 일(`xcode-select --install`, `brew install`), `sudo`가 필요한 일, 프로젝트 폴더 밖의 파일을 고치는 일, 셸 설정 파일(`~/.zshrc`)을 고치는 일 |
| 답하는 말 | 한국어. 처음 나오는 용어는 쉬운 말로 풀고 영어 원문을 함께 적습니다. |

과제의 제약은 아래와 같습니다. 세팅 단계에서도 이 제약을 깨는 도구를 넣지 않습니다.

1. C 언어와 표준 C 라이브러리, POSIX API만 씁니다.
2. 외부 HTTP 라이브러리 · 프레임워크(libevent, libuv, nginx, Apache, mongoose)를 쓰지 않습니다.
3. `Makefile` 또는 `CMakeLists.txt`를 제공해야 합니다. 이 명세는 `Makefile`을 씁니다.
4. 포트 · 정적 파일 루트 · 라우팅 규칙은 나중에 실행 인자나 설정 파일로 받습니다. 세팅 단계에서는 자리(`www/` 폴더)만 만듭니다.

작업 범위의 근거가 되는 자료는 아래 세 파일입니다. 학습자가 함께 전달합니다.

- `C 웹서버 · 프록시 구현 요구사항 라인 바이 라인 정밀 해석 — … 24개 항목.md` (요구사항 파일)
- `C 웹서버_AI 함께하기 가이드 라인 바이 라인 정밀 해석 — … 31개 항목.md` (가이드 파일)
- `c_webserver_proxy_stack_diagram_1단계.html` (전체 HTTP 메시지 흐름과 소켓 상태 머신 지도)

---

## 1. 완료 조건

아래 일곱 가지가 모두 참이면 세팅이 끝난 것입니다. 하나라도 확인하지 못했으면 "확인 못 함"이라고 그대로 보고합니다.

| 번호 | 조건 | 확인 방법 |
|---|---|---|
| 1 | C 컴파일러와 `make`가 동작한다 | `cc --version`과 `make --version`이 버전을 출력한다 |
| 2 | 소켓 헤더로 컴파일과 실행이 된다 | 4절의 `env_check.c`를 컴파일해 실행하면 `env ok`가 출력된다 |
| 3 | 프로젝트 폴더 뼈대가 3절의 모양과 같다 | `ls -R`로 대조한다 |
| 4 | `make`가 `src/*.c`를 `build/server`로 만든다 | 4절의 점검 절차를 따른다 |
| 5 | 확인 도구가 모두 있다 | `scripts/check_env.sh` 출력에 `MISSING`이 없다 (선택 도구 제외) |
| 6 | 8080 포트가 비어 있다 | 아래 두 가지가 모두 참이어야 한다.<br>`nc -z 127.0.0.1 8080; echo $?`가 `1`을 출력한다 (연결 거부 = 듣는 프로세스 없음).<br>`netstat -an -p tcp \| grep LISTEN \| grep '\.8080 '`이 아무 줄도 출력하지 않는다.<br>`lsof`는 다른 사용자의 프로세스를 보여 주지 않을 수 있어서, `lsof` 출력이 없는 것만으로는 판정하지 않는다. |
| 7 | 환경 기록이 남아 있다 | `docs/ENVIRONMENT.md`에 5절의 값이 적혀 있다 |

---

## 2. 도구 — 확인하고, 없으면 설치를 안내합니다

### 2-1. 필수 도구

| 도구 | 무엇에 쓰나 | 확인 명령 | 없을 때 |
|---|---|---|---|
| Apple clang (`cc`) | C 컴파일 | `cc --version` | `xcode-select --install`로 Xcode Command Line Tools를 설치합니다. 설치 창이 뜨므로 사용자가 직접 승인합니다. |
| `make` | 빌드 | `make --version` | Command Line Tools에 들어 있습니다. |
| `git` | 버전 관리 · 팀 저장소 | `git --version` | Command Line Tools에 들어 있습니다. |
| `lldb` | 디버거 | `lldb --version` | Command Line Tools에 들어 있습니다. |
| `leaks` | 메모리 누수 검사 | `which leaks` | Command Line Tools에 들어 있습니다. |
| `curl` | HTTP 요청 보내기 | `curl --version` | macOS에 기본으로 있습니다. |
| `nc` | TCP 연결을 손으로 열어 보기 | `which nc` | macOS에 기본으로 있습니다. |
| `netstat` | TCP 소켓 상태 보기 | `which netstat` | macOS에 기본으로 있습니다. |
| `lsof` | 포트를 쓰는 프로세스 찾기 | `which lsof` | macOS에 기본으로 있습니다. |
| `python3` | 테스트용 백엔드 서버 띄우기 | `python3 --version` | Command Line Tools에 들어 있습니다. |

Command Line Tools가 깔려 있는지는 `xcode-select -p`로 확인합니다. 경로가 출력되면 깔려 있는 것입니다.

### 2-2. 선택 도구 (부하 테스트 단계에서 필요)

| 도구 | 무엇에 쓰나 | 확인 명령 | 없을 때 |
|---|---|---|---|
| `ab` | 부하 테스트 | `which ab` | macOS에 기본으로 들어 있는 경우가 많습니다. 없으면 사용자에게 알립니다. |
| `wrk` | 부하 테스트 | `which wrk` | Homebrew가 있으면 `brew install wrk`를 제안합니다. |
| `hey` | 부하 테스트 | `which hey` | Homebrew가 있으면 `brew install hey`를 제안합니다. |

Homebrew(`brew`)가 없으면 설치를 강요하지 않습니다. 선택 도구는 8단계(검증)에서 필요하므로, 지금은 "없음"으로 기록만 해도 됩니다.

### 2-3. 버전을 추측하지 않습니다

이 문서에는 도구의 버전 번호를 적지 않았습니다. 실제 Mac에서 명령으로 확인한 값을 `docs/ENVIRONMENT.md`에 적습니다.

---

## 3. 프로젝트 폴더 뼈대

폴더 위치와 이름은 사용자에게 묻습니다. 사용자가 정하지 않으면 `~/jungle/c-webserver-proxy`를 제안합니다. 경로에 공백과 한글이 없는 편이 터미널 작업에 편합니다.

```
c-webserver-proxy/
├── Makefile
├── README.md
├── .gitignore
├── src/            ← 학습자가 직접 치는 C 소스. 비워 둡니다.
├── include/        ← 학습자가 직접 치는 헤더. 비워 둡니다.
├── tests/          ← 단위 테스트. 비워 둡니다.
├── www/
│   └── index.html  ← 정적 파일 서빙을 시험할 파일
├── scripts/
│   └── check_env.sh
└── docs/
    └── ENVIRONMENT.md
```

빈 폴더는 git에 올라가지 않으므로 `src/`, `include/`, `tests/`에는 빈 파일 `.gitkeep`을 하나씩 둡니다.

### 3-1. `.gitignore`

```
build/
*.o
*.dSYM/
.DS_Store
```

### 3-2. `www/index.html`

```html
<!doctype html>
<html lang="ko">
<head><meta charset="utf-8"><title>c-webserver-proxy</title></head>
<body><h1>정적 파일 서빙 시험 페이지</h1></body>
</html>
```

### 3-3. `README.md`

제목과 빌드 방법 두 줄만 적습니다. 발표용 README는 학습자가 코드를 완성한 뒤 직접 씁니다.

```markdown
# c-webserver-proxy

빌드: `make` → 실행 파일은 `build/server`
```

---

## 4. `Makefile`과 동작 점검

### 4-1. `Makefile`

아래 내용을 그대로 씁니다. **들여쓰기는 반드시 탭 문자입니다.** 공백으로 바꾸면 `make`가 `missing separator` 오류를 냅니다.

```make
CC      := cc
CSTD    := -std=c11
WARN    := -Wall -Wextra -Wpedantic
CFLAGS  := $(CSTD) $(WARN) -g -O0 -Iinclude
LDFLAGS :=

SRC := $(wildcard src/*.c)
OBJ := $(patsubst src/%.c,build/%.o,$(SRC))
BIN := build/server

.PHONY: all asan run clean

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o $@

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build:
	mkdir -p build

asan: CFLAGS  += -fsanitize=address,undefined -fno-omit-frame-pointer
asan: LDFLAGS += -fsanitize=address,undefined
asan: clean all

run: all
	./$(BIN)

clean:
	rm -rf build
```

이 `Makefile`에 대해 알아 둘 점입니다.

- `make`는 `src/` 안의 모든 `.c` 파일을 컴파일해 `build/server` 하나로 묶습니다. 파일이 늘어도 `Makefile`을 고칠 필요가 없습니다.
- `make asan`은 주소 검사(AddressSanitizer)와 정의되지 않은 동작 검사(UndefinedBehaviorSanitizer)를 켠 빌드입니다. 버퍼를 넘겨 쓰거나 해제한 메모리를 쓰면 실행 중에 바로 알려 줍니다.
- macOS에서는 pthread 함수가 기본 시스템 라이브러리에 들어 있어서 `-pthread`를 넣지 않았습니다. 나중에 Linux에서도 빌드하려면 `CFLAGS`와 `LDFLAGS`에 `-pthread`를 더합니다.
- `_POSIX_C_SOURCE`를 정의하지 않았습니다. macOS에서 이 매크로를 정의하면 `SO_NOSIGPIPE` 같은 macOS 전용 선언이 헤더에서 가려집니다.
- `src/`가 비어 있을 때 `make`를 실행하면 링크할 파일이 없어 실패합니다. 이것은 정상입니다.

### 4-2. 동작 점검 — 임시 파일로만 합니다

서버 코드를 대신 쓰지 않기 위해, 점검에는 아래 임시 파일만 씁니다. 점검이 끝나면 반드시 지웁니다.

1. `src/env_check.c`를 아래 내용으로 만듭니다.

```c
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>

int main(void) {
    printf("env ok\n");
    return 0;
}
```

2. `make`를 실행합니다. 경고와 오류가 0개여야 합니다.
3. `./build/server`를 실행합니다. `env ok`가 출력되어야 합니다.
4. `make asan`을 실행하고 `./build/server`를 다시 실행합니다. `env ok`가 출력되어야 합니다.
5. `make clean`을 실행하고 `src/env_check.c`를 지웁니다. `src/`에는 `.gitkeep`만 남아야 합니다.

### 4-3. `scripts/check_env.sh`

아래 내용으로 만들고 `chmod +x scripts/check_env.sh`로 실행 권한을 줍니다.

```sh
#!/bin/sh
# macOS 개발 환경 점검. 아무것도 설치하거나 고치지 않고 출력만 합니다.

echo "== 시스템 =="
sw_vers
echo "CPU: $(uname -m)"

echo "== 필수 도구 =="
for t in cc make git lldb leaks curl nc netstat lsof python3; do
  if command -v "$t" >/dev/null 2>&1; then
    echo "OK       $t  ($(command -v "$t"))"
  else
    echo "MISSING  $t"
  fi
done

echo "== 선택 도구 =="
for t in ab wrk hey brew; do
  if command -v "$t" >/dev/null 2>&1; then
    echo "OK       $t  ($(command -v "$t"))"
  else
    echo "없음     $t"
  fi
done

echo "== 버전 =="
cc --version | head -n 1
make --version | head -n 1
git --version

echo "== 한도와 커널 값 =="
echo "열 수 있는 fd 수 (ulimit -n): $(ulimit -n)"
sysctl kern.ipc.somaxconn
sysctl net.inet.tcp.msl

echo "== 8080 포트 =="
if nc -z 127.0.0.1 8080 >/dev/null 2>&1; then
  echo "사용 중 (연결됨):"
  lsof -nP -iTCP:8080 -sTCP:LISTEN
elif netstat -an -p tcp | grep LISTEN | grep -q '\.8080 '; then
  echo "사용 중 (LISTEN 줄 있음):"
  netstat -an -p tcp | grep LISTEN | grep '\.8080 '
else
  echo "비어 있음 (연결 거부 + LISTEN 줄 없음)"
fi
```

`sysctl` 두 줄이 권한 오류로 실패하면, AI의 실행 환경(샌드박스)이 막은 것일 수 있습니다. 그때는 값을 추측하지 말고 사용자에게 자기 터미널에서 `sysctl kern.ipc.somaxconn net.inet.tcp.msl`을 실행해 달라고 요청합니다. 이 두 값은 참고용이라 조회하지 못해도 세팅 완료를 막지 않습니다. `docs/ENVIRONMENT.md`에는 "조회 못 함"이라고 적습니다.

---

## 5. `docs/ENVIRONMENT.md`에 적을 것

`scripts/check_env.sh`의 출력을 바탕으로 아래 항목을 채웁니다. 값은 실제로 확인한 것만 적습니다.

| 항목 | 어디서 얻나 |
|---|---|
| 확인한 날짜 | 작업한 날 |
| macOS 버전 | `sw_vers` |
| CPU 종류 (`arm64` 또는 `x86_64`) | `uname -m` |
| 컴파일러 버전 | `cc --version` 첫 줄 |
| `make` 버전 | `make --version` 첫 줄 |
| 열 수 있는 fd 수 | `ulimit -n` |
| 연결 대기 큐 한도 | `sysctl kern.ipc.somaxconn` |
| MSL 값 | `sysctl net.inet.tcp.msl` |
| 없는 도구 | 점검 스크립트에서 `MISSING` 또는 `없음`으로 나온 것 |

---

## 6. macOS에서 주의할 점 — Linux 자료와 다른 곳

학습 자료와 인터넷 예제는 대부분 Linux 기준입니다. 아래 차이를 알고 있어야 학습자가 막혔을 때 바로 짚어 줄 수 있습니다. 세팅 단계에서 고칠 것은 없고, 이후 단계에서 참고합니다.

| Linux 자료에 나오는 것 | macOS에서는 | 이 과제에서 대신 쓰는 것 |
|---|---|---|
| `epoll` | 없습니다. | 스레드 풀(pthread)을 쓰고, 기다림이 필요하면 POSIX의 `poll()`을 씁니다. macOS의 `kqueue`는 POSIX가 아니어서 과제 제약에 맞지 않습니다. |
| `accept4()`, `SOCK_NONBLOCK`, `SOCK_CLOEXEC` | 없습니다. | `accept()` 뒤에 `fcntl()`로 옵션을 붙입니다. |
| `MSG_NOSIGNAL` | 쓸 수 있다고 가정하지 않습니다. | `signal(SIGPIPE, SIG_IGN)`과 소켓 옵션 `SO_NOSIGPIPE`를 씁니다. |
| `sendfile()` | 인자 모양이 Linux와 다릅니다. | `read()`와 `send()` 반복으로 구현합니다. |
| `sem_init()` (이름 없는 세마포어) | 지원하지 않습니다. | `pthread_mutex_t`와 `pthread_cond_t`를 씁니다. |
| `ss -ltnp` | 없습니다. | `lsof -nP -iTCP:8080 -sTCP:LISTEN`과 `netstat -an -p tcp`를 씁니다. |
| `valgrind` | 최신 macOS와 Apple Silicon에서 쓸 수 없는 경우가 많습니다. | `make asan`으로 메모리 접근 오류를 잡고, 누수는 `leaks --atExit -- ./build/server`로 봅니다. `leaks`는 `make asan` 빌드가 아닌 일반 빌드에 씁니다. |
| `strace` | 없습니다. | 필요하면 `dtruss`를 쓰지만 시스템 보호 설정 때문에 제한이 있습니다. 먼저 `lldb`와 로그 출력으로 봅니다. |
| `<endian.h>` | 없습니다. | 이 과제는 `<arpa/inet.h>`의 `htons` · `htonl` · `ntohs`만 쓰므로 필요 없습니다. |

실행할 때 알아 둘 점입니다.

- **포트는 8080을 씁니다.** 1024보다 작은 포트는 관리자 권한이 필요합니다. 5000번과 7000번은 macOS의 AirPlay 수신 기능이 쓰고 있을 수 있어 피합니다.
- **개발 중에는 `127.0.0.1`에 bind합니다.** 모든 주소(`0.0.0.0`)에 bind하면 macOS 방화벽이 켜져 있을 때 허용 창이 뜰 수 있습니다.
- **fd 한도가 낮습니다.** `ulimit -n`의 기본값이 작아서 동시 연결이 많은 부하 테스트에서 `accept()`가 실패할 수 있습니다. 그때는 테스트하는 터미널에서만 `ulimit -n 4096`을 실행합니다. 셸 설정 파일은 고치지 않습니다.
- **테스트용 백엔드**는 `python3 -m http.server 8001`처럼 포트만 바꿔 여러 개 띄웁니다.

---

## 7. 세팅이 끝난 뒤 이어질 일

세팅이 끝나면 학습자는 튜토리얼 1-1구간부터 코드를 직접 칩니다. 그 흐름과 맞추기 위해 아래를 지킵니다.

- 첫 소스 파일은 `src/server.c`입니다. 학습자가 만듭니다.
- 빌드는 `make`, 실행은 `./build/server`입니다.
- 1-1구간의 내용은 "`socket()`으로 소켓을 하나 만들고 fd 번호를 출력한 뒤 `close()`로 닫기"이고, 예상 출력은 `listen_fd = 3`입니다.
- 학습자가 코드에 대해 물으면 답을 대신 써 주지 말고, 어느 줄이 왜 틀렸는지와 고칠 방향을 설명합니다. 학습자가 직접 고칩니다.

---

## 8. 보고 형식

작업을 마치면 아래 순서로 보고합니다.

1. **완료 조건 일곱 가지의 결과.** 조건마다 "통과", "실패", "확인 못 함" 중 하나와 근거가 된 명령 출력을 적습니다.
2. **만든 파일 목록.** 프로젝트 폴더를 기준으로 한 경로로 적습니다.
3. **설치한 것.** 사용자 승인을 받아 설치한 도구와 그 명령을 적습니다. 설치하지 않았으면 "없음"이라고 적습니다.
4. **없는 도구.** 선택 도구 중 없는 것과, 그것이 필요한 단계를 적습니다.
5. **남은 문제.** 해결하지 못한 것이 있으면 무엇을 시도했고 어디서 막혔는지 적습니다.

하지 않은 검사를 했다고 쓰지 않습니다. 임시 파일 `src/env_check.c`를 지웠는지도 보고에 포함합니다.
