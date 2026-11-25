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
4. 응답이 끝나면 `cache_capture_finish()`가 상태 200, `Content-Length`와 실제 본문 길이의 일치 여부, 크기 제한을 확인한 뒤 저장한다. 기본 만료 시간은 30초이며, 원 서버가 더 짧은 `max-age` 또는 `s-maxage`를 지정하면 그 시간에 만료한다.
5. 저장 공간이 부족하면 연결 리스트의 가장 오래된 항목부터 제거한다.
6. 조회할 때 단조 시계(monotonic clock)로 만료된 항목을 지운다. wall clock(시스템 시각)이 조정되어도 만료 시간이 뒤로 돌아가지 않는다.
7. `GET`은 요청 조회부터 응답 전송까지 공유 읽기 잠금을 유지한다. 다른 메서드는 단독 쓰기 잠금을 얻어 기존 GET 처리가 끝난 뒤 실행되며, 처리 전후에 전체 캐시를 비우고 세대 번호도 올린다. 따라서 오래된 GET 응답이 변경 요청 뒤에 전송되지 않고, 잠금 밖에서 시작한 캡처도 세대 번호가 달라지면 저장되지 않는다.

## 경계와 제한

- 저장 조건은 GET, 상태 200, 완전한 Content-Length 본문, 항목 상한 이내다.
- 최대 항목 수 64개, 항목당 256 KiB, 전체 8 MiB다.
- 기본 TTL(Time To Live, 캐시 항목 유효 시간)은 30초다. `max-age`와 `s-maxage`는 이 시간보다 짧을 때만 만료 시간을 줄인다. `Expires` 응답은 해당 날짜를 해석하지 않고 캐시하지 않는다. `no-cache`, `private`, `no-store`, TTL 0도 저장하지 않는다.
- Authorization, Cookie, Range, 조건부 요청 헤더를 포함한 요청은 조회·저장을 건너뛴다.
- 응답에 Set-Cookie, Vary, Transfer-Encoding, Expires가 있으면 저장하지 않는다. `Cache-Control: private`, `no-store`, `no-cache`, `must-revalidate`, `proxy-revalidate`도 저장하지 않는다.
- GET은 공유 읽기 잠금, GET 이외 요청은 단독 쓰기 잠금을 사용한다. 이 잠금은 캐시 조회뿐 아니라 요청 경로 처리와 클라이언트 응답 전송까지 유지한다. 기존 GET 응답을 먼저 끝낸 뒤 비-GET 처리를 시작하며, 다른 GET은 변경 처리가 끝날 때까지 기다린다.
- 비-GET 요청은 쓰기인지 확정할 수 없어 보수적으로 전체 캐시를 처리 전후에 비운다. 영향 URL을 추론하지 않으며, 캐시 세대 번호도 올려 잠금 밖의 오래된 캡처가 뒤늦게 저장되지 않게 한다.
- CGI는 캐시 경로에서 제외한다.
- 저장할 수 없는 응답은 계속 원래 백엔드나 디스크 경로로 처리한다. 캐시를 사용할 수 없어도 요청 경로는 실패 처리하지 않는다.
- 외부 프로세스가 파일이나 백엔드 데이터를 바꾸면 쓰기 요청 무효화는 감지하지 못한다. 해당 항목은 최대 30초(원 응답의 max-age/s-maxage가 더 짧으면 그 시간) 뒤 만료한다. 즉, 변경 직후부터 만료 전까지의 짧은 오래된 응답 구간은 남을 수 있다.

## 코드 위치

- `include/cache.h:1–52`, `src/cache.c:13–32`: 캐시 인터페이스, 상한·TTL 상수, FIFO 항목, mutex/rwlock과 세대 번호
- `src/cache.c:39–58`: 단조 시계로 만료 시각을 비교하고 만료 항목을 제거
- `src/cache.c:41–57`: 단조 시계로 만료 시각을 비교하고 만료 항목을 제거
- `src/cache.c:79–147`: 원 서버의 Cache-Control 지시를 해석해 캐시 금지와 더 짧은 TTL을 적용
- `src/cache.c:171–199`: GET 및 민감·조건부 요청 제외, 절대 URL 키 구성
- `src/cache.c:203–220`: GET 읽기 잠금, 비-GET 쓰기 잠금의 획득과 해제
- `src/cache.c:222–329`: 상태 줄·헤더 분석, `X-Cache`와 `Connection: close` 구성, 저장 제외 응답 판정
- `src/cache.c:350–371`: 만료 항목 정리 후 키 검색과 응답 복사, 읽기 잠금 아래 HIT 전송
- `src/cache.c:392–559`: MISS 응답 캡처·중계, 세대 번호 저장, 200/길이 확인, TTL 설정, FIFO 저장 및 쓰기 무효화
- `src/conn.c:244–328`: `CONN_ROUTING`에서 요청별 읽기/쓰기 잠금, HIT 조기 반환, MISS 캡처, 비-GET 요청 전후 무효화
- `src/proxy.c:249–261, 289–335, 407–429`: 리버스·포워드 업스트림 응답을 캡처 함수로 전달하는 경로
- `src/static.c:131–170`: 정적 응답 헤더와 파일 바이트를 같은 캡처 함수로 전달하는 경로
- `src/response.c:29–64`: 정적 응답 헤더를 전송 전에 문자열로 만들도록 `resp_format_head()` 추가

## 확인 기록

세 브랜치에서 `make clean && make`가 경고 없이 완료됐다. `stage6-model-comparison`에서 `make test`의 HTTP 파서, MIME, 라우팅 테스트가 모두 통과했다. 정적 파일 경로에서 첫 응답은 `X-Cache: MISS`, 두 번째 응답은 `X-Cache: HIT`였고 본문은 같았다. 리버스 프록시도 두 번 요청해 첫 MISS·다음 HIT를 확인했으며, 백엔드는 한 번만 요청받았다.

## 일관성 개선 확인

- 응답에 `Cache-Control: max-age=1`을 넣은 임시 백엔드로 요청했습니다. 첫 번째 GET은 MISS, 바로 다음 GET은 HIT, 1.2초 후 GET은 다시 MISS였습니다.
- 같은 서버에서 POST로 백엔드 내용을 바꾼 뒤 GET을 보냈습니다. POST 전 캐시가 제거됐고, 다음 GET은 MISS로 새 본문을 받아 저장했습니다.
- 오래 걸리는 GET과 POST를 겹쳐 요청했습니다. GET의 이전 본문 응답이 먼저 끝난 뒤 POST가 처리됐고, POST 이후의 GET은 MISS로 변경된 본문을 받았습니다. 다음 동일 GET은 HIT, max-age 1초 경과 후에는 다시 MISS였습니다.
- 단위 테스트는 기존 `make test`를 다시 실행해 HTTP·MIME·라우팅 검사가 통과했습니다. TTL·쓰기 무효화 변경을 세 브랜치에 반영한 뒤 각 브랜치의 clean build도 완료했습니다.
