# 2 ~ 9단계 전체 설계서 — C 웹서버 · 프록시 튜토리얼의 나머지 전 구간

이 문서는 튜터 역할을 하는 AI가 **전체 길을 미리 알도록** 주는 설계서입니다. 진행 규칙은 `01_tutorial_workorder.md`를 따릅니다.

---

## 0. 이 문서를 쓰는 법

### 0-1. 1단계 대본과 무엇이 다른가

`02_stage1_sections.md`는 칠 줄이 한 줄씩 적혀 있는 **대본**입니다. 이 문서는 구간마다 목표 · 만들 파일 · 함수 모양 · 꼭 지킬 동작 · 실험 · 질문을 적은 **설계서**이고, 칠 줄 자체는 적혀 있지 않습니다.

그래서 2단계부터는 구간마다 당신이 아래 순서로 준비합니다.

1. 이 문서의 그 구간 절과, `ref/requirements_24.md`에서 "근거 항목"으로 적힌 항목을 읽습니다.
2. 그 구간이 끝났을 때의 코드를 `build/_ref/<단계>-<구간>/` 아래에 **참고 구현**으로 만듭니다. `src/`와 `include/`에는 만들지 않습니다.
3. 참고 구현을 `cc -std=c11 -Wall -Wextra -Wpedantic -g -O0`로 컴파일해 경고 0개인지 확인하고, 그 구간의 실험을 직접 돌려 봅니다. `-fsanitize=address,undefined`로도 한 번 돌립니다.
4. 참고 구현을 1 ~ 3줄 단위로 잘라, `01_tutorial_workorder.md` 4절의 형식으로 저에게 안내합니다. 저는 그 줄을 `src/` · `include/`에 직접 칩니다.
5. 참고 구현 전체를 저에게 보여 주지 않습니다.

참고 구현은 이 문서의 함수 모양과 "꼭 지킬 동작"을 따릅니다. 더 나은 설계가 있다고 판단하면 조용히 바꾸지 말고, 무엇을 왜 바꾸고 싶은지 저에게 말하고 결정을 기다립니다.

### 0-2. 구간을 시작할 때 저에게 먼저 보여 줄 것

구간마다 줄 단위 입력에 들어가기 전에, 아래를 한 화면으로 보여 주고 제가 "시작"이라고 하면 들어갑니다.

- 이 구간의 목표 한 문장
- 새로 만드는 파일과 고치는 파일
- 새로 치는 줄의 대략적인 수
- 이 구간이 끝나면 할 수 있게 되는 실험

### 0-3. 범위

- 작업 범위는 `ref/requirements_24.md`(24개 항목)와 `ref/guide_31.md`(31개 항목)입니다.
- 이 문서에 "선택"이라고 적힌 구간은 제가 하겠다고 말할 때만 합니다.
- 완성된 외부 라이브러리(libevent, libuv, mongoose)는 쓰지 않습니다. 표준 C 라이브러리와 POSIX(**P**ortable **O**perating **S**ystem **I**nterface) API(**A**pplication **P**rogramming **I**nterface)만 씁니다. (근거 항목 11)
- 두 범위 파일의 "확인" 줄에 적힌 수치(예: 초당 요청 수, 메모리 크기)는 해석을 쓴 쪽이 적은 값이고 측정값이 아닙니다. 목표나 사실로 말하지 않습니다.

### 0-4. macOS에서 항상 지킬 것

| 주제 | macOS에서 하는 방법 |
|---|---|
| 이벤트 통지 | `epoll`은 없습니다. `kqueue`는 POSIX가 아닙니다. 이 과제에서는 `pthread` 스레드 풀과 `poll()`을 씁니다. |
| 세마포어 | 이름 없는 세마포어 `sem_init`은 동작하지 않습니다. `pthread_mutex_t`와 `pthread_cond_t`를 씁니다. |
| `SIGPIPE` | `MSG_NOSIGNAL`에 기대지 않습니다. `signal(SIGPIPE, SIG_IGN)`과 연결 소켓의 `SO_NOSIGPIPE` 옵션을 씁니다. |
| 비차단 소켓 | `accept4`와 `SOCK_NONBLOCK`은 없습니다. `fcntl(fd, F_SETFL, O_NONBLOCK)`을 씁니다. |
| `sendfile` | Linux와 인자 모양이 다릅니다. 이 과제에서는 `read()`와 `send()` 반복으로 보냅니다. |
| 기능 매크로 | `_POSIX_C_SOURCE`를 정의하지 않습니다. 정의하면 macOS 헤더가 일부 선언을 숨깁니다. |
| 포트 | 5000과 7000은 AirPlay가 쓸 수 있습니다. 서버는 8080, 시험용 백엔드는 8001과 8002를 씁니다. |
| 주소 | 개발 중에는 127.0.0.1에만 붙입니다. |
| 메모리 검사 | `valgrind`는 쓰지 않습니다. `make asan`과 `leaks`를 씁니다. |
| 관찰 도구 | `ss`와 `strace`는 없습니다. `netstat`, `lsof`, `lldb`를 씁니다. |

### 0-5. 최종 파일 구성 (9단계가 끝났을 때)

| 파일 | 맡는 일 | 처음 만드는 구간 |
|---|---|---|
| `src/main.c` | 명령줄 인자 읽기, 시그널 설치, 듣는 소켓 열기, `accept()` 반복 | 2-1 (`server.c`에서 이름 변경) |
| `src/net.c` · `include/net.h` | 듣는 소켓 열기, 업스트림 연결, 끝까지 보내기, 시간 제한 | 2-1 |
| `src/conn.c` · `include/conn.h` | 연결 하나의 상태 머신 | 2-2 |
| `src/http.c` · `include/http.h` | HTTP 요청 파서 | 3-1 |
| `src/config.c` · `include/config.h` | 명령줄 인자와 설정 | 4-1 |
| `src/response.c` · `include/response.h` | 응답 조립, 오류 응답 | 5-1 |
| `src/mime.c` · `include/mime.h` | 확장자 → `Content-Type` | 5-2 |
| `src/static.c` · `include/static.h` | 정적 파일 보내기 | 5-3 |
| `src/cgi.c` · `include/cgi.h` | CGI 프로그램 실행 | 5-4 |
| `src/pool.c` · `include/pool.h` | 스레드 풀 | 6-2 |
| `src/route.c` · `include/route.h` | 리버스 프록시 라우팅 규칙, 라운드 로빈 | 7-2 |
| `src/proxy.c` · `include/proxy.h` | 요청 고쳐 쓰기, 업스트림 중계, 포워드 프록시 | 7-3 |
| `tests/test_http.c` · `tests/test_mime.c` · `tests/test_route.c` | 모듈 단위 테스트 | 3-5, 5-2, 7-2 |

`tests/*.c`도 제가 직접 칩니다. `Makefile`, `www/` 안의 시험용 파일, `scripts/`, `docs/`는 당신이 고쳐도 됩니다. `Makefile`을 고칠 때는 무엇을 왜 고치는지 먼저 말합니다.

---

## 2단계 — 연결 하나의 애플리케이션 상태 머신

1단계 지도의 TCP 상태(커널이 관리)와 **구분되는**, 제 프로그램이 관리하는 연결 상태를 만듭니다.

근거 항목: 2, 3, 13, 14.

### 2-1. 파일 나누기

- 목표: 1단계의 `main` 하나를 "듣는 소켓 열기", "연결 하나 처리하기", "반복"으로 나눕니다.
- 할 일:
  - 제가 `git mv src/server.c src/main.c`를 실행합니다.
  - `include/net.h` · `src/net.c`: `int net_listen(const char *host, int port, int backlog);` 성공하면 듣는 소켓 번호, 실패하면 -1을 돌려줍니다.
  - `include/conn.h` · `src/conn.c`: `void conn_handle(int fd);` 연결 하나를 처리하고 닫습니다.
  - 헤더마다 포함 가드(include guard) `#ifndef NET_H` · `#define NET_H` · `#endif`를 넣습니다.
