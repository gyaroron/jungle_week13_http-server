# 1단계 대본 — 소켓 하나로 요청 받고 고정 응답 돌려주기 (구간 1-1 ~ 1-6)

이 문서는 튜터 역할을 하는 AI가 보는 대본입니다. 진행 규칙은 `01_tutorial_workorder.md`를 따릅니다.

- 이 문서의 코드는 **사용자에게 한꺼번에 보여 주지 않습니다.** 한 번에 1 ~ 3줄씩 안내합니다.
- 이 문서의 코드는 Windows PC에서 작성했고 **macOS에서 컴파일해 보지 못했습니다.** 구간을 시작하기 전에 `01_tutorial_workorder.md` 3-4절의 사전 검사를 합니다.
- "기대 결과"는 예상입니다. 실제 출력과 다르면 실제 출력을 기록하고 사용자에게 알립니다.
- 지도 기록 이름(예: "LISTEN", "① TCP 연결 맺기")은 `ref/stack_diagram_stage1.html`에서 눌러 볼 수 있는 항목의 제목입니다.

실험에서 터미널을 여러 개 씁니다. VS Code 통합 터미널(Ctrl+`)의 오른쪽 위 `+`를 눌러 늘립니다. 이 문서에서는 **터미널 A**(서버 실행), **터미널 B**(클라이언트), **터미널 C**(상태 관찰)라고 부릅니다.

---

## 구간 1-1 — `socket()`으로 소켓 하나 만들기 (진행 중)

### 목표

제 프로그램이 `socket()`을 불러 커널에게 TCP 소켓 하나를 만들어 달라고 하고, 커널이 돌려준 파일 디스크립터(file descriptor) 번호를 출력한 뒤 `close()`로 돌려줍니다.

지도 기록: "CLOSED (시작)", "socket → bind → listen → accept".

### 구간이 끝났을 때의 `src/server.c`

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>

int main(void) {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        exit(1);
    }
    printf("listen_fd = %d\n", listen_fd);
    close(listen_fd);
    return 0;
}
```

사용자가 주석을 직접 달았으면 그대로 둡니다. 줄 번호는 사용자의 파일 기준으로 말합니다.

### 줄마다 설명할 것

| 줄 | 설명할 핵심 |
|---|---|
| `#include <sys/socket.h>` | `socket()`, `AF_INET`, `SOCK_STREAM`의 선언이 들어 있는 헤더입니다. |
| `socket(AF_INET, SOCK_STREAM, 0)` | `AF_INET`(**A**ddress **F**amily **I**nter**net**)은 IPv4 주소 체계, `SOCK_STREAM`은 바이트가 순서대로 이어지는 연결형 소켓, 마지막 `0`은 "그 조합의 기본 프로토콜을 쓰라"는 뜻이고 여기서는 TCP(**T**ransmission **C**ontrol **P**rotocol)가 됩니다. |
| `if (listen_fd < 0)` | `socket()`은 실패하면 `-1`을 돌려주고 전역 변수 `errno`에 이유를 적습니다. |
| `perror("socket")` | `errno`의 값을 사람이 읽는 문장으로 바꿔 `socket: <이유>` 모양으로 표준 오류(stderr)에 출력합니다. |
| `exit(1)` | 0이 아닌 종료 코드로 끝내 "실패했다"를 셸에게 알립니다. |
| `close(listen_fd)` | 제 프로그램이 커널에게 그 번호를 돌려줍니다. 돌려주지 않으면 번호가 프로세스가 끝날 때까지 잡혀 있습니다. |

### 체크포인트

`Cmd+Shift+B`로 빌드합니다. 경고 0개, 오류 0개여야 합니다.

### 실험 1-1-가 — 번호 확인

- 명령(터미널 A): `./build/server`
- 예상 질문: "몇 번이 출력될 것 같나요?"
- 기대 결과: `listen_fd = 3`

### 실험 1-1-나 — 번호가 밀리는지 확인

- 명령(터미널 A): `./build/server 3< /dev/null`
- 예상 질문: "셸이 3번을 미리 열어 준 채로 실행하면 몇 번이 나올까요?"
- 기대 결과: `listen_fd = 4`
- 깨달음: 커널은 "비어 있는 가장 작은 번호"를 줍니다.

### 질문과 정답 기준

1. "왜 0이 아니라 3인가요?" — 0(표준 입력, stdin), 1(표준 출력, stdout), 2(표준 오류, stderr)가 프로그램이 시작될 때 이미 열려 있고, 커널이 비어 있는 가장 작은 번호를 주기 때문입니다.
2. "지금 이 소켓으로 접속할 수 있나요?" — 없습니다. 주소가 붙지 않았고(`bind()` 전), 듣는 상태도 아닙니다(`listen()` 전). TCP 상태로는 CLOSED입니다.

---

## 구간 1-2 — `bind()`와 `listen()`으로 LISTEN 상태 만들기

### 목표

제 프로그램이 소켓에 주소 127.0.0.1:8080을 붙이고(`bind()`), 커널에게 "이 소켓으로 들어오는 연결 요청을 받아 달라"고 합니다(`listen()`). 커널이 소켓의 TCP 상태를 CLOSED에서 LISTEN으로 바꿉니다.

지도 기록: "CLOSED (시작)" → "LISTEN", "socket → bind → listen → accept".

### 바뀌는 곳

1. `#include` 3줄을 더합니다.

```c
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
```

2. `#include` 아래, `main` 위에 상수 2줄을 더합니다.

```c
#define PORT 8080
#define BACKLOG 128
```

3. 1-1구간의 `printf("listen_fd = %d\n", listen_fd);` 줄을 **지우고**, 그 자리에 아래를 칩니다. `close(listen_fd);`와 `return 0;`은 그대로 둡니다.

```c
    int on = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
        perror("setsockopt");
        exit(1);
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }

    if (listen(listen_fd, BACKLOG) < 0) {
        perror("listen");
        exit(1);
    }
    printf("listening on 127.0.0.1:%d (fd %d)\n", PORT, listen_fd);

    pause();
```

### 줄마다 설명할 것

| 줄 | 설명할 핵심 |
|---|---|
| `setsockopt(..., SO_REUSEADDR, ...)` | `SO_REUSEADDR`(**S**ocket **O**ption **REUSE** **ADDR**ess)을 켜면, 서버를 끝낸 직후 TIME_WAIT 상태의 연결이 남아 있어도 같은 포트에 다시 `bind()`할 수 있습니다. 실제로 없으면 무슨 일이 생기는지는 1-6구간에서 실험합니다. |
| `struct sockaddr_in addr` | IPv4 주소 하나(주소 체계 · 포트 · IP)를 담는 구조체입니다. `in`은 internet입니다. |
| `memset(&addr, 0, sizeof(addr))` | 구조체에 쓰지 않는 채움 칸이 있으므로 전부 0으로 지웁니다. 지우지 않으면 스택에 남아 있던 값이 섞여 들어갑니다. |
| `htons(PORT)` | `htons`(**h**ost **to** **n**etwork **s**hort)는 16비트 숫자를 네트워크 바이트 순서(big-endian)로 바꿉니다. 이 Mac(arm64)은 little-endian이므로 바이트가 뒤집힙니다. |
| `htonl(INADDR_LOOPBACK)` | `htonl`(**h**ost **to** **n**etwork **l**ong)은 32비트용입니다. `INADDR_LOOPBACK`은 127.0.0.1이고, 이 컴퓨터 안에서만 접속할 수 있습니다. 개발 중에는 외부에 열지 않기 위해 이 주소를 씁니다. |
| `(struct sockaddr *)&addr` | `bind()`는 주소 체계에 상관없이 쓰는 공통 모양 `struct sockaddr *`를 받으므로 포인터의 타입을 바꿔 넘깁니다. |
| `listen(listen_fd, BACKLOG)` | 둘째 인자는 "연결이 끝났지만 제 프로그램이 아직 `accept()`로 꺼내 가지 않은 연결"을 커널이 몇 개까지 줄 세워 둘지입니다. 이 Mac의 상한 `kern.ipc.somaxconn`은 128로 측정되었습니다. |
| `pause()` | 시그널이 올 때까지 프로세스를 재웁니다. 서버가 바로 끝나지 않게 붙잡아 두는 임시 줄이고 1-3구간에서 지웁니다. |

### 체크포인트

빌드합니다. 경고 0개, 오류 0개여야 합니다.

### 실험 1-2-가 — 포트가 열렸는지

- 서버를 띄우기 전(터미널 B): `nc -z 127.0.0.1 8080; echo $?` → 기대 결과 `1`
- 터미널 A: `./build/server` → 기대 결과 `listening on 127.0.0.1:8080 (fd 3)`
- 터미널 B: `nc -z 127.0.0.1 8080; echo $?` → 기대 결과 `0`
- 깨달음: 제 프로그램은 `accept()`를 부른 적이 없는데도 접속이 성공합니다. 연결 맺기(3-way handshake)는 **커널이** 끝내고, 끝난 연결을 줄에 세워 둡니다. 지도 기록 "① TCP 연결 맺기 (SYN · SYN-ACK · ACK)"와 "SYN_RCVD"가 이 과정입니다.

### 실험 1-2-나 — LISTEN 상태를 눈으로 보기

- 터미널 C: `lsof -nP -iTCP:8080 -sTCP:LISTEN`
- 기대 결과: `server`라는 이름의 줄 하나, 끝에 `TCP 127.0.0.1:8080 (LISTEN)`
- 터미널 C: `netstat -an -p tcp | grep '\.8080 '`
- 기대 결과: `127.0.0.1.8080`과 `LISTEN`이 있는 줄

### 실험 1-2-다 — 같은 포트에 두 번 붙이기

- 터미널 A에 서버가 떠 있는 채로, 터미널 B: `./build/server`
- 예상 질문: "`SO_REUSEADDR`을 켰으니 둘째 서버도 뜰까요?"
- 기대 결과: `bind: Address already in use`
- 깨달음: `SO_REUSEADDR`은 TIME_WAIT로 남은 연결 때문에 막히는 것을 풀어 줄 뿐이고, 살아 있는 듣는 소켓이 쓰는 주소와 포트를 둘이 함께 쓰게 해 주지 않습니다.

### 실험 1-2-라 — `htons`를 빼면 (선택)

- 터미널 A의 서버를 `Ctrl+C`로 끝냅니다.
- 사용자가 `htons(PORT)`를 `PORT`로 잠시 바꾸고 빌드해 실행합니다.
- 터미널 C: `lsof -nP -iTCP -sTCP:LISTEN | grep server`
- 예상 질문: "8080을 16진수로 쓰면 `0x1F90`입니다. 두 바이트의 순서가 뒤집히면 몇 번 포트가 될까요?"
- 기대 결과: 포트 `36895`(`0x901F`)
- 실험이 끝나면 `htons(PORT)`로 **되돌리고** 다시 빌드합니다. 되돌렸는지 파일을 읽어 확인합니다.

### 질문과 정답 기준

1. "`bind()`는 누가 누구에게 무엇을 하는 호출인가요?" — 제 프로그램이 커널에게 "이 소켓의 주소는 127.0.0.1:8080"이라고 알려 주는 호출입니다.
2. "`listen()`의 둘째 인자를 1로 줄이면 무엇이 달라지나요?" — 커널이 줄 세워 둘 수 있는, 아직 `accept()`되지 않은 연결의 수가 줄어듭니다. 줄이 가득 찬 뒤에 온 연결 요청은 커널이 받아 주지 않습니다.
3. "127.0.0.1 대신 `INADDR_ANY`(0.0.0.0)를 쓰면 무엇이 달라지나요?" — 이 컴퓨터의 모든 네트워크 주소에서 접속을 받습니다. 같은 와이파이의 다른 기기도 접속할 수 있게 됩니다.

---

## 구간 1-3 — `accept()`로 연결 하나 꺼내기

### 목표

제 프로그램이 `accept()`를 불러, 커널이 줄 세워 둔 연결 하나를 꺼냅니다. 커널은 그 연결 전용의 **새 파일 디스크립터**를 돌려줍니다.

지도 기록: "accept()가 연결 fd를 돌려준다", "ESTABLISHED".

### 바뀌는 곳

`pause();` 줄을 **지우고**, 그 자리에 아래를 칩니다.

```c
    struct sockaddr_in peer;
    socklen_t peer_len = sizeof(peer);
    int conn_fd = accept(listen_fd, (struct sockaddr *)&peer, &peer_len);
    if (conn_fd < 0) {
        perror("accept");
        exit(1);
    }

    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &peer.sin_addr, ip, sizeof(ip));
    printf("accepted fd %d from %s:%d\n", conn_fd, ip, ntohs(peer.sin_port));

    close(conn_fd);
```

그 아래의 `close(listen_fd);`와 `return 0;`은 그대로 둡니다.

### 줄마다 설명할 것

| 줄 | 설명할 핵심 |
|---|---|
| `struct sockaddr_in peer` | 접속해 온 상대(peer)의 주소를 커널이 채워 줄 빈 구조체입니다. |
| `socklen_t peer_len = sizeof(peer)` | 들어갈 때는 "제가 준비한 칸의 크기", 나올 때는 "커널이 실제로 채운 크기"가 되는 값입니다. 그래서 값이 아니라 주소(`&peer_len`)를 넘깁니다. |
| `accept(...)` | 줄에 연결이 없으면 제 프로그램은 여기서 **멈춰서 기다립니다**(blocking). 연결이 생기면 커널이 새 번호를 돌려줍니다. |
| `inet_ntop(...)` | `inet_ntop`(**inet** **n**etwork **to** **p**resentation)은 4바이트 IP 주소를 `127.0.0.1` 같은 글자로 바꿉니다. |
| `INET_ADDRSTRLEN` | IPv4 주소 글자가 들어가는 데 필요한 최대 길이(16)입니다. |
| `ntohs(peer.sin_port)` | `ntohs`(**n**etwork **to** **h**ost **s**hort)는 `htons`의 반대 방향입니다. |
| `close(conn_fd)` | 연결 소켓을 닫습니다. 듣는 소켓은 따로 닫습니다. |

### 체크포인트

빌드합니다. 경고 0개, 오류 0개여야 합니다.

### 실험 1-3-가 — 연결 하나 받기

- 터미널 A: `./build/server`
- 터미널 B: `nc 127.0.0.1 8080`
- 예상 질문: "`conn_fd`는 몇 번일까요? 상대의 포트는 8080일까요?"
- 기대 결과(터미널 A): `accepted fd 4 from 127.0.0.1:<5자리 숫자>` 출력 뒤 서버가 끝납니다. 터미널 B의 `nc`도 끝납니다.
- 깨달음: 상대의 포트는 8080이 아니라 커널이 클라이언트에게 임시로 준 포트(ephemeral port)입니다. 연결 하나는 (클라이언트 IP, 클라이언트 포트, 서버 IP, 서버 포트) 네 값으로 구분됩니다.

### 실험 1-3-나 — 실행할 때마다 포트가 바뀌는지

- 실험 1-3-가를 두 번 더 반복합니다.
- 기대 결과: `fd`는 매번 4, 상대 포트는 매번 다릅니다.

### 질문과 정답 기준

1. "`listen_fd`와 `conn_fd`는 무엇이 다른가요?" — `listen_fd`는 새 연결을 받기만 하는 듣는 소켓(listening socket)이고 데이터를 주고받지 않습니다. `conn_fd`는 클라이언트 한 명과 데이터를 주고받는 연결 소켓(connected socket)입니다.
2. "클라이언트가 100명 접속하면 포트 8080이 100개 필요한가요?" — 아닙니다. 서버 쪽 포트는 모두 8080이고, 파일 디스크립터만 100개 생깁니다. 연결은 네 값의 조합으로 구분됩니다.
3. "접속하는 클라이언트가 없으면 `accept()` 다음 줄은 언제 실행되나요?" — 실행되지 않습니다. 제 프로그램은 `accept()` 안에서 잠들어 있습니다.

---

## 구간 1-4 — `recv()`로 HTTP 요청의 실제 바이트 보기

### 목표

제 프로그램이 `recv()`로 연결 소켓에서 바이트를 읽어 그대로 출력합니다. `curl`이 실제로 무엇을 보내는지 눈으로 확인합니다.

지도 기록: "② HTTP 요청 (요청 줄 · 헤더 · 빈 줄 · 본문)", "recv() 반복 → 빈 줄까지 파싱 → 라우팅".

### 바뀌는 곳

1. 상수 한 줄을 더합니다.

```c
#define BUF_SIZE 4096
```

2. `printf("accepted fd ...");` 줄과 `close(conn_fd);` 줄 **사이에** 아래를 칩니다.

```c
    char buf[BUF_SIZE];
    ssize_t n = recv(conn_fd, buf, sizeof(buf) - 1, 0);
    if (n < 0) {
        perror("recv");
    } else if (n == 0) {
        printf("peer closed\n");
    } else {
        buf[n] = '\0';
        printf("---- %zd bytes ----\n%s\n", n, buf);
    }
```

### 줄마다 설명할 것

| 줄 | 설명할 핵심 |
|---|---|
| `ssize_t n` | 부호 있는 크기 타입입니다. 읽은 바이트 수(0 이상) 또는 오류(-1)를 담습니다. |
| `recv(conn_fd, buf, sizeof(buf) - 1, 0)` | 커널의 수신 버퍼에 **지금 와 있는 만큼**을, 최대 4095바이트까지 `buf`로 복사합니다. 와 있는 것이 없으면 올 때까지 멈춰서 기다립니다. |
| `sizeof(buf) - 1` | 끝에 `'\0'` 한 칸을 넣을 자리를 남깁니다. |
| `n == 0` | 오류가 아닙니다. 상대가 연결을 닫았다는 뜻입니다(상대의 FIN 도착). |
| `buf[n] = '\0'` | `recv()`는 문자열 끝 표시를 붙여 주지 않습니다. `%s`로 출력하려면 직접 붙여야 합니다. |
| `%zd` | `ssize_t`를 출력하는 형식 지정자입니다. |

### 체크포인트

빌드합니다. 경고 0개, 오류 0개여야 합니다.

### 실험 1-4-가 — ESTABLISHED 상태 보기

- 터미널 A: `./build/server`
- 터미널 B: `nc 127.0.0.1 8080` (아무것도 치지 않고 둡니다)
- 터미널 C: `netstat -an -p tcp | grep '\.8080 '`
- 예상 질문: "몇 줄이 나올까요?"
- 기대 결과: `LISTEN` 줄 하나와 `ESTABLISHED` 줄 **두 개**. 클라이언트와 서버가 같은 컴퓨터에 있어서 연결의 양쪽 끝이 모두 보입니다.
- 이어서 터미널 B에서 `hello`를 치고 Enter를 누릅니다.
- 기대 결과(터미널 A): `---- 6 bytes ----`와 `hello`. 6바이트인 이유는 `hello` 5글자와 줄바꿈 `\n` 1글자입니다.

### 실험 1-4-나 — `curl`이 보내는 것

- 터미널 A: `./build/server`
- 터미널 B: `curl -v http://127.0.0.1:8080/`
- 예상 질문: "`curl`은 몇 줄을 보낼까요? 첫 줄은 무엇일까요?"
- 기대 결과(터미널 A): `GET / HTTP/1.1`, `Host: 127.0.0.1:8080`, `User-Agent: curl/<버전>`, `Accept: */*`, 그리고 빈 줄
- 기대 결과(터미널 B): `curl: (52) Empty reply from server`
- 깨달음: 서버가 응답을 한 바이트도 보내지 않고 닫았기 때문에 `curl`이 오류로 끝납니다. 1-5구간에서 고칩니다.

### 실험 1-4-다 — 아무것도 안 보내고 끊기

- 터미널 A: `./build/server`
- 터미널 B: `nc 127.0.0.1 8080`을 실행한 뒤 바로 `Ctrl+C`
- 기대 결과(터미널 A): `peer closed`

### 질문과 정답 기준

1. "`recv()`를 한 번 부르면 HTTP 요청 하나가 통째로 온다고 믿어도 되나요?" — 안 됩니다. TCP는 메시지 경계가 없는 바이트 스트림(byte stream)이어서 요청이 여러 조각으로 나뉘어 올 수 있습니다. 헤더의 끝은 빈 줄(`\r\n\r\n`)로 찾아야 하고, 2단계와 3단계에서 다룹니다.
2. "`recv()`가 0을 돌려주는 것과 -1을 돌려주는 것은 어떻게 다른가요?" — 0은 상대가 정상적으로 닫은 것이고, -1은 오류이며 이유가 `errno`에 있습니다.
3. "요청이 4095바이트보다 길면 지금 코드에서 무슨 일이 생기나요?" — 앞의 4095바이트만 읽고 나머지는 커널의 수신 버퍼에 남습니다. 버퍼를 넘쳐 쓰지는 않습니다.

---

## 구간 1-5 — `send()`로 가장 작은 HTTP 응답 보내기

### 목표

제 프로그램이 고정된 HTTP 응답을 `send()`로 보내고, `curl`이 `200 OK`와 본문 `hello`를 받습니다.

지도 기록: "⑥ 응답 중계 send() 반복", "⑦ keep-alive면 ②로 · 아니면 close()".

### 바뀌는 곳

1. `#include` 한 줄을 더합니다.

```c
#include <signal.h>
```

2. `main`의 첫 줄(`int listen_fd = ...` 위)에 한 줄을 더합니다.

```c
    signal(SIGPIPE, SIG_IGN);
```

3. 1-4구간의 `printf("---- %zd bytes ----\n%s\n", n, buf);` 줄 **바로 아래**, 같은 `else` 블록 안에 아래를 칩니다.

```c
        const char *resp =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 6\r\n"
            "Connection: close\r\n"
            "\r\n"
            "hello\n";
        size_t len = strlen(resp);
        size_t sent = 0;
        while (sent < len) {
            ssize_t w = send(conn_fd, resp + sent, len - sent, 0);
            if (w < 0) {
                perror("send");
                break;
            }
            sent += (size_t)w;
        }
```

### 줄마다 설명할 것

| 줄 | 설명할 핵심 |
|---|---|
| `signal(SIGPIPE, SIG_IGN)` | 상대가 이미 닫은 소켓에 제 프로그램이 `send()`하면 커널이 `SIGPIPE`(**SIG**nal **PIPE**) 시그널을 보내고, 기본 동작은 **프로세스 종료**입니다. `SIG_IGN`(**SIG**nal **IGN**ore)으로 무시하게 하면 `send()`가 -1을 돌려주고 `errno`가 `EPIPE`가 됩니다. 서버가 클라이언트 한 명 때문에 죽지 않게 하는 줄입니다. |
| `"HTTP/1.1 200 OK\r\n"` | 상태 줄(status line)입니다. 버전, 상태 코드, 이유 문구 순서입니다. |
| `\r\n` | HTTP에서 줄의 끝은 `\n` 하나가 아니라 CR(**C**arriage **R**eturn)과 LF(**L**ine **F**eed) 두 글자입니다. |
| `"Content-Length: 6\r\n"` | 본문이 몇 바이트인지 알려 줍니다. `hello\n`은 6바이트입니다. |
| `"Connection: close\r\n"` | 이 응답을 보낸 뒤 서버가 연결을 닫겠다고 알립니다. |
| `"\r\n"` | 빈 줄입니다. 헤더가 끝났고 다음부터 본문이라는 경계입니다. |
| 이어 쓴 문자열 | C 컴파일러는 나란히 놓인 문자열 리터럴을 하나로 이어 붙입니다. |
| `while (sent < len)` | `send()`는 부탁한 것보다 **적게** 보내고 돌아올 수 있습니다. 보낸 만큼 더해 가며 다 보낼 때까지 반복합니다. |
| `resp + sent`, `len - sent` | 아직 안 보낸 곳의 시작 주소와 남은 길이입니다. |
| `(size_t)w` | `w`가 0 이상임을 확인한 뒤 부호 없는 타입으로 바꿔 더합니다. 타입을 바꾸지 않으면 부호가 다른 타입끼리 더한다는 경고가 날 수 있습니다. |

### 체크포인트

빌드합니다. 경고 0개, 오류 0개여야 합니다.

### 실험 1-5-가 — 첫 응답

- 터미널 A: `./build/server`
- 터미널 B: `curl -v http://127.0.0.1:8080/`
- 기대 결과(터미널 B): `< HTTP/1.1 200 OK`, `< Content-Type: text/plain`, `< Content-Length: 6`, `< Connection: close`, 그리고 `hello`
- 브라우저 주소창에 `http://127.0.0.1:8080/`을 넣어도 `hello`가 보이는지 확인합니다. 서버는 연결 하나만 받고 끝나므로, 브라우저로 볼 때는 서버를 다시 띄웁니다.

### 실험 1-5-나 — `Content-Length`를 틀리게 쓰면

- 사용자가 `Content-Length: 6`을 `Content-Length: 10`으로 잠시 바꾸고 빌드해 실행합니다.
- 터미널 B: `curl -v http://127.0.0.1:8080/`
- 예상 질문: "본문은 6바이트인데 10바이트라고 말하면 `curl`은 어떻게 할까요?"
- 기대 결과: `curl: (18)`로 시작하는 오류. 남은 4바이트를 받지 못한 채 연결이 닫혔다는 내용입니다.
- 이어서 `Content-Length: 3`으로 바꿔 실행합니다. 기대 결과: `hel`만 출력됩니다.
- 실험이 끝나면 `Content-Length: 6`으로 **되돌리고** 다시 빌드합니다. 되돌렸는지 파일을 읽어 확인합니다.

### 질문과 정답 기준

1. "클라이언트는 응답 본문이 어디서 끝나는지 어떻게 아나요?" — `Content-Length`의 바이트 수만큼 읽거나, 서버가 연결을 닫을 때까지 읽습니다.
2. "`send()`를 반복문 없이 한 번만 부르면 어떤 경우에 문제가 생기나요?" — 응답이 크거나 커널의 송신 버퍼가 가득 차 있을 때 일부만 보내지고, 나머지는 영영 보내지 않게 됩니다.
3. "`signal(SIGPIPE, SIG_IGN)`이 없으면 어떤 상황에서 서버가 죽나요?" — 클라이언트가 응답을 다 받기 전에 연결을 끊었고, 제 프로그램이 그 소켓에 계속 `send()`할 때입니다.

---

## 구간 1-6 — `accept()` 반복과 닫는 순서에 따른 TCP 상태

### 목표

서버가 연결 하나를 처리한 뒤 끝나지 않고 다음 연결을 기다립니다. 그리고 "누가 먼저 닫는가"에 따라 TCP 상태가 어떻게 달라지는지 관찰합니다.

지도 기록: "FIN_WAIT_1", "FIN_WAIT_2", "TIME_WAIT", "CLOSE_WAIT", "LAST_ACK", "CLOSED (끝)".

### 바뀌는 곳

1. `printf("listening on ...");` 줄 아래부터 `close(conn_fd);` 줄까지를 `for (;;) {`와 `}`로 감쌉니다. 감싼 안쪽은 4칸 더 들여 씁니다. VS Code에서는 줄들을 고르고 `Tab`을 누르면 됩니다.

2. `accept()`가 실패했을 때의 `exit(1);`을 `continue;`로 바꿉니다. 연결 하나를 받지 못했다고 서버 전체를 끝내지 않기 위해서입니다.

3. `for` 블록 아래의 `close(listen_fd);`와 `return 0;`은 **지웁니다.** `for (;;)`는 끝나지 않으므로 그 두 줄에는 도달할 수 없고, 도달할 수 없는 코드라는 경고가 날 수 있습니다. 서버를 끝내는 방법은 지금은 `Ctrl+C`이고, 깨끗하게 끝내는 방법은 4단계에서 다룹니다.

### 구간이 끝났을 때의 `src/server.c` (튜터 확인용, 사용자에게 보여 주지 않음)

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define BACKLOG 128
#define BUF_SIZE 4096

int main(void) {
    signal(SIGPIPE, SIG_IGN);

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        exit(1);
    }

    int on = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
        perror("setsockopt");
        exit(1);
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }

    if (listen(listen_fd, BACKLOG) < 0) {
        perror("listen");
        exit(1);
    }
    printf("listening on 127.0.0.1:%d (fd %d)\n", PORT, listen_fd);

    for (;;) {
        struct sockaddr_in peer;
        socklen_t peer_len = sizeof(peer);
        int conn_fd = accept(listen_fd, (struct sockaddr *)&peer, &peer_len);
        if (conn_fd < 0) {
            perror("accept");
            continue;
        }

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &peer.sin_addr, ip, sizeof(ip));
        printf("accepted fd %d from %s:%d\n", conn_fd, ip, ntohs(peer.sin_port));

        char buf[BUF_SIZE];
        ssize_t n = recv(conn_fd, buf, sizeof(buf) - 1, 0);
        if (n < 0) {
            perror("recv");
        } else if (n == 0) {
            printf("peer closed\n");
        } else {
            buf[n] = '\0';
            printf("---- %zd bytes ----\n%s\n", n, buf);

            const char *resp =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: 6\r\n"
                "Connection: close\r\n"
                "\r\n"
                "hello\n";
            size_t len = strlen(resp);
            size_t sent = 0;
            while (sent < len) {
                ssize_t w = send(conn_fd, resp + sent, len - sent, 0);
                if (w < 0) {
                    perror("send");
                    break;
                }
                sent += (size_t)w;
            }
        }

        close(conn_fd);
    }
}
```

`main`이 `return` 없이 끝나는 모양에 컴파일러가 경고를 내면, 3-4절의 사전 검사에서 먼저 드러납니다. 그때는 경고 원문을 사용자에게 보여 주고 어떻게 할지 묻습니다.

### 체크포인트

빌드합니다. 경고 0개, 오류 0개여야 합니다.

### 실험 1-6-가 — 번호 재사용

- 터미널 A: `./build/server`
- 터미널 B: `curl http://127.0.0.1:8080/`을 세 번 실행합니다.
- 예상 질문: "`conn_fd`는 4, 5, 6으로 늘어날까요?"
- 기대 결과: 세 번 모두 `accepted fd 4`
- 깨달음: 제 프로그램이 `close(conn_fd)`로 번호를 돌려주기 때문에 커널이 같은 번호를 다시 줍니다. 사용자가 `close(conn_fd);`를 잠시 주석으로 막고 같은 실험을 하면 4, 5, 6으로 늘어납니다. 이것이 파일 디스크립터 누수(file descriptor leak)입니다. 실험 뒤 주석을 **풀고** 다시 빌드합니다.

