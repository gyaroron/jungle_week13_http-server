#!/bin/bash

# 프로젝트 루트에서 서버·백엔드·curl 검사를 시작하고 종료 때 자원을 정리합니다.
set -u

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "$ROOT_DIR"

SERVER_PORT=${TEST_PORT:-8080}
BACKEND_A_PORT=8001
BACKEND_B_PORT=8002
BUILD_DIR="$ROOT_DIR/build"
BACKEND_A_DIR="$BUILD_DIR/functional_backend_a"
BACKEND_B_DIR="$BUILD_DIR/functional_backend_b"
ROUTES_FILE="$BUILD_DIR/functional_routes.conf"
SERVER_LOG="$BUILD_DIR/functional_server.log"
LARGE_TEST_FILE="$ROOT_DIR/www/large-test.bin"

server_pid=""
backend_a_pid=""
backend_b_pid=""
slow_backend_pid=""
failures=0

cleanup() {
    for pid in "$server_pid" "$backend_a_pid" "$backend_b_pid" "$slow_backend_pid"; do
        if [[ -n "$pid" ]]; then kill "$pid" 2>/dev/null || true; fi
    done
    for pid in "$server_pid" "$backend_a_pid" "$backend_b_pid" "$slow_backend_pid"; do
        if [[ -n "$pid" ]]; then wait "$pid" 2>/dev/null || true; fi
    done
    rm -rf "$BACKEND_A_DIR" "$BACKEND_B_DIR" "$ROUTES_FILE"
    rm -f "$LARGE_TEST_FILE"
}
trap cleanup EXIT INT TERM

pass() { printf 'PASS %s\n' "$1"; }
fail() { printf 'FAIL %s\n' "$1"; failures=$((failures + 1)); }

port_is_busy() { nc -z 127.0.0.1 "$1" >/dev/null 2>&1; }

for port in "$SERVER_PORT" "$BACKEND_A_PORT" "$BACKEND_B_PORT"; do
    if port_is_busy "$port"; then
        printf '포트 %s가 이미 사용 중입니다. 해당 프로세스는 종료하지 않았습니다.\n' "$port" >&2
        exit 2
    fi
done

if [[ ! -x ./build/server ]]; then
    printf 'build/server가 없습니다. 먼저 make를 실행하세요.\n' >&2
    exit 2
fi

mkdir -p "$BACKEND_A_DIR/api" "$BACKEND_B_DIR/api"
if [[ -e "$LARGE_TEST_FILE" ]]; then
    printf '시험 파일 경로가 이미 있습니다: %s\n' "$LARGE_TEST_FILE" >&2
    exit 2
fi
truncate -s 33554432 "$LARGE_TEST_FILE"
printf 'backend A\n' > "$BACKEND_A_DIR/index.html"
printf 'backend B\n' > "$BACKEND_B_DIR/index.html"
printf 'backend A\n' > "$BACKEND_A_DIR/api/index.html"
printf 'backend B\n' > "$BACKEND_B_DIR/api/index.html"

# 닫힌 백엔드는 운영체제가 고른 임시 포트로 지정합니다.
DEAD_PORT=$(python3 -c 'import socket; s=socket.socket(); s.bind(("127.0.0.1", 0)); print(s.getsockname()[1]); s.close()')
SLOW_PORT=$(python3 -c 'import socket; s=socket.socket(); s.bind(("127.0.0.1", 0)); print(s.getsockname()[1]); s.close()')
cat > "$ROUTES_FILE" <<ROUTES
* /api/ 127.0.0.1:$BACKEND_A_PORT 127.0.0.1:$BACKEND_B_PORT
* /dead/ 127.0.0.1:$DEAD_PORT
* /timeout/ 127.0.0.1:$SLOW_PORT
ROUTES

python3 -m http.server "$BACKEND_A_PORT" --bind 127.0.0.1 \
    --directory "$BACKEND_A_DIR" > "$BUILD_DIR/backend_a.log" 2>&1 &
backend_a_pid=$!
python3 -m http.server "$BACKEND_B_PORT" --bind 127.0.0.1 \
    --directory "$BACKEND_B_DIR" > "$BUILD_DIR/backend_b.log" 2>&1 &
backend_b_pid=$!
python3 -c 'import socket,time; s=socket.socket(); s.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1); s.bind(("127.0.0.1", int(__import__("sys").argv[1]))); s.listen(); c,_=s.accept(); c.recv(4096); time.sleep(5); c.close(); s.close()' "$SLOW_PORT" > "$BUILD_DIR/slow_backend.log" 2>&1 &
slow_backend_pid=$!