- 꼭 지킬 동작: 동작은 1단계와 똑같습니다. `curl`의 출력이 달라지지 않아야 합니다.
- 실험: `make` 뒤 1단계 실험 1-5-가와 1-6-가를 다시 돌려 같은 결과인지 봅니다.
- 질문: "헤더 파일에 함수의 몸통이 아니라 선언만 두는 이유는 무엇인가요?", "포함 가드가 없으면 어떤 오류가 나나요?"

### 2-2. 상태를 이름으로 만들기

- 목표: 연결 하나가 거치는 단계를 `enum`으로 적고, 단계가 바뀔 때마다 기록을 남깁니다.
- 자료 구조:

```c
enum conn_state {
    CONN_READING,
    CONN_PARSING,
    CONN_ROUTING,
    CONN_RESPONDING,
    CONN_KEEPALIVE_CHECK,
    CONN_CLOSING,
    CONN_CLOSED
};

struct conn {
    int fd;
    enum conn_state state;
    char rbuf[8192];
    size_t rlen;
};
```

- 꼭 지킬 동작: `conn_handle`은 `while (c.state != CONN_CLOSED)` 안의 `switch (c.state)`로 움직입니다. 상태가 바뀔 때 표준 오류(stderr)에 `fd 4: READING -> PARSING` 모양으로 한 줄을 출력합니다.
- 실험: `curl` 한 번에 상태 기록이 어떤 순서로 찍히는지 봅니다.
- 질문: "TCP 상태 ESTABLISHED와 제 프로그램의 상태 `CONN_READING`은 누가 관리하나요?" — ESTABLISHED는 커널, `CONN_READING`은 제 프로그램입니다.

### 2-3. 빈 줄이 올 때까지 읽기

- 목표: `recv()` 한 번이 아니라, 헤더의 끝 `\r\n\r\n`이 버퍼에 나타날 때까지 반복해서 읽습니다.
- 꼭 지킬 동작:
  - `rbuf`에 이어 붙이며 읽습니다. 읽을 때마다 `rlen`을 늘립니다.
  - `recv()`가 0을 돌려주면 `CONN_CLOSING`으로 갑니다.
  - 8192바이트를 다 채우도록 빈 줄이 없으면 요청이 너무 큰 것으로 보고 `CONN_CLOSING`으로 갑니다. 알맞은 오류 응답(431)은 5-1구간에서 붙입니다.
  - 버퍼 끝을 넘어 쓰지 않습니다. `recv()`의 길이 인자는 항상 `sizeof(rbuf) - rlen - 1` 이하입니다.
- 실험:
  - 쪼개 보내기: `(printf 'GET / HT'; sleep 1; printf 'TP/1.1\r\nHost: x\r\n\r\n') | nc 127.0.0.1 8080` — 1단계 코드에서는 앞 조각만 보고 응답했지만, 이제 요청 전체를 받은 뒤 응답하는지 봅니다.
  - 빈 줄 없이 보내기: `printf 'GET / HTTP/1.1\r\nHost: x\r\n' | nc 127.0.0.1 8080` — 서버가 무엇을 하는지 봅니다. (`nc`가 입력이 끝나면 연결을 닫는지에 따라 결과가 달라집니다. 실제로 본 대로 기록합니다.)
- 질문: "`recv()`가 요청의 중간에서 끊어 돌려줄 수 있는 이유는 무엇인가요?", "`rlen`을 검사하지 않으면 어떤 버그가 생기나요?"

### 2-4. 끝까지 보내기와 연결 유지의 뼈대

- 목표: 1-5구간의 `send()` 반복을 `net.c`의 함수로 옮기고, 응답 뒤에 "연결을 유지할지"를 정하는 자리를 만듭니다.
- 함수: `int net_send_all(int fd, const void *buf, size_t len);` 다 보내면 0, 실패하면 -1을 돌려줍니다.
- 꼭 지킬 동작: `CONN_KEEPALIVE_CHECK` 상태에서 지금은 항상 `CONN_CLOSING`으로 갑니다. 유지할지 정하는 규칙은 3-4구간에서 넣습니다.
- 질문: "`net_send_all`이 -1을 돌려주면 `conn_handle`은 무엇을 해야 하나요?" — 응답을 더 보내려 하지 않고 연결을 닫습니다.

### 2단계 완료 조건

- `make`와 `make asan`이 경고 0개로 끝납니다.
- 쪼개 보낸 요청에 응답합니다.
- 상태 기록이 `READING → PARSING → ROUTING → RESPONDING → KEEPALIVE_CHECK → CLOSING → CLOSED` 순서로 찍힙니다.

---

## 3단계 — HTTP 요청 파싱

근거 항목: 3, 14, 17, 19, 23.

### 3-1. 요청 줄

- 목표: `GET /index.html HTTP/1.1`을 메서드 · 대상 · 버전으로 나눕니다.
- 자료 구조와 함수:

```c
enum http_result {
    HTTP_OK,
    HTTP_INCOMPLETE,
    HTTP_BAD_REQUEST,
    HTTP_URI_TOO_LONG,
    HTTP_HEADER_TOO_LARGE,
    HTTP_NOT_IMPLEMENTED,
    HTTP_VERSION_NOT_SUPPORTED,
    HTTP_PAYLOAD_TOO_LARGE
};

struct http_header {
    char name[64];
    char value[1024];
};

struct http_request {
    char method[8];
    char target[2048];
    char version[16];
    struct http_header headers[32];
    int nheaders;
    long content_length;
    int keep_alive;
    size_t header_len;
};

enum http_result http_parse(const char *buf, size_t len, struct http_request *req);
const char *http_header_get(const struct http_request *req, const char *name);
```

- 꼭 지킬 동작:
  - 입력 버퍼를 고치지 않습니다(`const char *`). `strtok`을 쓰지 않습니다.
  - 빈 줄이 아직 없으면 `HTTP_INCOMPLETE`를 돌려줍니다.
  - 공백이 정확히 두 개가 아니거나 칸이 비어 있으면 `HTTP_BAD_REQUEST`입니다.
  - 대상이 2047바이트를 넘으면 `HTTP_URI_TOO_LONG`입니다.
  - 버전이 `HTTP/1.1`도 `HTTP/1.0`도 아니면 `HTTP_VERSION_NOT_SUPPORTED`입니다.
  - 메서드가 `GET`도 `POST`도 아니면 `HTTP_NOT_IMPLEMENTED`입니다. (요구사항은 GET과 POST입니다.)
  - 모든 복사는 길이를 확인한 뒤 합니다. `strcpy`와 `sprintf`를 쓰지 않고 `memcpy`와 `snprintf`를 씁니다.
- 실험: `printf 'GET /a HTTP/1.1\r\n\r\n' | nc 127.0.0.1 8080`, `printf 'HELLO\r\n\r\n' | nc 127.0.0.1 8080`으로 결과 값을 기록에 찍어 봅니다.

### 3-2. 헤더

- 목표: `이름: 값` 줄들을 `headers` 배열에 담습니다.
- 꼭 지킬 동작:
  - 이름은 대소문자를 구분하지 않고 찾습니다. `<strings.h>`의 `strcasecmp`를 씁니다.
  - 값의 앞뒤 공백과 탭을 떼어 냅니다.
  - 콜론이 없는 줄, 이름이 빈 줄은 `HTTP_BAD_REQUEST`입니다.
  - 헤더가 32개를 넘거나 한 줄이 칸보다 길면 `HTTP_HEADER_TOO_LARGE`입니다.
  - `HTTP/1.1` 요청에 `Host` 헤더가 없으면 `HTTP_BAD_REQUEST`입니다.
  - `header_len`에 요청 줄부터 빈 줄 끝까지의 바이트 수를 적습니다.