### 실험 1-6-나 — 서버가 먼저 닫을 때: TIME_WAIT

- 터미널 A: `./build/server`
- 터미널 B: `curl http://127.0.0.1:8080/`
- 곧바로 터미널 C: `netstat -an -p tcp | grep '\.8080 '`
- 예상 질문: "연결은 이미 닫혔습니다. `LISTEN` 줄 말고 다른 줄이 남아 있을까요?"
- 기대 결과: `TIME_WAIT` 상태의 줄 하나가 약 30초 동안 보이고 사라집니다. 이 Mac의 `net.inet.tcp.msl`은 15000(밀리초)으로 측정되었고, TIME_WAIT은 그 두 배 동안 유지됩니다.
- 30초 뒤 같은 명령을 다시 실행해 사라졌는지 확인합니다.
- 주의: 서버의 `close()`와 `curl`의 닫기 중 어느 쪽 FIN이 먼저 나가는지는 타이밍에 달려 있습니다. `TIME_WAIT` 줄이 보이지 않으면 보이지 않았다고 기록합니다.

### 실험 1-6-다 — 클라이언트가 먼저 닫을 때: CLOSE_WAIT

- 사용자가 `close(conn_fd);` 줄 바로 위에 `sleep(15);`를 잠시 넣고 빌드합니다.
- 터미널 A: `./build/server`
- 터미널 B: `nc 127.0.0.1 8080`을 실행한 뒤 바로 `Ctrl+C`
- 15초 안에 터미널 C: `netstat -an -p tcp | grep '\.8080 '`
- 예상 질문: "클라이언트는 닫았고 서버는 아직 `close()`를 부르지 않았습니다. 서버 쪽 연결은 어떤 상태일까요?"
- 기대 결과: 서버 쪽 줄이 `CLOSE_WAIT`, 클라이언트 쪽 줄이 `FIN_WAIT_2`
- 깨달음: CLOSE_WAIT은 "상대는 닫았는데 **제 프로그램이** 아직 `close()`를 부르지 않은" 상태입니다. 서버에 CLOSE_WAIT이 쌓여 있으면 `close()`를 빼먹은 것입니다.
- 실험이 끝나면 `sleep(15);`를 **지우고** 다시 빌드합니다. 지웠는지 파일을 읽어 확인합니다.