./build/server -p "$SERVER_PORT" -w 4 -t 2 -c "$ROUTES_FILE" -f \
    > "$SERVER_LOG" 2>&1 &
server_pid=$!

ready=0
for _ in $(seq 1 50); do
    if ! kill -0 "$server_pid" 2>/dev/null; then break; fi
    if curl -sS --max-time 1 "http://127.0.0.1:$SERVER_PORT/" >/dev/null 2>&1; then
        ready=1
        break
    fi
    sleep 0.1
done
if [[ "$ready" -ne 1 ]]; then
    printf '서버가 시작되지 않았습니다. 로그: %s\n' "$SERVER_LOG" >&2
    exit 2
fi

fd_before=""
if command -v lsof >/dev/null 2>&1; then
    fd_before=$(lsof -nP -a -p "$server_pid" 2>/dev/null | awk 'NR > 1 { n++ } END { print n + 0 }')
fi
zombies_before=$(ps -axo stat= 2>/dev/null | awk '$1 ~ /Z/ { n++ } END { print n + 0 }')

status_for() {
    curl -sS --max-time 5 -o "$BUILD_DIR/functional_body.tmp" \
        -w '%{http_code}' "$@" 2>/dev/null || true
}

status=$(status_for "http://127.0.0.1:$SERVER_PORT/")
if [[ "$status" == 200 ]] && grep -q '정적 파일 서빙 시험 페이지' "$BUILD_DIR/functional_body.tmp"; then
    pass '정적 파일 HTTP 200'
else fail "정적 파일 HTTP 200 (status=$status)"; fi

status=$(status_for "http://127.0.0.1:$SERVER_PORT/missing-file")
[[ "$status" == 404 ]] && pass '없는 파일 HTTP 404' || fail "없는 파일 HTTP 404 (status=$status)"

status=$(status_for --path-as-is "http://127.0.0.1:$SERVER_PORT/../Makefile")
[[ "$status" == 403 ]] && pass '루트 밖 경로 HTTP 403' || fail "루트 밖 경로 HTTP 403 (status=$status)"

bad_status=$(printf 'GARBAGE\r\n\r\n' | nc -w 2 127.0.0.1 "$SERVER_PORT" 2>/dev/null | head -n 1 | awk '{print $2}')
[[ "$bad_status" == 400 ]] && pass '잘못된 요청 HTTP 400' || fail "잘못된 요청 HTTP 400 (status=${bad_status:-없음})"

method_status=$(printf 'PUT / HTTP/1.1\r\nHost: localhost\r\n\r\n' | nc -w 2 127.0.0.1 "$SERVER_PORT" 2>/dev/null | head -n 1 | awk '{print $2}')
[[ "$method_status" == 501 ]] && pass '지원하지 않는 메서드 HTTP 501' || fail "지원하지 않는 메서드 HTTP 501 (status=${method_status:-없음})"

long_path=$(python3 -c 'print("a" * 3000)')
status=$(status_for "http://127.0.0.1:$SERVER_PORT/$long_path")
[[ "$status" == 414 ]] && pass '긴 요청 대상 HTTP 414' || fail "긴 요청 대상 HTTP 414 (status=$status)"

status=$(head -c 2000000 /dev/zero | curl -sS --max-time 5 -o /dev/null \
    -w '%{http_code}' --data-binary @- "http://127.0.0.1:$SERVER_PORT/echo" 2>/dev/null || true)
[[ "$status" == 413 ]] && pass '큰 본문 HTTP 413' || fail "큰 본문 HTTP 413 (status=$status)"

status=$(status_for "http://127.0.0.1:$SERVER_PORT/cgi-bin/hello.sh?name=test")
if [[ "$status" == 200 ]] && grep -q 'hello query=name=test' "$BUILD_DIR/functional_body.tmp"; then
    pass 'CGI GET'
else fail "CGI GET (status=$status)"; fi

status=$(status_for -d 'a=1&b=2' "http://127.0.0.1:$SERVER_PORT/cgi-bin/echo.sh")
if [[ "$status" == 200 ]] && grep -q 'a=1&b=2' "$BUILD_DIR/functional_body.tmp"; then
    pass 'CGI POST'
else fail "CGI POST (status=$status)"; fi

curl -sS --http1.1 -v "http://127.0.0.1:$SERVER_PORT/" \
    "http://127.0.0.1:$SERVER_PORT/" > /dev/null 2> "$BUILD_DIR/keepalive.log"
if grep -q 'Re-using existing connection' "$BUILD_DIR/keepalive.log"; then
    pass '연결 유지로 요청 두 건'