- 실험: `curl -H 'X-Test:   hi  ' -v http://127.0.0.1:8080/`으로 값의 공백이 떨어졌는지 기록에 찍어 봅니다.

### 3-3. `Content-Length`와 본문

- 목표: POST 요청의 본문을 정확한 바이트 수만큼 읽습니다.
- 꼭 지킬 동작:
  - `Content-Length` 값은 `strtol`로 바꾸고, 숫자가 아닌 글자 · 음수 · 넘침은 `HTTP_BAD_REQUEST`입니다.
  - 본문 상한을 상수로 둡니다(예: `MAX_BODY` 1048576). 넘으면 `HTTP_PAYLOAD_TOO_LARGE`입니다.
  - 요청에 `Transfer-Encoding` 헤더가 있으면 `HTTP_NOT_IMPLEMENTED`입니다. 청크 방식 본문은 이 과제의 최소 범위(경계 식별 · `Content-Length` · keep-alive) 밖입니다.
  - 본문은 `malloc`으로 잡고, 연결 처리가 어떤 경로로 끝나든 `free`합니다. (근거 항목 17)
  - 헤더를 읽을 때 본문의 앞부분이 `rbuf`에 함께 들어와 있을 수 있습니다. 그만큼을 먼저 옮긴 뒤 나머지를 `recv()`로 읽습니다.
- 실험:
  - `curl -d 'a=1&b=2' http://127.0.0.1:8080/echo` — 받은 본문 길이가 7인지 봅니다.
  - `head -c 2000000 /dev/zero | curl --data-binary @- http://127.0.0.1:8080/echo` — 상한을 넘을 때의 반응을 봅니다.
- 질문: "헤더와 본문의 경계는 무엇인가요?", "`Content-Length`를 믿고 읽는데 클라이언트가 그보다 적게 보내고 멈추면 어떻게 되나요?" — 지금은 `recv()`에서 끝없이 기다립니다. 4-3구간의 시간 제한이 이 문제를 풉니다.

### 3-4. 연결 유지(keep-alive) 결정

- 목표: 응답 뒤에 연결을 닫을지 다음 요청을 기다릴지 정합니다.
- 꼭 지킬 동작:
  - `HTTP/1.1`은 기본이 유지입니다. `Connection: close`가 있으면 닫습니다.
  - `HTTP/1.0`은 기본이 닫기입니다. `Connection: keep-alive`가 있으면 유지합니다.
  - 오류 응답(4xx · 5xx)을 보낸 뒤에는 닫습니다.
  - 유지할 때는 `rbuf`에서 방금 처리한 요청의 바이트를 빼고, 뒤에 남은 바이트(다음 요청의 앞부분)를 버퍼 앞으로 당깁니다(`memmove`).
  - 응답의 `Connection` 헤더가 실제 동작과 같아야 합니다.
- 실험:
  - `curl -v http://127.0.0.1:8080/ http://127.0.0.1:8080/` — `curl`이 둘째 요청에서 연결을 다시 썼다는 줄(`Re-using existing connection`)이 나오는지 봅니다.
  - `curl -v -H 'Connection: close' http://127.0.0.1:8080/`
  - 요청 두 개를 한 번에 보내기: `printf 'GET /a HTTP/1.1\r\nHost: x\r\n\r\nGET /b HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n' | nc 127.0.0.1 8080` — 응답이 두 개 오는지 봅니다.
- 질문: "연결을 유지하면 무엇이 절약되나요?" — 요청마다 하던 연결 맺기와 끊기입니다. 1단계 지도의 "⑦ keep-alive면 ②로 · 아니면 close()"가 이 결정입니다.

### 3-5. 파서 단위 테스트

- 목표: 서버를 띄우지 않고 `http_parse`만 검사합니다. (근거 항목 19)
- 할 일:
  - 제가 `tests/test_http.c`를 칩니다. `<assert.h>`의 `assert`를 씁니다.
  - 당신이 `Makefile`에 `test` 대상을 더합니다. `tests/test_<이름>.c`를 `main.c`를 뺀 `src/*.c`와 함께 링크해 `build/test_<이름>`을 만들고 차례로 실행합니다. 고치기 전에 바뀌는 줄을 저에게 보여 줍니다.
- 검사할 경우(최소): 정상 GET, 정상 POST와 `Content-Length`, 빈 줄이 없는 입력(`HTTP_INCOMPLETE`), 공백이 하나뿐인 요청 줄, 지원하지 않는 메서드, 지원하지 않는 버전, 2047바이트를 넘는 대상, 33개 헤더, 콜론이 없는 헤더, 숫자가 아닌 `Content-Length`, `Host`가 없는 `HTTP/1.1`, 대소문자가 섞인 헤더 이름 찾기.
- 완료 조건: `make test`가 모두 통과하고, `-fsanitize=address,undefined`로 지어도 통과합니다.

---

## 4단계 — 시스템 콜과 소켓 옵션

근거 항목: 2, 11, 13, 17, 18, 23.

### 4-1. 명령줄 인자

- 목표: 포트 · 정적 파일 루트 · 대기열 길이 · 일꾼 수를 코드에 박지 않고 실행할 때 받습니다. (근거 항목 11)
- 자료 구조와 함수:

```c
struct config {
    const char *host;
    int port;
    int backlog;
    const char *root;
    int workers;
    int timeout_sec;
    const char *routes_path;
    int forward_proxy;
};

int config_parse(int argc, char **argv, struct config *cfg);
```

- 꼭 지킬 동작: `<unistd.h>`의 `getopt`를 씁니다. `-p 포트`, `-r 루트`, `-b 대기열`, `-w 일꾼 수`, `-t 초`, `-c 라우팅 규칙 파일`, `-f`(포워드 프록시 켜기), `-h`(사용법). 기본값은 127.0.0.1, 8080, 128, `./www`, 8, 5입니다. 포트는 `strtol`로 바꾸고 1 ~ 65535가 아니면 사용법을 출력하고 끝냅니다.
- 실험: `./build/server -p 9090`, `./build/server -p abc`, `./build/server -h`.

### 4-2. `SIGPIPE`와 `SO_NOSIGPIPE`

- 목표: 클라이언트가 갑자기 끊어도 서버가 죽지 않게 합니다. (근거 항목 18, 23)
- 꼭 지킬 동작:
  - `signal(SIGPIPE, SIG_IGN)`은 유지합니다.
  - `accept()`로 받은 연결 소켓마다 `setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on))`을 부릅니다. `SO_NOSIGPIPE`는 macOS에만 있으므로 `#ifdef SO_NOSIGPIPE`로 감쌉니다. Linux에서도 같은 코드가 컴파일되게 하기 위해서입니다.
  - `send()`가 -1이고 `errno`가 `EPIPE` 또는 `ECONNRESET`이면 조용히 연결을 닫습니다.
- 실험:
  - `signal(SIGPIPE, SIG_IGN)`과 `SO_NOSIGPIPE`를 잠시 막고, 응답 전에 `sleep(3)`을 넣고, `nc`로 요청을 보낸 뒤 바로 `Ctrl+C` — 서버가 죽는지 봅니다. `echo $?`가 141이면 `SIGPIPE`(13번)로 죽은 것입니다.
  - 다시 켜고 같은 실험을 해서 서버가 살아 있는지 봅니다.
  - 실험 뒤 `sleep(3)`을 지웁니다.

### 4-3. 시간 제한