### 실험 1-6-라 — `SO_REUSEADDR`이 없으면

- 사용자가 `setsockopt` 블록 5줄을 잠시 주석으로 막고 빌드합니다. (`on` 변수를 쓰지 않는다는 경고가 나면 `int on = 1;` 줄도 함께 막습니다.)
- 터미널 A: `./build/server`
- 터미널 B: `curl http://127.0.0.1:8080/`
- 터미널 A: `Ctrl+C`로 서버를 끝내고 **곧바로** `./build/server`를 다시 실행합니다.
- 예상 질문: "서버 프로세스는 이미 끝났습니다. 바로 다시 띄울 수 있을까요?"
- 기대 결과: `bind: Address already in use`. 약 30초 뒤에는 다시 뜹니다.
- 실험이 끝나면 주석을 **풀고** 다시 빌드한 뒤, 같은 순서로 해서 이번에는 바로 뜨는지 확인합니다.

### 실험 1-6-마 — 한 번에 한 연결만 처리하는 한계

- 터미널 A: `./build/server`
- 터미널 B: `nc 127.0.0.1 8080` (아무것도 치지 않고 둡니다)
- 터미널 C: `curl -m 5 http://127.0.0.1:8080/`
- 예상 질문: "`curl`은 응답을 받을까요?"
- 기대 결과: `curl`이 5초 뒤 시간 초과로 끝납니다. 터미널 B에서 아무 글자나 치고 Enter를 누르면 서버가 첫 연결을 끝내고 다음 연결로 넘어갑니다.
- 깨달음: 제 프로그램이 첫 연결의 `recv()` 안에서 잠들어 있어서 `accept()`로 돌아가지 못합니다. 둘째 연결은 커널의 줄(backlog)에서 기다립니다. 이 한계가 6단계(동시성)를 하는 이유입니다.

