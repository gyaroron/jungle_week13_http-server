# 1단계 기능별 상세 명세 — 소켓 하나로 요청 받고 고정 응답 보내기

## 문서 범위

- 기준 대본: [`mac_tutorial_package/02_stage1_sections.md`](mac_tutorial_package/02_stage1_sections.md), 1-1~1-6
- 프로젝트 루트: `/Users/gyaroron/Krafton_Jungle_SWAI_Lab/c-webserver-proxy`
- 목적: 각 구간의 목표, 행위 주체, 입력·출력, 정상·오류 흐름, 구현 로직, 확인 기준을 한 문서에서 추적한다.
- 이 문서는 명세와 구간별 핵심 코드 조각이다. 전체 `src/server.c`의 교체본이 아니며, 해당 파일을 수정하지 않았다.
- C 코드 블록은 각 구간에서 추가·변경되는 핵심 로직이다. 대본의 앞선 줄을 생략해도 된다는 뜻은 아니다. 실제 입력은 반드시 해당 구간 대본과 대조한다.

## 전체 목표와 범위

1단계의 결과물은 macOS에서 실행되는 순차 TCP 서버다. 서버는 IPv4 loopback 주소 `127.0.0.1:8080`에 연결을 기다리고, 한 번에 연결 하나를 받아 요청 바이트를 읽고, 고정 HTTP 응답을 보낸 뒤 그 연결을 닫는다. 1-6에서는 같은 동작을 반복해 다음 연결을 받는다.

```text
서버 시작
  → socket 생성
  → 소켓 옵션 설정
  → 127.0.0.1:8080에 bind
  → listen 상태 진입
  → accept로 연결 하나 수락
  → recv로 현재 도착한 요청 바이트 읽기
  → 고정 HTTP 응답 전송
  → 연결 소켓 닫기
  → (1-6) 다음 연결을 기다리는 단계로 반복
```

### 범위에 포함되는 동작

- IPv4 TCP listening socket 생성, 주소 지정, 연결 대기
- 한 클라이언트 연결 수락 및 상대 IPv4 주소·포트 관찰
- `recv()`의 성공·상대 종료·오류 결과 구별
- 고정된 `200 OK` 응답 전송 및 부분 전송 반복 처리
- 1-6에서 연결 처리 반복, descriptor 재사용과 기본 TCP 종료 상태 관찰

### 범위에 포함되지 않는 기능

- HTTP 요청 전체를 완전하게 읽는 파서, 요청 경계 검출, 본문 처리
- 요청 경로에 따른 라우팅, 동적 응답, 정적 파일 제공
- 한 연결에서 여러 HTTP 요청을 처리하는 keep-alive
- 동시성, 프로세스·스레드 풀, 이벤트 루프
- 공개 네트워크 인터페이스에 바인딩하는 외부 공개 서버
- 인증·권한·TLS·속도 제한 등 운영 보안 기능

## 공통 기능 명세

| 항목 | 명세 |
|---|---|
| 실행 주체 | 서버 프로그램, 클라이언트 프로그램, 운영체제 커널 |
| 주소 범위 | IPv4 loopback `127.0.0.1`; 같은 Mac에서 접속하는 실험용 범위 |
| 서버 포트 | `8080`; `listen_fd` 및 각 연결의 서버 쪽 주소에 사용 |
| 연결 식별 | 각 연결은 클라이언트 IP·포트와 서버 IP·포트 조합으로 구분 |
| 듣는 descriptor | `listen_fd`; 새 연결 수락에 사용하고 클라이언트 데이터 전달에는 쓰지 않음 |
| 연결 descriptor | `conn_fd`; 수락한 클라이언트 한 명과 데이터 송수신 및 연결 종료에 사용 |
| 오류 정책 | 구간별 대본에 명시된 정책을 우선한다. 초기 시작 오류는 서버 종료, 1-6의 accept 오류는 로그 후 다음 반복으로 계속한다. |
| 관측 정책 | 예상 결과와 실제 결과를 구분한다. 출력·상태가 대본과 다르면 실제 관측을 기록한다. |

---

## 구간 1-1 — `socket()`으로 소켓 하나 만들기

### 기능 명세

