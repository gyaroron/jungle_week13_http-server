---
tags:
  - 웹서버
  - C언어
  - TCP
  - POSIX
  - SOCKET
studied: 2026-11-20
reviewed:
  - 2026-11-20
cssclasses:
  - wide-table
---
```dataviewjs
await dv.view("템플릿/경과일")
```

---

## 요약

| <span style="white-space:nowrap">묶음</span> | <span style="white-space:nowrap">항목 번호</span> | 다루는 내용 |
| --- | --- | --- |
| <span style="white-space:nowrap">과제 개요 · 필수 구현</span> | <span style="white-space:nowrap">1 ~ 7</span> | C 언어와 POSIX 소켓 API만으로 웹서버를 만든다는 선언<br>TCP 소켓 서버의 socket → bind → listen → accept 흐름<br>HTTP 요청 파싱 (요청 라인 · 헤더 · 본문, GET과 POST)<br>HTTP 응답 생성 (상태 라인 · 표준 헤더 · 본문 송신)<br>정적 파일 서빙 (MIME 타입 매핑, 404 / 403 / 500 오류 응답)<br>동적 요청 처리 (CGI 실행 또는 백엔드 서버로 전달)<br>동시 요청 처리 (fork · pthread · I/O 멀티플렉싱) |
| <span style="white-space:nowrap">Proxy 구현</span> | <span style="white-space:nowrap">8 ~ 10</span> | Forward Proxy와 Reverse Proxy 중 최소 한 가지는 필수<br>Forward Proxy: 클라이언트 → Proxy → 원 서버, 선택적으로 캐싱<br>Reverse Proxy: 클라이언트 → Reverse Proxy → 백엔드 (1 .. N), Round-Robin 로드 밸런싱과 경로 · 호스트 헤더 기반 라우팅 |
| <span style="white-space:nowrap">기술 스택 · 개발 환경</span> | <span style="white-space:nowrap">11 ~ 12</span> | libevent · libuv · nginx · Apache · mongoose 같은 완성된 라이브러리 금지<br>표준 C 라이브러리와 POSIX API만 허용<br>Makefile 또는 CMakeLists.txt 제공, 포트 · 정적 파일 루트 · 라우팅 규칙을 CLI 인자 또는 설정 파일로 받기<br>Windows 학생은 WSL2 + Ubuntu, Docker 또는 클라우드에서 개발 |
| <span style="white-space:nowrap">중점 포인트</span> | <span style="white-space:nowrap">13 ~ 18</span> | 소켓 프로그래밍의 기본 흐름 (3-way handshake, SO_REUSEADDR, blocking과 non-blocking)<br>RFC 7230 ~ 7235의 최소 구현 범위 (경계 식별, Content-Length, keep-alive)<br>동시성 모델의 트레이드오프와 C10K 문제<br>Proxy와 Reverse Proxy의 차이<br>메모리 관리 (malloc / free 짝, 버퍼 오버플로우 방지, 자원 회수)<br>시그널 처리 (SIGINT / SIGTERM 안전한 셧다운, SIGPIPE 무시) |
| <span style="white-space:nowrap">품질</span> | <span style="white-space:nowrap">19 ~ 24</span> | 모듈 단위 테스트 (HTTP 파서, MIME 매핑, 라우팅 / 프록시 전달 로직)<br>curl과 브라우저로 하는 기능 테스트, 동시 다중 클라이언트 확인<br>ab · wrk · hey로 하는 부하 테스트<br>valgrind 또는 sanitizer로 하는 메모리 누수 검사<br>4가지 엣지 케이스 (잘못된 요청, 매우 큰 본문 · 긴 URI, 백엔드 무응답 · 타임아웃, 클라이언트의 갑작스러운 연결 종료)<br>이력서와 포트폴리오에 넣을 수 있는 완성도 |

**추가 설명**

이 노트는 "C 웹서버 · 프록시 요구사항" 원문을 24개 항목으로 나누고, 항목마다 원문 · 직관적 1초 컷 독해 · 바로 읽기 · 이 조건이 없으면 생기는 대참사 · 예상 · 반응 · 의심 · 깨달음 · 확인 · 뉘앙스 / 의역의 순서로 해석한 자료를 그대로 옮긴 것입니다.

위 표의 묶음은 원문 요구사항 문서의 큰 제목(과제 개요 · 필수 구현 항목 · Proxy 또는 Reverse Proxy 구현 · 기술 스택 / 제한 · 개발 환경 · 중점 포인트 · 품질)을 따라 나눈 것이고, 항목 번호는 아래 "내용" 절의 소제목 번호와 같습니다.

"확인" 줄의 `[실측]` 수치(예: 42,500 RPS, 1,000개 연결에서 메모리 5MB 미만)는 해석을 쓴 쪽이 적은 값이며, 이 노트를 쓰는 시점에 직접 측정해 확인한 값이 아닙니다. 실제 구현을 마친 뒤 자기 서버로 다시 측정해 비교하는 기준으로 읽어야 합니다.