else fail '연결 유지로 요청 두 건'; fi

# 같은 URL은 GET 캐시가 응답하므로, 서로 다른 URL로 백엔드 분배를 확인합니다.
proxy_a=$(curl -sS --max-time 5 "http://127.0.0.1:$SERVER_PORT/api/?rr=1")
proxy_b=$(curl -sS --max-time 5 "http://127.0.0.1:$SERVER_PORT/api/?rr=2")
if [[ "$proxy_a" == 'backend A' && "$proxy_b" == 'backend B' ]]; then
    pass '리버스 프록시 라운드 로빈'
else fail "리버스 프록시 라운드 로빈 (A='$proxy_a', B='$proxy_b')"; fi

# 같은 URL을 다시 요청하면 백엔드 대신 캐시가 같은 응답을 돌려줍니다.
cache_body=$(curl -sS --max-time 5 -D "$BUILD_DIR/cache_headers.tmp" \
    "http://127.0.0.1:$SERVER_PORT/api/?rr=1")
if [[ "$cache_body" == 'backend A' ]] && grep -qi '^X-Cache: HIT' "$BUILD_DIR/cache_headers.tmp"; then
    pass '같은 URL 재요청 캐시 HIT'
else fail "같은 URL 재요청 캐시 HIT (body='$cache_body')"; fi

status=$(status_for "http://127.0.0.1:$SERVER_PORT/dead/")
[[ "$status" == 502 ]] && pass '꺼진 백엔드 HTTP 502' || fail "꺼진 백엔드 HTTP 502 (status=$status)"

status=$(status_for "http://127.0.0.1:$SERVER_PORT/timeout/")
[[ "$status" == 504 ]] && pass '응답 없는 백엔드 HTTP 504' || fail "응답 없는 백엔드 HTTP 504 (status=$status)"

