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