- 목표: 아무것도 보내지 않는 연결이 일꾼을 영원히 붙잡지 못하게 합니다. (근거 항목 23)
- 꼭 지킬 동작:
  - 연결 소켓에 `SO_RCVTIMEO`와 `SO_SNDTIMEO`를 `struct timeval`로 겁니다.
  - 시간이 지나면 `recv()`가 -1을 돌려주고 `errno`가 `EAGAIN` 또는 `EWOULDBLOCK`이 됩니다. 요청을 읽는 중이었으면 408 응답을 보내고 닫습니다. 유지 중인 연결에서 다음 요청을 기다리던 중이었으면 응답 없이 닫습니다.
- 실험: `nc 127.0.0.1 8080`을 띄우고 아무것도 치지 않은 채 시간을 잽니다. `time nc 127.0.0.1 8080 < /dev/null`도 봅니다.
- 질문: "시간 제한이 없으면 1-6-마 실험과 같은 일을 악의적으로 만들 수 있습니다. 어떻게 하면 되나요?" — 연결만 맺고 아무것도 보내지 않는 클라이언트를 일꾼 수만큼 띄웁니다.

### 4-4. `EINTR`과 안전한 종료

- 목표: `Ctrl+C`(`SIGINT`)와 `SIGTERM`을 받으면 새 연결을 그만 받고, 하던 일을 끝낸 뒤 깨끗하게 끝냅니다. (근거 항목 18)
- 꼭 지킬 동작:
  - `sigaction`으로 처리 함수를 답니다. `sa_flags`에 `SA_RESTART`를 넣지 않아, 잠들어 있던 `accept()`가 -1과 `EINTR`로 돌아오게 합니다.
  - 처리 함수 안에서는 `volatile sig_atomic_t g_stop = 1;` 하나만 합니다. `printf`와 `malloc`을 부르지 않습니다. 시그널 처리 함수 안에서 불러도 안전한 함수(async-signal-safe function)가 아니기 때문입니다.
  - `accept()` 반복은 `while (!g_stop)`이 됩니다. `EINTR`이면 조건을 다시 봅니다.
  - `recv()`와 `send()`가 `EINTR`로 돌아오면 다시 시도합니다.
  - 끝날 때 듣는 소켓을 닫고 `main`이 0을 돌려줍니다.
- 실험: 서버를 띄우고 `Ctrl+C` — "종료" 기록이 찍히고 `echo $?`가 0인지 봅니다. `kill -TERM <pid>`도 봅니다.

### 4-5. 차단과 비차단, `poll()`

- 목표: 소켓의 차단(blocking)과 비차단(non-blocking)의 차이를 직접 보고, 이후 단계에서 쓸 `poll()`을 익힙니다. (근거 항목 13)
- 꼭 지킬 동작:
  - `int net_set_nonblock(int fd);` — `fcntl(fd, F_GETFL)`로 읽고 `O_NONBLOCK`을 더해 `F_SETFL`로 씁니다.
  - `int net_wait(int fd, short events, int timeout_ms);` — `poll()` 한 번을 감싼 함수입니다. 준비되면 1, 시간이 지나면 0, 오류면 -1을 돌려줍니다.
  - 듣는 소켓을 비차단으로 바꾸고, `accept()` 앞에서 `net_wait(listen_fd, POLLIN, 1000)`으로 기다립니다. `accept()`가 `EAGAIN` 또는 `EWOULDBLOCK`을 돌려주면 다시 기다립니다.
  - macOS에서는 `accept()`가 돌려준 소켓이 듣는 소켓의 비차단 속성을 **물려받습니다**. Linux는 물려받지 않습니다. 연결 소켓은 차단으로 쓸 것이므로, `accept()` 직후 `O_NONBLOCK`을 명시적으로 끕니다.
- 실험: 연결 소켓의 `O_NONBLOCK`을 끄는 줄을 잠시 막고 `nc`로 접속만 합니다. `recv()`가 기다리지 않고 바로 `EAGAIN`으로 돌아오는 것을 기록으로 봅니다.
- 질문: "비차단 `recv()`가 `EAGAIN`을 돌려주는 것은 오류인가요?" — 아닙니다. "지금은 읽을 것이 없다"는 뜻입니다.

### 4단계 완료 조건

- 인자로 포트와 루트를 바꿀 수 있습니다.
- 클라이언트가 갑자기 끊어도 서버가 살아 있습니다.
- 말 없는 연결이 시간 제한 뒤 닫힙니다.
- `Ctrl+C`로 종료 코드 0으로 끝납니다.

---

## 5단계 — 응답 조립, 정적 파일, CGI

근거 항목: 4, 5, 6, 17, 19, 23.

### 5-1. 응답 조립과 오류 응답

- 함수:

```c
const char *resp_reason(int status);
int resp_send_head(int fd, int status, const char *content_type,
                   long content_length, int keep_alive);
int resp_send_simple(int fd, int status, const char *content_type,
                     const char *body, size_t len, int keep_alive);
int resp_send_error(int fd, int status);
```

- 꼭 지킬 동작:
  - 상태 줄, `Content-Type`, `Content-Length`, `Connection`, `Date`(`gmtime_r`와 `strftime`으로 만든 GMT 시각), `Server` 헤더를 보냅니다.
  - `resp_reason`은 200, 400, 403, 404, 405, 408, 413, 414, 431, 500, 501, 502, 504, 505를 압니다.
  - `resp_send_error`는 짧은 HTML 본문을 보내고, 호출한 쪽은 그 뒤 연결을 닫습니다.
  - 헤더는 `snprintf`로 고정 크기 버퍼에 만들고, 돌려준 길이가 버퍼 크기 이상이면 500으로 처리합니다.
  - 3단계의 `http_result` 값을 상태 코드로 잇습니다. `HTTP_BAD_REQUEST` → 400, `HTTP_URI_TOO_LONG` → 414, `HTTP_HEADER_TOO_LARGE` → 431, `HTTP_NOT_IMPLEMENTED` → 501, `HTTP_VERSION_NOT_SUPPORTED` → 505, `HTTP_PAYLOAD_TOO_LARGE` → 413.
- 실험: `printf 'HELLO\r\n\r\n' | nc 127.0.0.1 8080`이 400 응답을 받는지, `curl -X DELETE -v http://127.0.0.1:8080/`이 501을 받는지 봅니다.

### 5-2. MIME 타입

- 함수: `const char *mime_from_path(const char *path);`
- 꼭 지킬 동작: 마지막 점 뒤의 확장자를 대소문자 구분 없이 봅니다. `html`, `htm`, `css`, `js`, `json`, `txt`, `png`, `jpg`, `jpeg`, `gif`, `svg`, `ico`를 알고, 모르는 확장자와 확장자가 없는 이름은 `application/octet-stream`입니다. 글자 종류에는 `; charset=utf-8`을 붙입니다.
- 단위 테스트: 제가 `tests/test_mime.c`를 칩니다. `a.HTML`, `a.tar.gz`, `noext`, `.hidden`, `dir.d/file`을 검사합니다.

### 5-3. 정적 파일

- 함수: `int static_serve(int fd, const struct config *cfg, const struct http_request *req);`
- 꼭 지킬 동작:
  - 대상에서 `?` 뒤(질의 문자열)를 떼어 냅니다.
  - `%20` 같은 퍼센트 인코딩을 풉니다. 잘못된 인코딩과 `%00`은 400입니다.
  - 푼 뒤의 경로에 `..` 조각이 있으면 403입니다. 루트 밖의 파일(예: `/etc/passwd`)을 내주지 않기 위해서입니다. 이 검사는 퍼센트 인코딩을 **푼 뒤에** 합니다.
  - 경로가 `/`로 끝나면 `index.html`을 붙입니다.
  - `stat()`이 `ENOENT`면 404, `EACCES`면 403, 디렉터리면 403입니다. 보통 파일이 아니면 403입니다.
  - `open()` → `fstat()`으로 크기 확인 → 머리 보내기 → `read()`와 `net_send_all()` 반복 → `close()`. 파일 전체를 메모리에 올리지 않습니다.
  - 어떤 경로로 끝나든 연 파일을 닫습니다.
  - 메서드가 GET이 아니면 405입니다.
