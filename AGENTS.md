# C Web Server Proxy 작업 · 답변 고정 표준 (`All_Sentence.md` 기반)

📌 [단일 독점 Import와 절대 기준 마스터 파일 준수 선언]

본 작업자는 마스터 기준 파일인 `All_Sentence.md`([All_Sentence.md](file:///Users/gyaroron/Downloads/All_Sentence.md))를 최상위 표준으로 준수하며, 컨텍스트 압축 이후에도 아래 양식과 검증 원칙을 항상 유지한다.

---

## 1. 답변 · 문서 고정 골격 (베스트 답변 양식)

1. **선언과 기준 고정**: 답변과 재구성 문서 맨 앞에 제목(`# ...`)과 `📌 [단일 독점 Import와 절대 기준 마스터 파일 준수 선언]` 3원칙(원문과 소스 근거 보존 · 띄어쓰기와 색상 구분 · 문체와 어휘 통제)을 둔다.

2. **색상 범례 블록**: 본문 시작 전에 아래 두 줄 범례를 두고, 본문 핵심 수치와 생각 태그에 인라인 스타일(`<span style="color:...;font-weight:600">`)을 적용한다.

> 머릿속 색: <span style="color:#EA580C;font-weight:600">반응</span> · <span style="color:#2563EB;font-weight:600">예상</span> · <span style="color:#16A34A;font-weight:600">깨달음</span> · <span style="color:#DC2626;font-weight:600">의심</span> · <span style="color:#0891B2;font-weight:600">확인</span> · <span style="color:#A16207;font-weight:600">뉘앙스</span>.
>
> 숫자 색: <span style="color:#2458C6;font-weight:600">입력 값</span> · <span style="color:#B86E00;font-weight:600">제한 · 상수</span> · <span style="color:#11845A;font-weight:600">실행 · 예시 값</span> · <span style="color:#B3206C;font-weight:600">계산 결과</span>.

3. **본문 전개 순서**:
   - `**직관적 1초 컷 독해**`: 질문형 반말 한 문장 (`"~라는 거지?"`)
   - `**바로 읽기**`: 핵심 구조와 동작 원인 요약 (평서문 `~다`체)
   - **ASCII 아키텍처 흐름도**: `text` 코드 블록으로 계층·모듈·라인 번호·분기 조건 연결
   - **3열 대조표**: `체크포인트(또는 요구사항) | 일반 원리(또는 기준) | 우리 C 웹 서버 · 프록시 실제 구현`
   - **꼬리질문 · 심화 대응**: 실제 코드 근거와 범위 밖 요소 분리 서술
   - `**이 관점이 없으면 생기는 대참사**`: 감정어 없는 구체적 결함 명사구
   - **생각 흐름**: <span style="color:#EA580C;font-weight:600">반응</span> · <span style="color:#2563EB;font-weight:600">예상</span> · <span style="color:#DC2626;font-weight:600">의심</span> · <span style="color:#16A34A;font-weight:600">깨달음</span> · <span style="color:#0891B2;font-weight:600">확인</span>(`[실측]`) · <span style="color:#A16207;font-weight:600">[뉘앙스 / 의역]</span> 순서로 빈 줄을 두어 배치

---

## 2. 근거 · 문체 · 어휘 통제 원칙

1. **원본 보존과 실제 코드 우선**: 기존 HTML·원본 문서는 수정하지 않고 복사본에 반영한다. 답변 전에는 반드시 현재 디스크의 실제 C 소스(`src/` · `include/` · `tests/` · `scripts/` · `Makefile`)와 문서 라인을 직접 읽고 확인한 사실과 행 번호만 적는다.

2. **칸별 말투 분리**: `1초 컷`은 질문형 반말, `반응`·`깨달음`은 반말 감탄(`~네!`, `~구나!`), 나머지 본문·표·검증은 평서문(`~다`체)으로 통일한다.

3. **괄호·문장·근거 표지**: 괄호 안에는 영문 원문, 약어 풀이, 단위, 파일/행 범위만 넣고 서술 절은 독립 문장으로 분리한다. 실측·추론·가설을 `[실측]`, `[연역]`, `[귀납]`, `[가설]` 표지로 구분한다.

4. **어휘 자동 검사 필수 (`total 0`)**: 문서와 최종 답변 모두 `check_words.py` 24대 금지어(`'ㅁ+ㅣ+ㅊ'` · 명·확 · 정·확 · 완·전 · 적·절 · 정·도 · 선·명 · 또·렷 · 명·료 · 엄·밀 · 필·연 · 온·전 · 무·거 · 무·식 · 벽·돌 · 장·난·감 · 것·같 · 로·보·인 · 수·도·있 · 쩔 · 알·아·서 · 나·머·지·생·략`), 단일 음절 `'ㄷ+ㅡ+ㅇ'`(합성어 포함), 말줄임표(`U+2026`)를 **0회** 유지하며 출력 전 스크립트로 `total 0`을 확인한다.