| 항목 | 내용 |
|---|---|
| 목표 | 커널에 IPv4 TCP 소켓 생성을 요청하고 반환된 descriptor 번호를 출력한 뒤 닫는다. |
| 선행 조건 | 프로세스가 실행 중이며 표준 입력·출력·오류 descriptor가 열려 있다. |
| 입력 | `socket()` 인자 `AF_INET`, `SOCK_STREAM`, `0` |
| 정상 흐름 | 커널이 소켓 객체를 만들고 사용 가능한 descriptor를 돌려준다. 서버 프로그램은 `listen_fd`에 저장하고 번호를 출력한 뒤 `close()`한다. |
| 오류 흐름 | `socket()`이 `-1`을 반환하면 서버 프로그램은 `perror("socket")`로 실패 원인을 출력하고 종료 코드 1로 끝난다. |
| 결과 | 이 구간만으로는 주소가 연결되지 않고 `listen()`도 하지 않으므로 클라이언트 접속을 받을 수 없다. |
| 확인 기준 | 기본 실행에서 관측한 fd는 3이다. 표준 descriptor가 추가로 열려 있으면 다른 번호가 나올 수 있다. |

### 핵심 코드 로직

```c
int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
if (listen_fd < 0) {
    perror("socket");
    exit(1);
}
printf("listen_fd = %d\n", listen_fd);
close(listen_fd);
```

### 행위 흐름

1. 서버 프로그램이 `socket()`을 호출한다.
2. 커널은 IPv4 주소 체계와 TCP byte stream 조합의 소켓을 생성하고 descriptor를 반환한다.
3. 서버 프로그램은 descriptor가 음수인지 검사한다.
4. 성공이면 서버 프로그램이 번호를 출력하고 `close()`를 호출해 커널에 descriptor를 반환한다.

---

## 구간 1-2 — `setsockopt()`, `bind()`, `listen()`으로 LISTEN 상태 만들기

### 기능 명세

| 항목 | 내용 |
|---|---|
| 목표 | listening socket에 주소 `127.0.0.1:8080`을 지정하고 새 TCP 연결을 기다리는 상태로 만든다. |
| 선행 조건 | 1-1의 socket 생성이 성공했고 `listen_fd`가 유효하다. |
| 입력 | `SO_REUSEADDR` 옵션 값, IPv4 주소, 포트 `8080`, `BACKLOG` 값 `128` |
| 정상 흐름 | 서버 프로그램이 옵션을 설정하고 주소 구조체를 채운 뒤 `bind()`와 `listen()`을 차례로 호출한다. |
| 오류 흐름 | `setsockopt()`, `bind()`, `listen()` 중 하나가 실패하면 해당 함수 이름과 errno 원인을 출력하고 서버를 종료한다. |
| 결과 | 커널은 `listen_fd`를 통해 loopback 주소의 포트 8080에서 연결 요청을 기다린다. |
| 보안 경계 | `INADDR_LOOPBACK`은 같은 호스트의 loopback 인터페이스만 대상으로 한다. 이 대본은 외부 클라이언트 공개를 목표로 하지 않는다. |
| 옵션 한계 | `SO_REUSEADDR`은 재시작 뒤 주소 재바인딩을 돕지만, 다른 실행 중인 서버와 동일 주소·포트를 동시에 공유하게 해 주지 않는다. |
| 확인 기준 | 사용자가 수행한 `nc`, `netstat`, `lsof` 검사에서 포트와 LISTEN 상태를 확인한다. 실제 출력이 기준이다. |