- 실험:
  - `curl -v http://127.0.0.1:8080/` (index.html), `curl -v http://127.0.0.1:8080/none` (404)
  - `curl -v --path-as-is http://127.0.0.1:8080/../../etc/passwd` (403)
  - `curl -v http://127.0.0.1:8080/%2e%2e/%2e%2e/etc/passwd` (403)
  - `chmod 000 www/secret.txt` 뒤 요청 (403). 실험 뒤 `chmod 644`로 되돌립니다.
  - 큰 파일: `head -c 5000000 /dev/urandom > www/big.bin` 뒤 `curl -s http://127.0.0.1:8080/big.bin | shasum`과 `shasum www/big.bin`이 같은지 봅니다.
  - 브라우저로 HTML · CSS · 이미지가 있는 쪽을 엽니다. 시험용 파일은 당신이 `www/`에 만들어도 됩니다.
- 질문: "`..` 검사를 퍼센트 인코딩을 풀기 **전에** 하면 왜 뚫리나요?"

### 5-4. CGI

- 함수: `int cgi_serve(int fd, const struct config *cfg, const struct http_request *req, const char *body, size_t body_len);`
- 꼭 지킬 동작:
  - 대상이 `/cgi-bin/`으로 시작하면 CGI로 보냅니다. 경로 검사는 5-3구간과 같습니다. 실행 권한이 없으면 403입니다.
  - 파이프 두 개(`pipe()`): 부모 → 자식의 표준 입력(POST 본문), 자식의 표준 출력 → 부모.
  - `fork()` 뒤 자식은 `dup2()`로 파이프를 0번과 1번에 잇고, 쓰지 않는 끝을 모두 닫고, `execve()`를 부릅니다. `execve()`가 실패하면 `_exit(127)`입니다. `exit()`가 아니라 `_exit()`인 이유는 부모에게서 물려받은 출력 버퍼가 두 번 비워지지 않게 하기 위해서입니다.
  - 자식의 환경 변수: `REQUEST_METHOD`, `QUERY_STRING`, `CONTENT_LENGTH`, `CONTENT_TYPE`, `SCRIPT_NAME`, `SERVER_PROTOCOL`, `GATEWAY_INTERFACE=CGI/1.1`.
  - 부모는 쓰지 않는 끝을 닫고, 본문을 다 쓴 뒤 쓰는 끝을 닫고(자식이 입력의 끝을 알 수 있도록), 자식의 출력을 상한(예: 1048576바이트)까지 읽고, `waitpid()`로 자식을 거둡니다. 거두지 않으면 좀비 프로세스(zombie process)가 남습니다.
  - 자식의 출력은 "헤더 줄들 · 빈 줄 · 본문" 모양입니다. 부모가 `Content-Type`을 뽑고, 본문 길이를 세어 `Content-Length`를 붙여 200으로 보냅니다.
  - 자식이 0이 아닌 코드로 끝나거나 출력이 그 모양이 아니면 500입니다.
  - 자식이 제한 시간 안에 끝나지 않으면 `kill(pid, SIGKILL)` 뒤 `waitpid()`하고 504입니다. 출력 파이프를 `net_wait()`으로 기다리면 됩니다.
  - 어떤 경로로 끝나든 파이프 네 끝을 모두 닫습니다.
- 시험용 프로그램: 당신이 `www/cgi-bin/hello.sh`(질의 문자열을 그대로 출력), `www/cgi-bin/echo.sh`(표준 입력을 그대로 출력), `www/cgi-bin/slow.sh`(`sleep 30`), `www/cgi-bin/fail.sh`(`exit 1`)를 만들고 `chmod +x`합니다.
- 실험:
  - `curl 'http://127.0.0.1:8080/cgi-bin/hello.sh?name=jungle'`
  - `curl -d 'a=1' http://127.0.0.1:8080/cgi-bin/echo.sh`
  - `curl -v http://127.0.0.1:8080/cgi-bin/fail.sh` (500), `curl -v http://127.0.0.1:8080/cgi-bin/slow.sh` (504)
  - 요청 뒤 `ps -ax | grep defunct`로 좀비가 없는지, `lsof -p <서버 pid> | wc -l`이 요청 전후로 같은지 봅니다.
- 질문: "부모가 쓰는 끝을 닫지 않으면 자식은 왜 끝나지 않나요?", "`waitpid()`를 빼면 `ps`에 무엇이 보이나요?"

### 5단계 완료 조건

- 브라우저로 정적 쪽이 그림 · 스타일과 함께 보입니다.
- 404 · 403 · 500이 알맞게 나옵니다.
- CGI의 GET과 POST가 동작하고, 좀비 프로세스와 파일 디스크립터 누수가 없습니다.
- `make test`가 통과합니다.

---

## 6단계 — 동시성

근거 항목: 7, 15, 17, 20.

### 6-1. 연결마다 스레드 하나

- 목표: 1-6-마 실험의 한계를 가장 단순한 방법으로 풉니다.
- 꼭 지킬 동작:
  - `accept()` 뒤 `pthread_create()`로 스레드를 만들고 `pthread_detach()`합니다.
  - 연결 소켓 번호는 `malloc`한 칸에 담아 넘기고, 스레드가 받자마자 꺼낸 뒤 `free`합니다.
- 실험:
  - 1-6-마 실험을 다시 합니다. 이제 `curl`이 바로 응답을 받는지 봅니다.
  - 일부러 틀리기: `&conn_fd`(반복문의 지역 변수 주소)를 넘기도록 잠시 바꾸고 `ab -n 200 -c 20 http://127.0.0.1:8080/`을 돌립니다. 실패한 요청이나 엉뚱한 번호가 기록에 찍히는지 봅니다. 실험 뒤 되돌립니다. (경쟁 상태(race condition)는 매번 드러나지 않습니다. 드러나지 않으면 드러나지 않았다고 기록합니다.)
- 질문: "연결이 1만 개면 스레드도 1만 개입니다. 무엇이 부족해지나요?" — 스레드마다 드는 스택 메모리와 스레드를 만들고 없애는 비용, 문맥 교환(context switch) 비용입니다. (근거 항목 15의 C10K 문제)

### 6-2. 스레드 풀

- 자료 구조와 함수:

```c
struct pool;

struct pool *pool_create(int workers, int queue_cap, void (*handler)(int fd));
int pool_submit(struct pool *p, int fd);
void pool_shutdown(struct pool *p);
```

- 꼭 지킬 동작:
  - 일꾼 스레드 `workers`개를 처음에 만들어 둡니다.
  - 대기열은 고정 크기의 원형 버퍼(ring buffer)이고 `head` · `tail` · `count`를 가집니다.
  - `pthread_mutex_t` 하나와 `pthread_cond_t` 둘(`not_empty`, `not_full`)을 씁니다. `sem_init`은 쓰지 않습니다.
  - `pthread_cond_wait()`은 반드시 `while (조건)` 안에서 부릅니다. `if`가 아닙니다. 이유 없이 깨어나는 일(spurious wakeup)이 있기 때문입니다.
  - 대기열이 가득 차면 `pool_submit`이 `not_full`을 기다립니다. `accept()`를 부르는 쪽이 느려지고, 넘치는 연결은 커널의 대기열(backlog)에서 기다립니다.
- 실험: `-w 2`로 띄우고 `nc` 두 개로 일꾼을 모두 붙잡은 뒤 `curl`이 어떻게 되는지, `nc` 하나를 끝내면 어떻게 되는지 봅니다.
- 질문: "`pthread_cond_wait()`은 잠들기 전에 뮤텍스를 어떻게 하나요?" — 풀고 잠들고, 깨어나면 다시 잡습니다.

