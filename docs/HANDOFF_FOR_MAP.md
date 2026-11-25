# 설계도·진행 지도 인계 자료

이 문서는 현재 프로젝트의 실제 코드 위치를 설계도와 진행 지도에서 확인할 수 있도록 만든 인계 자료입니다. 발췌 행은 작성 시점의 소스 기준이며, 코드를 바꾸면 다시 생성해야 합니다.

- 확인 시각: 2026-11-25 15:15 KST
- 프로젝트 루트: `/Users/gyaroron/Krafton_Jungle_SWAI_Lab/c-webserver-proxy`
- 규칙: 코드가 없는 구간을 있는 것처럼 표시하지 않습니다.

## 파일 목록과 행 수

| 경로 | 행 수 | 역할 |
|---|---:|---|
| `src/cgi.c` | 266 | CGI fork/exec와 입출력 중계 |
| `src/config.c` | 100 | 명령줄 설정 |
| `src/conn.c` | 329 | 한 연결의 HTTP 상태 머신과 처리 분기 |
| `src/http.c` | 190 | 요청 줄·헤더·본문 길이 파싱 |
| `src/log.c` | 42 | 여러 스레드에서 줄 단위 로그 보호 |
| `src/main.c` | 196 | 인자·신호·listen·accept·스레드 풀 종료 |
| `src/mime.c` | 38 | 파일 확장자별 MIME 형식 |
| `src/net.c` | 232 | listen·connect·timeout·poll·send 반복 |
| `src/pool.c` | 164 | 고정 크기 pthread 스레드 풀과 대기열 |
| `src/proxy.c` | 406 | 리버스/포워드 프록시 요청·응답 중계 |
| `src/response.c` | 74 | HTTP 상태 줄·헤더·오류 응답 |
| `src/route.c` | 212 | 라우트 파일·호스트·경로·round-robin |
| `src/static.c` | 151 | 정적 파일 경로 검사·전송 |
| `include/cgi.h` | 10 | 헤더 선언 또는 프로젝트 설정 |
| `include/config.h` | 23 | 헤더 선언 또는 프로젝트 설정 |
| `include/conn.h` | 11 | 헤더 선언 또는 프로젝트 설정 |
| `include/http.h` | 42 | 헤더 선언 또는 프로젝트 설정 |
| `include/log.h` | 8 | 헤더 선언 또는 프로젝트 설정 |
| `include/mime.h` | 6 | 헤더 선언 또는 프로젝트 설정 |
| `include/net.h` | 18 | 헤더 선언 또는 프로젝트 설정 |
| `include/pool.h` | 15 | 헤더 선언 또는 프로젝트 설정 |
| `include/proxy.h` | 25 | 헤더 선언 또는 프로젝트 설정 |
| `include/response.h` | 13 | 헤더 선언 또는 프로젝트 설정 |
| `include/route.h` | 37 | 헤더 선언 또는 프로젝트 설정 |
| `include/static.h` | 10 | 헤더 선언 또는 프로젝트 설정 |
| `tests/test_http.c` | 91 | HTTP 파서 단위 검사 |
| `tests/test_mime.c` | 21 | MIME 단위 검사 |
| `tests/test_route.c` | 46 | 라우팅 단위 검사 |
| `scripts/check_env.sh` | 45 | 헤더 선언 또는 프로젝트 설정 |
| `scripts/functional_test.sh` | 225 | 통합 기능·동시성·자원 검사 |
| `Makefile` | 61 | 빌드 및 검사 대상 |
| `README.md` | 108 | 사용법·범위·측정 요약 |
| `routes.conf.example` | 2 | 리버스 프록시 규칙 예시 |
| `docs/TUTORIAL_LOG.md` | 1032 | 실제 진행·검사 기록 |
| `docs/benchmarks/apachebench_2026-11-25.txt` | 694 | ApacheBench 원문 |
| `docs/benchmarks/summary_2026-11-25.md` | 12 | 부하 측정 요약 |
| `docs/benchmarks/leaks_2026-11-25.txt` | 9 | leaks 실행 결과 |
| `docs/HANDOFF_FOR_MAP.md` | 1861 | 설계도와 지도 인계 자료; 이 표를 포함한 최종 행 수 |

## 공개 함수 선언과 선언 위치

| 파일:행 | 선언 원문 |
|---|---|
| `include/cgi.h:7` | `int cgi_serve(int fd, const struct config *cfg, const struct http_request *req, const char *body, size_t body_len);` |
| `include/config.h:20` | `int config_parse(int argc, char **argv, struct config *cfg);` |
| `include/config.h:21` | `void config_usage(const char *program);` |
| `include/conn.h:9` | `void conn_handle(int fd, const char *client_ip, const struct config *cfg);` |
| `include/http.h:39` | `const char *http_header_get(const struct http_request *req, const char *name);` |
| `include/http.h:40` | `const char *http_result_name(enum http_result result);` |
| `include/log.h:5` | `void server_log(const char *format, ...);` |
| `include/log.h:6` | `void server_log_errno(const char *operation, int error_number);` |
| `include/mime.h:4` | `const char *mime_from_path(const char *path);` |
| `include/net.h:10` | `int net_listen(const char *host, int port, int backlog);` |
| `include/net.h:11` | `int net_connect(const char *host, const char *port, int timeout_ms);` |
| `include/net.h:12` | `int net_send_all(int fd, const void *buf, size_t len);` |
| `include/net.h:13` | `int net_set_timeouts(int fd, int seconds);` |
| `include/net.h:14` | `int net_set_nonblock(int fd);` |
| `include/net.h:15` | `int net_set_blocking(int fd);` |
| `include/net.h:16` | `int net_wait(int fd, short events, int timeout_ms);` |
| `include/pool.h:10` | `int pool_submit(struct pool *p, int fd);` |
| `include/pool.h:13` | `void pool_shutdown(struct pool *p);` |
| `include/proxy.h:16` | `int proxy_reverse(int client_fd, const struct http_request *req, const char *body, size_t body_len, const struct config *cfg, const char *client_ip);` |
| `include/proxy.h:21` | `int proxy_forward(int client_fd, const struct http_request *req, const char *body, size_t body_len, int timeout_sec, const char *client_ip);` |
| `include/response.h:6` | `const char *resp_reason(int status);` |
| `include/response.h:7` | `int resp_send_head(int fd, int status, const char *content_type, long content_length, int keep_alive);` |
| `include/response.h:9` | `int resp_send_simple(int fd, int status, const char *content_type, const char *body, size_t len, int keep_alive);` |
| `include/response.h:11` | `int resp_send_error(int fd, int status);` |
| `include/route.h:25` | `int route_load(const char *path, struct route *routes, int max);` |
| `include/route.h:32` | `const struct backend *route_pick(struct route *route);` |
| `include/route.h:35` | `void route_destroy(struct route *routes, int count);` |
| `include/static.h:8` | `int static_serve(int fd, const struct config *cfg, const struct http_request *req);` |

## 요청 하나가 지나가는 경로

### 듣는 소켓과 연결 수락

1. `src/main.c:96–108`이 `net_listen()`으로 서버 주소에 바인딩하고 수신 대기 상태를 준비합니다.
2. `src/main.c:124–147`이 `poll()` 뒤 `accept()`를 호출해 연결 fd를 받습니다.
3. `src/main.c:149–188`이 연결 소켓 모드·시간 제한·주소 로그를 정리하고 `pool_submit()`으로 넘깁니다.
4. `src/pool.c:27–50`의 일꾼이 `serve_connection()`을 호출하고, 이는 `conn_handle()`을 실행합니다.

### 정적 파일

`conn_handle()` 상태 전이는 `src/conn.c:232–275`에 있습니다. 일치하는 프록시 규칙이 없고 대상 경로가 `/cgi-bin/`이 아니면 `static_serve()`를 호출합니다. 파일 경로 허용 범위를 확인한 뒤 `src/static.c:117–149`에서 파일을 읽고 보냅니다.

### CGI

프록시 규칙이 없고 요청 대상이 `/cgi-bin/`이면 `src/conn.c:254–258`에서 `cgi_serve()`로 갑니다. `src/cgi.c:129–159`가 자식 환경을 `fork()` 전에 준비하고 `src/cgi.c:167–181` 자식은 `dup2()`, `close()`, `execve()`, `_exit()`를 수행합니다.

### 리버스 프록시

`src/conn.c:250–264` → `src/proxy.c:283–320` (`route_match()` → `route_pick()` → `net_connect()`) → `src/proxy.c:107–171` 요청 재작성 → `src/proxy.c:201–260` 응답 중계와 업스트림 fd 닫기.

### 포워드 프록시

`-f`가 켜져 있고 요청 대상이 `http://` 또는 `https://`이면 `src/conn.c:243–249`에서 `proxy_forward()`를 호출합니다. `src/proxy.c:323–405`가 absolute-form을 검사해 업스트림 주소와 경로로 나눕니다. `https://`는 400, CONNECT는 요청 파서에서 501입니다.

## 연결 상태와 전이 위치

`src/conn.c:18–26`의 상태 열거형은 `CONN_READING`, `CONN_PARSING`, `CONN_ROUTING`, `CONN_RESPONDING`, `CONN_KEEPALIVE_CHECK`, `CONN_CLOSING`, `CONN_CLOSED`입니다. `conn_set_state()`는 `src/conn.c:56–60`이며, 실제 상태 전이 호출은 해당 파일의 102, 111, 127, 130, 140, 145, 177, 201, 209, 222, 227, 237, 270, 275, 281, 290, 293, 299, 314, 322행에 있습니다.

## 6~9단계 구현 상태와 확인한 편차

- 6-2~6-4: 고정 스레드 풀, 스레드 안전 로그·공유 라우트, SIGINT/SIGTERM 종료 drain/join을 구현하고 검사했습니다.
- 6-1 연결마다 스레드 모델은 별도 구현하지 않았습니다. 6-5 순차·연결별 스레드·풀 모델의 동일 조건 비교도 하지 않았습니다. 순차 기준선은 현재 작업 트리에서 찾지 못했고, 복원에 사용할 저장 상태 변경은 사용자 확인이 필요합니다.
- 7-1~7-5: timeout connect, 규칙 매칭·round-robin, 요청 재작성, 응답 중계, HTTP 포워드 프록시를 기능 검사했습니다.
- 7-6 캐시는 선택 항목이므로 진행하지 않았습니다.
- 8-1~8-5: 단위·기능·부하·메모리·자원·극한 검사를 실행했습니다. 세부 출력은 아래 측정 원문과 `TUTORIAL_LOG.md`에 있습니다.
- 9-1: `make`, `make test`는 실행했습니다. `-Werror`는 작업 지시서에서 먼저 물으라고 하여 적용하지 않았고 clean clone은 검증하지 않았습니다.
- 9-2: `docker`가 없어서 Linux 빌드를 하지 않았습니다.
- 9-3 README를 기록했습니다.

## 8단계 부하 측정 원문

다음 텍스트는 실제 ApacheBench 실행 기록 전문입니다. 측정값은 macOS 한 대에서 측정한 로컬 결과입니다.

```text
Date: 2026-11-25 KST
Build: make (C11, -Wall -Wextra -Wpedantic -g -O0 -pthread)
Server: ./build/server -p 8080 -w 8 -t 5
Target: http://127.0.0.1:8080/
Each mode: ApacheBench 2.3, -n 10000 -c 100, three runs.


===== no_keepalive run 1 =====
This is ApacheBench, Version 2.3 <$Revision: 1923142 $>
Copyright 1996 Adam Twiss, Zeus Technology Ltd, http://www.zeustech.net/
Licensed to The Apache Software Foundation, http://www.apache.org/

Benchmarking 127.0.0.1 (be patient)
Completed 1000 requests
Completed 2000 requests
Completed 3000 requests
Completed 4000 requests
Completed 5000 requests
Completed 6000 requests
Completed 7000 requests
Completed 8000 requests
Completed 9000 requests
Completed 10000 requests
Finished 10000 requests


Server Software:        c-webserver-proxy/1.0
Server Hostname:        127.0.0.1
Server Port:            8080

Document Path:          /
Document Length:        169 bytes

Concurrency Level:      100
Time taken for tests:   0.984 seconds
Complete requests:      10000
Failed requests:        0
Total transferred:      3360000 bytes
HTML transferred:       1690000 bytes
Requests per second:    10166.16 [#/sec] (mean)
Time per request:       9.837 [ms] (mean)
Time per request:       0.098 [ms] (mean, across all concurrent requests)
Transfer rate:          3335.77 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.2      0       3
Processing:     2   10   1.0     10      13
Waiting:        0   10   1.0     10      13
Total:          3   10   0.9     10      13

Percentage of the requests served within a certain time (ms)
  50%     10
  66%     10
  75%     10
  80%     10
  90%     11
  95%     12
  98%     12
  99%     12
 100%     13 (longest request)
ab_exit=0

===== no_keepalive run 2 =====
This is ApacheBench, Version 2.3 <$Revision: 1923142 $>
Copyright 1996 Adam Twiss, Zeus Technology Ltd, http://www.zeustech.net/
Licensed to The Apache Software Foundation, http://www.apache.org/

Benchmarking 127.0.0.1 (be patient)
Completed 1000 requests
Completed 2000 requests
Completed 3000 requests
Completed 4000 requests
Completed 5000 requests
Completed 6000 requests
Completed 7000 requests
Completed 8000 requests
Completed 9000 requests
Completed 10000 requests
Finished 10000 requests


Server Software:        c-webserver-proxy/1.0
Server Hostname:        127.0.0.1
Server Port:            8080

Document Path:          /
Document Length:        169 bytes

Concurrency Level:      100
Time taken for tests:   1.046 seconds
Complete requests:      10000
Failed requests:        0
Total transferred:      3360000 bytes
HTML transferred:       1690000 bytes
Requests per second:    9556.20 [#/sec] (mean)
Time per request:       10.464 [ms] (mean)
Time per request:       0.105 [ms] (mean, across all concurrent requests)
Transfer rate:          3135.63 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.2      0       2
Processing:     1   10   2.5     10      35
Waiting:        0   10   2.5     10      35
Total:          2   10   2.4     10      35

Percentage of the requests served within a certain time (ms)
  50%     10
  66%     10
  75%     11
  80%     11
  90%     12
  95%     12
  98%     13
  99%     16
 100%     35 (longest request)
ab_exit=0

===== no_keepalive run 3 =====
This is ApacheBench, Version 2.3 <$Revision: 1923142 $>
Copyright 1996 Adam Twiss, Zeus Technology Ltd, http://www.zeustech.net/
Licensed to The Apache Software Foundation, http://www.apache.org/

Benchmarking 127.0.0.1 (be patient)
Completed 1000 requests
Completed 2000 requests
Completed 3000 requests
Completed 4000 requests
Completed 5000 requests
Completed 6000 requests
Completed 7000 requests
Completed 8000 requests
Completed 9000 requests
Completed 10000 requests
Finished 10000 requests


Server Software:        c-webserver-proxy/1.0
Server Hostname:        127.0.0.1
Server Port:            8080

Document Path:          /
Document Length:        169 bytes

Concurrency Level:      100
Time taken for tests:   0.972 seconds
Complete requests:      10000
Failed requests:        0
Total transferred:      3360000 bytes
HTML transferred:       1690000 bytes
Requests per second:    10283.50 [#/sec] (mean)
Time per request:       9.724 [ms] (mean)
Time per request:       0.097 [ms] (mean, across all concurrent requests)
Transfer rate:          3374.27 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.1      0       2
Processing:     1   10   0.9     10      13
Waiting:        0    9   0.8     10      12
Total:          2   10   0.8     10      13
WARNING: The median and mean for the waiting time are not within a normal deviation
        These results are probably not that reliable.

Percentage of the requests served within a certain time (ms)
  50%     10
  66%     10
  75%     10
  80%     10
  90%     10
  95%     11
  98%     11
  99%     11
 100%     13 (longest request)
ab_exit=0

===== keepalive run 1 =====
This is ApacheBench, Version 2.3 <$Revision: 1923142 $>
Copyright 1996 Adam Twiss, Zeus Technology Ltd, http://www.zeustech.net/
Licensed to The Apache Software Foundation, http://www.apache.org/

Benchmarking 127.0.0.1 (be patient)
Completed 1000 requests
Completed 2000 requests
Completed 3000 requests
Completed 4000 requests
Completed 5000 requests
Completed 6000 requests
Completed 7000 requests
Completed 8000 requests
Completed 9000 requests
Completed 10000 requests
Finished 10000 requests


Server Software:        c-webserver-proxy/1.0
Server Hostname:        127.0.0.1
Server Port:            8080

Document Path:          /
Document Length:        169 bytes

Concurrency Level:      100
Time taken for tests:   30.693 seconds
Complete requests:      10000
Failed requests:        48
   (Connect: 0, Receive: 0, Length: 48, Exceptions: 0)
Keep-Alive requests:    9952
Total transferred:      3393973 bytes
HTML transferred:       1682057 bytes
Requests per second:    325.80 [#/sec] (mean)
Time per request:       306.935 [ms] (mean)
Time per request:       3.069 [ms] (mean, across all concurrent requests)
Transfer rate:          107.98 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   1.2      0      37
Processing:     0   99 1290.6      0   30655
Waiting:        0  138 1648.0      0   30655
Total:          0   99 1290.7      0   30657

Percentage of the requests served within a certain time (ms)
  50%      0
  66%      1
  75%      1
  80%      1
  90%      1
  95%      1
  98%      1
  99%     38
 100%  30657 (longest request)
ab_exit=0

===== keepalive run 2 =====
This is ApacheBench, Version 2.3 <$Revision: 1923142 $>
Copyright 1996 Adam Twiss, Zeus Technology Ltd, http://www.zeustech.net/
Licensed to The Apache Software Foundation, http://www.apache.org/

Benchmarking 127.0.0.1 (be patient)
Completed 1000 requests
Completed 2000 requests
Completed 3000 requests
Completed 4000 requests
Completed 5000 requests
Completed 6000 requests
Completed 7000 requests
Completed 8000 requests
Completed 9000 requests
Completed 10000 requests
Finished 10000 requests


Server Software:        c-webserver-proxy/1.0
Server Hostname:        127.0.0.1
Server Port:            8080

Document Path:          /
Document Length:        169 bytes

Concurrency Level:      100
Time taken for tests:   30.702 seconds
Complete requests:      10000
Failed requests:        48
   (Connect: 0, Receive: 0, Length: 48, Exceptions: 0)
Keep-Alive requests:    9952
Total transferred:      3393632 bytes
HTML transferred:       1681888 bytes
Requests per second:    325.71 [#/sec] (mean)
Time per request:       307.017 [ms] (mean)
Time per request:       3.070 [ms] (mean, across all concurrent requests)
Transfer rate:          107.94 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.2      0       3
Processing:     0  100 1293.0      0   30697
Waiting:        0  138 1651.4      0   30697
Total:          0  100 1293.1      0   30700

Percentage of the requests served within a certain time (ms)
  50%      0
  66%      1
  75%      1
  80%      1
  90%      1
  95%      1
  98%      1
  99%      3
 100%  30700 (longest request)
ab_exit=0

===== keepalive run 3 =====
This is ApacheBench, Version 2.3 <$Revision: 1923142 $>
Copyright 1996 Adam Twiss, Zeus Technology Ltd, http://www.zeustech.net/
Licensed to The Apache Software Foundation, http://www.apache.org/

Benchmarking 127.0.0.1 (be patient)
Completed 1000 requests
Completed 2000 requests
Completed 3000 requests
Completed 4000 requests
Completed 5000 requests
Completed 6000 requests
Completed 7000 requests
Completed 8000 requests
Completed 9000 requests
Completed 10000 requests
Finished 10000 requests


Server Software:        c-webserver-proxy/1.0
Server Hostname:        127.0.0.1
Server Port:            8080

Document Path:          /
Document Length:        169 bytes

Concurrency Level:      100
Time taken for tests:   30.686 seconds
Complete requests:      10000
Failed requests:        47
   (Connect: 0, Receive: 0, Length: 47, Exceptions: 0)
Keep-Alive requests:    9953
Total transferred:      3393973 bytes
HTML transferred:       1682057 bytes
Requests per second:    325.89 [#/sec] (mean)
Time per request:       306.856 [ms] (mean)
Time per request:       3.069 [ms] (mean, across all concurrent requests)
Transfer rate:          108.01 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.2      0       3
Processing:     0  102 1327.0      0   30681
Waiting:        0  139 1658.8      0   30681
Total:          0  102 1327.1      0   30683

Percentage of the requests served within a certain time (ms)
  50%      0
  66%      1
  75%      1
  80%      1
  90%      1
  95%      1
  98%      1
  99%      3
 100%  30683 (longest request)
ab_exit=0


## Repeat with worker count matched to concurrency
Server: ./build/server -p 8080 -w 128 -t 5
Each mode: ApacheBench 2.3, -n 10000 -c 100, three runs.

===== workers128 no_keepalive run 1 =====
This is ApacheBench, Version 2.3 <$Revision: 1923142 $>
Copyright 1996 Adam Twiss, Zeus Technology Ltd, http://www.zeustech.net/
Licensed to The Apache Software Foundation, http://www.apache.org/

Benchmarking 127.0.0.1 (be patient)
Completed 1000 requests
Completed 2000 requests
Completed 3000 requests
Completed 4000 requests
Completed 5000 requests
Completed 6000 requests
Completed 7000 requests
Completed 8000 requests
Completed 9000 requests
Completed 10000 requests
Finished 10000 requests


Server Software:        c-webserver-proxy/1.0
Server Hostname:        127.0.0.1
Server Port:            8080

Document Path:          /
Document Length:        169 bytes

Concurrency Level:      100
Time taken for tests:   1.080 seconds
Complete requests:      10000
Failed requests:        0
Total transferred:      3360000 bytes
HTML transferred:       1690000 bytes
Requests per second:    9257.13 [#/sec] (mean)
Time per request:       10.802 [ms] (mean)
Time per request:       0.108 [ms] (mean, across all concurrent requests)
Transfer rate:          3037.50 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.3      0      16
Processing:     1   11   1.9     11      26
Waiting:        0   10   1.9     11      25
Total:          3   11   1.8     11      26

Percentage of the requests served within a certain time (ms)
  50%     11
  66%     11
  75%     12
  80%     12
  90%     13
  95%     13
  98%     14
  99%     17
 100%     26 (longest request)
ab_exit=0

===== workers128 no_keepalive run 2 =====
This is ApacheBench, Version 2.3 <$Revision: 1923142 $>
Copyright 1996 Adam Twiss, Zeus Technology Ltd, http://www.zeustech.net/
Licensed to The Apache Software Foundation, http://www.apache.org/

Benchmarking 127.0.0.1 (be patient)
Completed 1000 requests
Completed 2000 requests
Completed 3000 requests
Completed 4000 requests
Completed 5000 requests
Completed 6000 requests
Completed 7000 requests
Completed 8000 requests
Completed 9000 requests
Completed 10000 requests
Finished 10000 requests


Server Software:        c-webserver-proxy/1.0
Server Hostname:        127.0.0.1
Server Port:            8080

Document Path:          /
Document Length:        169 bytes

Concurrency Level:      100
Time taken for tests:   1.066 seconds
Complete requests:      10000
Failed requests:        0
Total transferred:      3360000 bytes
HTML transferred:       1690000 bytes
Requests per second:    9384.72 [#/sec] (mean)
Time per request:       10.656 [ms] (mean)
Time per request:       0.107 [ms] (mean, across all concurrent requests)
Transfer rate:          3079.36 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.3      0      18
Processing:     1   10   1.9     10      29
Waiting:        0   10   1.9     10      29
Total:          2   11   1.8     10      29

Percentage of the requests served within a certain time (ms)
  50%     10
  66%     11
  75%     11
  80%     11
  90%     12
  95%     12
  98%     13
  99%     18
 100%     29 (longest request)
ab_exit=0

===== workers128 no_keepalive run 3 =====
This is ApacheBench, Version 2.3 <$Revision: 1923142 $>
Copyright 1996 Adam Twiss, Zeus Technology Ltd, http://www.zeustech.net/
Licensed to The Apache Software Foundation, http://www.apache.org/

Benchmarking 127.0.0.1 (be patient)
Completed 1000 requests
Completed 2000 requests
Completed 3000 requests
Completed 4000 requests
Completed 5000 requests
Completed 6000 requests
Completed 7000 requests
Completed 8000 requests
Completed 9000 requests
Completed 10000 requests
Finished 10000 requests


Server Software:        c-webserver-proxy/1.0
Server Hostname:        127.0.0.1
Server Port:            8080

Document Path:          /
Document Length:        169 bytes

Concurrency Level:      100
Time taken for tests:   1.398 seconds
Complete requests:      10000
Failed requests:        0
Total transferred:      3360000 bytes
HTML transferred:       1690000 bytes
Requests per second:    7153.99 [#/sec] (mean)
Time per request:       13.978 [ms] (mean)
Time per request:       0.140 [ms] (mean, across all concurrent requests)
Transfer rate:          2347.40 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.4      0      20
Processing:     1   14  13.5     11     133
Waiting:        0   13  12.9     10     132
Total:          2   14  13.5     11     133

Percentage of the requests served within a certain time (ms)
  50%     11
  66%     12
  75%     15
  80%     15
  90%     17
  95%     21
  98%     57
  99%     99
 100%    133 (longest request)
ab_exit=0

===== workers128 keepalive run 1 =====
This is ApacheBench, Version 2.3 <$Revision: 1923142 $>
Copyright 1996 Adam Twiss, Zeus Technology Ltd, http://www.zeustech.net/
Licensed to The Apache Software Foundation, http://www.apache.org/

Benchmarking 127.0.0.1 (be patient)
Completed 1000 requests
Completed 2000 requests
Completed 3000 requests
Completed 4000 requests
Completed 5000 requests
Completed 6000 requests
Completed 7000 requests
Completed 8000 requests
Completed 9000 requests
Completed 10000 requests
Finished 10000 requests


Server Software:        c-webserver-proxy/1.0
Server Hostname:        127.0.0.1
Server Port:            8080

Document Path:          /
Document Length:        169 bytes

Concurrency Level:      100
Time taken for tests:   0.673 seconds
Complete requests:      10000
Failed requests:        0
Keep-Alive requests:    10000
Total transferred:      3410000 bytes
HTML transferred:       1690000 bytes
Requests per second:    14854.51 [#/sec] (mean)
Time per request:       6.732 [ms] (mean)
Time per request:       0.067 [ms] (mean, across all concurrent requests)
Transfer rate:          4946.67 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.2      0       2
Processing:     0    7   4.7      6      41
Waiting:        0    7   4.7      6      40
Total:          0    7   4.7      6      41

Percentage of the requests served within a certain time (ms)
  50%      6
  66%      8
  75%      9
  80%     10
  90%     13
  95%     16
  98%     20
  99%     23
 100%     41 (longest request)
ab_exit=0

===== workers128 keepalive run 2 =====
This is ApacheBench, Version 2.3 <$Revision: 1923142 $>
Copyright 1996 Adam Twiss, Zeus Technology Ltd, http://www.zeustech.net/
Licensed to The Apache Software Foundation, http://www.apache.org/

Benchmarking 127.0.0.1 (be patient)
Completed 1000 requests
Completed 2000 requests
Completed 3000 requests
Completed 4000 requests
Completed 5000 requests
Completed 6000 requests
Completed 7000 requests
Completed 8000 requests
Completed 9000 requests
Completed 10000 requests
Finished 10000 requests


Server Software:        c-webserver-proxy/1.0
Server Hostname:        127.0.0.1
Server Port:            8080

Document Path:          /
Document Length:        169 bytes

Concurrency Level:      100
Time taken for tests:   0.648 seconds
Complete requests:      10000
Failed requests:        0
Keep-Alive requests:    10000
Total transferred:      3410000 bytes
HTML transferred:       1690000 bytes
Requests per second:    15430.55 [#/sec] (mean)
Time per request:       6.481 [ms] (mean)
Time per request:       0.065 [ms] (mean, across all concurrent requests)
Transfer rate:          5138.49 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.1      0       2
Processing:     0    6   4.3      6      36
Waiting:        0    6   4.3      6      36
Total:          0    6   4.3      6      36

Percentage of the requests served within a certain time (ms)
  50%      6
  66%      7
  75%      9
  80%     10
  90%     12
  95%     15
  98%     17
  99%     20
 100%     36 (longest request)
ab_exit=0

===== workers128 keepalive run 3 =====
This is ApacheBench, Version 2.3 <$Revision: 1923142 $>
Copyright 1996 Adam Twiss, Zeus Technology Ltd, http://www.zeustech.net/
Licensed to The Apache Software Foundation, http://www.apache.org/

Benchmarking 127.0.0.1 (be patient)
Completed 1000 requests
Completed 2000 requests
Completed 3000 requests
Completed 4000 requests
Completed 5000 requests
Completed 6000 requests
Completed 7000 requests
Completed 8000 requests
Completed 9000 requests
Completed 10000 requests
Finished 10000 requests


Server Software:        c-webserver-proxy/1.0
Server Hostname:        127.0.0.1
Server Port:            8080

Document Path:          /
Document Length:        169 bytes

Concurrency Level:      100
Time taken for tests:   0.665 seconds
Complete requests:      10000
Failed requests:        0
Keep-Alive requests:    10000
Total transferred:      3410000 bytes
HTML transferred:       1690000 bytes
Requests per second:    15031.22 [#/sec] (mean)
Time per request:       6.653 [ms] (mean)
Time per request:       0.067 [ms] (mean, across all concurrent requests)
Transfer rate:          5005.51 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.1      0       2
Processing:     0    7   4.4      6      37
Waiting:        0    7   4.4      6      37
Total:          0    7   4.4      6      37

Percentage of the requests served within a certain time (ms)
  50%      6
  66%      8
  75%      9
  80%     10
  90%     12
  95%     15
  98%     18
  99%     20
 100%     37 (longest request)
ab_exit=0
```