### 핵심 코드 로직

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
```

### 행위 흐름

1. 서버 프로그램이 `setsockopt()`로 `listen_fd`의 주소 재사용 옵션을 설정한다.
2. 서버 프로그램이 `sockaddr_in`의 주소 체계·포트·IP 필드를 채운다.
3. `htons()`와 `htonl()`이 포트·주소 값을 네트워크 바이트 순서로 바꾼다.
4. 서버 프로그램이 `bind()`에 구조체 주소를 전달한다. 커널은 그 로컬 주소를 listening socket에 연결한다.
5. 서버 프로그램이 `listen()`을 호출한다. 커널은 해당 socket에서 연결 요청을 받을 준비를 한다.

---

## 구간 1-3 — `accept()`로 연결 하나 수락하기

### 기능 명세

| 항목 | 내용 |
|---|---|
| 목표 | 커널의 연결 대기열에서 연결 요청 하나를 받아 새 연결 descriptor를 얻고, 상대 주소를 출력한다. |
| 선행 조건 | `listen_fd`가 유효하고 LISTEN 상태다. |
| 입력 | `listen_fd`, 상대 주소를 기록할 `peer`, 주소 길이 `peer_len` |
| 대기 동작 | 대기 연결이 없으면 서버 프로그램은 `accept()`에서 기다린다. |
| 정상 흐름 | 커널은 연결을 수락하고 새 `conn_fd`를 반환하며, 상대 주소를 `peer`에 기록한다. |
| 오류 흐름 | `accept()`가 음수를 반환하면 서버 프로그램은 `perror("accept")`를 출력하고 1-3에서는 종료한다. 1-6에서는 연결 하나의 실패로 서버 전체를 끝내지 않도록 `continue`한다. |
| 결과 | `listen_fd`는 계속 연결을 받는 역할을 유지하고, `conn_fd`는 해당 클라이언트 연결의 송수신에 쓰인다. |
| 확인 기준 | `nc 127.0.0.1 8080` 연결 시 보통 `conn_fd`는 4이며, 출력 상대 포트는 클라이언트 임시 포트다. |

### 핵심 코드 로직

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
```

### 행위 흐름

1. 서버 프로그램은 클라이언트 주소를 기록할 메모리 공간 `peer`를 준비한다.
2. 서버 프로그램은 `accept()`에 `listen_fd`와 주소 저장 공간을 전달한다.
3. 커널은 연결을 수락한 뒤 새 `conn_fd`를 돌려주고 클라이언트의 IP·포트를 `peer`에 기록한다.
4. 서버 프로그램은 `inet_ntop()`으로 IP 주소를 문자열로 바꾸고 `ntohs()`로 포트를 호스트 바이트 순서로 바꾸어 출력한다.

---

## 구간 1-4 — `recv()`로 요청 바이트 읽기

### 기능 명세

| 항목 | 내용 |
|---|---|
| 목표 | 연결 소켓에서 현재 수신 가능한 바이트를 최대 4095바이트 읽고 서버 터미널에 출력한다. |
| 선행 조건 | `accept()`가 성공해 `conn_fd`가 유효하다. `BUF_SIZE`는 4096이다. |
| 입력 | `conn_fd`, `buf`, `sizeof(buf)-1`, flags 0 |
| 성공 흐름 | `recv()`가 읽은 바이트 수를 `n`에 반환한다. 프로그램은 `buf[n]`에 NUL을 넣고 byte count와 내용을 출력한다. |
| 상대 종료 | `recv()` 반환값 0은 오류가 아니라 상대가 송신 방향을 닫았음을 나타낸다. 프로그램은 `peer closed`를 출력한다. |
| 오류 흐름 | 반환값이 음수면 서버 프로그램은 `perror("recv")`를 출력한다. |
| 한계 | `recv()` 한 번은 HTTP 요청 한 개와 같지 않다. TCP는 byte stream이며 이 코드는 전체 요청 경계나 더 긴 본문을 처리하지 않는다. |
| 확인 기준 | `hello`와 Enter의 줄바꿈은 6바이트다. `curl` 실험에서 요청 줄·헤더가 출력되지만 서버가 응답을 보내지 않으면 curl은 빈 응답을 보고한다. |

### 핵심 코드 로직

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

### 행위 흐름

1. 서버 프로그램이 `recv()`를 호출한다.
2. 커널은 `conn_fd`의 수신 버퍼에서 도착한 데이터를 최대 4095바이트까지 `buf`로 복사한다. 데이터가 아직 없고 연결이 열려 있으면 호출은 기다릴 수 있다.
3. 반환값에 따라 프로그램이 오류, 상대 종료, 데이터 수신을 구분한다.
4. 데이터가 있으면 프로그램이 문자열 종료 표시를 직접 넣고 출력한다.

---

## 구간 1-5 — `send()`로 고정 HTTP 응답 보내기