### 6-3. 공유 상태와 스레드 안전

- 목표: 여러 일꾼이 함께 쓰는 것을 찾아 보호합니다.
- 꼭 지킬 동작:
  - 기록 출력은 한 줄을 `snprintf`로 다 만든 뒤 한 번에 씁니다. 줄이 섞이지 않게 하기 위해서입니다.
  - `strtok`, `localtime`, `gmtime`, `strerror`, `gethostbyname`, `inet_ntoa`처럼 안에 공유 버퍼를 가진 함수를 쓰지 않습니다. `strtok_r`, `localtime_r`, `gmtime_r`, `strerror_r`, `getaddrinfo`, `inet_ntop`을 씁니다.
  - 전역 변수를 `src/` 전체에서 찾아(`grep`) 목록을 만들고, 하나씩 "읽기만 하는가, 쓰는가, 누가 보호하는가"를 저와 함께 적습니다.
  - 스레드가 여럿인 프로세스에서 `fork()`하는 5-4구간의 CGI: `fork()`와 `execve()` 사이에서는 `dup2`, `close`, `execve`, `_exit`만 부릅니다. 환경 변수 배열과 문자열은 `fork()` **전에** 만들어 둡니다. 자식에는 `fork()`를 부른 스레드 하나만 복사되고, 다른 스레드가 잡고 있던 `malloc`의 잠금이 풀리지 않은 채 남을 수 있기 때문입니다.
- 실험: `make asan` 대신 `-fsanitize=thread`로 지어 `ab -n 2000 -c 50`을 돌립니다. `Makefile`에 `tsan` 대상을 더해도 됩니다. ThreadSanitizer는 AddressSanitizer와 함께 켤 수 없습니다.

### 6-4. 풀과 함께 안전하게 끝내기

- 꼭 지킬 동작: `g_stop`이 서면 `accept()`를 그만하고, `pool_shutdown()`이 멈춤 표시를 세우고 `pthread_cond_broadcast()`로 일꾼을 모두 깨우고, 일꾼은 대기열에 남은 연결까지 처리한 뒤 끝나고, `pthread_join()`으로 모두 기다린 뒤 뮤텍스와 조건 변수를 없애고 메모리를 `free`합니다.
- 실험: `ab`를 돌리는 중에 `Ctrl+C` — 서버가 멈추지 않고 끝나는지, `leaks --atExit -- ./build/server`로 누수가 없는지 봅니다.

### 6-5. 동시성 모델 비교 정리

- 목표: 왜 이 구현이 스레드 풀인지 제 말로 설명할 수 있게 합니다. (근거 항목 7, 15)
- 할 일: 프로세스 방식(`fork`), 스레드 방식(`pthread`), I/O 멀티플렉싱(`select` · `poll` · `epoll` · `kqueue`) 셋을 "연결 하나에 드는 것", "공유 상태를 다루기 쉬운가", "한 연결이 죽으면 다른 연결은", "macOS와 Linux 양쪽에서 같은 코드로 되는가"로 비교하는 표를 **제가** 채웁니다. 당신은 빈 표를 주고 제 답을 고쳐 줍니다.
- 실험: 순차 서버(1단계 커밋), 연결마다 스레드(6-1), 스레드 풀(6-2)을 같은 `ab -n 2000 -c 50` 명령으로 재고 표로 적습니다. **잰 값만** 적습니다. 순차 서버는 `git stash` 또는 `git worktree`로 따로 지어야 하므로 방법을 저에게 먼저 묻습니다.

### 6단계 완료 조건

- 여러 클라이언트가 동시에 응답을 받습니다.
- ThreadSanitizer가 경쟁 상태를 보고하지 않습니다.
- 부하 중에 `Ctrl+C`로 깨끗하게 끝납니다.

---

## 7단계 — 프록시

요구사항은 포워드 프록시(forward proxy)와 리버스 프록시(reverse proxy) 중 최소 하나입니다. 이 튜토리얼은 리버스 프록시를 먼저 하고, 포워드 프록시를 이어서 합니다. 캐시는 선택입니다.

근거 항목: 8, 9, 10, 16, 19, 23.

1단계 지도의 "③ connect() · 업스트림 TCP 연결", "④ 고친 요청 send()", "⑤ 응답 recv() (상태 줄 · 헤더 · 본문)", "⑥ 응답 중계 send() 반복", "클라이언트 fd + 업스트림 fd"가 이 단계입니다.

### 7-1. 업스트림에 연결하기

- 함수: `int net_connect(const char *host, const char *port, int timeout_ms);`
- 꼭 지킬 동작:
  - `getaddrinfo()`로 이름을 주소로 바꾸고, 결과 목록을 차례로 시도하고, 어떤 경로로 끝나든 `freeaddrinfo()`합니다.
  - 연결 시간 제한: 소켓을 비차단으로 바꾸고 `connect()`를 부릅니다. -1과 `EINPROGRESS`면 `net_wait(fd, POLLOUT, timeout_ms)`으로 기다린 뒤 `getsockopt(fd, SOL_SOCKET, SO_ERROR, ...)`로 성공했는지 확인합니다. 성공하면 차단으로 되돌리고 `SO_RCVTIMEO` · `SO_SNDTIMEO` · `SO_NOSIGPIPE`를 겁니다.
  - 실패하면 만든 소켓을 닫고 -1을 돌려줍니다.
- 실험: 시험용 백엔드를 띄웁니다. 터미널 두 개에서 `python3 -m http.server 8001 --bind 127.0.0.1`과 `python3 -m http.server 8002 --bind 127.0.0.1`을 서로 다른 폴더에서 실행합니다. 닫힌 포트(8009)에 연결하면 어떤 `errno`가 나오는지 봅니다.

### 7-2. 라우팅 규칙과 라운드 로빈

- 자료 구조와 함수:

```c
struct backend {
    char host[256];
    char port[8];
};

struct route {
    char host[256];
    char prefix[256];
    struct backend backends[8];
    int nbackends;
    unsigned next;
};

int route_load(const char *path, struct route *routes, int max);
struct route *route_match(struct route *routes, int n, const char *host, const char *target);
const struct backend *route_pick(struct route *r);
```

- 규칙 파일 한 줄의 모양: `<호스트 또는 *> <경로 접두사> <백엔드 주소:포트> [<백엔드 주소:포트> ...]`. 예: `* /api/ 127.0.0.1:8001 127.0.0.1:8002`. `#`으로 시작하는 줄과 빈 줄은 건너뜁니다.
- 꼭 지킬 동작:
  - `route_match`는 `Host` 헤더(포트 부분을 뗀 값)와 경로 접두사가 모두 맞는 규칙 중 **접두사가 가장 긴 것**을 고릅니다. 호스트가 `*`인 규칙은 모든 호스트에 맞습니다.
  - `route_pick`은 `next`를 하나 늘리며 차례로 고릅니다(round-robin). `next`는 여러 일꾼이 함께 쓰므로 뮤텍스로 보호합니다.
  - 맞는 규칙이 없으면 정적 파일 · CGI로 갑니다.
- 단위 테스트: 제가 `tests/test_route.c`를 칩니다. 가장 긴 접두사 고르기, 호스트 규칙이 `*` 규칙보다 먼저 맞는지, 차례로 도는지, 맞는 규칙이 없을 때 `NULL`인지를 검사합니다.

### 7-3. 요청 고쳐 쓰기와 보내기