connect_output=$(curl -k -v --max-time 3 --proxy "http://127.0.0.1:$SERVER_PORT" \
    --noproxy '' https://example.com/ 2>&1 || true)
if grep -q 'response 501' <<< "$connect_output"; then
    pass 'HTTPS CONNECT 거부 HTTP 501'
else fail 'HTTPS CONNECT 거부 HTTP 501'; fi

https_status=$(printf 'GET https://example.com/ HTTP/1.1\r\nHost: example.com\r\n\r\n' | \
    nc -w 2 127.0.0.1 "$SERVER_PORT" 2>/dev/null | head -n 1 | awk '{print $2}')
[[ "$https_status" == 400 ]] && pass 'https absolute-form 거부 HTTP 400' || fail "https absolute-form 거부 HTTP 400 (status=${https_status:-없음})"

forward=$(curl -sS --max-time 5 --proxy "http://127.0.0.1:$SERVER_PORT" \
    --noproxy '' "http://127.0.0.1:$BACKEND_A_PORT/")
[[ "$forward" == 'backend A' ]] && pass '포워드 프록시' || fail "포워드 프록시 (body='$forward')"

concurrent_pids=()
for i in $(seq 1 20); do
    (curl -sS --max-time 5 -o /dev/null -w '%{http_code}' \
        "http://127.0.0.1:$SERVER_PORT/" > "$BUILD_DIR/concurrent.$i") &
    concurrent_pids+=("$!")
done
concurrent_failed=0
for pid in "${concurrent_pids[@]}"; do
    wait "$pid" || concurrent_failed=1
done
for i in $(seq 1 20); do
    [[ "$(cat "$BUILD_DIR/concurrent.$i" 2>/dev/null)" == 200 ]] || concurrent_failed=1
    rm -f "$BUILD_DIR/concurrent.$i"
done
if [[ "$concurrent_failed" -eq 0 ]]; then
    pass '동시 클라이언트 20개'
else fail '동시 클라이언트 20개'; fi

# 느린 수신자가 큰 파일 전송 도중 연결을 닫아도 서버가 살아 있는지 봅니다.
curl -sS --limit-rate 16k --max-time 0.2 \
    "http://127.0.0.1:$SERVER_PORT/large-test.bin" -o /dev/null \
    2> "$BUILD_DIR/client_abort.log" || true
sleep 0.2
status=$(status_for "http://127.0.0.1:$SERVER_PORT/")
[[ "$status" == 200 ]] && pass '큰 파일 수신 중 클라이언트 종료 뒤 서버 생존' || \
    fail "큰 파일 수신 중 클라이언트 종료 뒤 서버 생존 (status=$status)"

# 헤더를 조금씩 보내는 연결도 요청 전체 제한 시간(-t 2) 안에 408로 끊겨야 합니다.
# recv() 한 번마다 제한 시간이 새로 시작되면 이 연결이 일꾼을 무기한 붙잡습니다.
slow_header=$(python3 - "$SERVER_PORT" <<'PY'
import socket, sys, time
s = socket.create_connection(("127.0.0.1", int(sys.argv[1])))
s.sendall(b"GET / HTTP/1.1\r\n")
s.settimeout(0.5)
start = time.time()
while time.time() - start < 6:
    try:
        data = s.recv(64)
        if data:
            print(f"{data.split(b' ')[1].decode()} {time.time() - start:.1f}")
            break
    except socket.timeout:
        pass
    try:
        s.sendall(b"X-a: b\r\n")
    except OSError:
        print(f"closed {time.time() - start:.1f}")
        break
else:
    print("none 6.0")
PY
)
read -r slow_status slow_seconds <<< "$slow_header"
if [[ "$slow_status" == 408 ]] && awk "BEGIN { exit !($slow_seconds < 4) }"; then
    pass '헤더를 조금씩 보내는 연결은 제한 시간 뒤 408'
else fail "헤더를 조금씩 보내는 연결은 제한 시간 뒤 408 (결과=$slow_header)"; fi

# 큰 파일을 아주 느리게 받는 GET이 있어도 POST와 다른 GET이 기다리지 않아야 합니다.
# 캐시 잠금을 응답 전송 동안 잡으면 대기 중인 POST가 이후 GET 전체를 막습니다.
slow_reader=$(python3 - "$SERVER_PORT" <<'PY'
import socket, sys, threading, time
port = int(sys.argv[1])
slow = socket.socket()
slow.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 4096)
slow.connect(("127.0.0.1", port))
slow.sendall(b"GET /large-test.bin HTTP/1.1\r\nHost: t\r\n\r\n")
stop = threading.Event()
def trickle():
    while not stop.is_set():
        try:
            slow.recv(1024)
        except OSError:
            return
        time.sleep(0.5)
threading.Thread(target=trickle, daemon=True).start()
time.sleep(1.0)
def ask(raw):
    t = time.time()
    try:
        c = socket.create_connection(("127.0.0.1", port), timeout=3)
        c.settimeout(3)
        c.sendall(raw)
        line = c.recv(64).split(b"\r\n", 1)[0].decode()
        c.close()
        return line.split(" ")[1], time.time() - t
    except OSError:
        return "timeout", time.time() - t
post = ask(b"POST /cgi-bin/echo.sh HTTP/1.1\r\nHost: t\r\nContent-Length: 2\r\nConnection: close\r\n\r\nhi")
get = ask(b"GET / HTTP/1.1\r\nHost: t\r\nConnection: close\r\n\r\n")
stop.set()
slow.close()
print(f"{post[0]} {get[0]} {max(post[1], get[1]):.1f}")
PY
)
read -r reader_post reader_get reader_seconds <<< "$slow_reader"
if [[ "$reader_post" == 200 && "$reader_get" == 200 ]] && awk "BEGIN { exit !($reader_seconds < 1) }"; then
    pass '느린 GET 다운로드 중에도 POST와 GET 즉시 처리'
else fail "느린 GET 다운로드 중에도 POST와 GET 즉시 처리 (결과=$slow_reader)"; fi

status=$(status_for "http://127.0.0.1:$SERVER_PORT/")
[[ "$status" == 200 ]] && pass '극한 요청 뒤 서버 생존' || fail "극한 요청 뒤 서버 생존 (status=$status)"

if [[ -n "$fd_before" ]]; then
    fd_after=$(lsof -nP -a -p "$server_pid" 2>/dev/null | awk 'NR > 1 { n++ } END { print n + 0 }')
    [[ "$fd_after" == "$fd_before" ]] && pass '요청 전후 파일 디스크립터 수 동일' || fail "파일 디스크립터 수 (before=$fd_before after=$fd_after)"
else
    printf 'SKIP lsof 파일 디스크립터 확인 (lsof 없음)\n'
fi

zombies_after=$(ps -axo stat= 2>/dev/null | awk '$1 ~ /Z/ { n++ } END { print n + 0 }')
[[ "$zombies_after" == "$zombies_before" ]] && pass '좀비 프로세스 수 증가 없음' || fail "좀비 프로세스 수 (before=$zombies_before after=$zombies_after)"

rm -f "$BUILD_DIR/functional_body.tmp"
if [[ "$failures" -eq 0 ]]; then
    printf '모든 기능 검사 통과\n'
    exit 0
fi
printf '실패한 기능 검사: %s\n' "$failures"
exit 1