### 기능 명세

| 항목 | 내용 |
|---|---|
| 목표 | 받은 데이터가 있는 경우 고정 HTTP/1.1 `200 OK` 응답을 연결 소켓으로 보내고 연결 종료를 알린다. |
| 선행 조건 | 1-4의 `recv()`가 양수 바이트를 반환했다. `signal(SIGPIPE, SIG_IGN)`이 설정되어 있다. |
| 입력 | `conn_fd`, 응답 문자열 `resp`, 응답 길이 `len` |
| 정상 흐름 | 프로그램은 문자열 리터럴을 이어 응답을 만들고, 전체 길이를 구한 뒤 `send()` 반환 바이트를 누적해 모두 전송한다. |
| 부분 전송 | `send()`가 요청한 길이보다 적게 보낼 수 있으므로 `sent < len` 동안 남은 구간을 다시 보낸다. |
| 오류 흐름 | `send()`가 음수를 반환하면 오류를 출력하고 전송 반복을 멈춘다. `SIGPIPE` 기본 종료 동작은 무시하도록 설정되어 있다. |
| 응답 형식 | 상태 줄, `Content-Type`, `Content-Length`, `Connection: close`, 빈 줄, 본문 순서다. `hello\n` 본문은 6바이트다. |
| 한계 | 요청 내용과 무관하게 같은 응답을 보낸다. HTTP 요청 파싱·동적 라우팅·keep-alive는 수행하지 않는다. |
| 확인 기준 | `curl -v`가 `HTTP/1.1 200 OK`와 본문 `hello`를 관찰한다. Content-Length 실험은 원래 값 6으로 복구되어야 한다. |

### 핵심 코드 로직