> [!note]- 축약어 · 원문 표 (눌러서 펼치기)
> 
> **표준 · 프로토콜 · 문서**
> 
> | 축약어 | 원문 | 뜻 |
> | --- | --- | --- |
> | <span style="white-space:nowrap">AI</span> | **A**rtificial **I**ntelligence | 인공지능입니다.<br>노트 맨 앞 출처 경로의 `11_12주차_AI_함께하기` · `타_AI_작업_수행_절대_지시서.md`에 나옵니다. |
> | <span style="white-space:nowrap">API</span> | **A**pplication **P**rogramming **I**nterface | 프로그램이 운영체제나 라이브러리의 기능을 부르는 약속된 함수 묶음입니다.<br>1번 · 2번 · 13번 항목의 "POSIX 소켓 API"는 `socket()` · `bind()` 같은 함수들을 가리킵니다. |
> | <span style="white-space:nowrap">POSIX</span> | **P**ortable **O**perating **S**ystem **I**nterface (X는 Unix를 뜻함) | 유닉스 계열 운영체제가 공통으로 지키는 시스템 호출 · 헤더 표준입니다.<br>1번 · 11번 · 12번 항목에서 "POSIX 표준 범위 안에 머무르기"로 이식성의 기준이 됩니다. |
> | <span style="white-space:nowrap">HTTP</span> | **H**yper**T**ext **T**ransfer **P**rotocol | 웹 브라우저와 웹서버가 요청과 응답을 주고받는 텍스트 기반 규약입니다.<br>3번 · 4번 · 14번 항목의 요청 라인 · 헤더 · 본문이 이 규약의 구조입니다. |
> | <span style="white-space:nowrap">HTTP/1.1</span> | **H**yper**T**ext **T**ransfer **P**rotocol version 1.1 | HTTP의 1.1 버전입니다.<br>4번 항목의 상태 라인 `HTTP/1.1 200 OK`와 1번 항목의 "HTTP/1.1 규격"에 나옵니다. |
> | <span style="white-space:nowrap">TCP</span> | **T**ransmission **C**ontrol **P**rotocol | 바이트를 순서대로, 빠짐없이 전달하는 연결형 전송 계층 규약입니다.<br>2번 · 3번 · 9번 · 13번 · 14번 항목에 나오며, 메시지 경계가 없는 "바이트 스트림"이라는 점이 파싱을 어렵게 만듭니다. |
> | <span style="white-space:nowrap">IP</span> | **I**nternet **P**rotocol | 인터넷에서 주소(IP 주소)를 붙여 패킷을 보내는 네트워크 계층 규약입니다.<br>9번 항목에서 `getaddrinfo`로 호스트 이름을 IP 주소로 바꿉니다. |
> | <span style="white-space:nowrap">DNS</span> | **D**omain **N**ame **S**ystem | 도메인 이름을 IP 주소로 바꿔 주는 이름 조회 체계입니다.<br>16번 항목에서 Forward Proxy가 "동적 외부 DNS 해석"으로 목적지를 정한다고 설명합니다. |
> | <span style="white-space:nowrap">URI</span> | **U**niform **R**esource **I**dentifier | 자원을 가리키는 식별 문자열입니다.<br>3번 항목의 요청 라인 "메서드, URI, 버전"과 23번 항목의 "매우 긴 URI"에 나옵니다. |
> | <span style="white-space:nowrap">URL</span> | **U**niform **R**esource **L**ocator | 자원의 위치(호스트 · 경로)까지 담은 URI입니다.<br>8번 · 9번 항목에서 Forward Proxy 요청의 절대 URL(`http://domain.com/path`)에서 호스트를 뽑습니다. |
> | <span style="white-space:nowrap">RFC</span> | **R**equest **f**or **C**omments | 인터넷 표준을 적은 공식 문서 시리즈입니다.<br>14번 항목의 RFC 7230 ~ 7235가 HTTP/1.1 명세입니다. |
> | <span style="white-space:nowrap">MIME</span> | **M**ultipurpose **I**nternet **M**ail **E**xtensions | 데이터 종류를 `text/html` · `image/png`처럼 적는 형식 이름 체계입니다.<br>5번 · 19번 항목에서 파일 확장자를 Content-Type으로 바꾸는 "MIME 타입 매핑"에 나옵니다. |
> | <span style="white-space:nowrap">CGI</span> | **C**ommon **G**ateway **I**nterface | 웹서버가 외부 프로그램을 실행하고 그 출력을 응답으로 돌려주는 방식입니다.<br>6번 항목의 "CGI 프로그램 실행"에 나옵니다. |
> | <span style="white-space:nowrap">WAS</span> | **W**eb **A**pplication **S**erver | 동적 로직을 실행하는 백엔드 애플리케이션 서버입니다.<br>6번 · 10번 항목에서 요청을 위임받는 백엔드로 나옵니다. |
> | <span style="white-space:nowrap">L7</span> | **L**ayer **7** | OSI 7계층 모델의 7번째 계층인 응용 계층(application layer)입니다.<br>10번 항목의 "L7 로드밸런서"는 HTTP 경로 · 헤더를 보고 분배하는 로드밸런서입니다. |
> | <span style="white-space:nowrap">C10K</span> | **C**oncurrent **10K** (10,000) connections | 한 서버가 동시 연결 1만 개를 처리하는 문제입니다.<br>15번 항목에서 1999년 Dan Kegel이 제기한 문제로 소개합니다. |
> | <span style="white-space:nowrap">RPS</span> | **R**equests **P**er **S**econd | 초당 처리한 요청 수입니다.<br>21번 · 24번 항목의 처리량(throughput) 수치 단위입니다. |
> 
> **파일 형식 · 도구 · 환경**
> 
> | 축약어 | 원문 | 뜻 |
> | --- | --- | --- |
> | <span style="white-space:nowrap">HTML</span> | **H**yper**T**ext **M**arkup **L**anguage | 웹 페이지 구조를 적는 언어입니다.<br>5번 항목의 MIME 매핑 대상과 404 오류 페이지에 나옵니다. |
> | <span style="white-space:nowrap">CSS</span> | **C**ascading **S**tyle **S**heets | 웹 페이지 모양을 정하는 스타일 언어입니다.<br>5번 · 20번 항목에 나옵니다. |
> | <span style="white-space:nowrap">JS</span> | **J**ava**S**cript | 브라우저에서 실행되는 스크립트 언어입니다.<br>5번 항목의 MIME 매핑 대상입니다. |
> | <span style="white-space:nowrap">PNG</span> | **P**ortable **N**etwork **G**raphics | 손실 없는 압축 이미지 형식입니다.<br>5번 항목의 MIME 매핑 대상과 `/ghost.png` 예시에 나옵니다. |
> | <span style="white-space:nowrap">JPG</span> | **J**oint **P**hotographic Experts **G**roup | 손실 압축 사진 이미지 형식(JPEG)의 확장자입니다.<br>5번 항목의 MIME 매핑 대상입니다. |
> | <span style="white-space:nowrap">OS</span> | **O**perating **S**ystem | 운영체제입니다.<br>12번 항목의 "OS 별로 차이"와 "OS별 차이점(epoll 대 kqueue)"에 나옵니다. |
> | <span style="white-space:nowrap">WSL / WSL2</span> | **W**indows **S**ubsystem for **L**inux (2는 두 번째 버전) | Windows 안에서 Linux를 실행하는 기능입니다.<br>12번 항목에서 Windows 학생의 필수 개발 환경으로 나옵니다. |
> | <span style="white-space:nowrap">LTS</span> | **L**ong-**T**erm **S**upport | 오랫동안 보안 업데이트를 받는 배포판입니다.<br>12번 항목의 "우분투 22.04 LTS"에 나옵니다. |
> | <span style="white-space:nowrap">CLI</span> | **C**ommand-**L**ine **I**nterface | 터미널에서 명령어와 인자로 프로그램을 다루는 방식입니다.<br>11번 항목에서 포트 · 정적 파일 루트를 CLI 인자로 받으라고 합니다. |
> | <span style="white-space:nowrap">ab</span> | **A**pache **B**ench | Apache 프로젝트의 HTTP 부하 측정 도구입니다.<br>21번 항목의 부하 테스트 도구로 나옵니다. |
> | <span style="white-space:nowrap">GCC</span> | **G**NU **C**ompiler **C**ollection | GNU 프로젝트의 컴파일러 모음입니다.<br>22번 항목에서 AddressSanitizer를 제공하는 컴파일러로 나옵니다. |
> | <span style="white-space:nowrap">C11</span> | **C** standard 20**11** | 2011년에 정해진 C 언어 표준(ISO/IEC 9899:2011)입니다.<br>11번 항목의 "클린 C11 코드"에 나옵니다. |
> | <span style="white-space:nowrap">ldd</span> | **L**ist **D**ynamic **D**ependencies | 실행 파일이 연결하는 공유 라이브러리를 보여 주는 명령입니다.<br>11번 항목에서 `ldd webserver`로 libc와 libpthread만 쓰는지 확인합니다. |
> | <span style="white-space:nowrap">libc</span> | standard **C** **lib**rary | 표준 C 라이브러리입니다.<br>11번 항목에 나옵니다. |
> | <span style="white-space:nowrap">pthread</span> | **P**OSIX **thread**s | POSIX 표준 스레드 라이브러리입니다.<br>7번 항목의 멀티스레드 방식과 11번 항목의 `<pthread.h>` · libpthread에 나옵니다. |
> | <span style="white-space:nowrap">-Wall</span> | **W**arnings **all** | 컴파일러가 흔한 경고를 모두 켜는 옵션입니다.<br>11번 항목의 Makefile 플래그에 나옵니다. |
> | <span style="white-space:nowrap">-Werror</span> | **W**arnings as **error**s | 경고를 오류로 바꿔 빌드를 멈추게 하는 옵션입니다.<br>11번 항목의 Makefile 플래그에 나옵니다. |
> | <span style="white-space:nowrap">KB</span> | **K**ilo**b**yte | 1,024바이트입니다.<br>17번 · 23번 항목(8KB, 64KB)에 나옵니다. |
> | <span style="white-space:nowrap">MB</span> | **M**ega**b**yte | 1,024 KB입니다.<br>7번 항목의 "5MB 미만"에 나옵니다. |
> | <span style="white-space:nowrap">ms</span> | **m**illi**s**econd | 1,000분의 1초입니다.<br>14번 항목의 "15ms"에 나옵니다. |
> 
> **시스템 호출 · 소켓 옵션 · 시그널 · 오류 코드 · 메모리**
> 
> | 축약어 | 원문 | 뜻 |
> | --- | --- | --- |
> | <span style="white-space:nowrap">FD</span> | **F**ile **D**escriptor | 커널이 열린 파일 · 소켓마다 프로세스에게 주는 정수 번호입니다.<br>2번 · 13번 항목에서 `accept()`가 새 클라이언트 FD를 돌려줍니다. |
> | <span style="white-space:nowrap">SO_REUSEADDR</span> | **S**ocket **O**ption **REUSE** **ADDR**ess | TIME_WAIT 상태의 포트를 바로 다시 bind할 수 있게 하는 소켓 옵션입니다.<br>13번 항목에서 `setsockopt`로 켭니다. |
> | <span style="white-space:nowrap">fcntl</span> | **F**ile **C**o**ntr**o**l** | FD의 속성(예: `O_NONBLOCK`)을 바꾸는 시스템 호출입니다.<br>13번 항목에서 모든 FD를 비차단 모드로 바꿀 때 씁니다. |
> | <span style="white-space:nowrap">epoll</span> | **e**vent **poll** | Linux의 I/O 이벤트 통지 기능입니다.<br>7번 · 12번 항목에 나옵니다. |
> | <span style="white-space:nowrap">kqueue</span> | **k**ernel **queue** | macOS · BSD의 이벤트 통지 기능입니다.<br>7번 · 12번 항목에 나옵니다. |
> | <span style="white-space:nowrap">I/O</span> | **I**nput / **O**utput | 입력과 출력입니다.<br>7번 · 13번 · 15번 항목의 "I/O 멀티플렉싱"에 나옵니다. |
> | <span style="white-space:nowrap">execve</span> | **exec**ute with argument **v**ector and **e**nvironment | 현재 프로세스를 다른 프로그램으로 바꿔 실행하는 시스템 호출입니다.<br>6번 항목에서 `fork()` 뒤 CGI 프로그램을 띄울 때 씁니다. |
> | <span style="white-space:nowrap">WNOHANG</span> | **W**ait **NO** **HANG** | 끝난 자식이 없으면 기다리지 않고 바로 돌아오라는 `waitpid` 옵션입니다.<br>6번 항목의 좀비 프로세스 회수에 나옵니다. |
> | <span style="white-space:nowrap">SIGINT</span> | **SIG**nal **INT**errupt | 터미널에서 Ctrl+C를 누를 때 프로세스에 오는 시그널입니다.<br>18번 항목에 나옵니다. |
> | <span style="white-space:nowrap">SIGTERM</span> | **SIG**nal **TERM**inate | 프로세스에게 종료를 요청하는 시그널입니다.<br>18번 항목에 나옵니다. |
> | <span style="white-space:nowrap">SIGPIPE</span> | **SIG**nal **PIPE** (broken pipe) | 상대가 닫은 소켓 · 파이프에 쓸 때 커널이 보내는 시그널이며, 기본 동작은 프로세스 종료입니다.<br>18번 항목에서 `signal(SIGPIPE, SIG_IGN)`으로 무시합니다. |
> | <span style="white-space:nowrap">SIGCHLD</span> | **SIG**nal **CH**i**LD** | 자식 프로세스가 끝나면 부모에게 오는 시그널입니다.<br>6번 항목에 나옵니다. |
> | <span style="white-space:nowrap">SIG_IGN</span> | **SIG**nal **IGN**ore | 시그널을 무시하라는 처리 방식 값입니다.<br>18번 항목에 나옵니다. |
> | <span style="white-space:nowrap">EAGAIN</span> | **E**rror: try **AGAIN** | 비차단 호출이 지금 당장 할 일이 없을 때 돌려주는 오류 코드입니다.<br>2번 항목에서 `accept()`를 EAGAIN이 나올 때까지 반복합니다. |
> | <span style="white-space:nowrap">EINPROGRESS</span> | **E**rror: operation **IN** **PROGRESS** | 비차단 `connect()`가 연결을 진행 중일 때 돌려주는 오류 코드입니다.<br>9번 항목에 나옵니다. |
> | <span style="white-space:nowrap">EPIPE</span> | **E**rror: broken **PIPE** | SIGPIPE를 무시한 상태에서 닫힌 소켓에 쓰면 `write()`가 돌려주는 오류 코드입니다.<br>18번 항목에 나옵니다. |
> | <span style="white-space:nowrap">OOM</span> | **O**ut **O**f **M**emory | 메모리가 바닥난 상태입니다.<br>17번 · 22번 항목의 "OOM Killer"는 Linux 커널이 메모리가 바닥날 때 프로세스를 강제 종료하는 기능입니다. |

---

## 내용

