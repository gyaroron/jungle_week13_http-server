# 7-6 선택 캐시 구현 기록

## 적용 범위

캐시를 세 연결 처리 모델에 같은 방식으로 적용했다. 모델마다 `main.c`의 연결 분배 방식은 다르지만 모두 `conn_handle()`의 `CONN_ROUTING`에서 동일한 캐시 API를 호출한다.

- 순차 처리: main 스레드가 연결을 하나씩 처리하고 `conn_handle()`이 캐시를 조회·기록한다.
- 연결별 스레드: 연결마다 만들어진 스레드가 같은 `conn_handle()`을 호출한다.
- 스레드 풀: worker 8개가 대기열에서 연결을 가져와 같은 `conn_handle()`을 호출한다.

## 처리 흐름

1. `cache_key_for_request()`는 GET 요청의 절대 URL을 만든다. absolute-form 대상은 그대로 사용하고, origin-form 대상은 `Host`와 경로를 결합한다.
2. `cache_lookup_send()`는 뮤텍스를 잡고 키를 찾는다. 찾으면 응답 데이터를 별도 메모리에 복사한 뒤 뮤텍스를 풀고, 클라이언트에 `X-Cache: HIT`와 함께 보낸다.
3. 미스면 `cache_capture_write()`가 리버스 프록시 응답 또는 정적 파일 응답을 받으며 클라이언트로 중계한다. 응답 헤더를 받은 뒤 `X-Cache: MISS`와 `Connection: close`를 붙인다.
4. 응답이 끝나면 `cache_capture_finish()`가 상태 200, `Content-Length`와 실제 본문 길이의 일치 여부, 크기 제한을 확인한 뒤 저장한다.
5. 저장 공간이 부족하면 연결 리스트의 가장 오래된 항목부터 제거한다.

## 경계와 제한

- 저장 조건은 GET, 상태 200, 완전한 Content-Length 본문, 항목 상한 이내다.
- 최대 항목 수 64개, 항목당 256 KiB, 전체 8 MiB다.
- Authorization, Cookie, Range, 조건부 요청 헤더를 포함한 요청은 조회·저장을 건너뛴다.
- 응답에 Set-Cookie, Vary, Transfer-Encoding, Cache-Control의 private/no-store가 있으면 저장하지 않는다.
- CGI는 캐시 경로에서 제외한다.
- 저장할 수 없는 응답은 계속 원래 백엔드나 디스크 경로로 처리한다. 캐시를 사용할 수 없어도 요청 경로는 실패 처리하지 않는다.
- 만료 시간과 쓰기 요청에 따른 캐시 무효화는 구현하지 않았다. 백엔드나 파일 내용을 갱신하면 서버를 재시작해 메모리 캐시를 비운다.

## 코드 위치

- `include/cache.h:1–45`, `src/cache.c:12–48`: 캐시 인터페이스, 64개·8 MiB 상한, FIFO 목록과 뮤텍스 공유 상태
- `src/cache.c:61–89`: GET 및 민감·조건부 요청 제외, 절대 URL 키 구성
- `src/cache.c:92–192`: 상태 줄·헤더 분석, `X-Cache`와 `Connection: close` 헤더 구성, 저장 제외 응답 판정
- `src/cache.c:218–237`: 뮤텍스 안에서 키 검색과 안전한 응답 복사, 이후 `HIT` 응답 전송
- `src/cache.c:265–402`: MISS 응답 캡처·중계, 200/길이 확인, 크기 상한 내 저장, 오래된 항목 제거
- `src/conn.c:244–305`: `CONN_ROUTING`에서 HIT 조기 반환, MISS 캡처 설정, 프록시·정적·CGI 분기
- `src/proxy.c:249–261, 289–335, 407–429`: 리버스·포워드 업스트림 응답을 캡처 함수로 전달하는 경로
- `src/static.c:131–170`: 정적 응답 헤더와 파일 바이트를 같은 캡처 함수로 전달하는 경로
- `src/response.c:29–64`: 정적 응답 헤더를 전송 전에 문자열로 만들도록 `resp_format_head()` 추가

## 확인 기록

세 브랜치에서 `make clean && make`가 경고 없이 완료됐다. `stage6-model-comparison`에서 `make test`의 HTTP 파서, MIME, 라우팅 테스트가 모두 통과했다. 정적 파일 경로에서 첫 응답은 `X-Cache: MISS`, 두 번째 응답은 `X-Cache: HIT`였고 본문은 같았다. 리버스 프록시도 두 번 요청해 첫 MISS·다음 HIT를 확인했으며, 백엔드는 한 번만 요청받았다.