### 실험 1-6-바 — 메모리 검사 빌드

- 터미널 A: `make asan` 뒤 `./build/server`
- 터미널 B: `curl http://127.0.0.1:8080/`을 세 번 실행합니다.
- 기대 결과: AddressSanitizer와 UndefinedBehaviorSanitizer의 오류 보고가 없습니다.
- 실험이 끝나면 `make clean` 뒤 `make`로 보통 빌드로 되돌립니다.

### 질문과 정답 기준

1. "TIME_WAIT은 연결의 어느 쪽에 생기나요?" — 먼저 닫은 쪽(먼저 FIN을 보낸 쪽)입니다.
2. "TIME_WAIT은 왜 있나요?" — 마지막 ACK가 사라졌을 때 상대가 다시 보낸 FIN에 답하기 위해서이고, 이전 연결의 늦게 도착한 패킷이 같은 주소 · 포트 조합의 새 연결에 섞이지 않게 하기 위해서입니다.
3. "서버에 CLOSE_WAIT 연결이 계속 늘어난다면 코드의 무엇을 의심해야 하나요?" — 어떤 경로에서 `close(conn_fd)`를 부르지 않는 것입니다.
4. "실험 1-6-마의 문제를 풀 방법을 아는 대로 말해 보세요." — 연결마다 프로세스를 만드는 방법(`fork`), 연결마다 스레드를 만들거나 스레드 풀(thread pool)을 쓰는 방법, 한 스레드가 여러 소켓을 번갈아 보는 방법(I/O 멀티플렉싱)이 있습니다. 하나라도 말하면 통과이고, 나머지는 6단계에서 다룬다고 알려 줍니다.

### 1단계 마무리

- 사용자가 실험용으로 바꾼 곳(`htons` 제거, `Content-Length` 변경, `close` 주석, `sleep(15)`, `setsockopt` 주석)이 모두 되돌려졌는지 파일을 읽어 확인합니다.
- `make clean && make`가 경고 0개로 끝나는지 확인합니다.
- 커밋 메시지 초안을 제안합니다. 예: `1단계: 요청 하나를 받아 고정 응답을 돌려주는 순차 서버`. 커밋은 사용자가 합니다.
- `01_tutorial_workorder.md` 9절의 형식으로 보고합니다.
- 2단계는 `03_stage2_to_9_curriculum.md`를 따릅니다. 사용자가 "2단계 시작"이라고 말하기 전에는 넘어가지 않습니다.