## TUTORIAL_LOG.md 전문

<!-- 아래 영역은 생성 당시 docs/TUTORIAL_LOG.md를 그대로 복사한 내용입니다. -->

# 튜토리얼 진행 기록

이 파일을 `docs/TUTORIAL_LOG.md`로 복사해 씁니다. 구간 하나가 끝날 때마다 아래 "구간 기록 양식"을 복사해 채웁니다.

적는 규칙:

- 사용자가 실제로 한 말과, 터미널에 실제로 나온 출력만 적습니다.
- 대본의 기대 결과와 실제 출력이 다르면 실제 출력을 적고 "다름"이라고 표시합니다.
- 하지 않은 실험은 "하지 않음"과 이유를 적습니다.
- 재지 않은 숫자를 적지 않습니다.

---

## 진행 현황

진행 지도를 계속 유지한다. 구간의 시작·완료 상태가 바뀔 때 이 표와 프로젝트 상위 폴더의 [전체 진행 지도](../../c_webserver_proxy_progress_map.html)를 함께 갱신한다. 지도의 `STATE.asOf`에는 확인 날짜, `done`에는 완료가 확인된 구간, `current`에는 현재 진행 중인 구간을 기록한다. 구간 완료 후 다음 구간을 시작하기 전에는 `current`를 빈 문자열로 둔다. 다음 구간은 사용자가 진행을 결정한 뒤 표시한다.

| 단계 | 구간 | 상태 | 끝낸 날짜 | 커밋 |
|---|---|---|---|---|
| 1 | 1-1 | 통과 | 2026-11-21 | |
| 1 | 1-2 | 통과 | 2026-11-23 | |
| 1 | 1-3 | 통과 | 2026-11-24 | |
| 1 | 1-4 | 통과 | 2026-11-24 | |
| 1 | 1-5 | 설명 정리 완료 · 이해 확인은 미실시 | 2026-11-25 | |
| 1 | 1-6 | 통과 | 2026-11-25 | 핵심 확인 완료 |
| 2 | 2-1 | 통과 | 2026-11-25 | 파일 분리·빌드·curl 세 번 통과 |
| 2 | 2-2 | 통과 | 2026-11-25 | 상태 전환 순서 확인 |
| 2 | 2-3 | 통과 | 2026-11-25 | 분할 요청을 이어 읽고 응답 |
| 2 | 2-4 | 통과 | 2026-11-25 | net_send_all·asan·HTTP 200 통과 |
| 3 | 3-1 | 통과 | 2026-11-25 | 정상·잘못된 요청 줄 판정 |
| 3 | 3-2 | 통과 | 2026-11-25 | 헤더 파싱 구현·curl 및 거부 입력 확인 |
| 3 | 3-3 | 통과 | 2026-11-25 | Content-Length, 본문 7바이트, 2MB 제한의 HTTP 413 확인 |
| 3 | 3-4 | 통과 | 2026-11-25 | curl 재사용·close·요청 두 건 연속 수신 확인 |
| 3 | 3-5 | 통과 | 2026-11-25 | 파서 단위 검사·주소/정의되지 않은 동작 검사 통과 |
| 4 | 4-1 | 통과 | 2026-11-25 | -p 9090, -h, 잘못된 포트 거부 확인 |
| 4 | 4-2 | 통과 | 2026-11-25 | 강제 연결 종료 뒤 서버 생존 확인 |
| 4 | 4-3 | 통과 | 2026-11-25 | 유휴 연결·부분 요청 시간 초과 확인 |
| 4 | 4-4 | 통과 | 2026-11-25 | SIGINT 뒤 종료 코드 0 확인 |
| 4 | 4-5 | 통과 | 2026-11-25 | poll 기반 비차단 수락 확인 |
| 5 | 5-1 | 통과 | 2026-11-25 | 공통 응답·오류 상태 응답 확인 |
| 5 | 5-2 | 통과 | 2026-11-25 | 확장자·대소문자 MIME 단위 검사 통과 |
| 5 | 5-3 | 통과 | 2026-11-25 | 정적 파일·404·403·405·잘못된 인코딩 확인 |
| 5 | 5-4 | 미완료·현재 위치 미확인 | | CGI 코드가 있으나 마지막 순서 변경 뒤 기능 검사와 `make test` 미실시 |
| 6 | 6-1 | 남은 문제 | 2026-11-25 | 연결마다 스레드 모델을 별도 구현·측정하지 않음 |
| 6 | 6-2 | 통과 | 2026-11-25 | pthread 고정 스레드 풀·원형 대기열·조건 변수 구현 |
| 6 | 6-3 | 통과 | 2026-11-25 | 로그 잠금·라우트 round-robin 잠금·CGI fork 경계 확인, TSan 통과 |
| 6 | 6-4 | 통과 | 2026-11-25 | 대기열 처리·스레드 join·정리, 종료 실험 통과 |
| 6 | 6-5 | 남은 문제 | 2026-11-25 | 8·128 일꾼 수 측정은 했으나 순차·연결별 스레드 기준선 비교는 미실시 |
| 7 | 7-1 | 통과 | 2026-11-25 | 제한 시간 업스트림 연결 및 응답 대기 |
| 7 | 7-2 | 통과 | 2026-11-25 | 라우트 파서·호스트/최장 접두사·round-robin 단위 검사 통과 |
| 7 | 7-3 | 통과 | 2026-11-25 | 요청 헤더 재작성·hop-by-hop 제거·X-Forwarded-For 확인 |
| 7 | 7-4 | 통과 | 2026-11-25 | 응답 스트리밍·502/504 및 fd 수 확인 |
| 7 | 7-5 | 통과 | 2026-11-25 | HTTP 포워드 프록시 확인; CONNECT 501, https 400 |
| 7 | 7-6 (선택) | 시작 전 | | |
| 8 | 8-1 | 통과 | 2026-11-25 | HTTP·MIME·라우트 단위 검사 통과 |
| 8 | 8-2 | 통과 | 2026-11-25 | 21개 기능 검사 통과, 20개 동시 접속 포함 |
| 8 | 8-3 | 통과 · 한계 기록 | 2026-11-25 | 8·128 workers 각각 keep-alive 유무 3회씩 실측, 8 workers 실패도 보존 |
| 8 | 8-4 | 통과 | 2026-11-25 | ASan/UBSan·TSan·leaks 0바이트·fd·좀비·CLOSE_WAIT 확인 |
| 8 | 8-5 | 통과 | 2026-11-25 | 잘못된 요청·큰 요청·백엔드 timeout·클라이언트 중도 종료 뒤 서버 생존 |
| 9 | 9-1 | 남은 문제 | 2026-11-25 | 일반 빌드·make test 확인. -Werror 선택과 clean clone 확인 미완료 |
| 9 | 9-2 | 남은 문제 | 2026-11-25 | Docker 없음; Linux 빌드 하지 않음 |
| 9 | 9-3 | 통과 | 2026-11-25 | 사용 범위·제한·실측 수치를 README에 기록 |
| 9 | 9-4 | 통과 | 2026-11-25 | HANDOFF_FOR_MAP에 파일·행 수, 공개 선언, 호출 흐름, 측정 원문, 로그 전문 기록 |

## 2026-11-25 14:58 KST — 6~9단계 코드와 검증 이어서 진행

사용자는 6~9단계도 이어서 진행하라고 요청했습니다. 앞선 작업으로 실제 구현되어 있던 6~7단계 파일을 다시 확인하고, 8단계 부하·자원 검증과 9단계 문서화를 진행했습니다. 이 기록은 수업의 이해 완료를 뜻하지 않습니다.

### 구현 범위 확인

- 6-2~6-4: `src/pool.c` 고정 스레드 풀, `src/log.c` 한 줄 단위 로그 잠금, `src/route.c` 공유 라운드 로빈 잠금, `src/main.c` 신호 종료와 대기열 drain·join.
- 6-1 연결별 스레드 모델은 별도 구현하지 않았습니다. 6-5 순차·연결별 스레드·고정 풀의 동일 조건 비교도 하지 않았습니다. 과제 지시서가 기준선 복원을 위한 작업 트리·저장 상태 변경 전에 사용자 확인을 요구하므로 임의 복원을 하지 않았습니다.
- 7-1~7-5: 제한 시간 업스트림 연결, 규칙 파싱·최장 경로·round-robin, 요청 재작성, 응답 중계 및 502/504, HTTP 포워드 프록시가 코드와 검사에 포함됩니다. 7-6 캐시는 선택 구간이므로 시작하지 않았습니다.

### 빌드와 기능 검사

`make clean && make && make test && ./scripts/functional_test.sh`를 실행했습니다.

- C11, `-Wall -Wextra -Wpedantic -g -O0 -pthread` 빌드 경고 0개.
- HTTP 파서, MIME, 라우트 단위 검사 3개 실행 파일 모두 통과.
- 기능 검사 21개 모두 통과: 정적 200, 없는 파일 404, 루트 밖 403, 잘못된 요청 400, 지원하지 않는 메서드 501, 긴 대상 414, 큰 본문 413, CGI GET/POST, keep-alive 2요청, 라운드 로빈, 502, 504, CONNECT 501, https absolute-form 400, 포워드 프록시, 동시 접속 20개, 큰 응답 중 클라이언트 종료 뒤 서버 생존, 요청 뒤 생존, fd 수 변화 없음, 좀비 수 증가 없음.
- 새 중도 종료 검사는 희소 파일 `www/large-test.bin` 32 MiB를 일시 생성하고 `curl --limit-rate 16k --max-time 0.2`로 받다가 닫았습니다. 실제 curl 출력: `Operation timed out after 205 milliseconds with 16207 out of 33554432 bytes received`. 이후 정적 요청은 HTTP 200이었고 시험 파일은 스크립트 종료 때 지웠습니다.
- `netstat -an -p tcp | grep '\.8080 ' | grep CLOSE_WAIT` 결과는 빈 출력이었습니다.

### Sanitizer와 메모리·자원 검사

- `make test-asan && make asan && ./scripts/functional_test.sh`: 세 단위 검사와 기능 검사 21개 통과. AddressSanitizer/UndefinedBehaviorSanitizer 보고 없음.
- `make tsan && ./scripts/functional_test.sh`: 기능 검사 21개 통과. `build/functional_server.log`에서 `WARNING: ThreadSanitizer`, `data race`, `FATAL:`을 검색했으며 일치 항목 없음.
- `leaks --atExit -- ./build/server -p 18080 -w 4 -t 1`: 요청 2회를 보낸 뒤 서버를 SIGTERM으로 종료했습니다. 도구 출력에 `0 leaks for 0 total leaked bytes`가 있었습니다. macOS 보안 문구에는 프로세스가 debuggable이 아니라고 나왔지만 leaks 도구는 0바이트 결과를 출력하고 종료 코드 0으로 끝났습니다.
- 기능 스크립트의 서버 fd 행 개수는 요청 전후 같았고, 좀비 프로세스 수도 증가하지 않았습니다.

### 부하 측정

ApacheBench 2.3, `-n 10000 -c 100`, 정적 `/` (169 bytes), 서버와 클라이언트 모두 이 Mac, C11 `-g -O0` 보통 빌드에서 각 조건을 3회 측정했습니다. 전체 명령 출력은 `docs/benchmarks/apachebench_2026-11-25.txt`에 그대로 저장했습니다.

| 일꾼 수 | 조건 | Requests per second (1 / 2 / 3) | Failed requests (1 / 2 / 3) | Time per request (1 / 2 / 3) |
|---:|---|---|---|---|
| 8 | keep-alive 없음 | 10166.16 / 9556.20 / 10283.50 | 0 / 0 / 0 | 9.837 / 10.464 / 9.724 ms |
| 8 | `-k` keep-alive | 325.80 / 325.71 / 325.89 | 48 / 48 / 47 (모두 Length) | 306.935 / 307.017 / 306.856 ms |
| 128 | keep-alive 없음 | 9257.13 / 9384.72 / 7153.99 | 0 / 0 / 0 | 10.802 / 10.656 / 13.978 ms |
| 128 | `-k` keep-alive | 14854.51 / 15430.55 / 15031.22 | 0 / 0 / 0 | 6.732 / 6.481 / 6.653 ms |