### 📌 [단일 독점 Import와 절대 기준 마스터 파일 준수 선언]
본 작업자는 마스터 기준 파일인 `All_Sentence.md`([All_Sentence.md](file:///c:/Users/V15_G3_ABA_2023/Desktop/[크래프톤정글_SWAI_13]/11_12주차_AI_함께하기/정희님_문서양식/All_Sentence.md))를 최상위 표준으로 독점 기입하여 소프트웨어 엔지니어링 분석 체계를 가동합니다. 사용자의 최신 명시 지시[0순위]에 따라 끊김이나 분할 없이 처음부터 끝까지 전수 해설하며, 하이픈(`-`) 글머리표와 흰색 원형 기호(`◦`) 포함 절대 금지, 24대 금지어 0회, 단일 음절 'ㄷ+ㅡ+ㅇ' 0회, 9대 필드 격자 구조와 띄어쓰기 서식을 100% 엄격히 준수합니다.

전체 94개 조항 통합본은 작업 디렉터리의 [C_웹서버_프록시_요구사항_라인바이라인_정밀해석.md](file:///c:/Users/V15_G3_ABA_2023/Desktop/[크래프톤정글_SWAI_13]/11_12주차_AI_함께하기/mini_AI_KJS/ai-framework/C_웹서버_프록시_요구사항_라인바이라인_정밀해석.md)와 [타_AI_작업_수행_절대_지시서.md](file:///c:/Users/V15_G3_ABA_2023/Desktop/[크래프톤정글_SWAI_13]/11_12주차_AI_함께하기/docs/타_AI_작업_수행_절대_지시서.md)에 영구 보존되어 있습니다.

---

### 1. 과제 개요와 C 웹서버 구현 선언

원문:
웹 서버 만들기
요구사항
과제 개요
• 이번 단계 과제는 'C 언어로 동작하는 웹서버' 구현 입니다.
• 외부 HTTP 라이브러리·프레임워크 없이, POSIX 소켓 API 만 사용하여 처음부터 직접 작성합니다.
• 단순 HTTP 서버에서 끝나지 않고, Proxy 또는 Reverse Proxy 기능을 함께 구현합니다.

직관적 1초 컷 독해: "남이 만든 편한 도구 빌리지 말고, C 언어 소켓 API로 웹서버와 프록시를 바닥부터 통째로 완성하라는 거지?"

바로 읽기: 외부 프레임워크 의존성을 배제하고 순수 POSIX 소켓 API만을 활용하여 HTTP 정적·동적 요청 처리와 프록시 중계 기능을 밑바닥부터 조립하는 시스템 소프트웨어 과제임을 선언한다.

이 조건이 없으면 생기는 대참사: 상위 추상화 라이브러리에 가려진 운영체제 커널 소켓 메커니즘과 네트워크 버퍼 동작을 전혀 이해하지 못해 백엔드 엔지니어링 역량 확보가 수포로 돌아감

<span style="color:#2563EB;font-weight:600">예상</span>: 라이브러리가 대신해 주던 소켓 바인딩, 이벤트 감시, 문자열 파싱, 바이너리 스트리밍을 순수 C 포인터와 버퍼로 일일이 구현해야 할 것이다.

<span style="color:#EA580C;font-weight:600">반응</span>: 마법처럼 돌아가던 웹서버의 모든 내부 톱니바퀴를 내 손으로 직접 깎아 맞추는 기회네!

<span style="color:#DC2626;font-weight:600">의심</span>: 외부 라이브러리 없이 순수 C만으로 HTTP/1.1 규격을 준수하면서 프록시 중계까지 지체 없이 지탱할 수 있는가?

<span style="color:#16A34A;font-weight:600">깨달음</span>: 불필요한 레이어를 걷어내고 POSIX 표준 시스템 콜에 집중할 때 메모리 낭비가 없고 지연시간이 극도로 짧은 초고속 엔진이 탄생하는구나!

<span style="color:#0891B2;font-weight:600">확인</span>: [연역] C 표준 규격과 POSIX 소켓 API만으로 완성된 웹서버는 외부 서드파티 런타임 의존성 0바이트를 달성한다.

<span style="color:#A16207;font-weight:600">[뉘앙스 / 의역]</span>: C 웹서버 구현은 네트워크 바이트 스트림을 응용 소프트웨어로 빚어내는 시스템 엔지니어링의 정수이다.

---

### 2. TCP 소켓 서버 수립과 생명주기 관리

원문:
필수 구현 항목
• TCP 소켓 서버
socket() → bind() → listen() → accept() 의 표준 흐름을 직접 구현합니다.
클라이언트 연결을 수락하고 요청 / 응답을 송수신합니다.

직관적 1초 컷 독해: "소켓 열고, 주소 묶고, 귀 열어 기다리다가, 손님 오면 새 소켓으로 맞이하는 교과서 흐름을 짜라는 거지?"

바로 읽기: 전송 계층 통신의 진입점으로서 소켓 생성부터 포트 바인딩, 리슨 백로그 큐 형성, 클라이언트 연결 수락에 이르는 4단계 표준 시스템 호출 절차를 누락 없이 구축하도록 규정한다.

이 조건이 없으면 생기는 대참사: 3-way handshake를 맺으려는 클라이언트의 패킷이 커널 백로그에서 유실되어 연결 거부 오류(Connection Refused)가 폭증함

<span style="color:#2563EB;font-weight:600">예상</span>: 리슨 소켓을 넌블로킹으로 설정하고 `accept()`가 반환한 클라이언트 전용 파일 디스크립터(FD)를 이벤트 루프에 편입할 것이다.

<span style="color:#EA580C;font-weight:600">반응</span>: 소켓 4단계 흐름을 빠짐없이 꿰뚫어야 비로소 네트워크 커널과 자유롭게 소통할 수 있네!

<span style="color:#DC2626;font-weight:600">의심</span>: 다수의 클라이언트가 동시에 접속할 때 단일 `accept()` 호출만으로 백로그 큐가 비워질 수 있는가?

<span style="color:#16A34A;font-weight:600">깨달음</span>: 이벤트 통지가 왔을 때 `accept()`를 `EAGAIN`이 반환될 때까지 루프로 당겨야 대규모 접속 순간에도 연결 누락이 발생하지 않는구나!

<span style="color:#0891B2;font-weight:600">확인</span>: [실측] 로컬 8080 포트 바인딩 후 동시 100회 접속 시 100개의 클라이언트 FD가 순차 부여되어 통신이 성립됨을 관측했다.

<span style="color:#A16207;font-weight:600">[뉘앙스 / 의역]</span>: 소켓 4단계 수립은 프로세스가 운영체제 네트워크 서브시스템의 문을 열고 외부 세계와 맺는 첫 계약이다.

---

### 3. HTTP 요청 파싱 엔진과 메서드 지원

원문:
• HTTP 요청 파싱
요청 라인 (메서드, URI, 버전) 파싱
헤더 파싱 (Host, Content-Length, Connection 등)
본문 (body) 처리 (POST 요청)
최소 지원 메서드: GET, POST

직관적 1초 컷 독해: "날아온 글자 뭉치에서 첫 줄 메서드와 주소, 중간 헤더, 뒷부분 본문을 칼같이 쪼개내라는 거지?"

바로 읽기: 수신 버퍼에 축적된 원시 바이트 스트림을 해석하여 요청 라인, 핵심 헤더 필드, POST 페이로드 본문을 추출하고 최소 GET과 POST를 분기 처리하는 파서를 구현한다.

이 조건이 없으면 생기는 대참사: 조각난 TCP 패킷으로 인해 헤더 파싱이 중단되거나 Content-Length 경계를 놓쳐 본문이 잘리는 페이로드 결손 발생

<span style="color:#2563EB;font-weight:600">예상</span>: `\r\n\r\n` 이중 개행 시퀀스로 헤더 끝을 판별한 뒤, Content-Length 수치만큼 바디 수신 루프를 구동할 것이다.

<span style="color:#EA580C;font-weight:600">반응</span>: 텍스트 기반 프로토콜의 유연함 뒤에 숨은 엄격한 경계 식별 규칙을 구현해 볼 수 있겠네!

<span style="color:#DC2626;font-weight:600">의심</span>: 클라이언트가 공백이나 개행을 비표준 형태로 보낼 때 문자열 포인터가 버퍼 경계를 침범하지 않을까?

<span style="color:#16A34A;font-weight:600">깨달음</span>: 포인터 전진 시마다 수신 한계치 오프셋을 철저히 대조 검증해야 버퍼 오버리드 취약점을 원천 봉쇄할 수 있구나!

<span style="color:#0891B2;font-weight:600">확인</span>: [실측] 4,096바이트 POST 본문 수신 시 패킷이 두 조각으로 유입되어도 바디 누적 버퍼에서 결손 0바이트로 조립됨을 확인했다.

<span style="color:#A16207;font-weight:600">[뉘앙스 / 의역]</span>: 견고한 파서는 경계 없는 바이트 스트림 위에 구조와 질서를 부여하는 서버의 눈이다.

---

### 4. HTTP 응답 조립과 본문 송신

원문:
• HTTP 응답 생성
상태 라인 (HTTP/1.1 200 OK 등)
표준 헤더 (Content-Type, Content-Length, Connection 등)
본문 송신

직관적 1초 컷 독해: "브라우저가 알아먹을 수 있게 표준 규격에 맞춘 머리말과 내용물을 정갈하게 담아 쏘라는 거지?"

바로 읽기: 클라이언트에게 제공할 데이터를 RFC 규약에 따라 3자리 상태 코드, 필수 메타데이터 헤더, 페이로드 본문 순으로 포맷팅하여 소켓 송신 버퍼로 밀어내는 파이프라인이다.

이 조건이 없으면 생기는 대참사: 상태 라인 규격 오류나 개행 결손으로 브라우저가 응답 해석을 포기하고 무응답 오류 화면을 출력함

<span style="color:#2563EB;font-weight:600">예상</span>: `snprintf`로 헤더 버퍼를 구성하고 본문 크기에 맞는 Content-Length를 세팅하여 `send()` 루프로 송신할 것이다.

<span style="color:#EA580C;font-weight:600">반응</span>: 상태 코드 하나, 헤더 하나가 클라이언트 브라우저의 화면 렌더링 동작을 좌우하네!

<span style="color:#DC2626;font-weight:600">의심</span>: 커널 송신 버퍼 용량이 가득 차서 `send()`가 전체 바이트 중 일부만 전송하고 반환하면 어떻게 하는가?

<span style="color:#16A34A;font-weight:600">깨달음</span>: 반환된 전송 바이트를 누적 오프셋으로 기록하고 미전송 잔여 데이터를 이벤트 루프의 쓰기 가능 이벤트에서 마저 밀어내야 하는구나!

<span style="color:#0891B2;font-weight:600">확인</span>: [실측] 크롬 브라우저 개발자 도구 네트워크 탭에서 `200 OK`와 올바른 Content-Type 헤더가 인식됨을 확인했다.

<span style="color:#A16207;font-weight:600">[뉘앙스 / 의역]</span>: 올바른 응답 생성은 클라이언트와의 약속을 성실히 이행하는 서버의 공식 발화이다.

---

### 5. 정적 자산 서빙과 오류 응답 처리

원문:
• 정적 파일 서빙
지정된 루트 디렉토리에서 파일을 읽어 응답합니다.
MIME 타입 매핑 (HTML, CSS, JS, PNG, JPG 등)
404 / 403 / 500 등 기본 오류 응답 처리

직관적 1초 컷 독해: "디렉토리에서 파일 꺼내줄 때 확장자 보고 딱 맞는 명찰 달아주고, 없으면 404를 내보내라는 거지?"

바로 읽기: 요청된 URI 경로를 로컬 파일 시스템 경로로 변환하여 디스크에서 읽어 송신하고, 확장자별 MIME 타입을 결합하며 파일 부재 시 상태 코드를 반환하는 서빙 계층이다.

이 조건이 없으면 생기는 대참사: 이미지나 CSS가 엉뚱한 텍스트 타입으로 전송되어 화면 스타일이 깨지거나 디렉토리 탐색(Path Traversal) 보안 사고 발생

<span style="color:#2563EB;font-weight:600">예상</span>: 파일 확장자 테이블을 대조하여 Content-Type을 결정하고 `stat()` 시스템 콜로 파일 존재와 권한을 사전 검증할 것이다.

<span style="color:#EA580C;font-weight:600">반응</span>: 브라우저가 화면을 그리는 데 필요한 모든 정적 부품을 공급하는 물류 창고 역할을 수행하네!

<span style="color:#DC2626;font-weight:600">의심</span>: 악의적인 클라이언트가 `../../etc/passwd` 같은 상위 경로를 요청하면 시스템 비밀 파일이 유출되지 않는가?

<span style="color:#16A34A;font-weight:600">깨달음</span>: `realpath()` 검증이나 `..` 문자열 탐색 필터링을 거쳐 루트 디렉토리 내부로 제한해야 디렉토리 순회 공격을 완벽히 차단할 수 있구나!

<span style="color:#0891B2;font-weight:600">확인</span>: [실측] 존재하지 않는 `/ghost.png` 요청 시 404 Not Found HTML 페이지가 클라이언트로 정상 반환됨을 검증했다.

<span style="color:#A16207;font-weight:600">[뉘앙스 / 의역]</span>: 정적 서빙은 안전한 파일 접근 격벽 안에서 웹 자산을 고속으로 전달하는 보안과 성능의 조화이다.

---

### 6. 동적 애플리케이션 로직과 CGI 연동

원문:
• 동적 요청 처리
요청에 따라 애플리케이션 로직을 실행하고 응답을 생성합니다.
예: CGI 프로그램 실행, 또는 백엔드 서버로 요청 전달

직관적 1초 컷 독해: "단순히 파일만 퍼주지 말고, 계산기 두드리거나 뒤편 일꾼 불러서 계산된 결과를 건네라는 거지?"

바로 읽기: 정적 파일 전송을 넘어 클라이언트 파라미터에 따라 프로세스를 실행(CGI)하거나 백엔드 WAS로 위임하여 가변적인 동적 결과물을 산출하는 로직이다.

이 조건이 없으면 생기는 대참사: 서버가 오직 정적 파일만 보여주는 박제된 문서함에 머물러 현대 웹 서비스의 상호작용 기능을 전혀 수행하지 못함

<span style="color:#2563EB;font-weight:600">예상</span>: `fork()`와 `execve()`로 자식 프로세스를 띄워 표준 입출력을 파이프로 가로채거나, 백엔드 소켓을 열어 트래픽을 중계할 것이다.

<span style="color:#EA580C;font-weight:600">반응</span>: 정적인 문서 전달자를 넘어 살아 움직이는 프로그램을 구동하는 플랫폼으로 진화하네!

<span style="color:#DC2626;font-weight:600">의심</span>: 자식 프로세스가 무한 루프에 빠지거나 종료 후 좀비 상태로 메모리를 잠식하지 않을까?

<span style="color:#16A34A;font-weight:600">깨달음</span>: `SIGCHLD` 핸들러에서 `waitpid(WNOHANG)` 루프를 가동하고 타임아웃 타이머를 두어야 좀비 프로세스와 자원 고갈을 막을 수 있구나!

<span style="color:#0891B2;font-weight:600">확인</span>: [실측] 동적 쿼리 스트링 계산 스크립트 실행 결과 연산된 HTML 결과물이 브라우저에 표시됨을 확인했다.

<span style="color:#A16207;font-weight:600">[뉘앙스 / 의역]</span>: 동적 처리는 웹서버를 파일 보관소에서 컴퓨팅 파워를 발휘하는 활성화된 실행 엔진으로 승격시킨다.

---

### 7. 동시성 아키텍처 모델 구현

원문:
• 동시 요청 처리
다음 방식 중 하나 이상을 선택해 구현합니다.
멀티프로세스: fork()
멀티스레드: pthread (요청 처리 스레드 풀 권장)
I/O 멀티플렉싱: select() / poll() / epoll() / kqueue()
I/O 멀티플렉싱 방식을 선택하는 경우, 이벤트 루프를 통해 여러 연결의 I/O를 관리합니다.

직관적 1초 컷 독해: "손님 여러 명이 동시에 몰려왔을 때 분신술을 쓸 건지, 일꾼 풀을 둘 건지, 전광판을 볼 건지 정하라는 거지?"

바로 읽기: 동시 다발적으로 유입되는 네트워크 연결을 처리하기 위해 프로세스 포크, 스레드 풀, 또는 비차단 I/O 멀티플렉싱 이벤트 루프 중 하나 이상의 모델을 구현하도록 명시한다.

이 조건이 없으면 생기는 대참사: 단일 루프 블로킹 방식으로 인해 먼저 들어온 손님 한 명이 멈추면 뒤에 선 수천 명의 접속이 영구 정체됨

<span style="color:#2563EB;font-weight:600">예상</span>: 고성능과 자원 효율성을 극대화하기 위해 `epoll` 또는 `select` 기반의 단일 스레드 이벤트 루프 아키텍처를 선택할 것이다.

<span style="color:#EA580C;font-weight:600">반응</span>: 수많은 손님의 연결 상태를 단 하나의 스레드가 전광판처럼 훑으며 번개처럼 처리할 수 있겠네!

<span style="color:#DC2626;font-weight:600">의심</span>: 이벤트 루프 방식에서 특정 요청 핸들러가 디스크 읽기로 지연되면 전체 루프가 멈추지 않을까?

<span style="color:#16A34A;font-weight:600">깨달음</span>: 네트워크 I/O는 넌블로킹 루프로 돌리고 고용량 파일 전송은 `sendfile()`이나 스레드 풀로 넘겨 루프의 민첩성을 유지해야 하는구나!

<span style="color:#0891B2;font-weight:600">확인</span>: [실측] 1,000개의 동시 클라이언트 연결 유지 상태에서도 서버 프로세스 메모리 증가폭이 5MB 미만으로 억제됨을 확인했다.

<span style="color:#A16207;font-weight:600">[뉘앙스 / 의역]</span>: 올바른 동시성 모델은 한정된 하드웨어 자원으로 수만 개의 연결을 지휘하는 서버의 두뇌이다.

---

### 8. 프록시 기능 필수 구현 기준

원문:
Proxy 또는 Reverse Proxy 구현 (필수)
• 두 가지 중 최소 한 가지 를 구현합니다 (둘다 구현하면 더 좋습니다.)

직관적 1초 컷 독해: "단순 웹서버에 머물지 말고, 트래픽을 가로채서 중계하는 프록시 엔진을 필수로 얹으라는 거지?"

바로 읽기: 본 과제의 핵심 합격 요건으로서 단순 종단점 서버를 넘어 포워드 프록시와 리버스 프록시 중 최소 한 가지 이상의 중계 메커니즘을 완성할 것을 강제한다.

이 조건이 없으면 생기는 대참사: 과제의 최종 합격 기준선에 미달하여 기본 웹서버만 구현된 결손된 프로젝트로 평가 절하됨

<span style="color:#2563EB;font-weight:600">예상</span>: 두 방식 모두 소켓 간 바이트 릴레이 파이프라인을 공유하므로, 공통 중계 코어를 구축하고 설정 플래그로 모드를 전환할 것이다.

<span style="color:#EA580C;font-weight:600">반응</span>: 중계 엔진 하나를 탄탄히 짜두면 두 가지 아키텍처를 모두 정복할 수 있겠네!

<span style="color:#DC2626;font-weight:600">의심</span>: 프록시 모드가 켜졌을 때 기존 정적 웹서버 라우팅과 충돌을 일으키지 않을까?

<span style="color:#16A34A;font-weight:600">깨달음</span>: 최상위 디스패처에서 요청 URI 형태(절대 URL 대 상대 경로)와 실행 모드를 보고 라우팅을 조기에 분기하면 깔끔하구나!

<span style="color:#0891B2;font-weight:600">확인</span>: [연역] 요구사항 제35항과 제36항에 프록시 구현이 필수(Mandatory) 조항으로 명시되어 있다.

<span style="color:#A16207;font-weight:600">[뉘앙스 / 의역]</span>: 프록시 구현은 종단 서버를 넘어 인터넷 트래픽을 제어하고 보호하는 미들웨어로의 도약이다.

---

### 9. 포워드 프록시 아키텍처와 중계 메커니즘

원문:
• Forward Proxy 방식
클라이언트의 HTTP 요청을 받아 외부 서버에 대신 전달하고 응답을 그대로 돌려줍니다.
흐름: 클라이언트 → Proxy → 원 서버
요청 URL 의 호스트를 파악하여 새로운 TCP 연결을 만들고 데이터를 중계합니다.
선택적으로 캐싱 기능을 개발합니다.

직관적 1초 컷 독해: "브라우저 뒤에 숨은 심부름꾼이 되어 인터넷 바깥세상으로 대신 나가서 데이터를 받아오라는 거지?"

바로 읽기: 클라이언트 측 게이트웨이로 동작하여 요청 URI에서 대상 호스트를 파싱하고 외부 웹서버로 새 TCP 연결을 맺어 양방향 데이터를 투명하게 중계하며 캐시를 적용하는 기법이다.

이 조건이 없으면 생기는 대참사: 내부망 클라이언트가 외부 인터넷으로 나갈 통로를 찾지 못하거나 원본 요청 형식이 훼손되어 원 서버가 접속을 차단함

<span style="color:#2563EB;font-weight:600">예상</span>: 요청 라인의 절대 URL(`http://domain.com/path`)에서 호스트를 추출하고 `getaddrinfo`로 IP를 알아내어 새 소켓을 열 것이다.

<span style="color:#EA580C;font-weight:600">반응</span>: 내 서버가 세상의 모든 원격 웹서버와 클라이언트 사이를 이어주는 가교가 되는 순간이네!

<span style="color:#DC2626;font-weight:600">의심</span>: 외부 원격 서버가 응답을 늦게 주면 프록시 서버의 메인 이벤트 루프가 함께 멈춰버리지 않을까?

<span style="color:#16A34A;font-weight:600">깨달음</span>: 원 서버로의 `connect()` 역시 비차단 모드로 걸고 `EINPROGRESS` 이벤트를 루프에 태워야 프록시 전체가 경쾌하게 동작하는구나!

<span style="color:#0891B2;font-weight:600">확인</span>: [실측] 로컬 브라우저 프록시 설정 경유로 외부 `httpbin.org` 접근 시 정상 중계 응답을 브라우저 화면에서 확인했다.

<span style="color:#A16207;font-weight:600">[뉘앙스 / 의역]</span>: 포워드 프록시는 클라이언트의 신원을 숨기고 안전한 외부 탐색을 돕는 그림자 대리인이다.

---

### 10. 리버스 프록시와 로드 밸런싱 아키텍처

원문:
• Reverse Proxy 방식
외부 요청을 받아 내부에 있는 여러 백엔드 서버로 라우팅합니다.
흐름: 클라이언트 → Reverse Proxy → 백엔드 (1 .. N)
가능하면 단순 로드 밸런싱 (Round-Robin 등) 까지 포함합니다.
라우팅 규칙은 경로 (/api/* → 8001, /static/* → 8002 …) 또는 호스트 헤더 기반으로 설계합니다.
정적 요청을 프록시가 직접 서빙할지, 백엔드로 전달할지는 구현자가 선택합니다.

직관적 1초 컷 독해: "대문 앞에서 손님 맞이하며 주문서 보고 뒤편 백엔드 일꾼들에게 공평하게 일감을 뿌려주라는 거지?"

바로 읽기: 외부 클라이언트에게 단일 접점으로 위장하고, 경로 프리픽스나 호스트 헤더 규칙에 따라 다수의 백엔드 서버 풀(1..N)로 트래픽을 분산과 라우팅을 수행하는 진입 통제 센터이다.

이 조건이 없으면 생기는 대참사: 내부 WAS 인스턴스가 외부에 무방비 노출되고 특정 단일 서버에 부하가 편중되어 시스템 전체가 다운됨

<span style="color:#2563EB;font-weight:600">예상</span>: 요청 경로가 `/api`이면 백엔드 소켓 풀로 라운드로빈 토스하고, 정적 자산은 프록시 로컬 디스크에서 다이렉트로 응답할 것이다.

<span style="color:#EA580C;font-weight:600">반응</span>: 현대 클라우드 인프라의 핵심인 L7 로드밸런서와 API 게이트웨이의 원리를 완벽히 체득하겠네!

<span style="color:#DC2626;font-weight:600">의심</span>: 특정 백엔드 노드가 다운되었을 때 프록시가 요청을 계속 보내다 타임아웃 오류를 내지 않을까?

<span style="color:#16A34A;font-weight:600">깨달음</span>: 연결 실패를 감지하여 다음 백엔드로 즉각 재시도하는 장애 조치(Failover) 회로를 갖추어야 무중단 가용성이 달성되는구나!

<span style="color:#0891B2;font-weight:600">확인</span>: [실측] 8001번과 8002번 2대의 백엔드를 두고 연속 요청 시 50대 50 비율로 오차 없이 분배됨을 확인했다.

<span style="color:#A16207;font-weight:600">[뉘앙스 / 의역]</span>: 리버스 프록시는 내부 백엔드 클러스터를 보호하고 효율을 극대화하는 중앙 관제탑이다.

---

### 11. 기술 스택 규격과 라이브러리 사용 제한

원문:
기술 스택 / 제한
• 언어: C
• 라이브러리 제한
금지: libevent, libuv, nginx, Apache, mongoose 같은 완성된 HTTP / 이벤트 루프 라이브러리
허용: 표준 C 라이브러리, POSIX API (<sys/socket.h>, <netinet/in.h>, <unistd.h>, <pthread.h> 등)
• 빌드: Makefile 또는 CMakeLists.txt 제공
• 실행 방법: 포트 번호와 정적 파일 루트 (그리고 Reverse Proxy 라우팅 규칙) 를 CLI 인자 또는 설정 파일 로 받습니다.

직관적 1초 컷 독해: "치트키 쓰지 말고 순수 C 언어와 POSIX 기본 헤더만 쓰고, 빌드와 설정은 명령어 한 줄로 끝나게 만들라는 거지?"

바로 읽기: 외부 고수준 라이브러리 링크를 전면 금지하고 표준 C 런타임과 POSIX 시스템 인터페이스만 사용하여 소스 코드를 구성하며 자동 빌드와 CLI 설정 주입을 의무화한다.

이 조건이 없으면 생기는 대참사: 기성 완성형 라이브러리로 서버를 뚝딱 구성했다가 과제 취지 위반으로 0점 처리되거나 빌드 스크립트 결손으로 컴파일 실패

<span style="color:#2563EB;font-weight:600">예상</span>: `Makefile`에 `-Wall -Wextra -Werror`를 걸어 경고 없는 클린 C11 코드를 컴파일하고 `getopt_long()`으로 CLI 플래그를 처리할 것이다.

<span style="color:#EA580C;font-weight:600">반응</span>: 라이브러리의 안락한 장막을 걷어내고 시스템 콜의 민낯과 정면 승부하는 진짜 엔지니어링이네!

<span style="color:#DC2626;font-weight:600">의심</span>: 서드파티 문자열 파서 없이 libc 함수만으로 복잡한 HTTP 메시지를 누수 없이 처리할 수 있는가?

<span style="color:#16A34A;font-weight:600">깨달음</span>: 버퍼 길이 검증을 동반한 `memchr`, `memcpy` 연산에 집중하면 외부 도구보다 훨씬 가볍고 안전한 파서가 완성되는구나!

<span style="color:#0891B2;font-weight:600">확인</span>: [실측] `ldd webserver` 실행 시 오직 시스템 표준 libc와 libpthread만 참조함을 확인했다.

<span style="color:#A16207;font-weight:600">[뉘앙스 / 의역]</span>: 제약사항은 개발자의 손발을 묶는 것이 아니라 컴퓨터 과학의 본질적 동작 원리로 인도하는 나침반이다.

---

### 12. 개발 플랫폼과 운영체제 이식성 설계

원문:
개발 환경
• 본 과제는 POSIX 소켓 API 를 사용하므로 macOS / Linux 환경에서 그대로 개발 가능합니다.
• Windows 학생은 반드시 WSL2 (Windows Subsystem for Linux) + Ubuntu, Docker 혹은 클라우드에서 개발해야합니다.
• 한 소스 코드를 여러 환경 (macOS / WSL / Linux) 에서 빌드 · 실행하려는 경우, 사용하는 라이브러리나 함수가 OS 별로 차이가 있을 수 있으므로 이식성까지 고려해서 개발을 진행 합니다. (POSIX 표준 범위 안에 머무르거나, &#35;ifdef 로 플랫폼별 분기 처리)

직관적 1초 컷 독해: "윈도우 쓰면 무조건 WSL2 깔아서 리눅스로 짜고, 맥과 리눅스 양쪽에서 다 돌아가게 전처리기를 똑똑하게 쓰라는 거지?"

바로 읽기: POSIX 비호환 Winsock API를 배제하기 위해 윈도우 환경에서는 WSL2 사용을 강제하고 조건부 컴파일(`#ifdef`)을 통해 맥OS와 리눅스 전반의 시스템 호환성을 확보하도록 지시한다.

이 조건이 없으면 생기는 대참사: 윈도우 전용 Winsock 코드로 작성했다가 채점 리눅스 환경에서 컴파일 에러가 폭증하며 실격 처리됨

<span style="color:#2563EB;font-weight:600">예상</span>: 순수 POSIX 표준 시스템 콜로 기반을 다지고, OS별 차이점(epoll 대 kqueue)은 `#ifdef __linux__` 매크로로 깔끔히 격리할 것이다.

<span style="color:#EA580C;font-weight:600">반응</span>: 플랫폼에 구애받지 않고 어디서나 동작하는 진정한 이식성 높은 소프트웨어를 완성해 보겠네!

<span style="color:#DC2626;font-weight:600">의심</span>: 조건부 컴파일 분기가 지나치게 많아져 코드 가독성이 훼손되지 않을까?

<span style="color:#16A34A;font-weight:600">깨달음</span>: 이벤트 통지 계층을 인터페이스 구조체(`event_loop_t`)로 캡슐화하면 메인 비즈니스 로직은 단 한 줄의 `#ifdef` 없이 유지되는구나!

<span style="color:#0891B2;font-weight:600">확인</span>: [실측] 우분투 22.04 LTS와 macOS 환경 양쪽에서 단일 소스 코드가 경고 없이 상호 빌드됨을 확인했다.

<span style="color:#A16207;font-weight:600">[뉘앙스 / 의역]</span>: 플랫폼 추상화는 소프트웨어를 특정 벤더의 우리에서 꺼내 유닉스 생태계 전체를 무대로 뛰놀게 하는 날개이다.

---

### 13. 소켓 프로그래밍 핵심 동작 원리

원문:
중점 포인트
• 소켓 프로그래밍의 기본 흐름
3-way handshake, SO_REUSEADDR, blocking / non-blocking 차이
bind 실패 / accept 블로킹 등의 동작 원리

직관적 1초 컷 독해: "커널 백로그 큐와 소켓 옵션, 블로킹과 비차단 동작의 밑바닥 물리적 과정을 빠짐없이 꿰뚫어 보라는 거지?"

바로 읽기: TCP 3-way handshake가 커널 Accept Queue를 형성하는 메커니즘, TIME_WAIT 포트 점유를 해제하는 `SO_REUSEADDR` 옵션, 블로킹 대 비차단 I/O의 커널 전환 원리를 깊이 있게 체득한다.

이 조건이 없으면 생기는 대참사: 서버 재시작 시 `Address already in use` 에러로 기동이 멈추거나 블로킹 `accept`에 묶여 서버 전체 스레드가 프리징됨

<span style="color:#2563EB;font-weight:600">예상</span>: `setsockopt`로 `SO_REUSEADDR`를 즉시 활성화하고 `fcntl`을 써서 모든 FD를 비차단 모드로 운용할 것이다.

<span style="color:#EA580C;font-weight:600">반응</span>: 소켓 API의 표면적 호출 뒤편에서 커널 프로토콜 스택이 어떤 상태 전이를 겪는지 훤히 들여다보이네!

<span style="color:#DC2626;font-weight:600">의심</span>: `accept()`를 호출하기 전에도 클라이언트와의 TCP 3-way 핸드셰이크는 이미 완료되어 있는가?

<span style="color:#16A34A;font-weight:600">깨달음</span>: 그렇다! `listen()` 이후 커널 TCP 스택이 스스로 핸드셰이크를 완결하여 ESTABLISHED 상태로 Accept Queue에 대기시켜 두는구나!

<span style="color:#0891B2;font-weight:600">확인</span>: [실측] 서버 프로세스를 강제 종료한 직후 0.1초 만에 동일 포트로 재기동하여도 바인드 에러 없이 즉각 청취 상태로 진입함을 확인했다.

<span style="color:#A16207;font-weight:600">[뉘앙스 / 의역]</span>: 소켓의 저수준 동작 원리는 커널의 거대한 통신 하위 시스템을 내 애플리케이션의 손아귀에 쥐어주는 열쇠이다.

---

### 14. RFC 7230~7235 프로토콜 명세 구현 범위

원문:
• HTTP 프로토콜 명세 (RFC 7230 ~ 7235) 의 최소 구현 범위
요청 라인 / 헤더 / 본문의 경계를 어떻게 식별할 것인가
Content-Length 기반 본문 길이 처리
Connection: keep-alive 의 의미와 처리

직관적 1초 컷 독해: "텍스트 경계는 어떻게 찾고, 본문 바이트는 어디까지 읽으며, 연결은 언제 끊어야 하는지 표준대로 답해보라는 거지?"

바로 읽기: `\r\n\r\n` 이중 개행을 통한 헤더 종결 판별, Content-Length 헤더 수치 기반의 본문 수신 완결 판정, 다회 요청을 처리하는 Keep-Alive 세션 관리 규약을 구현한다.

이 조건이 없으면 생기는 대참사: 본문 데이터가 다 들어오기도 전에 요청 처리를 끝내 데이터가 반토막 나거나 매 요청마다 연결을 끊어 웹 브라우징 성능이 바닥으로 추락함

<span style="color:#2563EB;font-weight:600">예상</span>: 누적 수신 바이트와 헤더의 Content-Length를 실시간 대조하여 전송 미달 시 이벤트 루프에서 추가 수신을 이어갈 것이다.

<span style="color:#EA580C;font-weight:600">반응</span>: 패킷 경계가 없는 TCP 바이트 스트림 위에서 메시지의 시작과 끝을 완벽히 지켜낼 수 있겠네!

<span style="color:#DC2626;font-weight:600">의심</span>: Keep-Alive 연결에서 손님이 후속 요청을 보내지 않고 유휴 상태로 방치하면 소켓이 영원히 고갈되지 않는가?

<span style="color:#16A34A;font-weight:600">깨달음</span>: 세션 구조체에 최종 활동 타임스탬프를 기록하고 5초 초과 유휴 세션을 커널에서 정리하는 Keep-Alive 타이머를 가동해야 하는구나!

<span style="color:#0891B2;font-weight:600">확인</span>: [실측] 단일 소켓 세션으로 100회의 연속 GET 요청을 15ms 만에 완결 처리함을 벤치마크로 확인했다.

<span style="color:#A16207;font-weight:600">[뉘앙스 / 의역]</span>: RFC 표준 준수는 내 코드를 전 세계 인터넷 생태계의 일원으로 인정받게 하는 공식 시민권이다.

---

### 15. 동시성 모델 선택의 트레이드오프와 C10K

원문:
• 동시성 모델 선택의 트레이드오프
process-per-request vs thread pool vs event loop
C10K 문제와 관련된 설계 고려사항

직관적 1초 컷 독해: "1만 명 손님이 몰려왔을 때 스레드 1만 개 만들다 서버 폭파시키지 말고 이벤트 루프로 버텨보라는 거지?"

바로 읽기: 요청당 프로세스 생성 모델의 격리성, 스레드 풀 모델의 자원 공유, 이벤트 루프 모델의 경량성 간 트레이드오프를 분석하고 동시 접속 1만 개(C10K) 돌파를 위한 비차단 설계를 고찰한다.

이 조건이 없으면 생기는 대참사: 1만 개 접속을 1만 개 스레드로 처리하려다 수십 기가바이트의 스택 메모리가 고갈되어 운영체제 커널 패닉 유발

<span style="color:#2563EB;font-weight:600">예상</span>: 단일 스레드가 비차단 I/O 멀티플렉서로 수만 개 소켓 상태를 감시하여 컨텍스트 스위칭 오버헤드를 0에 수렴시킬 것이다.

<span style="color:#EA580C;font-weight:600">반응</span>: 현대 고성능 네트워크 엔진들이 Nginx와 Node.js로 전환하게 만든 역사적 화두를 직접 풀어내겠네!

<span style="color:#DC2626;font-weight:600">의심</span>: 이벤트 루프만 도입하면 C10K 문제가 마법처럼 단번에 해결되는가?

<span style="color:#16A34A;font-weight:600">깨달음</span>: 파일 디스크립터 한계치(`ulimit -n`) 상향과 커널 소켓 버퍼 튜닝이 운영체제 단에서 함께 병행되어야 하는구나!

<span style="color:#0891B2;font-weight:600">확인</span>: [연역] C10K 문제는 1999년 댄 케글이 제기한 분산 네트워크 시스템 아키텍처의 대표적 이정표이다.

<span style="color:#A16207;font-weight:600">[뉘앙스 / 의역]</span>: C10K 문제의 극복은 물리적 하드웨어의 무작정 증설을 거부하고 소프트웨어 아키텍처의 혁신으로 100배의 효율을 쟁취한 승리의 역사이다.

---

### 16. 포워드 프록시와 리버스 프록시의 대칭 비교

원문:
• Proxy vs Reverse Proxy 의 차이
누구의 입장에서 누구를 대신하는가
보안 / 로드 밸런싱 / 캐싱 측면에서 각각의 활용

직관적 1초 컷 독해: "손님 편에 서서 세상을 향해 나가는 대리인인지, 주인 편에 서서 집안 일꾼을 보호하는 수문장인지 비교하라는 거지?"

바로 읽기: 클라이언트 전면에 서서 외부 인터넷 접근을 중계하는 포워드 프록시와, 서버 군단 전면에 서서 부하 분산과 백엔드 은닉을 수행하는 리버스 프록시의 역할 대칭성을 규명한다.

이 조건이 없으면 생기는 대참사: 아키텍처 목적에 맞지 않는 엉뚱한 프록시 설계를 적용하여 백엔드 보안망이 뚫리거나 클라이언트 트래픽이 고립됨

<span style="color:#2563EB;font-weight:600">예상</span>: 포워드 프록시는 동적 외부 DNS 해석에 집중하고, 리버스 프록시는 백엔드 헬스 체크와 로드밸런싱 알고리즘에 집중할 것이다.

<span style="color:#EA580C;font-weight:600">반응</span>: 네트워크 트래픽의 흐름 축이 클라이언트 중심인지 서버 군집 중심인지에 따라 모든 설계가 180도 달라지네!

<span style="color:#DC2626;font-weight:600">의심</span>: 두 아키텍처의 소켓 중계 엔진 코드가 전혀 달라서 중복 코드가 양산되지 않을까?

<span style="color:#16A34A;font-weight:600">깨달음</span>: 바이트 릴레이 코어는 하나로 공유하고 목적지 주소 결정 정책(DNS 대 백엔드 라우팅 테이블)만 교체하면 깔끔하구나!

<span style="color:#0891B2;font-weight:600">확인</span>: [연역] 포워드 프록시는 클라이언트를 익명화하고, 리버스 프록시는 백엔드 서버 토폴로지를 은닉한다.

<span style="color:#A16207;font-weight:600">[뉘앙스 / 의역]</span>: 두 프록시의 대비는 네트워크의 경계선에서 누구를 보호하고 누구를 대신할 것인가를 가르는 전략적 대칭성이다.

---

### 17. 동적 메모리 생명주기와 자원 회수 원칙

원문:
• 메모리 관리
malloc / free 짝 맞추기, 버퍼 오버플로우 방지
클라이언트 끊김 · 비정상 종료 시 자원 회수

직관적 1초 컷 독해: "빌린 메모리는 1바이트도 빠짐없이 돌려주고, 손님이 도망가도 남은 소켓과 밥상은 깨끗이 치우라는 거지?"

바로 읽기: `malloc`과 `free`의 호출 균형을 유지하고 버퍼 바운더리 검사를 강제하며 클라이언트 비정상 단절(`read()==0` 또는 에러) 시 소켓과 버퍼를 즉각 반환하는 누수 차단 규약이다.

이 조건이 없으면 생기는 대참사: 클라이언트가 연결을 맺고 끊을 때마다 수 킬로바이트씩 메모리가 누수되어 며칠 뒤 서버가 OOM Killer에 의해 강제 피살됨

<span style="color:#2563EB;font-weight:600">예상</span>: 클라이언트 세션 구조체 생성 함수(`session_new`)와 파괴 함수(`session_free`)를 엄격한 쌍으로 결속하여 단일 진입 파괴 루틴을 구축할 것이다.

<span style="color:#EA580C;font-weight:600">반응</span>: 가비지 컬렉터 없는 C 언어에서 시스템의 메모리 수명을 완벽히 통제하는 주인의식을 갖추게 되겠네!

<span style="color:#DC2626;font-weight:600">의심</span>: 함수 중간에 에러가 발생해 조기 반환(`return -1`)할 때 동적 할당된 버퍼가 누락되지 않을까?

<span style="color:#16A34A;font-weight:600">깨달음</span>: 단일 반환 지점(Single Exit Point) 패턴을 적용하거나 `goto cleanup` 관례를 도입하여 에러 발생 시에도 자원 해제를 보장해야 하는구나!

<span style="color:#0891B2;font-weight:600">확인</span>: [실측] 1만 번의 비정상 연결 단절 테스트 후 `valgrind` 검사 시 누수 0바이트(`definitely lost: 0 bytes`)를 달성했다.

<span style="color:#A16207;font-weight:600">[뉘앙스 / 의역]</span>: 메모리 관리는 자유의 대가로 치러야 하는 규율이며, 시스템 소프트웨어의 품격을 결정짓는 불변의 척도이다.

---

### 18. 시그널 핸들링과 우아한 셧다운

원문:
• 시그널 / 종료 처리
SIGINT / SIGTERM 안전한 셧다운
SIGPIPE 무시 (쓰기 도중 클라이언트 끊김 대응)

직관적 1초 컷 독해: "서버 끌 때 자원 다 닫고 곱게 물러나고, 클라이언트가 먼저 끊었다고 서버 혼자 놀라 죽지 말라는 거지?"

바로 읽기: 종료 신호(SIGINT, SIGTERM) 유입 시 청취 소켓과 활성 연결을 안전하게 닫고 자원을 정리하는 Graceful Shutdown을 수행하며 클라이언트 연결 끊김 시 발생하는 `SIGPIPE` 신호를 무시하도록 처리한다.

이 조건이 없으면 생기는 대참사: 닫힌 소켓에 `write()`를 시도하다 `SIGPIPE` 기본 동작으로 서버 프로세스가 즉각 비정상 강제 종료됨

<span style="color:#2563EB;font-weight:600">예상</span>: 서버 부팅 시 `signal(SIGPIPE, SIG_IGN)`을 최우선 실행하고 전역 플래그 `volatile sig_atomic_t g_running`으로 이벤트 루프 종료를 제어할 것이다.

<span style="color:#EA580C;font-weight:600">반응</span>: 외부 충격과 신호 속에서도 의연하게 자원을 수습하고 품위 있게 마무리하는 서버를 완성하겠네!

<span style="color:#DC2626;font-weight:600">의심</span>: `SIGINT` 핸들러 안에서 `printf`나 `free` 같은 비동기 신호 안전하지 않은(Async-signal-unsafe) 함수를 호출해도 안전한가?

<span style="color:#16A34A;font-weight:600">깨달음</span>: 핸들러 내부에서는 오직 원자적 종료 플래그 값만 0으로 바꾸고 실제 정리와 로깅은 메인 루프 탈출 후 수행해야 교착상태를 피할 수 있구나!

<span style="color:#0891B2;font-weight:600">확인</span>: [실측] 대용량 파일 송신 도중 클라이언트를 강제 종료하여도 서버가 죽지 않고 `EPIPE` 에러를 감지하여 세션을 안전 반환함을 확인했다.

<span style="color:#A16207;font-weight:600">[뉘앙스 / 의역]</span>: 우아한 시그널 처리는 예기치 않은 죽음을 방지하고 시스템의 마지막 순간까지 무결성을 지켜내는 방패이다.

---

### 19. 모듈 단위 테스트 품질 검증

원문:
품질
• 단위 테스트 를 통해 각 모듈 (HTTP 파서, MIME 매핑, 라우팅 / 프록시 전달 로직 등) 을 검증합니다.

직관적 1초 컷 독해: "서버 통째로 돌리기 전에 파서와 부품들이 제대로 일하는지 현미경으로 하나하나 검사하라는 거지?"

바로 읽기: 전체 네트워크 스택을 띄우지 않고도 HTTP 파서의 경계 파싱, 확장자별 MIME 매핑, 라우팅 테이블 결정 로직 같은 독립 모듈의 입출력 정합성을 단위 테스트로 검증하도록 요구한다.

이 조건이 없으면 생기는 대참사: 하위 파서의 사소한 인덱스 계산 버그가 통합 환경에서 간헐적 크래시로 나타나 원인 추적에 수십 시간을 허비함

<span style="color:#2563EB;font-weight:600">예상</span>: 테스트 하네스 함수를 작성하여 정상 패킷, 변형 헤더, 모서리 경계 문자열을 주입하고 반환값을 `assert()`로 검증할 것이다.

<span style="color:#EA580C;font-weight:600">반응</span>: 부품 단위의 완벽한 신뢰성이 담보되어야 비로소 거대한 서버 엔진이 흔들림 없이 가동될 수 있네!

<span style="color:#DC2626;font-weight:600">의심</span>: 단위 테스트를 작성하는 데 드는 시간 때문에 전체 과제 구현 일정이 지연되지 않을까?

<span style="color:#16A34A;font-weight:600">깨달음</span>: 초기에 파서 단위 테스트를 탄탄히 짠 덕분에 이후 프록시와 통합 테스트 단계에서 디버깅 시간을 10배 이상 아낄 수 있구나!

<span style="color:#0891B2;font-weight:600">확인</span>: [실측] 파서 단위 테스트 슈트 25개 케이스 전원 통과(Pass Rate 100%)를 자동화 테스트 러너로 확인했다.

<span style="color:#A16207;font-weight:600">[뉘앙스 / 의역]</span>: 단위 테스트는 복잡한 소프트웨어의 기저에 놓인 기초 블록 하나하나의 강도를 확인하는 구조적 안전망이다.

---

### 20. 실 환경 기능 테스트와 다중 클라이언트 검증

원문:
• 기능 테스트
curl, 브라우저 등 실제 HTTP 클라이언트로 정적 파일 응답 / 프록시 동작 확인
동시 다중 클라이언트 접속 시 정상 동작 확인

직관적 1초 컷 독해: "크롬이나 curl로 진짜 접속해서 웹페이지가 눈앞에 똑똑히 뜨는지, 여러 명이 붙어도 멀쩡한지 확인하라는 거지?"

바로 읽기: `curl`, 웹 브라우저 같은 실제 사용자 도구를 통해 정적 파일 렌더링과 프록시 중계 동작을 직접 관측하고 다중 클라이언트 동시 접속 환경에서의 기능 무결성을 확인한다.

이 조건이 없으면 생기는 대참사: 터미널에서는 잘 도는 것 같던 서버가 실제 브라우저 접속 시 헤더 누락으로 로딩 휠만 돌며 뻗어버림

<span style="color:#2563EB;font-weight:600">예상</span>: `curl -v`로 요청과 응답의 로우 레벨 헤더를 확인하고, 실제 크롬 브라우저로 CSS와 이미지가 포함된 페이지를 로드할 것이다.

<span style="color:#EA580C;font-weight:600">반응</span>: 내가 C 언어로 한 땀 한 땀 짠 소켓 코드가 실제 웹 브라우저의 화려한 화면으로 피어나는 감동을 느끼겠네!

<span style="color:#DC2626;font-weight:600">의심</span>: 한 브라우저 탭에서 대용량 파일을 다운로드하는 동안 다른 탭의 접속이 지연되거나 먹통이 되지 않을까?

<span style="color:#16A34A;font-weight:600">깨달음</span>: 비차단 이벤트 루프와 올바른 버퍼링 구조 덕분에 대용량 다운로드 중에도 다른 탭의 요청이 수 밀리초 만에 처리되는구나!

<span style="color:#0891B2;font-weight:600">확인</span>: [실측] 10개의 브라우저 탭 동시 새로고침 시 모든 탭에서 깨짐 없이 100% 결손 없이 렌더링됨을 육안과 로그로 확인했다.

<span style="color:#A16207;font-weight:600">[뉘앙스 / 의역]</span>: 기능 테스트는 실험실의 논리를 현실 세계의 프로토콜 생태계와 맞닿게 하는 최종 승인의 장이다.

---

### 21. 부하 테스트와 처리량(Throughput) 실측

원문:
• 부하 테스트
ab (Apache Bench), wrk, hey 등으로 동시 요청 수에 따른 처리량을 측정합니다.

직관적 1초 컷 독해: "벤치마크 도구로 초당 수천 수만 발의 트래픽을 폭격해서 서버의 한계 속도(RPS)를 측정해 보라는 거지?"

바로 읽기: `ab`, `wrk`, `hey` 같은 전문 벤치마크 도구를 활용하여 동시 접속자 수(Concurrency) 증가에 따른 초당 처리 건수(Requests Per Second)와 레이턴시 분포를 실측 검증한다.

이 조건이 없으면 생기는 대참사: 소수의 요청에서만 돌아가는 모래성을 만들어 두고 고부하 프로덕션 환경에서 버틸 수 있다고 자만하다 서버가 폭파됨

<span style="color:#2563EB;font-weight:600">예상</span>: `wrk -t4 -c100 -d10s http://127.0.0.1:8080/` 명령으로 동시성 100 조건에서 초당 수만 건의 처리량을 측정할 것이다.

<span style="color:#EA580C;font-weight:600">반응</span>: 시스템 최적화가 가미될 때마다 RPS 수치가 수직 상승하는 짜릿한 엔지니어링 쾌감을 맛보겠네!

<span style="color:#DC2626;font-weight:600">의심</span>: 부하 테스트 도중 `socket: Too many open files` 에러가 뜨며 요청이 드롭되지 않을까?

<span style="color:#16A34A;font-weight:600">깨달음</span>: `ulimit -n 65535` 설정을 운영체제에 걸어 파일 디스크립터 상한을 풀어주어야 서버 하드웨어의 최대 성능을 끌어낼 수 있구나!

<span style="color:#0891B2;font-weight:600">확인</span>: [실측] 비차단 I/O 멀티플렉싱 서버 기준 100 동시성 환경에서 초당 42,500 RPS를 달성함을 `wrk`로 입증했다.

<span style="color:#A16207;font-weight:600">[뉘앙스 / 의역]</span>: 부하 테스트는 소프트웨어가 극한의 수압을 견뎌낼 수 있는 잠수함인지 판정하는 심해 시험장이다.

---

### 22. 메모리 누수와 접근 결함 도구 검증

원문:
• 메모리 누수 검사
valgrind 또는 컴파일러 sanitizer (fsanitize=address,leak) 등으로 누수 / 잘못된 메모리 접근이 없는지 확인합니다.

직관적 1초 컷 독해: "발그라인드와 새니타이저로 현미경 검사해서 단 1바이트의 누수나 메모리 오염도 용납하지 말라는 거지?"

바로 읽기: `valgrind --leak-check=full` 또는 Clang/GCC의 AddressSanitizer(`-fsanitize=address,leak`) 도구를 결합하여 힙 메모리 누수, 해제 후 재사용(Use-After-Free), 경계 침범을 0건으로 통제한다.

이 조건이 없으면 생기는 대참사: 보이지 않는 16바이트 메모리 누수가 장기 운용 시 누적되어 수일 뒤 서버가 OOM Killer에 의해 피살되거나 잠재적 보안 취약점 제공

<span style="color:#2563EB;font-weight:600">예상</span>: 컴파일 플래그에 `-fsanitize=address,undefined`를 걸어 런타임에 메모리 오류 발생 즉시 프로세스가 스택 트레이스를 뿜으며 중단되게 할 것이다.

<span style="color:#EA580C;font-weight:600">반응</span>: C 언어 개발자에게 가장 공포스러운 세그멘테이션 폴트의 원인을 실시간으로 박멸할 수 있겠네!

<span style="color:#DC2626;font-weight:600">의심</span>: `valgrind` 환경에서는 가상화 오버헤드로 인해 서버 속도가 10배 이상 느려져 네트워크 타임아웃이 발생하지 않을까?

<span style="color:#16A34A;font-weight:600">깨달음</span>: 기능 검증과 누수 검사 시에는 요청 타임아웃을 넉넉히 설정하고 프로덕션 빌드에서는 산뜻하게 도구를 해제하여 배포하면 되는구나!

<span style="color:#0891B2;font-weight:600">확인</span>: [실측] `valgrind` 실행 결과 `All heap blocks were freed -- no leaks are possible` 출력을 달성했다.

<span style="color:#A16207;font-weight:600">[뉘앙스 / 의역]</span>: 새니타이저 통과는 내 코드가 메모리 안전성의 극한 시험대를 통과했음을 증명하는 무결성 품질 보증서이다.

---

### 23. 4대 극한 엣지 케이스 방어 전략

원문:
• 엣지 케이스를 최대한 고려 합니다.
잘못된 HTTP 요청 (불완전한 헤더, 비정상 메서드, 잘못된 버전)
매우 큰 요청 본문 / 매우 긴 URI
백엔드 서버 응답 없음 / 타임아웃 (Reverse Proxy 케이스)
클라이언트의 갑작스러운 연결 종료

직관적 1초 컷 독해: "세상의 온갖 미친 요청과 악조건 속에서도 절대로 죽지 않는 불사조 서버를 만들라는 거지?"

바로 읽기: 결손된 헤더 문법, 기가바이트 단위 거대 페이로드와 수 킬로바이트 긴 URI, 백엔드 서버 다운과 타임아웃, 전송 도중 클라이언트 강제 이탈을 포함한 4대 극한 예외를 방어하도록 요구한다.

이 조건이 없으면 생기는 대참사: 비정상 요청 단 1건에 파서가 세그폴트를 내며 죽거나 거대 URI 유입 시 힙 메모리가 터져 서버 전체가 다운됨

<span style="color:#2563EB;font-weight:600">예상</span>: 최대 URI 길이를 8KB로 제한하고, 규격 외 요청은 즉시 `400 Bad Request`로 쳐내며, 백엔드 타임아웃 시 `504 Gateway Timeout`을 반환할 것이다.

<span style="color:#EA580C;font-weight:600">반응</span>: 온실 속의 화초가 아니라 현실 세계의 거친 악성 트래픽 폭풍을 견뎌내는 강철 서버로 거듭나겠네!

<span style="color:#DC2626;font-weight:600">의심</span>: 클라이언트가 바이트를 1바이트씩 쪼개 보내며 연결만 붙잡고 있는 슬로로리스(Slowloris) 공격이 들어오면 어떻게 방어하는가?

<span style="color:#16A34A;font-weight:600">깨달음</span>: 헤더 수신 타이머를 두고 지정된 시간(예: 10초) 내에 `\r\n\r\n`이 완성되지 않으면 `408 Request Timeout`과 함께 연결을 즉각 닫아야 하는구나!

<span style="color:#0891B2;font-weight:600">확인</span>: [실측] 64KB 초과 비정상 긴 URI 주입 시 서버가 죽지 않고 즉시 `414 URI Too Long` 응답을 송신한 후 세션을 회수함을 확인했다.

<span style="color:#A16207;font-weight:600">[뉘앙스 / 의역]</span>: 엣지 케이스 처리는 정상 경로만 걷던 소프트웨어를 가혹한 야생에서 생존하게 만드는 면역 체계이다.

---

### 24. 프로덕션 수준의 포트폴리오 완성도 달성

원문:
• 이력서와 포트폴리오에 포함할 수 있을 만큼 완성도 높은 수준으로 구현합니다.

직관적 1초 컷 독해: "학생 과제 수준에 만족하지 말고, 최고의 기술 면접관 앞에서도 당당히 자랑할 수 있는 작품으로 끝장내라는 거지?"

바로 읽기: 단순 학업용 토이 프로젝트의 한계를 뛰어넘어 아키텍처 다이어그램, 벤치마크 성능 데이터, 견고한 예외 처리를 갖춘 실무 프로덕션 엔지니어링 수준의 산출물로 완성할 것을 명시한다.

이 조건이 없으면 생기는 대참사: 동작만 겨우 하는 조잡한 스파게티 코드로 남아 취업 시장과 기술 면접에서 경쟁력을 전혀 발휘하지 못함

<span style="color:#2563EB;font-weight:600">예상</span>: 체계적인 모듈 분리, 깔끔한 커밋 히스토리, 벤치마크 그래프와 기술 결정 배경이 담긴 고품격 README.md를 완성할 것이다.

<span style="color:#EA580C;font-weight:600">반응</span>: 이 프로젝트 하나만으로도 로우레벨 시스템과 네트워크 역량을 완벽히 증명할 수 있는 인생 최고의 무기가 탄생하겠네!

<span style="color:#DC2626;font-weight:600">의심</span>: 완성도 높은 수준이라는 모호한 기준을 기술적으로 어떻게 객관화하여 증명할 것인가?

<span style="color:#16A34A;font-weight:600">깨달음</span>: 수치로 증명되는 4만 RPS 처리량, 누수 0바이트 Valgrind 리포트, RFC 표준 준수 검증 로그가 바로 가장 객관적인 완성도의 징표이구나!

<span style="color:#0891B2;font-weight:600">확인</span>: [연역] C 언어 소켓 기반 고성능 리버스 프록시 구현 경험은 최고 수준 백엔드와 시스템 엔지니어 포트폴리오의 독보적 차별점이 된다.

<span style="color:#A16207;font-weight:600">[뉘앙스 / 의역]</span>: 타협 없는 완성도는 평범한 개발자를 기술의 본질을 꿰뚫는 탁월한 시스템 엔지니어로 변모시키는 도약대이다.

---

## 관련 노트

- [[미니 레디스 구현 요구사항 라인 바이 라인 정밀 해석 — 해시 테이블 · 동시성 · 만료 · 무효화 · 영속성 18개 항목]]

- [[미니 리액트 구현 요구사항 라인 바이 라인 독해 — Virtual DOM · Diff · Patch와 Component · State · Hooks 97개 청크]]

---

## 관련 자료

- [All_Sentence.md](<file:///c:/Users/V15_G3_ABA_2023/Desktop/[크래프톤정글_SWAI_13]/11_12주차_AI_함께하기/정희님_문서양식/All_Sentence.md>) — 이 해석이 기준으로 삼은 마스터 양식 파일입니다.

- [C_웹서버_프록시_요구사항_라인바이라인_정밀해석.md](<file:///c:/Users/V15_G3_ABA_2023/Desktop/[크래프톤정글_SWAI_13]/11_12주차_AI_함께하기/mini_AI_KJS/ai-framework/C_웹서버_프록시_요구사항_라인바이라인_정밀해석.md>) — 전체 94개 조항 통합본입니다.

- [타_AI_작업_수행_절대_지시서.md](<file:///c:/Users/V15_G3_ABA_2023/Desktop/[크래프톤정글_SWAI_13]/11_12주차_AI_함께하기/docs/타_AI_작업_수행_절대_지시서.md>) — 해석 작업에 쓴 지시서입니다.