- 함수: `int proxy_reverse(int client_fd, const struct http_request *req, const char *body, size_t body_len, struct route *r, const char *client_ip);`
- 꼭 지킬 동작:
  - 요청 줄을 다시 만듭니다. 메서드와 대상은 그대로, 버전은 `HTTP/1.1`입니다.
  - 홉 사이에서만 뜻이 있는 헤더(hop-by-hop header)는 넘기지 않습니다: `Connection`, `Keep-Alive`, `Proxy-Connection`, `Proxy-Authenticate`, `Proxy-Authorization`, `TE`, `Trailer`, `Transfer-Encoding`, `Upgrade`.
  - `Host`는 백엔드의 `주소:포트`로 바꿉니다.
  - `X-Forwarded-For: <클라이언트 IP>`와 `Connection: close`를 더합니다. 업스트림 연결은 요청 하나에 하나씩 쓰고 닫습니다.
  - 나머지 헤더는 그대로 넘깁니다. 본문이 있으면 `Content-Length`와 함께 보냅니다.
  - 고친 요청을 만드는 버퍼가 넘치면 500입니다.
- 실험: 백엔드 대신 `nc -l 8001`을 띄워, 프록시가 실제로 보내는 요청을 눈으로 봅니다. `curl`이 보낸 것과 줄 단위로 비교합니다.

### 7-4. 응답 중계와 오류

- 꼭 지킬 동작:
  - 업스트림에서 `recv()`한 만큼을 클라이언트에게 `net_send_all()`로 보내는 일을, 업스트림이 닫을 때(`recv()`가 0)까지 반복합니다. 응답 전체를 메모리에 올리지 않습니다.
  - 응답을 그대로 흘려보내므로, 중계가 끝나면 클라이언트 연결도 닫습니다.
  - 업스트림 연결 실패는 502, 연결 시간 초과와 응답의 첫 바이트를 기다리다 시간 초과는 504입니다. 이미 응답의 일부를 클라이언트에게 보낸 뒤에 실패하면 오류 응답을 덧붙이지 않고 닫습니다.
  - 클라이언트가 중간에 끊으면(`EPIPE`) 업스트림 연결도 닫습니다.
  - 어떤 경로로 끝나든 업스트림 소켓을 닫습니다.
  - 백엔드 하나가 연결에 실패하면 같은 규칙의 다음 백엔드를 한 번씩 시도하고, 모두 실패하면 502입니다.
- 실험:
  - `curl -v http://127.0.0.1:8080/api/`을 네 번 — 백엔드 두 개의 기록에 번갈아 찍히는지 봅니다.
  - 백엔드 하나를 끄고 반복 — 남은 하나로 가는지 봅니다. 둘 다 끄면 502인지 봅니다.
  - 응답하지 않는 백엔드: `nc -l 8001`(받기만 하고 답하지 않음)을 띄우고 요청 — 504인지와 걸린 시간을 봅니다.
  - 요청 전후로 `lsof -p <서버 pid> | wc -l`이 같은지 봅니다.

### 7-5. 포워드 프록시

- 함수: `int proxy_forward(int client_fd, const struct http_request *req, const char *body, size_t body_len, const char *client_ip);`
- 꼭 지킬 동작:
  - `-f`로 켰을 때만 동작합니다.
  - 요청 줄의 대상이 `http://호스트[:포트]/경로` 모양(absolute-form)이면 포워드 프록시 요청입니다. 호스트 · 포트(없으면 80) · 경로(없으면 `/`)로 나눕니다. 모양이 틀리면 400입니다.
  - 업스트림에는 대상을 경로만(origin-form) 넣어 보내고, `Host`는 뽑은 호스트로 맞춥니다. 나머지는 7-3 · 7-4구간과 같습니다.
  - `CONNECT` 메서드(HTTPS 터널)는 요구사항에 없으므로 501입니다.
  - `https://` 대상은 400입니다.
- 실험:
  - `curl -v -x http://127.0.0.1:8080 http://127.0.0.1:8001/` — 이 컴퓨터 안의 백엔드로 먼저 확인합니다.
  - 인터넷이 되는 곳에서 `curl -v -x http://127.0.0.1:8080 http://example.com/`
  - `curl -v -x http://127.0.0.1:8080 https://example.com/` — `curl`이 `CONNECT`를 보내고 501을 받는지 봅니다.
- 질문(근거 항목 16): "포워드 프록시와 리버스 프록시는 각각 누구를 대신하나요?" — 포워드 프록시는 클라이언트를 대신해 바깥의 원 서버(origin server)로 나가고, 목적지는 요청마다 클라이언트가 정합니다. 리버스 프록시는 서버들을 대신해 클라이언트를 맞이하고, 목적지는 서버 쪽의 규칙이 정합니다.

### 7-6. 캐시 (선택)

- 제가 하겠다고 말할 때만 합니다.
- 꼭 지킬 동작(한다면): 메서드가 GET이고 상태가 200인 응답만, 열쇠는 절대 URL, 항목 수와 항목 크기에 상한, 뮤텍스로 보호, 꽉 차면 가장 오래된 항목을 버립니다. 응답에 `X-Cache: HIT` 또는 `X-Cache: MISS`를 붙여 눈으로 확인합니다.

### 7단계 완료 조건

- `/api/` 요청이 백엔드 두 개에 번갈아 갑니다.
- 백엔드가 죽었을 때 502, 답이 없을 때 504가 나옵니다.
- `curl -x`로 포워드 프록시가 동작합니다.
- `make test`가 통과하고, 프록시 요청 전후로 파일 디스크립터 수가 같습니다.

---

## 8단계 — 검증

근거 항목: 19, 20, 21, 22, 23.

### 8-1. 단위 테스트 정리

- `make test`가 파서 · MIME · 라우팅 테스트를 모두 돌립니다. 통과한 개수와 실패한 개수를 기록합니다.

### 8-2. 기능 테스트

- 당신이 `scripts/functional_test.sh`를 씁니다. 서버와 시험용 백엔드를 띄우고, `curl`로 아래를 차례로 확인하고, 경우마다 "통과" 또는 "실패"를 찍고, 끝나면 띄운 프로세스를 모두 끝냅니다.
- 확인할 경우: 정적 200, 404, 403(경로 벗어나기), 400(망가진 요청), 501(지원하지 않는 메서드), 414(긴 대상), 413(큰 본문), CGI GET, CGI POST, 연결 유지로 요청 두 개, 리버스 프록시 번갈아 가기, 백엔드 꺼짐 502, 포워드 프록시.
- 동시 클라이언트: `curl`을 백그라운드로 20개 띄워(`&`와 `wait`) 모두 200인지 봅니다.
- 브라우저 확인은 제가 직접 하고 결과를 말합니다.

### 8-3. 부하 테스트

- 이 Mac에는 `ab`가 있고 `wrk`와 `hey`는 없습니다. 설치하려면 저에게 먼저 묻습니다.
- 명령 예: `ab -n 10000 -c 100 http://127.0.0.1:8080/`, 연결 유지로 `ab -k -n 10000 -c 100 http://127.0.0.1:8080/`.
- 꼭 지킬 것:
  - `ab`의 출력에서 `Requests per second`, `Failed requests`, `Time per request`를 그대로 옮겨 적습니다. 반올림하거나 좋은 값만 고르지 않습니다.
  - 같은 명령을 세 번 돌려 세 값을 모두 적습니다.
  - 측정 조건(일꾼 수, 빌드 옵션, 정적 파일의 크기, 클라이언트와 서버가 같은 컴퓨터라는 사실)을 함께 적습니다.
  - 보통 빌드로 잽니다. `make asan`으로 지은 것은 느립니다.
- macOS에서 생길 수 있는 일: 연결 유지 없이 요청 수가 많으면, 닫힌 연결이 TIME_WAIT(이 Mac에서는 30초)으로 남아 클라이언트의 임시 포트가 바닥나 `ab`가 연결 오류로 멈출 수 있습니다. 그런 일이 생기면 오류 원문을 기록하고, `-k`를 쓰거나 요청 수를 줄여 다시 잽니다. 시스템 설정(`sysctl -w`)은 바꾸지 않습니다.