관찰: worker 8개로 동시성 100의 keep-alive를 유지하면 스레드가 연결을 붙잡아 대기열 처리가 지연되고 위 실패가 발생했습니다. worker 128개 조건에서는 같은 측정 세 번이 모두 실패 0건이었습니다. 이는 특정 Mac·빌드·응답 크기·도구 조건에서 얻은 결과이며 일반 성능 보장은 아닙니다. 기본 worker 수를 조용히 바꾸지 않았습니다.

### 9단계 산출물 및 하지 않은 항목

- `README.md`를 갱신했습니다. 기능 범위, 인자, 아키텍처, 성능표, 알려진 제한과 실제 코드 작성 역할을 기록했습니다. 이번 과제에서 Codex가 코드 작성·수정을 했고 사용자가 구현 방향을 정했습니다.
- Docker가 설치되어 있지 않았습니다. Linux 검사는 하지 않았고 Docker/이미지를 설치·다운로드하지 않았습니다.
- 지시서가 먼저 확인하도록 한 `-Werror` 적용 여부는 보류했습니다. clean clone 검사는 하지 않았습니다. 원격 저장소 복제·공유·병합·발표도 하지 않았습니다.
- `docs/HANDOFF_FOR_MAP.md`와 지도 반영은 이 기록 뒤 이어서 작성해야 합니다.

## 구간 1-1 — `socket()`으로 소켓 하나 만들기

- 날짜: 2026-11-21
- 고친 파일과 끝났을 때의 줄 수: 사용자가 직접 작성한 `src/server.c`, 18행. 튜터는 소스 파일을 수정하지 않음.
- 빌드: 사용자가 공유한 `make` 출력에서 경고 0개, 오류 0개.

**실험**

| 실험 | 사용자의 예상 | 실제 출력 | 대본의 기대 결과와 같은가 |
|---|---|---|---|
| `./build/server` | 양수라고 예상. 설명 후 다음 빈 번호 `3`을 예상 | `listen_fd = 3` | 같음 |
| `./build/server 3< /dev/null` | `4` | `listen_fd = 4` | 같음 |

**질문**

| 질문 | 사용자의 답 (요약) | 몇 번째에 통과했나 |
|---|---|---|
| 왜 `listen_fd`가 `3`인가? | 0은 standard input, 1은 standard output, 2는 standard error이며 `socket()`이 3을 반환한다고 답함 | 1회 |
| 지금 만든 소켓으로 다른 프로그램이 접속할 수 있는가? | `bind()`와 `listen()`을 아직 호출하지 않았으므로 접속할 수 없다고 답함 | 1회 |

**사용자가 헷갈린 곳과 풀린 방법**

- `socket()` 시스템 호출 번호 `97`과 반환된 파일 디스크립터 번호를 혼동할 수 있어 두 번호의 역할이 다름을 구분함.
- 표준 입력·출력·오류 번호는 Apple 문서 근거로 확인함.

**대본 · 설계서와 달랐던 곳**

- 없음.

**확인하지 못한 것과 이유**

- 실행 결과의 `close()` 이후 fd 재사용은 관찰하지 않음. 1-1 대본의 실험 목표 밖임.

## 구간 1-2 — `bind()`와 `listen()`으로 LISTEN 상태 만들기

- 날짜: 2026-11-23
- 고친 파일과 끝났을 때의 줄 수: 사용자가 직접 작성한 `src/server.c`, 54행. 튜터는 소스 파일을 수정하지 않음.
- 빌드: 사용자가 공유한 `make` 출력에서 경고 0개, 오류 0개.

**실험**

| 실험 | 사용자의 예상 | 실제 출력 | 대본의 기대 결과와 같은가 |
|---|---|---|---|
| `./build/server` | `listening on 127.0.0.1:8080 (fd 3)` 예상 | `listening on 127.0.0.1:8080 (fd 3)` | 같음 |
| 서버 실행 중 `nc -z 127.0.0.1 8080; echo $?` | `3`이라고 예상 | `Connection to 127.0.0.1 port 8080 [tcp/http-alt] succeeded!` 다음 줄에 `0` | 대본 기대값 `0`과 같음. 예상 답은 다름 |
| `lsof -nP -iTCP:8080 -sTCP:LISTEN` | 해당 없음 | `server 41888 gyaroron 3u IPv4 0xaffd2eeeda9dfb3b 0t0 TCP 127.0.0.1:8080 (LISTEN)` | 같음 |
| `netstat -an -p tcp \| grep '\.8080 '` | 해당 없음 | `tcp4 0 0 127.0.0.1.8080 *.* LISTEN` | 같음 |
| 첫 서버 실행 중 두 번째 `./build/server` | `SO_REUSEADDR`를 설정해도 같은 주소·포트를 함께 쓸 수 없어 두 번째 실행이 실패한다고 예상 | `bind: Address already in use` | 같음 |
| 서버 종료 뒤 `./build/server` 재실행 | 해당 없음 | `listening on 127.0.0.1:8080 (fd 3)` 출력. 이후 Control+C로 종료 | 재실행 성공 확인 |

사전 상태 확인 `nc -z ...`의 서버 실행 전 출력은 공유되지 않았으므로 확인하지 않음. `htons()`를 제거하는 선택 실험은 진행하지 않음.

**질문**

| 질문 | 사용자의 답 (요약) | 몇 번째에 통과했나 |
|---|---|---|
| `bind()`는 누가 누구에게 무엇을 하는 호출인가? | 프로그램이 커널에게 할당된 주소를 지정하라고 요청한다고 답함. 프로그램이 지정한 로컬 주소를 소켓에 연결한다는 점을 보충함 | 1회 |
| `listen()`의 둘째 인자를 1로 줄이면 무엇이 달라지는가? | 첫 답에서 128을 비트 수 및 통신 빈도와 연결했으나, 힌트 뒤 커널의 연결 대기 공간이 128에서 1로 줄어든다고 답함 | 2회 |
| `127.0.0.1` 대신 `INADDR_ANY`를 쓰면 무엇이 달라지는가? | loopback만이 아니라 모든 로컬 주소로 접속 범위가 넓어진다고 답함. 방화벽과 실제 네트워크 접근 가능성은 별도 조건이라고 보충함 | 1회 |

**사용자가 헷갈린 곳과 풀린 방법**

- 서버의 fd 번호 `3`과 `nc` 종료 코드 `0`을 혼동함. `nc`는 연결 성공 여부를 종료 코드로 출력한다는 점을 구분함.
- `BACKLOG`의 값 128을 8비트 대기 큐로 해석하고 byte order와 연결 빈도를 연관 지었음. `listen()`의 값은 연결 대기 공간의 크기 지정값이라는 힌트 뒤 정정함.
- `INADDR_ANY`가 접근 제어를 해제하는 것은 아니며, 서버가 모든 로컬 인터페이스 주소에 바인딩한다는 점을 보충함. 실제 외부 접근은 네트워크 경로와 방화벽 설정에도 좌우됨.

**대본 · 설계서와 달랐던 곳**

- 없음.

**확인하지 못한 것과 이유**

- 서버를 실행하기 전 `nc` 포트 검사 결과는 공유되지 않음.
- `htons()` 제거 선택 실험은 선택 사항이며 진행하지 않음.
- 실제 다른 기기에서 접속할 수 있는지는 검사하지 않음.

## 구간 1-3 — `accept()`로 연결 하나 받기

- 날짜: 2026-11-24
- 고친 파일과 끝났을 때의 줄 수: 사용자가 직접 작성한 `src/server.c`, 67행. 튜터는 소스 파일을 수정하지 않음.
- 빌드: 사용자가 공유한 `make` 출력에서 컴파일과 링크가 완료되었고, 경고·오류 메시지는 없었음.

**실험**

| 실험 | 사용자의 예상 | 실제 출력 | 대본의 기대 결과와 같은가 |
|---|---|---|---|
| 첫 `./build/server`와 `nc 127.0.0.1 8080` 연결 | `conn_fd = 4`, 클라이언트 포트 `8080`으로 예상 | `accepted fd 4 from 127.0.0.1:50007` | 부분 일치. fd는 맞고, 클라이언트 포트 예상은 다름 |
| 서버를 새로 실행해 두 번 더 연결 | 매번 fd가 `4`, 클라이언트 포트는 달라질 것으로 안내 | `accepted fd 4 from 127.0.0.1:50031`, `accepted fd 4 from 127.0.0.1:50033` | 같음 |

**질문**

| 질문 | 사용자의 답 (요약) | 몇 번째에 통과했나 |
|---|---|---|
| `listen_fd`와 `conn_fd`는 무엇이 다른가? | 처음에는 `client_fd`를 기다린다고 표현했으나, 정정 뒤 `listen_fd`는 서버의 연결 요청을 기다리는 소켓 번호이고 `conn_fd`는 특정 클라이언트 연결에 쓰인다고 설명함 | 정정 뒤 통과 |
| 클라이언트 100명이 접속하면 서버 포트 8080이 100개 필요한가? | 서버 포트 8080은 하나로 유지되고 `conn_fd` 번호가 달라질 것으로 예상함. 동시 연결에서 열린 fd는 각각 구별되며 닫은 fd 번호는 다시 사용될 수 있음을 보충함 | 1회 |
| 접속하는 클라이언트가 없으면 `accept()` 다음 줄은 언제 실행되는가? | 처음에는 `accept()`가 음수를 반환한다고 답했으나, 정정 뒤 클라이언트 연결 요청이 올 때까지 대기한다고 답함 | 정정 뒤 통과 |

**사용자가 헷갈린 곳과 풀린 방법**

- 서버가 기다리는 대상은 클라이언트 프로세스의 `client_fd` 번호가 아니라 연결 요청임을 구분함. `accept()`는 요청이 없으면 반환하지 않으며, 실제 오류가 있을 때 `-1`을 반환해 `perror()`와 `exit(1)` 경로로 들어감.
- 파일 디스크립터 번호와 포트 번호가 서로 다른 값임을 구분함. `conn_fd`는 서버 프로세스에서 소켓을 가리키는 번호이고, `8080`은 연결의 서버 쪽 주소에 쓰이는 포트임.
- 현재 코드는 `accept()`를 한 번 호출한 뒤 `conn_fd`와 `listen_fd`를 닫고 종료함. 여러 연결을 계속 받는 서버와 현재 1-3 프로그램의 동작을 구분함.

**대본 · 설계서와 달랐던 곳**

- 없음.

**확인하지 못한 것과 이유**

- 화면에 `nc 127.0.0.1 8080` 명령이 네 번 보였으나, 서버의 `accepted fd ...` 출력은 세 건만 확인됨. 출력이 연결된 세 실험만 기록했고, 나머지 한 명령이 연결에 성공했는지는 확인하지 않음.

## 구간 1-4 — 진행 중 실험 관측 및 지도 시각화

1-4구간은 진행 중입니다. 아래 표는 사용자가 공유한 출력으로 확인한 세 실험의 결과를 기록합니다. 구간 전체 완료로 처리하지 않습니다.

| 실험 | 실제로 확인한 결과 | 확인한 핵심 |
|---|---|---|
| `nc` 연결 상태와 데이터 수신 | `netstat`의 실제 행: `127.0.0.1.8080 → 127.0.0.1.53215 ESTABLISHED`, `127.0.0.1.53215 → 127.0.0.1.8080 ESTABLISHED`, `127.0.0.1.8080 → *.* LISTEN`. 서버 출력은 `---- 6 bytes ----`와 `hello` | 첫 두 행은 같은 연결의 서버 쪽·클라이언트 쪽 관점이고, 세 번째 행은 서버의 대기 소켓임. `hello` 5바이트와 줄바꿈 1바이트를 합쳐 서버가 6바이트를 받음 |
| `curl` HTTP 요청 수신 | 서버가 요청을 한 번의 `recv()`에서 77바이트로 출력. 클라이언트는 `curl: (52) Empty reply from server` 출력 | 연결과 요청 전달은 됐으나, 당시 서버 코드가 HTTP 응답을 보내지 않음 |
| 데이터 없이 클라이언트 종료 | `nc`에서 `Control+C` 입력 후 서버가 `peer closed` 출력 | 서버의 `recv()`가 0을 반환해 `n == 0` 분기로 들어감 |

**진행 지도 시각화**

- `c_webserver_proxy_progress_map.html`의 1-4 항목 아래에 세 실험을 실제 터미널 화면 재구성, 관측 출력, 확인한 의미로 표시함.
- 첫 번째 실험은 사용자가 공유한 실제 터미널 화면을 바탕으로 터미널 모양의 화면을 재구성함. `nc` 클라이언트와 서버 창을 위에 나란히 놓고, `netstat` 창을 아래에 배치함. 세 `netstat` 행의 주소·포트 순서와 상태를 그대로 표시함. `Local Address`(로컬 주소)와 `Foreign Address`(상대 주소)가 행마다 어느 끝점을 가리키는지도 함께 설명함.
- `hello` 입력과 서버의 6바이트 출력은 별도의 두 터미널 창으로 나란히 보여 줌. 이 출력은 `53215`가 표시된 연결 상태 캡처와 동일 실행이라고 합쳐 쓰지 않음.
- 두 번째 실험도 `curl` 클라이언트 창과 서버 창을 나란히 배치함. 클라이언트의 연결·요청·빈 응답 오류와 서버의 77바이트 요청 출력을 각각 기록함.
- 세 번째 실험도 `nc` 클라이언트 창과 서버 창을 나란히 배치함. 클라이언트의 `Control+C` 입력과 서버의 `peer closed` 출력을 연결해 표시함.
- 세 실험은 독립된 실험이므로 서로를 잇는 시간 흐름 화살표를 그리지 않음. 각 실험 카드는 전체 폭을 사용하고, 카드 안의 대응 터미널 창은 나란히 배치하며 좁은 화면에서는 세로로 쌓음.
- 사용자가 터미널 안 가로 스크롤 막대를 지적함. 터미널 출력은 명령 프롬프트와 `netstat` 행을 보존하면서 화면 폭에 맞춰 줄바꿈하도록 바꾸고, 터미널 화면 컨테이너의 가로 오버플로를 숨김. 작은 화면에서도 하단 가로 스크롤 막대가 생기지 않도록 함.
- 기존 실험의 실제 결과를 표시하며, 예상 결과나 일반적인 예시와 섞지 않음.

## 진행 지도 보완 기록 — 2026-11-22

사용자 요청에 따라 `c_webserver_proxy_progress_map.html`에 다음 항목을 추가했습니다.