```c
signal(SIGPIPE, SIG_IGN);

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

### 행위 흐름

1. 서버 프로그램은 `SIGPIPE`의 기본 동작인 프로세스 종료를 무시하도록 설정한다.
2. 프로그램은 응답의 각 부분을 CRLF(CR: Carriage Return, LF: Line Feed)로 구분해 구성한다.
3. 프로그램은 `strlen()`으로 NUL 종단 문자를 제외한 응답 바이트 수를 계산한다.
4. 프로그램은 `send()`로 남은 응답을 보내고, 실제 보낸 양 `w`만큼 `sent`를 증가시킨다.
5. 모든 바이트를 보내거나 오류가 발생하면 연결 처리를 마친다.

---

## 구간 1-6 — 연결 처리 반복과 소켓 닫기

### 기능 명세

| 항목 | 내용 |
|---|---|
| 목표 | 한 연결의 처리를 마친 뒤 listening socket은 유지하고 다음 연결을 계속 받는다. |
| 선행 조건 | 1-5까지의 소켓 초기화·수락·수신·응답 흐름이 있다. |
| 반복 범위 | `accept()`부터 각 연결의 `close(conn_fd)`까지를 무한 반복문 안에 둔다. 초기화와 `listen_fd` 생성은 반복문 밖에 둔다. |
| 연결 오류 | `accept()` 오류는 출력하고 `continue`하여 서버 프로세스가 다음 연결을 기다리게 한다. |
| 연결 종료 | 매 연결 처리 후 `close(conn_fd)`를 호출해 descriptor를 커널에 반환한다. `listen_fd`는 반복 중 닫지 않는다. |
| 서버 종료 | 현재 대본은 Ctrl+C로 서버를 끝낸다. 무한 반복 뒤의 `close(listen_fd)`와 `return`은 도달할 수 없어 제거하도록 지정되어 있다. |
| 순차 처리 한계 | 첫 클라이언트가 `recv()`에서 기다리는 동안 서버 프로그램은 다음 `accept()`로 돌아가지 않는다. 커널은 후속 연결을 대기열에 둘 수 있지만, 응답 처리는 동시에 되지 않는다. |
| TCP 상태 확인 | FIN을 먼저 보낸 쪽, 상대 종료 후 애플리케이션이 아직 닫지 않은 쪽 등 종료 순서에 따라 `TIME_WAIT`, `CLOSE_WAIT`가 관찰될 수 있다. 상태와 지속 시간은 실측값으로 기록한다. |
| 확인 기준 | 여러 curl 요청에서 fd 재사용을 확인하고, `close(conn_fd)`를 빼면 descriptor 누수가 증가할 수 있음을 비교한다. 동시 처리 실험에서 첫 연결이 `recv()`를 막으면 두 번째 요청이 기다리는 것을 확인한다. |

### 핵심 코드 로직

```c
for (;;) {
    struct sockaddr_in peer;
    socklen_t peer_len = sizeof(peer);
    int conn_fd = accept(listen_fd, (struct sockaddr *)&peer, &peer_len);
    if (conn_fd < 0) {
        perror("accept");
        continue;
    }

    /* 상대 주소 기록, recv(), 응답 send()를 수행한다. */

    close(conn_fd);
}
```

위 주석은 구간 위치를 표시한 설명이며, 대본의 실제 코드를 대체하지 않는다. 반복문 안에는 1-3의 상대 주소 처리, 1-4의 수신 처리, 1-5의 응답 처리가 이어진다.

### 행위 흐름

1. 서버 프로그램은 반복문 밖에서 `listen_fd`를 만들고 주소에 묶어 LISTEN 상태로 둔다.
2. 반복문에서 `accept()`를 호출하고 연결 하나를 기다린다.
3. 커널이 연결 descriptor를 반환하면 프로그램은 그 연결에서 요청을 읽고 응답을 보낸다.
4. 프로그램은 `close(conn_fd)`로 해당 연결 자원을 반환한다.
5. 반복문은 다시 `accept()`로 돌아간다. `listen_fd`는 계속 열려 있다.

---

## 구간 사이 상태 전이 요약

| 구간 | 새로 추가되는 기능 | 서버가 할 수 있게 되는 일 |
|---|---|---|
| 1-1 | `socket()` | 커널 소켓을 만들고 fd 번호를 관찰 |
| 1-2 | 옵션·주소 지정·`listen()` | `127.0.0.1:8080` 연결 대기 |
| 1-3 | `accept()` 및 peer 주소 | 클라이언트 한 연결을 수락하고 출처 출력 |
| 1-4 | `recv()` | 도착한 데이터 일부를 읽고 출력 |
| 1-5 | HTTP 응답과 `send()` | 고정 응답을 보내 curl에 결과 반환 |
| 1-6 | 반복문과 연결별 close | 순차적으로 여러 연결을 처리 |

## 검토 시 주의할 설계 경계

- `BACKLOG`는 `listen()`에서 연결 대기 공간의 크기를 요청하는 값이다. 128이 8비트 대기열이라는 뜻은 아니다.
- `recv()`는 HTTP 메시지 전체를 반환한다는 보장이 없다. 1단계 코드는 의도적으로 단 한 번 읽는 실험용 구현이다.
- `send()`를 반복해도 응답 전체 전송 중 연결 오류가 생길 수 있다. 이 단계는 오류를 출력하고 현재 전송을 멈춘다.
- `127.0.0.1` 바인딩은 같은 호스트의 loopback 연결을 대상으로 한다. 외부 사용자에게 공개하려면 주소·방화벽·TLS 등 별도 검토가 필요하며, 1단계 범위에 넣지 않는다.
- `SO_REUSEADDR`는 이미 실행 중인 listener와 같은 주소·포트를 동시에 공유하도록 허용하는 옵션으로 설명하지 않는다.
- 1-6은 여러 연결을 차례로 처리할 뿐 동시 처리하지 않는다.

## 현재 자료와 진행 상태를 읽을 때의 주의

작성 시 확인 결과:

- 진행 지도 `STATE.current`는 `1-4`이며 `done`은 `1-1`, `1-2`, `1-3`이다.
- `docs/TUTORIAL_LOG.md` 상태 표에는 1-4와 1-5가 모두 진행 중으로 표시되어 있다.
- 실제 `src/server.c`에는 고정 HTTP 응답을 `send()`하는 1-5 로직까지 들어 있다.
- 위 세 자료의 표시가 서로 완전히 일치하지 않는다. 이 명세는 02 대본의 기능 범위를 설명할 뿐, 진행 지도·로그의 완료 상태를 변경하거나 새로 판정하지 않는다.