### 8-4. 메모리와 자원 검사

- `make asan`으로 지은 서버에 8-2의 기능 테스트 전체를 돌립니다. AddressSanitizer와 UndefinedBehaviorSanitizer의 보고가 0개여야 합니다.
- 보통 빌드에 `leaks --atExit -- ./build/server`를 돌리고, 기능 테스트 뒤 `Ctrl+C`로 끝내 누수 보고를 봅니다. `leaks`는 AddressSanitizer로 지은 것에는 쓰지 않습니다.
- 파일 디스크립터: 기능 테스트 전후로 `lsof -p <서버 pid> | wc -l`이 같은지 봅니다.
- 좀비 프로세스: `ps -ax | grep defunct`.
- TCP 상태: 테스트 뒤 `netstat -an -p tcp | grep '\.8080 ' | grep CLOSE_WAIT`이 비어 있는지 봅니다.

### 8-5. 네 가지 극한 경우

| 경우 | 만드는 방법 | 기대하는 반응 |
|---|---|---|
| 잘못된 요청 | `printf 'GARBAGE\r\n\r\n' \| nc 127.0.0.1 8080`<br>`head -c 100 /dev/urandom \| nc 127.0.0.1 8080` | 400 응답 뒤 닫기. 서버는 살아 있음 |
| 매우 큰 본문 · 긴 대상 | `head -c 5000000 /dev/zero \| curl --data-binary @- http://127.0.0.1:8080/x`<br>3000자짜리 경로로 `curl` | 413, 414. 메모리 사용량이 요청 크기만큼 늘지 않음 |
| 백엔드 무응답 · 시간 초과 | `nc -l 8001`을 백엔드 자리에 띄우고 `/api/` 요청 | 시간 제한 뒤 504. 일꾼이 풀려남 |
| 클라이언트의 갑작스러운 끊김 | 큰 파일을 받는 `curl`을 도중에 `Ctrl+C`<br>`curl --max-time 0.01 http://127.0.0.1:8080/big.bin` | 서버가 죽지 않음. 그 연결의 번호와 파일이 닫힘 |

경우마다 실행한 명령, 서버의 기록, 서버가 살아 있는지(`curl`로 다음 요청)를 적습니다.

### 8단계 완료 조건

- 단위 · 기능 테스트가 모두 통과합니다.
- 부하 테스트 값이 조건과 함께 기록되어 있습니다.
- 메모리 누수 · 번호 누수 · 좀비 · CLOSE_WAIT이 없습니다.
- 네 가지 극한 경우에 서버가 살아 있습니다.

---

## 9단계 — 산출물

근거 항목: 11, 12, 24. `ref/guide_31.md`의 제출 · 발표에 관한 항목.

### 9-1. 빌드 다듬기

- `Makefile`의 경고 옵션에 `-Werror`를 더할지 저에게 묻습니다. 요구사항 해석(근거 항목 11)이 `-Wall`과 `-Werror`를 언급합니다.
- `make`, `make test`, `make asan`, `make clean`이 깨끗한 복제본(`git clone` 직후)에서 동작하는지 확인합니다.

### 9-2. Linux에서도 지어지는지

- 요구사항(근거 항목 12)은 Linux 계열 환경을 기준으로 합니다. 이 Mac에 Docker가 있는지 확인하고, 있으면 `ubuntu:22.04` 안에서 `make`와 `make test`를 돌려 볼지 저에게 묻습니다. 이미지를 받는 것은 내려받기이므로 제 승인이 필요합니다.
- 돌려 보지 못했으면 README에 "macOS에서만 확인함"이라고 그대로 적습니다.
- Linux에서 달라지는 곳(`SO_NOSIGPIPE`의 `#ifdef`, `accept()`가 비차단 속성을 물려받는지, `-pthread` 링크 옵션)을 목록으로 적습니다.

### 9-3. README

- 당신이 초안을 쓰고 제가 고칩니다. 들어갈 것:
  - 무엇을 만들었나(한 문단)
  - 구조: 파일마다 맡는 일, 요청 하나가 지나가는 길
  - 빌드와 실행: 명령과 인자 표
  - 지원하는 것과 지원하지 않는 것(청크 방식 본문, `CONNECT`, HTTPS)
  - 동시성 모델을 고른 이유
  - 테스트 방법과 **직접 잰** 결과, 측정 조건
  - 알려진 한계
  - AI를 어떻게 썼는지: 코드는 사람이 직접 쳤고 AI는 안내와 확인을 맡았다는 사실
- 재지 않은 수치와 하지 않은 테스트를 적지 않습니다.

### 9-4. 통합 지도와 학습 노트를 위한 자료 넘기기

통합 아키텍처 지도(HTML)와 학습 노트는 Windows PC 쪽에서 만듭니다. 당신은 만들지 않고, 아래를 `docs/HANDOFF_FOR_MAP.md`에 모아 줍니다.

- 파일 목록과 파일마다의 줄 수
- 함수 목록: 파일 이름, 줄 번호, 함수 선언 원문
- 요청 하나가 지나가는 호출 순서: 정적 파일, CGI, 리버스 프록시, 포워드 프록시 각각
- 연결 상태 `enum`의 값과 상태가 바뀌는 곳의 줄 번호
- 8단계의 측정 결과 원문
- `docs/TUTORIAL_LOG.md` 전체

코드 조각을 옮길 때는 실제 파일의 실제 줄을 그대로 옮기고 줄 번호를 붙입니다. 지어낸 코드를 "소스"라고 적지 않습니다.

### 9단계 완료 조건

- 깨끗한 복제본에서 `make`와 `make test`가 됩니다.
- README가 있고, 그 안의 수치가 모두 `docs/TUTORIAL_LOG.md`의 기록과 같습니다.
- `docs/HANDOFF_FOR_MAP.md`가 있습니다.

---

## 부록 — 근거 항목 번호와 단계의 대응

| `ref/requirements_24.md`의 항목 | 다루는 단계 |
|---|---|
| 1 과제 개요 | 전 단계 |
| 2 TCP 소켓 서버 | 1, 2, 4 |
| 3 HTTP 요청 파싱 | 2, 3 |
| 4 HTTP 응답 | 1-5, 5-1 |
| 5 정적 파일 · 오류 응답 | 5-2, 5-3 |
| 6 동적 요청 · CGI | 5-4, 7 |
| 7 동시 요청 처리 | 6 |
| 8 프록시 필수 기준 | 7 |
| 9 포워드 프록시 | 7-1, 7-5, 7-6 |
| 10 리버스 프록시 · 로드 밸런싱 | 7-2, 7-3, 7-4 |
| 11 기술 스택 · 라이브러리 제한 | 0-3, 4-1, 9-1 |
| 12 개발 플랫폼 · 이식성 | 0-4, 9-2 |
| 13 소켓 프로그래밍 원리 | 1, 4-5 |
| 14 RFC 7230 ~ 7235 구현 범위 | 3 |
| 15 동시성 트레이드오프 · C10K | 6-1, 6-5 |
| 16 포워드와 리버스의 비교 | 7-5 |
| 17 메모리 생명주기 | 3-3, 5-3, 5-4, 6-4, 8-4 |
| 18 시그널 · 안전한 종료 | 1-5, 4-2, 4-4, 6-4 |
| 19 모듈 단위 테스트 | 3-5, 5-2, 7-2, 8-1 |
| 20 기능 테스트 · 다중 클라이언트 | 8-2 |
| 21 부하 테스트 | 8-3 |
| 22 메모리 누수 검사 | 8-4 |
| 23 네 가지 극한 경우 | 4-2, 4-3, 5-1, 7-4, 8-5 |
| 24 완성도 | 9 |