- 페이지 맨 아래에 기본 닫힘 `<details>` 토글 두 개를 두었습니다. 첫 표는 지도와 1-2 구조 다이어그램에 나오는 축약 표기·영어 원문·한글 해설을 모읍니다. 둘째 표는 `sockaddr_in` 클래스 다이어그램의 `struct`, `+`, `:`, `*--`, `.`, `[8]` 표기를 풀이합니다. 약어표에는 다이어그램에서 사용한 SDK와 자료형 표기도 추가했습니다.
- 1-2 구간을 선택하면 `struct sockaddr_in`과 내부 필드 `struct in_addr`의 포함 관계를 클래스 다이어그램으로 표시합니다. SDK에 선언된 `sin_len`, `sin_family`, `sin_port`, `sin_addr`, `sin_zero`, `s_addr`와 실제 접근 경로 `addr.sin_addr.s_addr`를 반영했습니다.
- 다이어그램은 C의 `struct`를 UML 스타일로 표현한다는 점을 함께 표시해, C에 클래스나 공개 접근 제한이 있는 것처럼 오해하지 않도록 했습니다.
- 2026-11-22 추가 보완: 사용자가 요청한 외부 웹 검색으로 Mermaid 공식 클래스 다이어그램 문법을 확인한 뒤, 1-2 자료 구조 그림을 실제 클래스 상자와 `+`, `:`, `*--` 관계 기호로 렌더링하도록 바꿨습니다. 노드 간격과 상자 안 여백, 글자 크기, 줄 간격을 늘리고 구조체 멤버 경로를 간격이 있는 개별 표기로 보여 줍니다. 기호 표도 렌더링 문법과 맞췄습니다.
- 기호 표 추가 보완: 합성 관계 행에서 Mermaid 원문 `*--`만 보여 주지 않고, 다이아몬드가 포함하는 쪽에 붙고 실선이 반대쪽으로 이어지는 기호 모양을 함께 표시합니다. 표의 열을 그림 기호, Mermaid 입력, 뜻, 이 그림에서 읽는 방법으로 나누어 실제 보이는 기호와 입력 문법을 구분했습니다.
- 표 머리글 보완: 두 접기 표 모두 첫 번째 열의 머리글(`표기`, `기호`)은 화면 폭이 좁아도 줄바꿈되지 않도록 지정했습니다.
- 사용자 승인 뒤 색상 적용: `sockaddr_in`의 `+sin_addr : in_addr` 멤버 줄과 관계선 이름 `sin_addr`를 같은 파스텔 민트 색으로 표시합니다. 어두운 테마와 밝은 테마에서 각각 읽기 쉬운 민트 색 값을 사용하고, 그림 설명에도 같은 색이 두 표기를 연결한다는 점을 적었습니다.
- 민트 표시 수정: Mermaid가 출력하는 두 글자 표기를 찾아 벡터 텍스트와 내장 웹 문서 텍스트 양쪽에 같은 테마 색을 지정합니다.
- `*--`는 C 포인터를 뜻하지 않습니다. C의 `sin_addr` 필드가 `struct in_addr` 값을 직접 포함한다는 구조를 UML 합성 기호로 단순화해 보여 주며, UML 합성의 전체 수명 의미까지 C에 부여하지 않는다고 그림 아래와 기호 표에 적었습니다.
- 외부 기준: [Mermaid 공식 클래스 다이어그램 문법](https://mermaid.js.org/syntax/classDiagram)에서 멤버 공개 표기와 합성 관계 문법을 확인했습니다.
- 앞으로 사용자가 특정 직접 입력 구간에서 클래스 다이어그램을 요청하면, 사실과 맞는 구조만 해당 구간의 지도 상세 내용에 추가하고 이 기록에 추적합니다. 실제 C 자료구조와 UML 표기 차이도 함께 설명합니다.
- 변경 당시 진행 위치는 1-1 완료, 1-2 진행 중입니다. 이 지도 변경은 튜토리얼 구간 완료를 뜻하지 않습니다.

### 축약어 표의 표기 열 줄바꿈 규칙

- 진행 지도의 축약어 표에서 첫 열은 새로 추가하는 표기까지 모두 한 줄로 유지합니다. `SO_REUSEADDR`와 `SO_NOSIGPIPE`처럼 긴 식별자도 중간에서 나누지 않습니다.
- 줄바꿈 방지 규칙은 표기 열 전체에 적용합니다. 영어 원문과 한글 해설은 화면 폭에 따라 줄을 바꿀 수 있게 두고, 표가 화면보다 넓어지면 표 영역에서 가로로 스크롤합니다.
- 표를 늘리거나 스타일을 수정할 때는 좁은 화면에서 두 표기와 새로 추가한 긴 식별자가 잘리지 않고 한 줄에 표시되는지 확인합니다.

### Mermaid 민트 색상 미반영과 재발 대응

**증상과 원인.** 진행 지도 파일에 민트 색상 값은 저장되어 있었지만, 사용자가 브라우저에서 변경을 볼 수 없다고 보고했습니다. 최초 코드는 `text`와 `tspan`만 찾아 `fill`을 바꿨습니다. Mermaid가 글자를 SVG(Scalable Vector Graphics, 벡터 그래픽) 내부의 `foreignObject`에 HTML(HyperText Markup Language, 웹 문서 마크업 언어) 요소로 출력하면 이 검색에서 빠지며, 해당 글자에는 `color`가 필요합니다. 처음에는 파일에 코드가 저장된 것만 확인했고, 브라우저가 생성한 요소와 최종 글자색은 확인하지 않았습니다.

**재발 시 확인 순서**

1. 브라우저에서 진행 지도를 새로고침하고 1-2 구간을 엽니다. 개발자 도구의 요소 검사로 관계선 이름 `sin_addr`와 필드 줄 `+sin_addr : in_addr`을 각각 확인합니다.
2. 다이어그램이 있다면 두 글자가 `text`·`tspan`인지, `foreignObject` 안의 `span`·`p`·`div`인지 확인합니다. 다이어그램 자체가 없다면 콘솔 오류와 Mermaid 로딩부터 확인합니다.
3. 지도 파일의 `mermaid.run(...)` 완료 후 처리에서 실제 요소를 선택하도록 고칩니다. 공백을 정리한 뒤 대상 문자열 두 개만 일치시키고, 벡터 텍스트의 `fill`과 웹 문서 텍스트의 `color`에 같은 `var(--mint)`를 지정합니다.
4. 브라우저 개발자 도구에서 최종 계산된 색상을 확인합니다. 밝은 테마와 어두운 테마 각각에서 두 대상이 같은 민트이고, `sin_family`처럼 관련 없는 필드는 기본 색인지 확인합니다. 구간을 바꿨다가 1-2로 돌아와 다시 확인합니다.

**검증 범위.** 수정 당시 두 대상 문자열에만 스타일이 지정되는 것을 모의 요소로 확인했습니다. 실제 브라우저 화면에서의 색상 확인은 하지 않았습니다.

### 자료 구조 다이어그램 작성 규칙

- 자료 구조 다이어그램을 추가하거나 갱신할 때는 구조체마다 어떤 값을 담는지, 해당 구간의 코드가 그 값을 어디에 쓰는지 설명합니다. 구조체가 다른 구조체에 들어 있으면 필드 이름과 포함 방식도 함께 적습니다.
- 구조체 형식이 정의된 헤더와 프로젝트에서 변수를 선언하거나 필드를 사용하는 위치를 구분해, 각각 파일 경로와 행 번호를 적습니다. 편집으로 행 번호가 바뀔 수 있으므로 갱신할 때 원본 파일을 다시 확인합니다.
- 1-2 지도에 적용했습니다. macOS 헤더 `netinet/in.h`에서 `sockaddr_in`은 374행, `in_addr`는 301행에 정의됩니다. 프로젝트의 `src/server.c`에서는 33행에서 주소 변수를 선언하고 38행에서 루프백 주소를 설정하며 40행에서 `bind()`에 넘깁니다.

### 직접 치는 구간에서 코드와 지도 기록이 달라질 때

1. C 웹서버 · 프록시 진행 위치 지도에서 현재 ‘직접 치는 구간’을 확인하고, 실제 소스와 ‘구조체의 역할과 코드 위치’를 대조합니다. 이 규칙은 모든 직접 입력 구간에서 소스 경로나 행 번호를 인용하는 지도 설명과 다이어그램에 적용합니다.
2. 행 번호만 이동했거나 코드 내용이 바뀌었거나 인용한 코드를 찾을 수 없으면 기존 설명, 경로, 행 번호를 그대로 보존합니다.
3. 구간 번호, 기존 기록의 경로와 행, 현재 소스에서 확인한 경로와 행, 변경 내용을 사용자에게 알리고 지도에 어떻게 반영할지 먼저 질문합니다. 현재 위치를 찾지 못했다면 확인이 필요한 항목을 구체적으로 묻습니다.
4. 사용자 답을 받은 뒤 합의한 내용을 반영합니다. 기존 기록을 보존하고 변경 후 위치나 설명을 별도로 덧붙입니다.

### 매 답변 전 진행 지도 확인 규칙 — 2026-11-24

- 사용자는 모든 답변을 작성하기 전에 `c_webserver_proxy_progress_map.html`을 확인하도록 요청했습니다. 이 규칙을 `docs/mac_tutorial_package/01_tutorial_workorder.md` 4절에 반영했습니다.
- 확인 순서는 지도 `STATE.asOf`·`STATE.done`·`STATE.current`, 이 로그의 진행 현황, 실제 `src/server.c`입니다. 답변이 간단한 설명이나 질문이어도 확인을 생략하지 않습니다.
- 불일치가 있으면 기존 기록을 임의로 바꾸지 않고, 차이와 근거를 사용자에게 알린 뒤 지도 반영 방법을 묻습니다. 사용자가 방향을 정하기 전에는 진행 위치 변경이나 다음 구간 시작을 보류합니다.
- 규칙을 추가할 당시 확인한 차이: 진행 지도는 `current:"1-4"`, `done`은 `1-1`부터 `1-3`까지입니다. 로그는 1-4와 1-5를 진행 중으로 표시하고, 실제 `src/server.c`에는 1-5 `send()` 응답 코드가 있습니다. 지도 상태 변경은 하지 않았으며, 사용자의 진행 위치 선택을 기다립니다.
- 2026-11-25 업데이트: 사용자가 “이어서 진행”을 지시해 1-6을 시작했습니다. 1-4 실험은 완료했고, 1-5의 응답 코드와 `Content-Length` 실험도 확인했지만 핵심 문답 중 `send()` 반복과 `SIGPIPE` 이해 확인은 남아 있어 1-5를 `남은 문제`로 기록했습니다. 지도는 완료가 확인된 1-1부터 1-4까지만 `done`으로 두고 `current`를 1-6으로 설정했습니다. 1-6은 반복 `accept()`와 연결 종료 위치를 반영한 코드 변경 및 `make`까지 확인했으며, 1-6 실험은 아직 하지 않았습니다.

## 구간 1-6 — `accept()` 반복 시작 기록

- 날짜: 2026-11-25
- 변경 파일: `src/server.c` — `listen()` 성공 뒤 연결 대기·처리 코드를 무한 반복문(`for (;;)` )으로 감쌌습니다. `accept()` 실패 뒤 `exit(1)` 대신 `continue`를 사용하고, 연결마다 `close(conn_fd)`를 호출합니다. 반복문 바깥의 도달 불가능한 `close(listen_fd)`와 `return 0`은 제거했습니다.
- 빌드: `make` 완료, 경고 0개, 오류 0개.
- 실험 1-6-가 — 번호 재사용: VS Code 통합 터미널에서 서버를 실행하고 `curl -sS -w '\\nHTTP %{http_code}\\n' http://127.0.0.1:8080/`을 세 번 실행했습니다. 클라이언트는 세 번 모두 `hello`와 `HTTP 200`을 받았습니다. 서버는 매번 `accepted fd 4`를 출력했고, 클라이언트 임시 포트는 `49697`, `49703`, `49704`였습니다. 서버를 `Control+C`로 종료했습니다. `close(conn_fd)`가 연결마다 호출되어 서버 프로세스가 fd 번호 4를 반납하므로 다음 연결에서 다시 쓸 수 있음을 확인했습니다.
- 빌드 확인: VS Code 통합 터미널에서 `make`를 실행했고 `make: Nothing to be done for 'all'.`이 출력되었습니다. 기존 빌드가 최신이라 다시 컴파일할 필요가 없었습니다.
- 실험 1-6-나 — `TIME_WAIT`: 서버를 실행한 뒤 `curl http://127.0.0.1:8080/`을 호출하고 `netstat -an -p tcp | grep '\\.8080 '`을 실행했습니다. 첫 확인에서 `127.0.0.1.49738 → 127.0.0.1.8080` 연결이 `TIME_WAIT`으로 표시되었고, `LISTEN` 줄도 남아 있었습니다. 약 30초 뒤 다시 확인했을 때 `TIME_WAIT` 행은 사라지고 `LISTEN`만 남았습니다. 실험 서버는 `Control+C`로 종료했습니다.
- 실험 1-6-다 — `CLOSE_WAIT`: `close(conn_fd)` 바로 앞에 임시 주석과 `sleep(15);`를 넣고 빌드했습니다. `nc 127.0.0.1 8080` 연결을 끊은 직후 `netstat`에서 서버 쪽 `CLOSE_WAIT`, 클라이언트 쪽 `FIN_WAIT_2`, 서버 대기 소켓의 `LISTEN`을 관찰했습니다. 실험 뒤 임시 주석과 `sleep(15);`를 삭제하고 다시 `make`했습니다. 컴파일과 링크가 끝났으며 경고·오류는 없었습니다.
- 실험 1-6-마 — 한 번에 한 연결만 처리하는 한계: 서버가 `nc 127.0.0.1 8080`의 첫 연결에서 입력을 기다리는 동안 다른 터미널에서 `curl -m 5 -sS http://127.0.0.1:8080/`을 실행했습니다. `curl: (28) Operation timed out after 5000 milliseconds with 0 bytes received`와 종료 코드 28을 확인했습니다. 첫 연결에서 서버 프로그램이 `recv()`를 기다리는 동안 다음 `accept()`로 돌아오지 못해 두 번째 요청에 응답하지 못한 결과입니다. 이어서 같은 상황을 다시 만들고 `nc`에 `x`와 줄바꿈을 보냈습니다. 서버는 첫 연결의 2바이트를 처리한 뒤 `accepted fd 4 from 127.0.0.1:49994`로 대기하던 `curl` 연결을 받아 요청 77바이트를 읽었습니다. 클라이언트 `curl`은 `hello`를 받았습니다. 이 실험으로 첫 연결 처리가 끝나야 서버가 다음 연결을 처리한다는 점을 확인했습니다.
- 실험 1-6-라 — `SO_REUSEADDR` 없이 재시작: `setsockopt()` 설정과 `on` 선언을 잠시 주석 처리해 빌드했습니다. 서버를 실행하고 `curl`로 연결을 만든 뒤 서버를 `Control+C`로 끝내고 곧바로 재실행했을 때 `bind: Address already in use`가 출력됐습니다. 실험 뒤 `setsockopt()` 설정과 `on` 선언을 복구하고 `make`했습니다. 컴파일과 링크가 끝났으며 경고·오류는 없었습니다.
- 실험 1-6-바 — 메모리 검사 빌드: `make asan`으로 AddressSanitizer(주소 검사기)와 UndefinedBehaviorSanitizer(정의되지 않은 동작 검사기)를 켜서 빌드했습니다. 서버에서 `curl` 요청 세 건을 처리했고 세 응답은 모두 `hello`, `HTTP 200`이었습니다. 서버 터미널에 두 검사기의 오류 보고가 없었습니다. 이어 `make clean && make`로 일반 빌드를 복원하고 서버가 `127.0.0.1:8080 (fd 3)`에서 시작하는 것을 확인한 뒤 종료했습니다.
- 임시 변경 정리: `sleep(15);`와 `SO_REUSEADDR` 주석 처리를 제거·복구했습니다. 현재 `src/server.c`의 123행은 `close(conn_fd);`이며, 실험용 변경은 남아 있지 않습니다.
- 남은 확인: 1-6의 핵심 문답을 아직 하지 않아 구간 상태는 `진행 중`입니다. TCP(Transmission Control Protocol, 전송 제어 프로토콜) 연결에서 `TIME_WAIT`을 어느 쪽이 가지는지, `CLOSE_WAIT`이 서버 프로그램에서 어떤 종료 누락을 가리키는지, 순차 처리 실험에서 두 번째 요청이 기다린 이유를 사용자의 말로 확인합니다.
- 진행 지도 (이전 기록의 표기): `STATE.current`를 `1-6`으로, `STATE.done`을 `1-1`부터 `1-5`까지로 갱신했다고 적었습니다.
- 2026-11-25 대조 메모: 현재 실제 지도의 `STATE.done`은 `1-1`부터 `1-4`까지이고, 로그 현황표에서 1-5는 `남은 문제`입니다. 위 이전 기록과 현재 지도가 서로 다릅니다. 이전 문장은 보존했으며, 이 역사 기록을 고치는 방법은 사용자 확인을 기다립니다. 현재 진도는 지도에서 `current:"1-6"`, `done:["1-1","1-2","1-3","1-4"]`이고, 1-6은 핵심 문답이 남아 완료로 표시하지 않았습니다.

## 구간 기록 양식

### 구간 <번호> — <제목>

- 날짜:
- 고친 파일과 끝났을 때의 줄 수:
- 빌드: `make` 경고 <개수>개, 오류 <개수>개

**실험**

| 실험 | 사용자의 예상 | 실제 출력 | 대본의 기대 결과와 같은가 |
|---|---|---|---|
| | | | |

**질문**

| 질문 | 사용자의 답 (요약) | 몇 번째에 통과했나 |
|---|---|---|
| | | |

**사용자가 헷갈린 곳과 풀린 방법**

-

**대본 · 설계서와 달랐던 곳**

-

**확인하지 못한 것과 이유**

-

---

## 대본 · 설계서 수정 요청 모음

대본(`02_stage1_sections.md`)이나 설계서(`03_stage2_to_9_curriculum.md`)가 이 Mac에서 맞지 않았던 곳을 모읍니다. 사용자가 이 목록을 대본을 쓴 쪽에 전달합니다.

| 문서와 구간 | 적힌 내용 | 실제로 본 것 (원문) | 제안 |
|---|---|---|---|
| | | | |

## 구간 1-5 — `Content-Length` 실험 기록

- 날짜: 2026-11-24
- 고친 파일: `src/server.c` 95행의 `Content-Length` 값을 실험 중 `10`, `3`으로 바꾸고, 실험 뒤 `6`으로 복구함.
- 빌드: 값 10, 값 3, 복구한 값 6에서 각각 `make`를 실행함. 세 번 모두 컴파일 명령과 링크 명령이 완료되었고 경고·오류 출력은 없었음.
- 실험은 VS Code 통합 터미널에서 서버 터미널과 curl 터미널을 나누어 실행해 화면으로 확인함.

| 실험 | 실제 관측 | 해석 |
|---|---|---|
| `Content-Length: 10` | `curl`이 응답 헤더에서 길이 10을 읽고 본문 `hello`를 표시한 뒤 `transfer closed with 4 bytes remaining to read`라고 출력함. 종료 코드는 18. | 서버 본문은 6바이트인데 클라이언트는 10바이트를 기다려 4바이트가 모자람. |
| `Content-Length: 3` | `curl`이 본문 `hel`을 표시하고 `Excess found writing body: excess = 3, size = 3, maxdownload = 3, bytecount = 3`을 출력함. | 클라이언트는 본문에서 지정한 3바이트만 읽고 나머지 `lo\n` 3바이트를 초과 데이터로 알림. |
| 값 복구 `Content-Length: 6` | `make` 완료. 현재 파일 95행에 `Content-Length: 6\r\n`이 있음을 확인함. | 최종 소스 값을 기준값 6으로 복구함. 복구 후 서버·curl 재실행은 하지 않음. |

**확인하지 못한 것과 이유**

- 값 10과 3의 각 실험에서 서버 터미널 요청 출력의 세부 행은 기록하지 않음. 이 실험의 확인 대상은 curl이 길이를 기준으로 본문을 어떻게 해석하는지였음.
- SIGPIPE 실험과 구간 1-5의 핵심 문제 답변은 아직 진행하지 않음.

## 구간 1-5 — 핵심 이해 확인 문제

- 기록 날짜: 2026-11-24
- 기준 코드: `src/server.c` 21–22행, 88–119행
- 출제 범위: HTTP 응답의 본문 길이, `send()` 반복 전송, `SIGPIPE` 처리
- 답변 상태: 질문을 제시함. 사용자 답변은 아직 받지 않음.
- 2026-11-25 재개: 사용자가 1-5의 남은 확인부터 진행하도록 지시했습니다. 현재 지도 위치를 1-5로 바꾸고, 본문 길이 문항은 사용자가 중요하지 않다고 한 점을 반영해 다시 묻지 않습니다. `send()`의 부분 전송과 반복문 확인을 먼저 진행한 뒤 `SIGPIPE` 확인을 이어갑니다.

| 번호 | 문제 | 확인할 핵심 이해 | 사용자 답변 |
|---|---|---|---|
| 1 | 응답에서 `Content-Length: 6`은 어느 부분의 길이를 나타내나요? `hello\n`이 왜 6바이트인지 설명해 보세요. | 길이 값은 헤더 뒤 본문의 바이트 수이며, `\n`도 본문에 포함되는 한 바이트임 | 아직 받지 않음 |
| 2 | `send()`가 요청한 바이트보다 적은 양만 보낸 뒤 돌아올 수 있다면, 이 코드의 `while (sent < len)` 반복문은 어떤 역할을 하나요? `sent`, `resp + sent`, `len - sent`가 어떻게 이어지는지도 설명해 보세요. | 성공적으로 보낸 양을 누적하고, 남은 위치에서 남은 길이를 다시 전송함 | 아직 받지 않음 |
| 3 | 클라이언트가 연결을 먼저 닫은 뒤 서버가 `send()`를 호출할 때, `signal(SIGPIPE, SIG_IGN)`은 서버 동작에 어떤 영향을 주나요? | 기본 `SIGPIPE` 동작에 따른 프로세스 종료를 막고, 프로그램이 `send()`의 실패 반환값을 검사할 수 있게 함 | 아직 받지 않음 |

**정답 확인 기준**

- 1번: `Content-Length`는 상태 줄과 헤더를 제외한 본문 길이를 바이트로 나타냄. `hello` 다섯 글자와 `\n` 한 바이트를 합쳐 6바이트임.
- 2번: `sent`는 누적 전송 바이트 수임. `resp + sent`는 아직 보내지 않은 첫 바이트의 위치이고, `len - sent`는 남은 바이트 수임. 각 성공 결과 `w`를 `sent`에 더해 전체 전송이 끝날 때까지 반복함.
- 3번: `SIGPIPE`의 기본 동작은 프로세스 종료임. `SIG_IGN`은 해당 신호를 무시하게 하므로 서버가 신호 때문에 바로 끝나지 않고, `send()`가 실패를 반환하면 오류 처리 코드가 실행될 수 있음.


## 2026-11-25 진도 이어가기 메모

- 1-5의 응답 코드·길이 실험과 전송 로직은 완료했습니다. 사용자는 `send()` 반복 전송 및 `SIGPIPE` 질문에 대해 답과 해설을 별도 학습 노트로 요청했고, 독립 답변 대신 설명 문서화를 요청했습니다. `send()` 호출 주체·커널의 처리·클라이언트 데이터 수신·서버 변수 `w`가 반환값을 받는 관계도 설명하고 별도 노트에 기록했습니다.
- 이 기록은 설명 자료를 제공했다는 뜻이며, 사용자가 개념을 자기 말로 답해 확인했다는 뜻은 아닙니다. 따라서 이해 확인은 미실시로 분리해 기록합니다.
- 진행 지도는 다음 실험과 확인이 남은 1-6을 현재 위치로 표시합니다.
- 2026-11-25 상태 갱신 정정: 위 273행의 ‘현재 지도는 1-1~1-4 완료’는 당시 확인 기록이며 현재 상태를 나타내지 않습니다. 이후 지도는 1-5를 코드·실험·해설 자료 제공 완료로, 독립 이해 확인은 미실시로 구분하고 `current:"1-6"`으로 갱신했습니다. 과거 문장은 이력으로 보존합니다.

### 1-6 핵심 확인 — `TIME_WAIT`의 주체

- 확인 날짜: 2026-11-25
- 사용자 답변: `127.0.0.1:49738`은 서버 포트 `8080`과 다르고 클라이언트 임시 포트이므로, 기록된 `TIME_WAIT`은 클라이언트 쪽 연결 상태라고 답했습니다. `listen_fd`는 연결 요청을 기다리는 소켓이므로 그 상태와 관련 없다고 구분했습니다.
- 판정: 맞습니다. 해당 `netstat` 행의 로컬 포트 `49738`은 클라이언트 프로세스 쪽 임시 포트이고, `TIME_WAIT`은 그 연결의 클라이언트 쪽 소켓 상태입니다. 서버의 `listen_fd`는 계속 `LISTEN` 상태로 남아 있었습니다. `TIME_WAIT`은 특정 TCP 연결에 속하며, 대기 소켓 자체에 붙는 상태가 아닙니다.
- 다음 확인: 실험 1-6-다에서 서버 쪽 `CLOSE_WAIT`이 관찰된 동안 `close(conn_fd)` 직전에 `sleep(15)`를 두었습니다. 이때 서버 커널이 무엇을 기다리는지 확인합니다.

### 1-6 핵심 확인 — `CLOSE_WAIT`과 `close(conn_fd)`

- 확인 시각: 2026-11-25 11:17 KST (Asia/Seoul)
- 사용자 답변: 서버 커널은 `sleep(15)`이 끝난 뒤 서버 프로그램이 `close(conn_fd)`를 호출해 연결 소켓을 닫기를 기다린다고 설명했습니다.
- 판정: 핵심은 맞습니다. 더 정확히 말하면, 클라이언트가 보낸 연결 종료(FIN)를 서버 커널이 받은 뒤 서버 쪽 연결은 `CLOSE_WAIT`에 들어갑니다. 이 상태는 서버 프로그램이 해당 소켓을 닫기를 기다리는 상태입니다. `sleep(15)`는 서버 프로그램이 `close(conn_fd)` 실행을 15초 늦추도록 넣은 실험용 지연입니다. 커널이 `sleep()`의 종료 시각을 기다리는 것은 아닙니다. `sleep()`이 반환된 뒤 서버 프로그램이 다음 줄에서 `close(conn_fd)`를 호출합니다.
- 소스 근거: 현재 원본은 `/Users/gyaroron/Krafton_Jungle_SWAI_Lab/c-webserver-proxy/src/server.c` 123행에서 `close(conn_fd)`를 호출합니다. `sleep(15)`는 관찰을 위해 잠시 넣었다가 제거한 임시 실험 코드입니다.
- 확인 결과: `TIME_WAIT`의 연결 끝점 구분과 `CLOSE_WAIT`에서의 로컬 close 책임을 확인했습니다. 남은 핵심 확인은 순차 처리 서버가 첫 연결의 `recv()`에서 기다릴 때 두 번째 요청이 지연된 이유입니다.

### 1-6 핵심 확인 — 한 번에 한 연결만 처리하는 구조

- 확인 시각: 2026-11-25 11:20 KST (Asia/Seoul)
- 사용자 답변: 현재 서버 프로그램은 `conn_fd`가 동시에 여러 작업을 수행하도록 설계되지 않았다고 답했습니다.
- 판정: 핵심 이해가 맞습니다. 더 정확히는 `conn_fd` 자체가 작업을 수행하는 것이 아니라, 서버 프로그램이 `conn_fd`로 연결 하나를 순서대로 처리합니다. 첫 연결에서 블로킹 `recv()`를 호출하면 데이터가 오거나 연결이 닫힐 때까지 서버 프로그램의 현재 실행 흐름이 멈춥니다. 그동안 프로그램은 반복문의 다음 `accept()`로 돌아가지 않으므로 두 번째 연결을 애플리케이션에서 처리하지 못합니다. 커널은 두 번째 연결을 대기시킬 수 있지만, 서버 프로그램이 처리하기 전까지 요청은 기다립니다.
- 소스 근거: `/Users/gyaroron/Krafton_Jungle_SWAI_Lab/c-webserver-proxy/src/server.c` 55–70행에서 연결을 하나 수락한 뒤 `recv()`를 호출하고, 연결 처리가 끝난 후 123행에서 `close(conn_fd)`를 호출합니다. 이후 반복문 처음으로 돌아가 다음 `accept()`를 실행합니다.
- 확인 결과: 1-6의 핵심 확인을 마쳤습니다. 단계 1은 코드·실험·설명 자료를 완료했습니다. 1-5의 독립 답변 평가는 별도로 받지 않았으며, 사용자는 해당 설명을 학습 노트로 요청했습니다.

- 2026-11-25 진도 갱신: 1-6 핵심 질문에 답변을 받아 1단계의 확인을 마쳤습니다. 진행 지도는 1-1~1-6 완료, 2-1 현재로 갱신했습니다.

## 구간 2-1 — 파일 나누기

- 날짜: 2026-11-25
- 고친 파일: `src/server.c`를 `src/main.c`로 옮겼습니다. 작업 트리 전체가 Git에서 미추적 상태라 `git mv`는 staging까지 바꾸게 되므로 일반 파일 이동으로 처리했습니다. `src/net.c`, `src/conn.c`, `include/net.h`, `include/conn.h`를 만들었습니다.
- 역할 분리: `net_listen()`은 주소·포트 설정과 듣는 소켓 생성, `conn_handle()`은 연결 하나의 요청 읽기·응답 보내기·연결 소켓 닫기를 담당합니다. `main()`은 듣는 소켓을 준비하고 `accept()`를 반복해 연결 하나씩 `conn_handle()`에 넘깁니다.
- 헤더: 두 헤더에 요청된 `#ifndef` / `#define` / `#endif` 포함 가드를 넣었습니다.
- 사전 참고 컴파일: 처음 만든 참고 구현에서 `main.c`의 `close()` 선언을 포함하지 않아 컴파일 오류가 있었습니다. `<unistd.h>`를 포함한 뒤 `main.c`, `net.c`, `conn.c`를 각각 `-std=c11 -Wall -Wextra -Wpedantic`로 컴파일했고 진단은 없었습니다.
- 빌드: `make clean && make` 성공. `src/conn.c`, `src/main.c`, `src/net.c` 세 파일을 컴파일하고 `build/server`로 링크했습니다. 이후 최종 상태에서 `make`를 다시 실행했고 `make: Nothing to be done for 'all'.`이 출력됐습니다.
- 실험: `./build/server`는 `listening on 127.0.0.1:8080 (fd 3)`을 출력했습니다. `curl -sS -w '\nHTTP %{http_code}\n' http://127.0.0.1:8080/`을 세 번 실행했고 모두 `hello`, `HTTP 200`을 받았습니다. 서버는 세 요청 모두 `accepted fd 4`를 출력했습니다. 클라이언트 임시 포트는 차례로 `50570`, `50571`, `50572`였습니다. 서버는 실험 뒤 `Control+C`로 종료했습니다.
- VS Code: `src/main.c`를 열고 `net_listen()`을 호출하는 22행에 커서를 두었습니다.
- 질문 확인: 헤더에 함수 몸통 대신 선언을 두는 이유와 포함 가드가 막는 문제는 아직 확인하지 않았습니다. 이 핵심 질문을 확인한 뒤 2-1을 통과 처리합니다.


### 2-2 — 애플리케이션 연결 상태

- 날짜: 2026-11-25
- 변경: `src/conn.c`에 `enum conn_state`, 내부 연결 구조체, 상태 이름 변환, `stderr` 전환 로그를 추가했습니다.
- 빌드: `make clean && make` 성공, 경고·오류 0개.
- 실험 출력: `fd 4: READING -> PARSING -> ROUTING -> RESPONDING -> KEEPALIVE_CHECK -> CLOSING -> CLOSED` 순서. 클라이언트 응답은 `hello`, HTTP 200.
- 도구 목적: 처리 단계를 이름으로 출력해 어느 단계에서 요청 처리가 멈추거나 실패하는지 추적합니다. TCP 커널 상태와 서버 프로그램 상태를 구별합니다.
- 질문: 개별 문답은 시간 단축을 위해 생략하고 위 차이를 설명으로 전달했습니다.

### 2-3 — 요청 헤더 끝까지 누적 수신

- 날짜: 2026-11-25
- 변경: `src/conn.c`의 `CONN_READING`에서 남은 `rbuf` 공간만큼 `recv()`하고 `rlen`을 누적합니다. `\r\n\r\n`을 찾을 때까지 READING을 유지하며, 버퍼가 가득 차면 범위를 넘겨 쓰지 않고 연결을 닫습니다.
- 빌드: `make clean && make` 성공, 경고·오류 0개.
- 실험: `(printf 'GET / HT'; sleep 1; printf 'TP/1.1\r\nHost: x\r\n\r\n') | nc 127.0.0.1 8080`. 둘로 나눠 보낸 요청을 서버가 총 27바이트로 누적해 읽었고 `hello`, HTTP 200 응답을 반환했습니다. 서버 상태 로그는 READING에서 PARSING으로 이동한 뒤 이전 순서를 유지했습니다.
- 도구 목적: TCP 바이트 스트림에서는 한 번의 `recv()`가 HTTP 요청 한 건과 일치하지 않을 수 있습니다. 끝 구분자를 찾을 때까지 누적해야 요청 중간에서 성급히 처리하지 않습니다.
- 서버는 실험 뒤 `Control+C`로 종료했습니다.

- 2026-11-25 11:31 KST 진행 갱신: 2-1의 헤더 선언·포함 가드 질문은 마감 목표에 맞춰 설명으로 제공하고 별도 답변 검증은 생략했습니다. 2-1부터 2-3까지 지정 구현·실험을 완료해 진행 지도는 2-4를 현재로 가리킵니다.


### 2-4 — 전송 완료와 연결 닫기 지점

- 날짜: 2026-11-25
- 변경: `include/net.h`에 `net_send_all()` 선언, `src/net.c`에 남은 바이트를 반복 전송하는 구현을 추가했습니다. `src/conn.c`는 이 함수를 호출하며 실패하면 오류를 출력하고 `CONN_CLOSING`으로 이동합니다. `CONN_KEEPALIVE_CHECK`는 현재 항상 닫기로 갑니다.
- 빌드: 일반 `make clean && make` 성공. `make asan`도 성공했고, 검사 빌드에서 `curl` 응답 `hello`, HTTP 200 및 정상 상태 전환을 확인했습니다. 이후 일반 빌드로 복원했습니다. 경고·오류 없음.
- 도구 목적: 부분 전송 처리를 한 함수에 모아 여러 단계에서 재사용하고, 연결 처리 코드는 전송 결과를 확인하는 역할에 집중합니다.
- 확인 상태: 2단계 구현·분할 요청·상태 기록·검사 빌드를 마쳤습니다. 단계 3-1 요청 줄 파싱을 시작합니다.
- 2026-11-25 11:34 KST 진도 갱신: 지도에서 2-1~2-4를 완료하고 3-1을 현재로 표시합니다.

## 2026-11-25 — 1~7단계 당일 학습 계획

- 계획 작성 시각: 2026-11-25 11:35 KST
- 마감: 2026-11-25 23:00 KST
- 학습 제외 시간: 11:50–13:00, 17:50–19:00
- 계획 시점 진도: 1-1~1-6, 2-1~2-4 완료. 3-1 진행 중.
- 역할: `Web_Server만들기_code_generator` 세션이 코드 생성·수정·빌드 및 구현 실험을 맡고, 이 튜토리얼 세션은 핵심 로직과 도구 선택 이유 설명, 질문 답변, 진행 확인을 맡습니다.
- 진행 방식: 단계마다 구현 목표와 도구를 쓴 이유를 먼저 설명하고, 핵심 동작과 눈으로 확인할 결과만 확인합니다. 계산·지엽 문항은 지정 Obsidian vault의 누적 노트에 보내고, 오늘 진행 중에는 다시 묻지 않습니다. 선택 구간 7-6 캐시는 범위에서 제외합니다.

| 한국 시간 | 학습 범위 | 마칠 때 확인할 결과 |
|---|---|---|
| 11:35–11:50 | 계획 공유·3-1 준비 | 현재 진도와 역할 분담을 확정 |
| 11:50–13:00 | 학습 제외 | — |
| 13:00–13:10 | 2-1~2-4 연결 상태 머신 복습 | 상태가 코드에서 어떻게 이어지고 각 단계가 왜 필요한지 설명 |
| 13:10–14:10 | 3-1~3-5 HTTP 요청 파싱 | 요청 줄·헤더·본문 길이·연결 유지 판정·파서 테스트의 역할 설명 |
| 14:10–15:05 | 4-1~4-5 시스템 호출·소켓 옵션 | 실행 인자, 끊김 신호, 시간 제한, 안전 종료, poll 기반 대기 선택 이유 설명 |
| 15:05–16:05 | 5-1~5-4 응답·정적 파일·CGI | 응답 생성, 파일 제공, CGI 실행 경로의 목적과 경계 설명 |
| 16:05–17:50 | 6-1~6-5 동시성 | 스레드·스레드 풀·공유 상태·종료·모델 비교의 설계 이유 설명 |
| 17:50–19:00 | 학습 제외 | — |
| 19:00–21:45 | 7-1~7-5 프록시 | 업스트림 연결, 라우팅, 요청 전달, 응답 중계, 포워드 프록시 흐름 설명 |
| 21:45–22:35 | 누락 보충·통합 흐름 | 1~7단계 요청의 전체 경로와 남은 구현/검증 확인 |
| 22:35–23:00 | 당일 마감 정리 | 진행 지도·튜토리얼 로그 갱신, 핵심 도구 선택 근거 복습 |

- 일정 위험: 실제 구현·빌드가 계획 구간을 넘기면 선택 기능과 반복 확인 문제를 생략하고, 필수 동작의 목적·주체·입출력과 핵심 성공 경로를 우선 설명합니다. 7단계 완료 여부는 지도·소스·실험 기록으로 확인하며, 하지 않은 검증을 완료로 쓰지 않습니다.


### 3-1 — HTTP 요청 줄 파싱

- 날짜: 2026-11-25
- 변경: `include/http.h`, `src/http.c`에 파서 결과 열거형, 요청·헤더 구조체, `http_parse()`를 추가했습니다. `src/conn.c`의 PARSING 단계가 파서 결과와 정상 요청 필드를 출력합니다. 입력 버퍼는 수정하지 않습니다.
- 처음 빌드: `case CONN_PARSING:` 바로 뒤의 변수 선언 때문에 C11 경고가 1개 나왔습니다. 분기 본문을 중괄호로 묶어 지역 범위를 만들었고, 이후 `make clean && make`는 경고·오류 없이 완료됐습니다.
- 정상 실험: `printf 'GET /a HTTP/1.1\r\n\r\n' | nc 127.0.0.1 8080`. 출력 결과 `HTTP_OK`, `method=GET`, `target=/a`, `version=HTTP/1.1`; 응답 `hello`, HTTP 200.
- 잘못된 요청 실험: `printf 'HELLO\r\n\r\n' | nc 127.0.0.1 8080`. `HTTP_BAD_REQUEST`로 판정하고 응답 처리 단계에 들어가지 않은 뒤 연결을 닫았습니다.
- 도구 목적: 요청 원문을 method·target·version으로 구조화해 뒤 단계가 검증된 필드를 기준으로 라우팅할 수 있게 합니다.
- 확인 상태: 3-1의 지정 실험 완료. 3-2에서 헤더의 이름·값을 구조체 배열에 넣습니다.

### 3-2 — HTTP 헤더 파싱

- 날짜: 2026-11-25
- 변경: `src/http.c`가 요청 줄 뒤의 헤더를 `struct http_request.headers`에 복사합니다. 헤더 이름은 `strcasecmp()`로 대소문자 구분 없이 찾고, 값 앞뒤의 공백·탭은 제거합니다. 잘못된 콜론 형식·빈 이름은 `HTTP_BAD_REQUEST`, 32개 초과나 저장 배열에 들어가지 않는 필드 길이는 `HTTP_HEADER_TOO_LARGE`로 처리합니다. HTTP/1.1에서 Host가 없으면 거부합니다. `header_len`은 요청 줄 시작부터 헤더 끝의 빈 줄 뒤까지의 바이트 수입니다. `src/conn.c`는 확인용으로 헤더와 길이를 로그에 출력합니다.
- 빌드: `make clean && make` 성공, C11 경고·오류 없음.
- 지정 실험: `curl -H 'X-Test:   hi  ' -v http://127.0.0.1:8080/` 결과 HTTP 200. 서버 로그에 `header: X-Test = [hi]`, `header_len = 93 bytes`가 출력되어 값 앞뒤 공백 제거를 확인했습니다.
- 추가 입력 확인: HTTP/1.1에서 Host가 없는 요청과 콜론이 빠진 헤더는 각각 `HTTP_BAD_REQUEST` 뒤 연결을 닫았습니다. `host: example.test`처럼 소문자로 쓴 헤더도 HTTP/1.1 필수 Host 검색에 통과했습니다.
- 도구 목적: 파서가 헤더 원문을 이름·값 필드로 바꾸어 다음 구간이 Host·Content-Length·Connection 같은 항목을 이름으로 조회하게 합니다. 이 단계에서는 요청 본문을 읽거나 `Content-Length`를 해석하지 않습니다.
- 실행 종료: 실험 서버를 `Control+C`로 종료했습니다.
- 학습 상태: 구현·빌드·동작 확인 완료. 사용자의 설계 설명 학습 여부는 아직 확인하지 않았습니다.
- 진도: 3-2 구현 완료, 3-3 본문 길이 처리 시작.


## 2026-11-25 11:39 KST — 2단계 학습 상태 정정 및 일정 보완

- 사용자가 확인: 2 연결 상태 머신은 코드만 작성했고, 설계·동작 학습은 아직 하지 않았습니다.
- 앞선 요약표의 2-1~2-4 “통과”는 구현 및 실험 결과를 뜻할 뿐, 사용자의 학습 완료를 확인한 기록이 아닙니다. 이전 표기를 학습 완료로 해석하지 않습니다.
- 진행 지도에서 2-1~2-4를 학습 완료 목록에서 제외하고 “코드 완료 · 학습 남음”으로 구분했습니다. 구현·실험 기록은 보존했습니다.
- 일정 보완: 13:00–13:10에 2단계의 상태 머신 구조와 각 상태의 역할을 설명합니다. 13:10부터 기존 3단계 학습을 이어가며, 제외 시간과 23:00 마감은 유지합니다.

## 2026-11-25 11:42 KST — 진행 중 시각 확인 원칙

- 사용자의 요청: 진행 중 현재 시각을 항상 확인하고, 학습 일정이 밀리지 않도록 관리합니다.
- 운영: 새 구간에 들어가기 전 현재 한국 시간(Asia/Seoul)을 확인하고 진행 지도와 계획표의 위치를 대조합니다. 길게 이어지는 작업 뒤에도 시간을 다시 확인합니다.
- 일정 기준: 11:50–13:00과 17:50–19:00의 제외 시간 및 23:00 마감은 유지합니다. 시간이 부족해지면 핵심 설계 목적·주체·흐름을 먼저 다루고, 반복 실험·계산·지엽 설명은 줄이거나 학습 노트로 넘깁니다. 확인하지 않은 구간은 완료로 표시하지 않습니다.
- 확인 시점의 지도: 3-1은 학습 완료, 2-1~2-4와 3-2는 코드 작성·실험 완료 후 학습이 남음, 현재 위치는 3-3입니다.

### 3-3 — Content-Length와 본문 수신

- 날짜: 2026-11-25
- 변경: `src/http.c`가 `Content-Length`를 십진수로 검사합니다. 숫자 아님·음수·변환 범위 초과는 `HTTP_BAD_REQUEST`, 1 MiB 초과는 `HTTP_PAYLOAD_TOO_LARGE`로 처리합니다. `Transfer-Encoding` 요청은 구현 범위 밖이므로 `HTTP_NOT_IMPLEMENTED`로 거부합니다. `src/conn.c`는 헤더와 함께 이미 받은 본문을 먼저 복사하고, 남은 바이트만 `recv()`로 읽습니다. 할당한 본문 메모리는 처리 뒤 `free()`합니다. 파싱 오류에는 HTTP 오류 응답을 보내고 연결을 닫습니다.
- 빌드: `make clean && make` 성공, C11 경고·오류 없음.
- 지정 정상 실험: `curl -d 'a=1&b=2' http://127.0.0.1:8080/echo`. 서버가 `request body: 7 bytes`를 출력하고 HTTP 200을 반환했습니다.
- 크기 제한 실험: `head -c 2000000 /dev/zero | curl --data-binary @- http://127.0.0.1:8080/echo`. 서버가 `HTTP_PAYLOAD_TOO_LARGE`로 판정했고 클라이언트가 HTTP 413을 받았습니다.
- 추가 확인: `host: example.test`처럼 소문자로 쓴 HTTP/1.1 Host 헤더가 통과했습니다.
- 도구 목적: `Content-Length`가 가리키는 본문 바이트 수만큼 수신하고, 본문 크기 상한으로 서버 메모리 점유를 제한합니다. 선언한 길이보다 적은 본문을 기다릴 때의 시간 제한은 4-3에서 추가합니다.
- 실행 종료: 실험 서버를 `Control+C`로 종료했습니다.
- 학습 상태: 구현·빌드·동작 확인 완료. 설계 설명 학습 여부는 아직 확인하지 않았습니다.
- 진도: 3-3 구현 완료, 3-4 연결 유지 결정 시작.

### 3-4 — 연결 유지 결정

- 날짜: 2026-11-25
- 변경: `src/conn.c`가 HTTP/1.1의 기본 유지와 HTTP/1.0의 기본 종료를 적용하고, `Connection: close` 및 `Connection: keep-alive` 값으로 이를 조정합니다. 응답의 `Connection` 헤더도 실제 선택과 맞춥니다. 유지할 때는 처리한 요청 바이트만 버퍼에서 제거하고 `memmove()`로 뒤의 요청 바이트를 앞으로 옮깁니다.
- 빌드: `make clean && make` 성공, C11 경고·오류 없음.
- 실험 1: `curl -v`로 URL 두 개를 차례로 요청했습니다. 둘 다 HTTP 200이고 두 번째 요청에서 `Re-using existing connection`이 출력됐습니다.
- 실험 2: `curl -H 'Connection: close'` 응답에 `Connection: close`가 들어갔습니다.
- 실험 3: 요청 두 개를 `nc`로 한 번에 전송했습니다. 서버가 한 연결에서 HTTP 200 응답 두 개를 반환했고 두 번째 요청의 `Connection: close` 뒤 연결을 닫았습니다.
- 도구 목적: 매 요청마다 새 TCP 연결을 맺고 끊는 비용을 줄이며, 클라이언트가 종료를 요청하면 같은 연결을 재사용하지 않습니다. 이미 버퍼에 도착한 다음 요청은 앞 요청 처리 뒤 보존합니다.
- 실행 종료: 실험 서버를 `Control+C`로 종료했습니다.
- 학습 상태: 구현·빌드·동작 확인 완료. 설계 설명 학습 여부는 아직 확인하지 않았습니다.
- 진도: 3-4 구현 완료, 3-5 파서 단위 테스트 시작.

### 3-5 — HTTP 파서 단위 테스트

- 날짜: 2026-11-25
- 변경: `tests/test_http.c`에 정상 GET, 정상 POST와 본문 길이, 불완전 입력, 요청 줄 형식 오류, 미지원 메서드·버전, 대상 길이 초과, 헤더 33개, 콜론 없는 헤더, 잘못된·음수·과대 Content-Length, Host 누락·대소문자, Transfer-Encoding 거부 검사를 추가했습니다. `Makefile`에 `make test`, `make test-asan` 대상을 추가했습니다.
- 결과: `make clean && make test`에서 `all HTTP parser tests passed`. `make test-asan`에서도 주소 검사기(AddressSanitizer)와 정의되지 않은 동작 검사기(UndefinedBehaviorSanitizer) 진단 없이 같은 결과를 확인했습니다. 끝에 일반 `make`를 실행해 서버 바이너리를 정상 빌드했습니다.
- 도구 목적: 네트워크 서버를 켜지 않고 파서 함수의 입력별 판정을 반복 검사합니다. 검사 빌드는 잘못된 메모리 접근과 일부 정의되지 않은 동작을 찾아냅니다.
- 학습 상태: 코드·검사 통과. 사용자의 파서 설계 학습은 아직 확인하지 않았습니다.
- 진도: 3단계 코드는 완료했습니다. 현재 4-1 명령줄 인자 구현을 시작할 위치입니다.
- 일정: 학습 제외 시간은 2026-11-25 11:50–13:00 KST입니다. 3-5 기록과 VS Code 위치 설정을 11:52 KST에 마치고 멈췄습니다.

### 4단계 — 설정, 연결 오류, 제한 시간, 종료, poll

- 날짜: 2026-11-25
- 4-1 변경: `include/config.h`, `src/config.c`에 설정 구조체와 `getopt()` 기반 인자 처리를 추가했습니다. 기본값은 127.0.0.1, 8080, 대기열 128, `./www`, 작업자 8, 제한 시간 5초입니다. 숫자는 `strtol()`과 범위 검사로 확인합니다. `src/main.c`는 설정한 주소·포트·대기열을 사용합니다.
- 4-1 확인: `-h` 사용법 출력, `-p abc`와 `-p 0` 거부, `-p 9090` 서버에 대한 curl HTTP 200을 확인했습니다. 나머지 설정 인자의 실제 기능은 아직 검증하지 않았습니다.
- 4-2 변경: 프로세스에서 `SIGPIPE`를 무시하고 macOS에서는 각 연결 소켓에 `SO_NOSIGPIPE`를 설정합니다. 전송 중 `EPIPE` 또는 `ECONNRESET`이 발생하면 연결만 닫습니다.
- 4-2 확인: 클라이언트 RST 뒤 서버가 계속 실행됐고 다음 curl 요청에서 HTTP 200을 받았습니다. RST 실험에서 `send()` 실패 자체가 일어났는지는 관측하지 않았습니다.
- 4-3 변경·확인: 연결마다 `SO_RCVTIMEO`와 `SO_SNDTIMEO`를 설정합니다. 1초 제한에서 유휴 연결은 닫혔고, 일부 요청을 보낸 연결에는 HTTP 408을 보냈습니다.
- 4-4 변경·확인: `sigaction()`이 SIGINT·SIGTERM을 받아 종료 플래그만 바꿉니다. SIGINT 뒤 `server stopped`와 종료 코드 0을 확인했습니다. `send()`와 `recv()`는 EINTR이면 재시도합니다.
- 4-5 변경·확인: 듣는 소켓에 `O_NONBLOCK`을 설정하고 `poll()`로 준비를 기다립니다. 수락한 연결은 차단 모드로 되돌립니다. 이 설정에서 curl HTTP 200을 확인했습니다.
- 빌드: `make clean && make` 성공, C11 경고·오류 없음.
- 학습 상태: 코드와 대표 동작 확인 완료. 사용자의 설계 학습은 별도 확인 전입니다.
- 진도: 현재 5-1 공통 응답 조립을 구현합니다.

### 5-1 — 공통 응답 조립과 오류 응답

- 날짜: 2026-11-25
- 변경: `include/response.h`, `src/response.c`에 `resp_reason()`, `resp_send_head()`, `resp_send_simple()`, `resp_send_error()`를 추가했습니다. 응답 머리에는 상태 줄, Content-Type, Content-Length, Connection, GMT Date, Server를 넣습니다. 본문은 같은 전송 도우미를 통해 끝까지 보냅니다. 오류 응답은 HTML 본문을 만들며 호출한 연결 처리는 이후 닫습니다.
- 연결: `src/conn.c`의 성공 응답과 요청 파싱 오류, 시간 초과 응답이 공통 응답 함수를 사용합니다. 파서 오류는 400·413·414·431·501·505 상태에 대응합니다.
- 빌드·검사: `make clean && make`와 `make test` 성공, C11 컴파일 경고·오류 없음.
- 실험: `HELLO` 요청이 HTTP 400과 HTML 본문을 받았습니다. `curl -X DELETE`는 HTTP 501, 정상 GET은 HTTP 200을 받았습니다. Date, Content-Length, Connection, Server 헤더가 출력된 것을 확인했습니다.
- 도구 목적: 각 처리 경로가 상태 줄과 길이·연결 헤더를 따로 조립하지 않도록 한 곳에 모아, 오류·정상 응답 형식을 일치시킵니다.
- 학습 상태: 코드·대표 출력 확인 완료. 설계 설명 학습 여부는 아직 확인하지 않았습니다.
- 진도: 현재 5-2 MIME 타입 판정을 구현합니다.

### 5-2 — MIME 타입 판정

- 날짜: 2026-11-25
- 변경: `include/mime.h`, `src/mime.c`에 경로 확장자를 보고 `Content-Type`을 돌려주는 `mime_from_path()`를 추가했습니다. 마지막 경로 조각의 마지막 확장자만 읽으며 대소문자를 구분하지 않습니다. 글자 파일에는 UTF-8 문자셋을 붙이고 모르는 확장자는 `application/octet-stream`으로 돌려줍니다.
- 검사: `tests/test_mime.c`에서 `a.HTML`, `a.tar.gz`, 확장자 없는 파일, `.hidden`, `dir.d/file`, CSS, JPEG를 확인했습니다. `make test`와 `make test-asan`의 HTTP 파서·MIME 검사 모두 통과했습니다. 일반 서버 빌드도 복원했습니다.
- 도구 목적: 파일 바이트를 보낼 때 브라우저가 데이터 종류를 해석하도록 응답의 Content-Type을 선택합니다.
- 학습 상태: 구현·검사 통과. 설계 설명 학습 여부는 확인하지 않았습니다.
- 진도: 현재 5-3 정적 파일 요청 처리를 구현합니다.

### 5-3 — 정적 파일 제공

- 날짜: 2026-11-25
- 변경: `include/static.h`, `src/static.c`에 `static_serve()`를 추가했습니다. GET만 허용합니다. 질의 문자열은 경로에서 제외하고 `%XX`를 해독한 뒤 `..` 경로 조각을 검사합니다. 잘못된 인코딩과 `%00`은 400, 루트 이탈은 403입니다. `realpath()` 결과를 기준 디렉터리와 비교해 심볼릭 링크를 통한 루트 탈출도 막습니다. 파일은 `stat()`·`open()`·`fstat()` 뒤 16 KiB 버퍼로 읽어 전송하고 닫습니다. 전체 파일을 메모리에 올리지 않습니다.
- 연결: `conn_handle()`에 설정 포인터를 전달해 `cfg.root`를 사용합니다. 파일 MIME은 `mime_from_path()`로 고르고 연결 유지 여부를 응답에 반영합니다. 파일 오류는 HTML 오류 응답을 보낸 뒤 연결을 닫습니다.
- 빌드: `make clean && make` 성공, C11 경고·오류 없음.
- 실험: `/`는 HTTP 200, `text/html; charset=utf-8`, 169바이트 파일을 반환했습니다. 없는 파일은 404, 일반·퍼센트 인코딩 경로 이탈은 각각 403, POST는 405, `%ZZ`는 400을 받았습니다.
- 확인하지 않은 항목: 권한 거부(403), 5MB 파일 해시 비교는 하지 않았습니다. 시험 `www/` 파일은 `index.html`만 사용했습니다.
- 진도: 현재 5-4 CGI 실행 경로를 구현합니다.

## 2026-11-25 11:45 KST — 학습자 분석 기록 시작

- 사용자의 요청에 따라 개념 이해, 문맥 파악, 어휘·용어, 문장 구성, 독해, 시간당 학습량을 기록하는 별도 문서 `docs/LEARNER_PROGRESS_ANALYSIS.md`를 만들었습니다.
- 초기 관찰은 이전 대화에서 확인 가능한 답변만 근거로 작성했습니다. 개별 답변 시각과 집중 학습 시간이 없어 시간당 학습량은 측정하지 않았습니다.
- 이후 각 학습 구간에서 답변 근거와 설명·힌트 횟수, 확인 가능한 학습 시간을 기록하고 그 결과로 다음 설명의 난이도와 표현 방식을 조정합니다.


## 2026-11-25 12:44 KST — 현재 시각 기준 실시간 학습 계획 갱신

- 사용자가 시작 시각을 오후 1시에서 현재 시각으로 변경했습니다. 확인 시각은 12:44 KST이며, 이전의 11:50–13:00 대기 구간을 오늘 계획에서 해제하고 지금 학습을 시작합니다. 오후 5:50–7:00 제외 시간과 오후 11시 마감은 유지합니다.
- 진행 지도는 코드 생성 위치와 학습 위치를 분리했습니다. 학습 위치는 2-2, 별도 코드 생성 세션의 위치는 4-1입니다. 2-2는 학습 중 상태가 코드 완료 상태보다 우선 표시됩니다.
- 현재까지 학습 확인 완료: 1-1~1-6, 3-1. 코드·실험 완료 후 학습 확인이 남은 구간: 2-1~2-4, 3-2~3-5.

| 한국 시간 | 학습 범위 | 확인 목표 |
|---|---|---|
| 12:44–13:05 | 2단계 연결 상태 머신 | 코드 분리, 상태 전환, 누적 수신, 끝까지 보내기의 설계 이유와 관계 설명 |
| 13:05–13:50 | 3단계 HTTP 파싱 | 요청 줄·헤더·본문 길이·연결 유지·테스트의 핵심 역할 연결 |
| 13:50–14:35 | 4단계 시스템 호출·소켓 옵션 | 실행 인자, 끊김, 제한 시간, 안전 종료, poll의 선택 이유 |
| 14:35–15:35 | 5단계 응답·정적 파일·CGI | 응답 조립, 파일 경로 처리, CGI 실행 경계 |
| 15:35–17:40 | 6단계 동시성 | 스레드, 스레드 풀, 공유 상태, 종료, 모델 선택 |
| 17:40–17:50 | 시간 점검·짧은 완충 | 진행 상태 확인 후 다음 학습 위치 확정 |
| 17:50–19:00 | 학습 제외 | — |
| 19:00–21:10 | 7단계 프록시 | 업스트림 연결, 라우팅, 요청·응답 중계, 포워드 프록시 흐름 |
| 21:10–22:20 | 1~7단계 통합 | 요청이 서버와 프록시를 통과하는 전체 흐름 및 미확인 항목 |
| 22:20–23:00 | 기록·마감 | 실제 학습 확인에 맞춰 지도·로그 갱신 |

- 일정 운영: 다음 구간 전에 현재 시각과 지도 상태를 다시 확인합니다. 시간이 부족해지면 복습용 반복 실험과 지엽 설명부터 줄이고, 핵심 설계 목적과 흐름을 우선합니다. 미학습 구간은 완료로 표시하지 않습니다.


## 2026-11-25 12:45 KST — 동시 진행 상태 확인 및 지도 분리 갱신

- 동시 진행 중인 코드 생성 세션의 지도 갱신을 다시 확인했습니다. 4-1이 아니라 4-1~4-5가 구현·대표 실험 완료이고, 현재 코드는 5-1 응답 조립입니다. 12:44 계획 메모의 코드 생성 위치 4-1은 이 시점에 이미 오래된 값이므로, 현재값은 5-1로 정정합니다. 앞선 기록은 이력으로 보존합니다.
- 진행 지도에 `studyCurrent`와 `codeCurrent`를 분리했습니다. 현재 학습 위치는 2-2, 코드 생성 위치는 5-1입니다. 현재 학습 상태가 해당 구간의 코드 완료 표시보다 우선 보입니다.
- 시작 기준 시각은 다시 확인한 12:45 KST입니다. 오늘 첫 제외 시간은 사용자의 “지금 시작” 지시로 대체하고, 17:50–19:00 제외 시간과 23:00 마감은 유지합니다.

### 현재 시각 기준 일정 (12:45 시작)

| 한국 시간 | 학습 범위 | 확인 목표 |
|---|---|---|
| 12:45–13:05 | 2단계 연결 상태 머신 | 상태와 TCP 상태의 구분, 상태 전환·수신·송신 구성의 이유 |
| 13:05–13:50 | 3단계 HTTP 파싱 | 요청 줄·헤더·본문·연결 유지·검사의 핵심 역할 |
| 13:50–14:35 | 4단계 시스템 호출·소켓 옵션 | 실행 인자, 연결 종료 복구, 제한 시간, 안전 종료, poll |
| 14:35–15:35 | 5단계 응답·정적 파일·CGI | 응답 조립, 정적 파일 경로 처리, CGI 실행 경계 |
| 15:35–17:40 | 6단계 동시성 | 스레드, 스레드 풀, 공유 상태, 안전 종료, 모델 선택 |
| 17:40–17:50 | 진도·시간 확인 | 늦은 핵심 주제를 다음 구간에 우선 배치 |
| 17:50–19:00 | 학습 제외 | — |
| 19:00–21:10 | 7단계 프록시 | 업스트림 연결, 라우팅, 요청·응답 중계, 포워드 프록시 |
| 21:10–22:20 | 통합 흐름 | 1~7단계 연결 및 남은 핵심 확인 |
| 22:20–23:00 | 진도·학습 분석 기록 | 실제 답변과 확인 결과를 반영하고 미학습 구간은 남은 상태로 표기 |

- 구간 사이에 현재 시각과 지도 위치를 재확인합니다. 시간이 부족하면 핵심 설계 설명을 유지하고 반복·지엽 항목을 줄입니다.


## 2026-11-25 12:48 KST — 실시간 지도 커서 분리 완료

- 지도 동시 편집을 확인한 최신 코드 생성 위치는 5-3입니다. 앞서 5-1·5-2로 적힌 위치는 관찰 당시 값으로 보존하고, 현재값을 5-3으로 갱신했습니다.
- 지도는 `studyCurrent`와 `codeCurrent`를 별도로 표시합니다. 현재 학습은 2-2, 코드 생성은 5-3입니다. 학습 중 구간은 코드 완료·학습 남음 표시보다 우선하며, 지도 상단에 각각의 위치가 표시됩니다.
- 오늘 일정은 12:48 시작으로 다시 기준을 맞췄습니다. 11:50–13:00 블록은 사용자의 현재 시작 지시로 대체했고, 17:50–19:00 제외와 23:00 마감은 유지합니다.

| 한국 시간 | 학습 범위 | 핵심 확인 |
|---|---|---|
| 12:48–13:05 | 2단계 연결 상태 머신 | 코드 분리, 상태 전환, 누적 수신, 끝까지 보내기의 설계 목적 |
| 13:05–13:50 | 3단계 HTTP 파싱 | 요청 줄·헤더·본문·연결 유지·검사의 역할 |
| 13:50–14:35 | 4단계 시스템 호출·소켓 옵션 | 실행 인자, 끊김 복구, 시간 제한, 종료, poll |
| 14:35–15:35 | 5단계 응답·정적 파일·CGI | 응답 형식, MIME, 경로 안전, CGI 경계 |
| 15:35–17:40 | 6단계 동시성 | 스레드, 풀, 공유 상태, 종료, 모델 선택 |
| 17:40–17:50 | 진도·시간 확인 | 이후 핵심 항목 우선순위 확인 |
| 17:50–19:00 | 학습 제외 | — |
| 19:00–21:10 | 7단계 프록시 | 업스트림 연결, 라우팅, 양방향 중계, 포워드 프록시 |
| 21:10–22:20 | 전체 흐름 통합 | 1~7단계의 연결과 필수 미확인 항목 |
| 22:20–23:00 | 지도·분석 기록 | 실제로 학습한 구간과 확인 결과를 반영 |

- 다음 구간 이동 전 현재 시각과 코드·학습 커서를 모두 확인합니다. 시간이 부족하면 핵심 설계 설명을 유지하고 반복·지엽 내용을 압축하며, 이해 확인 전에는 학습 완료로 표시하지 않습니다.


## 2026-11-25 12:50 KST — 현재 시각 기준 일정 시작점 재확인

- 현재 시각을 다시 확인해 일정 시작점을 12:50 KST로 맞췄습니다. 이전 12:48 기록은 변경 이력으로 보존합니다. 현재 학습 위치는 2-2이며, 코드 위치는 마지막으로 확인한 5-3입니다.
- 남은 시간표의 첫 블록은 12:50–13:05입니다. 나머지 블록과 17:50–19:00 제외, 23:00 마감은 유지합니다.

| 한국 시간 | 학습 범위 | 핵심 확인 |
|---|---|---|
| 12:50–13:05 | 2단계 연결 상태 머신 | 상태 전환·TCP 상태 구분·누적 수신·끝까지 보내기의 설계 목적 |
| 13:05–13:50 | 3단계 HTTP 파싱 | 요청 줄·헤더·본문·연결 유지·검사 역할 연결 |
| 13:50–14:35 | 4단계 시스템 호출·소켓 옵션 | 인자·끊김 복구·시간 제한·안전 종료·poll |
| 14:35–15:35 | 5단계 응답·정적 파일·CGI | 응답 조립·MIME·파일 경로 안전·CGI 경계 |
| 15:35–17:40 | 6단계 동시성 | 스레드·스레드 풀·공유 상태·종료·모델 선택 |
| 17:40–17:50 | 시간·진도 확인 | 뒤 구간 우선순위 조정 |
| 17:50–19:00 | 학습 제외 | — |
| 19:00–21:10 | 7단계 프록시 | 업스트림·라우팅·요청/응답 중계·포워드 프록시 |
| 21:10–22:20 | 통합 흐름 | 1~7단계 연결 및 남은 핵심 확인 |
| 22:20–23:00 | 마감 기록 | 지도·학습 분석 갱신, 미학습 구간 명시 |


## 2026-11-25 13:07 KST — 설명 요청 반영·일정 재기준화

- 사용자가 첨부한 최상위 핵심 3가지를 더 쉬운 수준으로 다시 설명해 달라고 요청했습니다. 현재 학습 위치는 2-2이며, 핵심 확인 답변을 아직 받지 않아 학습 완료로 바꾸지 않았습니다.
- 지도 업데이트 시각을 13:07 KST로 갱신했습니다. 마지막으로 확인한 별도 코드 생성 위치는 5-4입니다.
- 오후 1시 이후 일정은 현재 시각에 맞춰 재배치합니다. 오후 5:50–7:00 제외 시간과 오후 11시 마감은 유지합니다.

| 한국 시간 | 학습 범위 | 확인 목표 |
|---|---|---|
| 13:07–13:25 | 2단계 연결 상태 머신 | 세 핵심의 쉬운 설명과 `ESTABLISHED`/`CONN_READING` 구분 |
| 13:25–14:10 | 3단계 HTTP 파싱 | 요청 줄·헤더·본문·연결 유지·테스트 역할 |
| 14:10–15:00 | 4단계 시스템 호출·소켓 옵션 | 실행 인자, 종료 복구, 시간 제한, 안전 종료, poll |
| 15:00–16:00 | 5단계 응답·정적 파일·CGI | 응답, MIME, 파일 경로, CGI의 경계 |
| 16:00–17:40 | 6단계 동시성 | 스레드·풀·공유 상태·종료·모델 선택 |
| 17:40–17:50 | 시간·진도 확인 | 남은 핵심 항목 재배치 |
| 17:50–19:00 | 학습 제외 | — |
| 19:00–21:10 | 7단계 프록시 | 업스트림·라우팅·양방향 중계·포워드 프록시 |
| 21:10–22:20 | 통합 흐름 | 1~7단계 전체 요청 경로와 필수 확인 |
| 22:20–23:00 | 마감 기록 | 실제 학습 진도와 분석 기록 갱신 |


## 2026-11-25 13:09 KST — 전체 단계 설계도에 구현 핵심 로직 연결

- 사용자 요청: `c_webserver_proxy_자료/ref/blueprint_all_stages.html`의 작업 내역과 9개 단계 페이지를 읽고, 해당 위치에 핵심 로직을 기재합니다. 설계도 안의 지시 문구도 코드 생성 때 반영합니다. 기존 설명과 충돌하면 설계도를 기준으로 삼고, 충돌 사실과 처리 내용을 기록합니다.
- 확인 자료: 설계도 안에 들어 있는 1~9단계 페이지 전체, `docs/TUTORIAL_LOG.md`, 상위 진행 지도 `c_webserver_proxy_progress_map.html`, 현재 `src/`와 `include/`의 파일들.
- 설계도 수정: 1단계의 socket → bind/listen, accept, 요청 수신 흐름 카드와 2~5단계의 관련 흐름도 상자를 열면 “실제 프로젝트 코드 · 경로와 행 번호”가 설계도 핵심 로직과 별도 칸으로 보이게 했습니다. 1단계 코드는 2-1에서 파일을 나눈 뒤의 현재 대응 코드이므로, 과거의 단일 src/server.c 원문과 동일하다고 표시하지 않습니다. 각 발췌는 현재 소스에서 가져왔고, 2026-11-25 13:09 KST에 파일 경로와 행 번호를 대조했습니다. 6~9단계에는 아직 구현 소스를 확인하지 않았으므로 설계 로직만 남기고 실제 코드로 표시하지 않았습니다.
- 첫 페이지 C 로드맵: 하나로 섞여 있던 오래된 현재 위치를 학습 커서와 코드 구현 커서로 나눴습니다. 진행 지도에 기록된 값인 학습 `2-2`, 코드 `5-4`와 완료·구현 확인 구간을 표시합니다. 구간 목록에서도 학습 완료와 코드 완료를 구별합니다. 앞으로 한 구간의 진도나 완료 상태를 바꿀 때 `docs/TUTORIAL_LOG.md`, 상위 진행 지도, 이 C 로드맵을 같은 확인 시점 기준으로 함께 갱신합니다.

### 충돌 및 기준 적용

- 설계도 1~9단계에 공통으로 있던 “구현 소스 코드는 아직 없습니다” 문구는 현재 파일 상태와 충돌했습니다. 실제 2~5단계 소스가 있으므로 이 문구를 단계별 상태 설명으로 바꿨습니다. 2~5단계는 실제 코드 발췌를 표시하고, 6~9단계는 아직 설계 전용이라고 표시합니다.
- 첫 페이지 C 로드맵의 `NOW` 값은 2026-11-24의 1-4였고, 상위 진행 지도의 위치를 2026-11-25 13:09 KST에 다시 확인했으며, 학습 2-2·코드 5-4는 바뀌지 않았습니다. 최신 진행 지도 값을 적용했습니다.
- 문서 내부 상태에도 차이가 있습니다. 진행 현황 표의 2-2는 “통과”로 적혀 있지만 같은 로그의 최신 계획 기록과 상위 지도는 2-2를 현재 학습 위치로 표시하고 핵심 답변 확인이 남았다고 적습니다. 완료로 임의 통합하지 않고, 현재 커서는 최신 상위 지도와 시간 기록에 맞춰 2-2에 두었습니다. 이 충돌은 학습 완료 여부를 다시 확인할 때 해소해야 합니다.
- 설계도 지시와 현재 코드가 다르면 설계도 요구를 기준으로 코드 생성을 진행하고, 코드 변경과 검증 결과를 별도 기록합니다. 13:09 문서 갱신 뒤 아래 CGI 환경 변수 생성 충돌을 확인해 설계도에 따라 소스를 수정했습니다.

- 확인: 설계도 외부 파일에서 PAGES 배열 9개가 정상적으로 읽히며, 각 페이지의 내장 JavaScript를 `node --check`로 구문 검사했습니다. 모든 페이지 검사를 통과했습니다.
- 프로젝트 빌드·CGI 동작 검사는 이번 문서 작업에서 실행하지 않았습니다. 5-4는 진행 중 상태를 유지합니다.


## 2026-11-25 13:13 KST — 설계도와 CGI 환경 변수 생성 순서 충돌 수정

- 충돌: 설계도 5단계 `PIPE` 설명은 환경 변수 배열을 `fork()` 전에 준비하라고 지시합니다. 기존 `src/cgi.c`는 자식 프로세스 분기 안에서 `snprintf()`를 호출해 환경 문자열을 만들고 있었습니다.
- 기준: 사용자의 지시에 따라 설계도를 따랐습니다. `src/cgi.c`에서 요청 메서드·질의 문자열·본문 길이·Content-Type·스크립트 경로·프로토콜 환경 문자열과 `envp`를 파이프·`fork()`보다 먼저 구성하도록 옮겼습니다. 각 `snprintf()` 반환 길이도 확인해 환경 문자열이 잘렸으면 CGI에 전달하지 않고 HTTP 500으로 처리합니다. 자식은 파이프 연결 후 준비된 `envp`를 `execve()`에 전달합니다.
- 빌드: 첫 `make`에서 기존 지역 변수 이름 `content_type`이 나중의 응답 Content-Type 배열과 충돌했습니다. 요청 헤더 쪽 포인터를 `request_content_type`으로 바꾼 뒤 `make`를 다시 실행했고 컴파일·링크가 성공했습니다. 경고는 없었습니다.
- 설계도: CGI `PIPE`, `CHILD`, `PARENT`, `WAIT`, 오류 노드의 실제 코드 발췌 경로와 행 번호를 새 소스 기준으로 갱신했습니다.
- 미실시: 이 순서 변경 뒤 CGI 기능 실험과 `make test`는 다시 실행하지 않았습니다. 5-4 상태는 진행 중으로 유지합니다.


## 2026-11-25 13:15 KST — B 지도에 현재 코드 대응 추가

- 사용자 지적: `B. 서버 쪽 소켓의 TCP 상태 머신`에는 실제 프로젝트 코드에 대한 설명이 없었습니다.
- 수정: TCP 상태 카드에 해당 전이를 유발하거나 그 전이 뒤 프로그램이 실행하는 현재 코드 발췌를 추가했습니다. 포함한 대응은 소켓 생성, `bind()`/`listen()`, `accept()`와 수신, 서버의 `close()`, 상대 종료 때 `recv() == 0`, 프로세스 종료 시 듣는 소켓 정리입니다. 경로와 행 번호는 프로젝트 소스와 대조했습니다.
- 구분: `SYN_RCVD`, `FIN_WAIT_*`, `TIME_WAIT`, `CLOSE_WAIT`, `LAST_ACK` 상태 처리는 Mac의 커널이 수행합니다. 프로젝트 코드는 그 커널 동작을 일으키는 API 호출과 반환값만 보여 줍니다. 커널 구현 코드가 프로젝트 안에 있는 것처럼 표현하지 않았습니다.
- 확인: 설계도 PAGES 9개를 다시 읽고, 내장 JavaScript 모두를 `node --check`로 확인했습니다. 프로젝트 코드는 수정하지 않았고 빌드·기능 검사는 실행하지 않았습니다.


## 2026-11-25 13:19 KST — 2-2 핵심 확인 답변

- 사용자의 답변: “ESTABLISHED는 client와 server가 연결이 된 상태를 알리는 것이며, CONN_READING은 server program이 요청을 읽는 중이라는 상태를 가리킵닏다”
- 판정: TCP 연결 상태와 서버 프로그램 처리 단계를 설명 뒤 올바르게 구분했습니다. `ESTABLISHED`를 관리하는 커널을 답변에 명시하지 않았고, 상태 머신의 종료·유지 경로도 아직 확인하지 않았으므로 2-2 전체는 완료로 표시하지 않습니다.
- 현재 학습 커서: 2-2 유지. 현재 계획 구간(13:07–13:25) 안에서 종료·재사용 흐름을 쉽게 설명합니다.
- 학습자 분석: `LEARNER_PROGRESS_ANALYSIS.md`에 관찰 근거와 설명 조정 사항을 추가했습니다. 답변 간격은 집중 학습 시간으로 계산하지 않았습니다.


## 2026-11-25 13:20 KST — 2-2 연결 유지·종료 분기 설명

- `ESTABLISHED`와 `CONN_READING`의 차이를 학습자가 올바르게 구분한 답변을 기록했습니다. 커널이 TCP 상태를 관리하고 서버 프로그램이 `c.state`를 관리한다는 점을 다시 분명히 했습니다.
- 다음 핵심인 `CONN_KEEPALIVE_CHECK`의 유지·종료 선택과 `CONN_CLOSING → CONN_CLOSED` 흐름을 짧게 설명합니다. 답변 확인 전에는 2-2 완료로 표시하지 않습니다.
- 확인 시각: 13:20 KST. 지도 학습 커서 2-2, 코드 생성 커서 마지막 확인 5-4입니다.

## 2026-11-25 13:20 KST — 1~9단계 빈 코드 칸 전체 채움

- 사용자 지시: 1단계부터 9단계까지 설계도에서 비어 있던 코드 칸을 채우고, 공백·들여쓰기·줄바꿈을 읽기 쉽게 유지합니다.
- 수정: 1단계의 28개 상자에는 실제 프로젝트 코드가 있으면 구현 발췌를 표시하고, 프로토콜 주체·검사 도구·커널 상태처럼 프로젝트 C 함수 코드가 없는 상자에는 짧은 의사 코드 또는 실행 명령을 넣었습니다. 2~9단계 모든 상자에도 기존 설계 코드·실제 코드가 없던 경우 핵심 흐름 의사 코드 또는 검증 명령을 추가했습니다.
- 표기 구분: 실제 파일에서 가져온 코드는 경로와 행 번호가 있는 “실제 프로젝트 코드”로 표시합니다. 이후 단계에만 있는 예시는 “설계 의사 코드”로 표시해 이미 작성된 소스로 오해하지 않게 했습니다. 셸 검사 명령은 C 코드와 구별되는 명령 예시로 표시합니다.
- 가독성: 한 줄에 한 동작을 배치하고, 블록은 4칸 들여쓰기를 적용했습니다. 코드 상자는 `pre-wrap`과 긴 줄 줄바꿈을 사용해 작은 화면에서 가로로 넘치지 않게 했습니다.
- 확인: 9페이지 모두의 내장 JavaScript를 `node --check`로 검사했습니다. 모든 검사 통과. 프로젝트 C 소스·빌드·기능 테스트는 변경하지 않았고 실행하지 않았습니다.


## 2026-11-25 13:26 KST — 시간표에 따라 3-2로 이동

- 시간 확인: 13:26 KST. 13:25에 시작하기로 한 3단계 시간 블록에 들어왔습니다. 현재 학습 위치를 3-2 HTTP 헤더로 옮겼고, 코드 생성 위치는 지도에서 마지막으로 확인한 5-4입니다.
- 2단계 상태: 코드와 실험은 끝났으나 학습은 미완료입니다. `ESTABLISHED`/`CONN_READING` 차이는 설명 뒤 확인했지만 `CONN_CLOSING` 종료 경로 답변과 2-1·2-3·2-4 설명 확인은 남았습니다. 이를 완료로 표시하지 않고 통합·보충 시간에 다시 다룹니다.
- 설명 조정: 사용자의 답변에서 확인된 구분을 바탕으로, 이번 3-2는 헤더 한 줄을 이름과 값 두 칸에 기록하는 목적부터 짧게 설명합니다.


## 2026-11-25 13:31 KST — 1~9단계 코드 표시 라벨·가독성 재점검

- 재점검 결과: 2~9단계의 설계 예시 칸에는 의사 코드뿐 아니라 셸 확인 명령도 들어 있으므로, 기존의 “설계 의사 코드”라는 제목만으로는 칸의 내용을 정확히 설명하지 못했습니다. 제목을 “핵심 로직 예시 · 설계 코드/실행 명령 (실제 소스와 구분)”으로 바꿔 예시와 실제 프로젝트 코드를 명확히 구분했습니다. 실제 구현 발췌는 별도 “실제 프로젝트 코드 · 경로와 행 번호” 칸에 유지했습니다.
- 1단계 실제 코드 발췌는 긴 줄이 가로 스크롤을 만들 수 있어, 공백·들여쓰기를 보존하면서 화면 폭에서 줄바꿈하도록 표시 규칙을 조정했습니다. 긴 식별자도 상자 밖으로 밀려나지 않도록 줄바꿈을 허용했습니다.
- 확인: 9개 페이지 배열이 정상적으로 파싱되었고, 각 페이지의 내장 JavaScript 구문 검사가 모두 통과했습니다. 설계 예시 라벨과 1단계 줄바꿈 규칙도 파일에서 다시 확인했습니다.
- 소스 코드와 빌드 결과는 이번 설계도 표시 점검에서 변경하지 않았습니다.


## 2026-11-25 13:34 KST — 3-2 헤더 이름·값 대응 확인

- 사용자의 답변: “name에는 Host, value에는 127.0.0.1:8080이 문자열로 들어갑니다”
- 판정: 예시 헤더를 `name`과 `value`에 올바르게 대응했습니다. 구체적인 설명 예시 뒤의 확인이므로 독립 설명으로 판정하지 않습니다.
- 남은 내용: 헤더 이름을 대소문자 구분 없이 찾는 이유, 값 앞뒤 공백 제거, 잘못된 헤더 처리와 크기 제한은 아직 확인하지 않았습니다. 3-2는 학습 중으로 유지합니다.
- 현재 지도 위치: 학습 3-2, 코드 생성 위치 마지막 확인 5-4.

## 2026-11-25 13:36 KST — 빈 핵심 로직 코드 칸 추가 보완

- 재점검 정정: 직전 점검은 `code` 속성이 비어 있는 배열뿐 아니라, 속성 자체가 없는 상자까지 검사하지 못했습니다. 따라서 “모든 상자에 코드 예시가 있다”는 이전 확인은 불완전했습니다.
- 보완: 2단계 `KEEPALIVE_CHECK`, `CONN_CLOSING`; 3단계 `Content-Length`, 본문 읽기, keep-alive 결정; 4단계 `socket`/`SO_REUSEADDR`, `bind`/`listen`, 시그널 종료; 5단계 처리기 선택, 경로 검증, CGI 파이프, 자식 실행, 부모 송수신, 500/504 처리 등 총 14개 상자에 핵심 흐름 예시를 추가했습니다.
- 예시는 “설계 코드/실행 명령” 칸에 두어 실제 구현으로 오해하지 않도록 했습니다. 실제 프로젝트 코드 인용은 기존 별도 칸에 유지했습니다. 들여쓰기는 블록마다 4칸으로 맞추고, 동작 단계별로 줄을 나눴습니다.
- 확인: 1단계의 28개 `logic` 및 10개 `impl` 배열이 모두 비어 있지 않습니다. 2~9단계의 모든 노드(총 115개)에서 코드 예시가 비어 있지 않습니다. 9개 페이지를 파싱했고 내장 JavaScript 구문 검사도 모두 통과했습니다.
- 프로젝트 C 소스는 수정하지 않았고, `make`와 실행 검사는 수행하지 않았습니다.


## 2026-11-25 13:37 KST — 3-2 답변 수준 기준 학습 속도 기록

- 사용자는 `Host`를 헤더 이름(`name`)에, `127.0.0.1:8080`을 헤더 값(`value`)에 정확히 대응했습니다. 예시 설명 뒤 적용한 답변으로 확인했습니다.
- 약 13:27의 예시 설명부터 13:34 답변 확인까지 대화상 약 7분이 경과했습니다. 집중 시간으로 간주하지 않습니다.
- 답변 수준 기준 잠정값: 확인된 핵심 항목 1개 ÷ 약 7분 × 60 = 시간당 약 8.6개. 단일 문항의 힌트 뒤 적용률이며, 독립 학습 속도나 장기 예측값은 아닙니다. 세부 근거는 `docs/LEARNER_PROGRESS_ANALYSIS.md`에 기록했습니다.
- 학습 위치는 3-2를 유지합니다. 대소문자 처리, 공백 제거, 잘못된 헤더와 크기 제한은 아직 확인하지 않았습니다.


## 2026-11-25 13:39 KST — 설계 코드 예시의 구체성 보완

- 예시 칸 제목을 “의사 코드/실행 명령”으로 더 정확히 바꿨습니다. 실행 가능한 프로젝트 코드와 설계 설명용 조각을 구별합니다.
- 방금 추가한 일부 조각에서 생략된 경계 검사를 채우고, 설명에 나온 `c.request_len`, `Content-Length`, 본문 누적 수신, 소켓 정리의 순서를 예시 코드에 드러냈습니다. 4칸 들여쓰기와 한 동작 한 줄 원칙을 유지했습니다.
- 실제 코드 인용은 별도 칸에 있으므로 의사 코드 조각은 실제 구현이라고 주장하지 않습니다. 프로젝트 C 소스는 변경하지 않았습니다.


## 2026-11-25 13:39 KST — 3-2 헤더 이름 대소문자 처리 설명

- 시간표 확인: 13:25~14:10의 3단계 학습 시간 안에서 3-2를 계속 진행했습니다.
- 앞서 확인한 수준: `Host`는 이름, `127.0.0.1:8080`은 값으로 예시 뒤 정확히 대응했습니다. 그 한 답변에서 확인한 항목은 1개이며, 잠정 응답 수준 속도는 약 8.6개/시간입니다. 독립 학습 속도로 일반화하지 않습니다.
- 이번 설명: 같은 HTTP 헤더 이름은 `Host`, `host`처럼 대소문자가 달라도 같은 이름으로 찾아야 한다는 점을 설명했습니다. 학습자의 확인 답변은 아직 받지 않았습니다.
- 다음 확인: 대소문자 무시가 필요한 이유를 핵심 한 문장으로 확인합니다. 이어서 헤더 값 양쪽 공백 제거를 다룹니다.

## 2026-11-25 13:41 KST — 실제 코드 경로·행 번호 대조 및 편집 검증

- 2~5단계의 실제 코드 표시 31개 경로·행 범위를 현재 프로젝트 파일과 대조했습니다. 모든 파일이 존재하고 행 범위가 파일 안에 있으며, 각 코드 발췌의 코드 행도 지정 범위에서 확인했습니다.
- 코드 배열을 재정렬하는 과정에서 C 식 `c.rbuf[c.rlen]` 안의 `]`를 배열 끝으로 잘못 처리해 내장 스크립트 구문 검사가 한 번 실패했습니다. 코드 배열의 종료 위치를 다음 `actual` 필드 경계로 제한해 복구했습니다. 복구 후 9페이지의 내장 JavaScript 검사를 다시 실행해 모두 통과했습니다.
- 마지막 검사에서 2~9단계 115개 노드의 코드 예시가 모두 비어 있지 않음을 확인했습니다. 1단계 28개 핵심 로직 및 10개 실제 발췌 배열도 모두 채워져 있습니다.
- `src/`와 `include/` 파일은 수정하지 않았고, 프로젝트 빌드나 기능 검사는 실행하지 않았습니다.


## 2026-11-25 13:41 KST — 3-2 세부 질문 이관 후 다음 핵심 진행

- 사용자는 헤더 이름 대소문자 비교 질문을 지엽적이라고 판단해 학습노트 기록을 요청했습니다. 답을 요구하지 않고 `C_Webserver_Proxy_학습질문_누적.md`에 요점과 출처를 기록했습니다.
- 현재 학습 시간표는 13:25~14:10의 3단계 블록입니다. 3-2의 다음 항목인 헤더 값 앞뒤 공백과 탭 제거를 설명합니다. 이해 확인 답변은 아직 받지 않았습니다.

## 2026-11-25 13:42 KST — 실제 코드 발췌의 줄바꿈·들여쓰기 표시 수정

- 사용자 제보의 원인: 실제 프로젝트 코드 배열은 `join("\\n")`으로 줄바꿈을 만들었지만, 표시 함수는 `design` 클래스에만 `<pre>` 요소를 적용했습니다. `actual` 칸은 일반 문단(`<p>`)으로 출력되어 HTML의 연속 공백 축약 규칙에 따라 줄바꿈과 들여쓰기가 사라졌습니다.
- 수정: 2~9단계에서 `design`과 `actual` 코드를 모두 `<pre>`로 표시하도록 바꿨습니다. 기존 `.box pre` 규칙은 고정폭 글꼴, 공백 보존, 줄바꿈을 이미 제공하므로 실제 코드에도 적용됩니다.
- 2~5단계 실제 발췌의 파일 경로·행 번호를 현재 소스와 대조했고 모두 유효했습니다. 줄 번호 표기와 코드는 별도 줄에 나타나도록 데이터의 개행을 보존합니다.
- C 소스는 수정하지 않았습니다. HTML 내장 JavaScript 구문 검사를 다시 수행합니다.


## 2026-11-25 13:43 KST — 공백 제거 세부 학습노트 이관

- 사용자는 헤더 값 양끝 공백·탭 제거 설명을 지엽적이라고 판단해 학습노트 기록과 다음 내용 진행을 요청했습니다. 예시와 목적을 `C_Webserver_Proxy_학습질문_누적.md`에 기록했습니다. 추가 확인 질문은 하지 않았습니다.
- 시간표의 13:25~14:10 3단계 블록에서 3-2를 계속합니다. 다음 항목은 콜론이 없거나 헤더 이름이 빈 입력을 거르는 이유입니다.
- 최종 표시 검사: 2~9단계 렌더러가 실제 코드도 `<pre>`로 감싸고, `.box pre`가 줄바꿈·들여쓰기를 보존하는지 확인했습니다. 9페이지의 내장 JavaScript 구문 검사도 모두 통과했습니다. 브라우저의 `file:` URL 열기는 정책상 거부되어 시각적 브라우저 미리보기는 하지 않았고, 정책을 우회하지 않았습니다.


## 2026-11-25 13:45 KST — 3-2 Host 헤더 요구 조건 설명 시작

- 시간표 확인: 13:25~14:10 3단계 블록에서 3-2를 이어갑니다.
- 최근 이해 근거: `Host` 이름과 값 대응 1개를 예시 뒤 적용 확인했습니다. 한 문항으로 산출한 잠정 속도는 약 8.6개/시간이며, 장기 속도로 일반화하지 않습니다. 이후 항목의 확인 답변은 없어 새 이해량을 추가하지 않았습니다.
- 이번에 다룰 핵심: HTTP/1.1 요청에는 목적지 서버를 나타내는 `Host` 헤더가 필요합니다. `Host`의 주소는 클라이언트의 주소가 아니라 요청을 보내려는 서버의 주소입니다.
- 이해 확인 답변은 아직 없습니다.

## 2026-11-25 13:48 KST — 코드 표시 전수 확인 마무리

- 1단계의 실제 구현 발췌와 설계 예시는 이미 `<pre>` 블록을 사용합니다. 2~9단계의 코드 예시 렌더러도 다시 확인해 `actual`과 `design`을 모두 줄바꿈·들여쓰기 보존 블록으로 출력합니다.
- 실제 프로젝트 소스가 있는 2~5단계에는 각 상자의 실제 발췌가 별도 칸으로 표시되고, 6~9단계는 설계 예시로 구분됩니다. 일반 문단으로 코드를 출력하는 경로는 코드 상자에서 사용하지 않습니다.
- 화면에서 직접 여는 미리보기는 로컬 `file:` URL 정책 차단으로 확인하지 못했습니다. 정적 렌더러·CSS 검사와 9개 내장 스크립트 구문 검사를 완료했습니다.

## 2026-11-25 13:52 KST — 설계 예시 코드 띄어쓰기·가독성 추가 정리

- 실제 코드 표시만이 아니라 2~9단계 설계 예시도 전수로 살펴, 한 줄에 여러 필드·호출·분기를 몰아넣은 곳을 줄 단위로 다시 배치했습니다. 구조체 필드, 함수 인자, 시그널 처리, 소켓 옵션, CGI 파이프 처리, 스레드 종료, 프록시 라우트 구조, 발표 흐름을 읽기 쉬운 블록으로 나눴습니다.
- 명령 예시는 단계별로 한 명령씩 분리했습니다. 8단계 curl 명령에는 대상 서버 주소를 넣어 그대로 읽고 실행할 수 있게 했습니다.
- 변경 범위는 설계 예시 표시 데이터입니다. 실제 프로젝트 C 발췌의 코드는 줄 번호와 대조할 수 있도록 원문을 유지했습니다.


## 2026-11-25 13:53 KST — Host 주소 구분 확인 및 질문 설계 기준 보강

- 사용자 답변: “Host: 127.0.0.1:8080은 클라이언트 주소가 아닌 요청대상 서버 주소에 해당합니다.”
- 판정: 맞습니다. `Host`는 요청 대상 서버의 주소이고, 클라이언트 출발지 주소와 구분했습니다. 설명 뒤 핵심 관계를 회상해 적용한 답변으로 확인했습니다.
- 시간당 관찰값: 앞선 헤더 이름·값 대응과 이번 Host 목적지 구분, 핵심 확인 2건을 답변 수준으로 확인했습니다. 각 설명 뒤 답변까지 약 7분과 약 5분으로 기록된 범위에서 2건 ÷ 약 12분 × 60 = 시간당 약 10건입니다. 실제 집중 시간·독립 학습 속도로 일반화하지 않는 잠정값입니다.
- 질문 작성 기준을 `All_Sentence.md`의 “질문 설계 — 생각을 코드와 모듈 조립으로 잇기”에 기록했습니다. 상황의 목표, 담당 모듈, 입력·출력, 다음 모듈과의 연결을 묻고, 단순 도구 실행 질문은 피하도록 했습니다.
- 다음 위치: 3-3 `Content-Length`와 본문. 시간표의 3단계 블록에서 남은 시간에 맞춰 핵심 데이터 흐름만 다룹니다.

## 2026-11-25 13:54 KST — 설계 예시 가독성 검사 완료

- 구조체와 함수 선언을 여러 줄로 풀고, 한 줄에 두 개의 실행문이 들어간 예시를 분리했습니다. `for` 반복문의 초기화·조건·증가식은 문법상 한 줄 머리부이므로 유지했습니다.
- 2~9단계 모든 노드의 코드 예시가 남아 있고, 여러 실행문을 한 줄에 압축한 사례가 없는지 확인했습니다. 9페이지의 내장 스크립트 구문 검사도 통과했습니다.
- 실제 프로젝트 코드 발췌와 C 소스는 이번 가독성 추가 수정에서 바꾸지 않았습니다.


## 2026-11-25 13:55 KST — 질문 설계 기준과 시간당 관찰 기준 갱신

- 사용자는 질문이 사고를 코드 흐름으로 바꾸고, 함수·자료 구조·모듈을 레고 조각처럼 입력·출력으로 연결하도록 구성되어야 한다고 명시했습니다. 이 기준을 `All_Sentence.md`에 추가하고 파일을 다시 읽어 반영을 확인했습니다.
- 실제 집중 시간이 확인되지 않아도 답변 수준 기준의 잠정 속도를 계산할 수 있도록 `LEARNER_PROGRESS_ANALYSIS.md` 기준을 바꿨습니다. 집중 시간 기반 속도와 대화 경과 시간 기반 관찰값을 분리합니다.
- 현재 13:55 KST, 시간표의 13:25~14:10 3단계 블록입니다. Host 주소 주체 구분을 확인하고 학습 위치를 3-3으로 옮겨 `Content-Length`가 파서와 본문 수신 처리 사이에서 어떻게 전달되는지 다룹니다.

## 2026-11-25 13:58 KST — 진행 위치 지도에도 실제 코드 가독성 반영

- 누락 원인: 앞선 수정은 `blueprint_all_stages.html`과 작업 기록에만 반영했고, 별도 파일인 `c_webserver_proxy_progress_map.html`에는 같은 표시 기준을 전파하지 않았습니다. 진행 지도 반영을 빠뜨린 작업 누락입니다.
- 수정: 진행 위치 지도에 실제 코드 발췌 표시 기준을 적고, 4-1을 선택하면 `src/config.c`의 `config_parse()` 전체(37–100행)를 실제 파일과 같은 줄바꿈·들여쓰기로 확인하도록 추가했습니다. 파일 경로와 행 범위는 코드 블록 위에 분리해 표시합니다.
- 작은 화면에서도 코드가 가로로 넘치지 않게 `pre-wrap`과 긴 줄 줄바꿈을 적용했습니다. 지도 기준 시각을 2026-11-25 13:58 KST로 갱신했습니다.
- 검증: 표시할 발췌가 현재 `src/config.c` 37–100행과 정확히 일치함을 확인했습니다. HTML 구조와 인라인 JavaScript 구문 검사도 통과했습니다. 소스 코드는 변경하지 않았습니다.


## 2026-11-25 14:04 KST — Content-Length 모듈 연결 학습노트 이관

- 사용자는 `http_parse()`가 `req.content_length`에 저장한 값을 본문 수신 모듈로 전달하는 설명을 지엽적이라고 판단해 학습노트 기록과 다음 내용 진행을 요청했습니다.
- 요점과 출처를 `C_Webserver_Proxy_학습질문_누적.md`에 기록했고 답변을 요구하지 않았습니다.
- 현재 시각 14:04 KST. 3단계 시간 블록은 14:10까지이므로, 다음 설명은 헤더와 본문을 가르는 빈 줄 및 본문 길이의 핵심 관계만 다룹니다.


## 2026-11-25 14:06 KST — 3-3 세부 이관 후 3-4 연결 유지로 이동

- 사용자는 Content-Length 값을 파서 출력에서 본문 수신 모듈 입력으로 연결하는 설명을 지엽적으로 판단했습니다. 세부는 학습노트에 기록했고 답을 요구하지 않았습니다.
- 3-3의 핵심(이 값이 본문 길이를 나타냄)은 이미 설명했습니다. 세부 구현 연결은 후속 참고로 남기고, 시간표에 맞춰 3-4의 연결 재사용 목적을 다룹니다.
- 현재 학습 위치를 3-4로 변경했습니다.


## 2026-11-25 14:10 KST — 진행 위치 지도 전 구간 코드 가독성 확장

- 이전 작업은 4-1 한 구간에만 코드 읽기 패널을 적용했고, 사용자가 요청한 전 단계 반영을 빠뜨렸습니다.
- 진행 위치 지도 1-1부터 9-4까지 총 44개 구간에 코드·명령 패널을 연결했습니다. 구간을 선택하면 관련 발췌가 줄바꿈과 들여쓰기를 살린 코드 블록으로 표시됩니다.
- 이미 구현한 현재 프로젝트 코드는 실제 파일 경로와 행 범위를 표시하고, 아직 구현하지 않은 6-9단계의 자료는 설계 예시 또는 의사 코드로 분명히 구분했습니다. 1단계는 현재 모듈형 프로젝트 코드의 대응 부분을 연결했습니다.
- 실제 소스 발췌 48개, 설계 코드·명령을 포함한 총 143개 패널 항목을 연결했습니다. 실제 소스 참조 표기를 `경로:시작–끝행` 형식으로 통일했습니다.
- 프로젝트의 `src/`와 `include/` 코드는 수정하지 않았습니다.


## 2026-11-25 14:13 KST — 시간표에 따라 4-1 명령줄 인자로 이동

- 사용자는 3-4의 `keep_alive` 분기 질문을 지엽적이라고 판단해 학습노트 기록과 다음 내용 진행을 요청했습니다. 해당 분기와 프로토콜 버전별 예외를 누적 노트에 옮겼고 답변을 요구하지 않았습니다.
- 시각 확인: 14:13 KST. 14:10부터 시작하는 4단계 블록에 진입했습니다. 현재 학습 위치를 4-1 명령줄 인자로 변경합니다.
- 3-5 파서 단위 테스트는 아직 학습하지 않았습니다. 완료로 표시하지 않고 후속 검토 대상으로 남깁니다.


## 2026-11-25 14:17 KST — 5-4 현재 코드 작업 표시 정정

- 지도와 이전 로그에는 마지막 확인 위치인 5-4 CGI가 현재 코드 작성 위치처럼 표시되어 있었습니다. 마지막으로 확인된 수정은 13:13 KST이며, 해당 수정 뒤 CGI 기능 검사와 `make test`를 실행하지 않았습니다. 이 기록은 미완료 상태를 뒷받침하지만 지금도 누군가 코드를 작성 중이라는 증거는 아닙니다.
- 진행 지도에서 `codeCurrent:"5-4"`를 제거했습니다. 5-4는 `코드 미완료 · 현재 작업 위치 확인 안 됨`으로 표시하여 코드 작성 중 상태와 구분합니다. 요약의 현재 코드 작성 위치 항목도 제거했습니다.
- 튜토리얼 로그의 요약 표도 `미완료·현재 위치 미확인`으로 고쳤습니다. 앞서 기록한 당시 위치와 변경 내역은 과거 기록으로 보존했습니다.


## 2026-11-25 14:27 KST — 4-1을 전체 웹서버 아키텍처에 연결

- 사용자는 명령줄 설정이 웹서버 전체 구축에 필요한 이유를 모르겠다고 했습니다. 다음 설명은 설정을 독립 문법처럼 다루지 않고, 터미널 인자 → 설정 파서 → struct config → 소켓·파일 처리 소비처로 연결합니다.
- All_Sentence.md에 모든 튜토리얼 주제의 전체 아키텍처상 목적과 후속 소비처를 설명하도록 기준을 추가하고 다시 읽어 확인했습니다.
- 시간표: 현재 4단계 14:10~15:00 블록, 학습 위치 4-1입니다.


## 2026-11-25 14:32 KST — 23시 마감에 맞춘 거시 설계 중심 일정 개정

- 사용자 요청: 학습 속도로 볼 때 23:00까지 전 과정을 마치기 어려우므로, 세부·지엽 사항은 학습노트에 넣고 거시적이며 모듈화해 쓸 핵심만 학습합니다.
- 새 목표: 23:00까지 1~7단계의 목적, 핵심 입력·출력, 모듈 연결, 전체 서버에서 얻는 기능을 한 번씩 연결해 봅니다. 설명 확인 전에는 완전 이해로 표시하지 않습니다.
- 설명 필터: 세부 문법·예외 목록·계산·반복 시험 명령은 본수업에서 생략하고 누적 학습노트로 보냅니다. 한 단계에는 핵심 설계 흐름과 큰 모듈 조립 질문 하나만 둡니다.

| 한국 시간 | 거시 학습 범위 | 산출
|---|---|---|
| 14:31–15:00 | 4단계 시스템 실행 안정성 | 설정·오류 복구·시간 제한·종료·이벤트 감시가 서버를 어떻게 지키는지 연결 |
| 15:00–16:00 | 5단계 응답과 콘텐츠 제공 | 요청 처리 결과를 HTTP 응답·파일·CGI로 조립하는 흐름 |
| 16:00–17:40 | 6단계 동시성 | 연결 작업을 나누고 공유 상태·종료를 관리하는 구조 |
| 17:40–17:50 | 진도 점검 | 시간·미확인 핵심 재배치 |
| 17:50–19:00 | 제외 시간 | 학습하지 않음 |
| 19:00–21:10 | 7단계 프록시 | 업스트림 선택과 요청·응답 양방향 중계 |
| 21:10–22:05 | 2~3단계 핵심 재연결 | 연결 상태 머신과 HTTP 파서가 서버 처리 흐름에서 맡는 역할 |
| 22:05–22:20 | 전체 구조 조립 | 1~7단계를 하나의 요청 경로로 연결 |
| 22:20–23:00 | 마감 기록 | 지도·학습자 분석·미학습 세부 노트 정리 |

- 일정은 23:00에 끝나도록 고정합니다. 구간별 기능은 거시 흐름만 다루며, 세부 주제를 더 넣어 다음 시간 블록을 침범하지 않습니다.


## 2026-11-25 14:35 KST — 4단계 시스템 안정성 전체 지도 설명

- 시간 확인: 14:35 KST. 4단계 시간 블록(14:10~15:00) 안입니다.
- 설명 범위: 4-1 설정부터 4-5 poll까지를 서버의 실행 안정성·자원 보호·여러 소켓 감시라는 큰 목적에 묶습니다. 옵션 이름과 세부 예외는 다루지 않습니다.
- 사용자의 4-1 전체 목적 질문을 반영해 각 모듈이 다음 서버 기능에 어떤 능력을 더하는지 시각화합니다. 학습 확인 전 완료로 표시하지 않습니다.


## 2026-11-25 14:46 KST — 4-3 질문 이관, 4-2 거시 핵심으로 이동

- 사용자는 무응답 연결에 시간 제한을 둘 위치와 자원 회수에 관한 질문을 지엽적이라고 판단했습니다. 문제·핵심 답·근거를 누적 학습노트에 기록하고 답을 요구하지 않았습니다.
- 현재 14:46 KST로 14:10~15:00의 4단계 블록 안입니다. 다음 핵심은 클라이언트의 조기 연결 종료가 서버 프로세스 전체를 종료시키지 않도록 보호하는 4-2입니다.


## 2026-11-25 14:55 KST — 마감형 초압축 수업 방식 적용

- 사용자는 23:00까지 전 단계를 거시적으로 배우도록 세부·지엽 내용을 학습노트로 보내고 모듈 역할만 다루라고 재요청했습니다. 직전 답변의 열 칸 형식과 개별 질문이 여전히 길었습니다.
- 새 형식: 선언 + 목적 한 줄 + 모듈 흐름 그림 + 서버 전체에서 얻는 기능. 단계당 통합 설계 질문은 하나만 두고, 지엽적이라면 답을 기다리지 않고 노트로 보냅니다.
- 현재 시간 14:55 KST. 남은 일정: 14:55–15:00 4단계 핵심 마감, 15:00–16:00 5단계, 16:00–17:40 6단계, 17:40–17:50 점검, 17:50–19:00 제외, 19:00–21:10 7단계, 21:10–22:05 2~3단계 핵심 재연결, 22:05–22:20 전체 흐름, 22:20–23:00 기록.
- 23:00 목표는 1~7단계의 목적과 모듈 연결을 한 번씩 설명할 수 있는 거시 설계 이해입니다. 세부 이해 확인은 별도 기록합니다.

## 15:04 (2026-11-25) — 5-1 응답 조립
서버 터미널에 출력한 hello와 클라이언트에 보낼 본문을 연결해 설명한다. 배경 개념을 익히는 예시는 속도 산출이나 정답 수에 포함하지 않는다. 질문은 다음 사례에서 재사용할 기능을 고르는 방식으로 구성한다.

## 2026-11-25 15:09 KST — 통합 설계도·진행 지도와 handoff 완료

- `docs/HANDOFF_FOR_MAP.md`를 작성했습니다. 파일별 행 수, 공개 함수 선언 위치와 원문, 정적 파일·CGI·리버스·포워드 요청 흐름, `enum conn_state`와 실제 전이 위치, ApacheBench 출력 전문, 당시 `docs/TUTORIAL_LOG.md` 전문을 담았습니다.
- `c_webserver_proxy_자료/ref/blueprint_all_stages.html`의 1~9단계 각 페이지 끝에 실제 프로젝트 핵심 코드 패널을 추가했습니다. 접힌 패널을 열면 실제 경로와 행 번호가 보입니다. 6-1 별도 스레드 모델 미구현과 6-5 비교 미실시도 명시했습니다.
- `c_webserver_proxy_progress_map.html`의 6~9 구간에 설계 의사 코드 대신 현재 소스·검사·측정 결과 발췌를 연결했습니다. 6-1, 6-5, 9-1, 9-2 등 미완료 항목은 완료로 표시하지 않았습니다. 7-6 선택 캐시도 시작 전 상태로 남겼습니다.
- 지도 발췌 6~9의 실제 코드 조각을 소스 파일의 해당 행과 대조했습니다. 설계도 1~9 코드 발췌도 각 파일의 행 범위와 대조했습니다.
- 마무리 확인 `make clean && make && make test && ./scripts/functional_test.sh`: C11 빌드 경고 0개, 단위 검사 3개와 기능 검사 21개 통과. 8080의 CLOSE_WAIT 출력은 비었고, 실행 중인 시험 서버도 남지 않았습니다.
- 최종 `build/server`는 sanitizer가 아닌 보통 빌드 실행 파일입니다.
- 이 작업으로 학습 완료를 표시하지는 않았습니다. 이 날의 최신 학습 위치는 15:04 KST에 확인한 5-1이며, 새 학습 답변은 받지 않았습니다.

## 2026-11-25 15:10 KST — 설계도 9-4 실제 인계 자료 연결

- 설계도 9단계의 실제 코드 패널에 `docs/HANDOFF_FOR_MAP.md`의 파일 목록·공개 선언 부분을 추가했습니다. “인계 문서를 작성 중”이던 문구를 완료 상태에 맞게 바꿨습니다.
- 진행 지도에서 학습 위치는 로그의 최신 확인인 15:04 KST 5-1로 유지하고, 코드 생성 위치는 별도 진행 중인 것으로 확인되지 않아 표시하지 않았습니다.
- 6~9 단계의 구현·검사 발췌와 handoff 행 번호를 다시 대조했습니다.

## 2026-11-25 15:15 KST — 1~9 지도 코드 발췌와 문서 상태 재확인

- 진행 지도 1~5단계에 남아 있던 과거 함수 시그니처와 코드 조각을 현재 `src/`, `include/`, `tests/` 파일의 실제 발췌로 바꿨습니다. 6~9단계에도 실제 구현·검사 기록을 연결했습니다.
- 진행 지도 전체의 경로·행 범위 코드 발췌와 설계도 1~9 각 페이지의 코드 발췌를 현재 소스와 비교했습니다. 모두 정확히 일치했습니다.
- README의 기능 검사 설명을 실제 검사 내용에 맞춰 “좀비 프로세스 수”로 구체화했습니다.
- 수업 학습 완료 상태는 바꾸지 않았습니다.

상태는 "시작 전", "진행 중", "통과", "남은 문제" 중 하나입니다.

---


## 2026-11-25 17:04 KST — 6단계 브랜치 구현·비교 갱신

- 학습 위치는 `docs/TUTORIAL_LOG.md`의 16:22 KST 기록인 6-2로 유지합니다. 코드 구현·측정 완료를 학습 완료로 바꾸지 않습니다.
- `stage6-sequential`, `stage6-per-connection`, `stage6-thread-pool`, `stage6-model-comparison`은 프로젝트 내부 독립 Git 저장소의 로컬 브랜치입니다. 상위 `Krafton_Jungle_SWAI_Lab/.git`에는 추가·커밋·푸시하지 않았습니다. 원격 push도 하지 않았습니다.
- 진행 지도 `c_webserver_proxy_progress_map.html`의 6-1에는 연결별 thread 함수, `pthread_create()`와 `pthread_detach()`, 종료 대기를 브랜치 출처와 실제 행으로 연결했습니다. 6-5에는 세 모델의 4회 측정 중앙값·범위와 측정 한계를 추가했습니다.
- `ref/blueprint_all_stages.html` 6단계 페이지의 접힌 실제 코드 패널에도 네 로컬 브랜치의 처리 로직과 측정 요약을 추가했습니다. 기본 구조도는 스레드 풀 브랜치임을 설명에 표시했습니다.
- 측정 요약: 순차 11,681.28 req/s, 연결별 스레드 15,027.83 req/s, 8-worker 풀 14,997.58 req/s. 각 모델 `ab -n 2000 -c 50` 네 번, 실패 요청 0건. 같은 Mac의 loopback 디버그 빌드 결과이며 일반적인 성능 순위를 뜻하지 않습니다. 순차 버전은 과거 1단계 커밋이 아닌 통제 비교 브랜치입니다.
- 연결별 스레드 브랜치의 빌드·단위 검사·21개 기능 검사·TSan 기능 검사를 통과했습니다. 일부러 잘못된 `&conn_fd`를 넘기는 실험은 하지 않았습니다. 6-5 개념 비교표에 대한 학습자 작성도 미실시입니다.
