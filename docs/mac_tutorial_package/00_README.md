# Mac 전달 묶음 — C 웹서버 · 프록시 반자동 튜토리얼

만든 날짜: 2026-11-20

이 묶음은 Windows PC에서 만들어 Mac으로 옮기는 자료입니다. Mac에서 사용자와 함께 일하는 AI가 읽고, 사용자가 C 코드를 직접 한 줄씩 치는 튜토리얼을 끝까지 진행하는 데 씁니다.

---

## 1. 들어 있는 파일

| 파일 | 무엇인가 | 읽는 순서 |
|---|---|---|
| `00_README.md` | 이 문서 | 1 |
| `01_tutorial_workorder.md` | 작업 지시서. 튜터 AI의 역할, 경계, 한 번의 주고받기 형식, 보고 형식 | 2 |
| `02_stage1_sections.md` | 1단계(구간 1-1 ~ 1-6)의 줄 단위 대본 | 3 |
| `03_stage2_to_9_curriculum.md` | 2 ~ 9단계 전 구간의 설계서 | 4 |
| `04_TUTORIAL_LOG_template.md` | 진행 기록 양식 | 5 |
| `ref/requirements_24.md` | 전제 자료. 과제 요구사항 해석 24개 항목. 작업 범위의 근거 | 필요할 때 |
| `ref/guide_31.md` | 전제 자료. 과제 가이드 해석 31개 항목. 작업 범위의 근거 | 필요할 때 |
| `ref/base_setup_spec.md` | 전제 자료. macOS 기반 세팅 명세서 (이미 수행함) | 필요할 때 |
| `ref/vscode_setup_workorder.md` | 전제 자료. Visual Studio Code 환경 구성 지시서 | 필요할 때 |
| `ref/stack_diagram_stage1.html` | 1단계 지도. HTTP 메시지 흐름 ①–⑦과 TCP 상태 10개. 브라우저로 엽니다 | 사용자가 봄 |
| `ref/visual/stack_diagram_SKILL.md` | 전제 자료. 스택 다이어그램을 만드는 규칙 | 9-4구간 |
| `ref/visual/mini_redis_stack_diagram_workorder.md` | 전제 자료. 스택 다이어그램 작업지시서의 본보기 | 9-4구간 |
| `ref/visual/FLOW_DIAGRAM_SKILL.md` | 전제 자료. 흐름도와 툴팁을 만드는 규칙 | 9-4구간 |

파일 이름을 영어로 바꾼 이유는 Windows에서 만든 zip을 macOS에서 풀 때 한글 파일 이름이 깨지는 일을 피하기 위해서입니다. 내용은 원본과 같습니다. 원본 이름은 아래와 같습니다.

| 묶음 안의 이름 | 원본 이름 |
|---|---|
| `ref/requirements_24.md` | `C 웹서버 · 프록시 구현 요구사항 라인 바이 라인 정밀 해석 — TCP 소켓 · HTTP 파싱 · 동시성 · Forward · Reverse Proxy · 품질 검증 24개 항목.md` |
| `ref/guide_31.md` | `C 웹서버_AI 함께하기 가이드 라인 바이 라인 정밀 해석 — 학습 목표 · 학습 키워드 · 과제 수행 원칙 · 주요 일정 · CSAPP 11장 31개 항목.md` |
| `ref/base_setup_spec.md` | `c_webserver_proxy_macOS_기반세팅_명세서.md` |
| `ref/stack_diagram_stage1.html` | `c_webserver_proxy_stack_diagram_1단계.html` |
| `ref/visual/stack_diagram_SKILL.md` | `stack-diagram-main/skills/stack-diagram/SKILL.md` |
| `ref/visual/mini_redis_stack_diagram_workorder.md` | `mini_redis_stack_diagram_작업지시서.md` |
| `ref/visual/FLOW_DIAGRAM_SKILL.md` | `FLOW_DIAGRAM_AND_MAPLESCOUTER_TOOLTIP_SKILL.md` |

---

## 2. Mac에서 놓을 자리

zip을 풀어 나온 `mac_tutorial_package` 폴더를 프로젝트 폴더의 `docs/` 아래에 그대로 넣습니다.

```text
/Users/gyaroron/Krafton_Jungle_SWAI_Lab/c-webserver-proxy/docs/mac_tutorial_package/
```

이 묶음을 git에 올릴지는 사용자가 정합니다. 올리지 않으려면 `.gitignore`에 `docs/mac_tutorial_package/`를 더합니다.

---

## 3. Mac의 AI에게 보낼 첫 메시지

묶음을 놓은 뒤 아래를 그대로 보냅니다.

```text
docs/mac_tutorial_package/ 폴더에 튜토리얼 자료를 넣었습니다.

1. 00_README.md, 01_tutorial_workorder.md, 02_stage1_sections.md, 03_stage2_to_9_curriculum.md, 04_TUTORIAL_LOG_template.md를 이 순서로 끝까지 읽어 주세요. 전 구간(1 ~ 9단계)의 길을 먼저 알아야 합니다.
2. ref/ 안의 자료는 01_tutorial_workorder.md 2절의 표에 적힌 쓰임대로 읽어 주세요.
3. 지금 하던 1-1구간을 처음부터 다시 하지 말고, src/server.c를 읽어 제가 어디까지 쳤는지 확인한 뒤 02_stage1_sections.md의 1-1구간에서 이어 가 주세요.
4. 04_TUTORIAL_LOG_template.md를 docs/TUTORIAL_LOG.md로 복사해 기록을 시작해 주세요.
5. 지금 따르고 있는 All_Sentence.md 규칙은 그대로 유지하고, 이 자료와 어긋나는 곳이 있으면 고르지 말고 저에게 물어 주세요.
6. 시작하기 전에, 읽은 내용을 아래 형식으로 짧게 확인해 주세요.
   - 당신의 역할과 하면 안 되는 일 (3줄)
   - 9단계의 이름과 단계마다 구간 수
   - 지금 src/server.c가 몇 행까지 쳐져 있고 다음에 칠 줄이 무엇인지
   - 자료에서 이상하거나 이 Mac과 맞지 않아 보이는 곳
```

---

## 4. 알아 둘 것

- `02_stage1_sections.md`의 C 코드는 Windows PC에서 작성했고 **macOS에서 컴파일해 보지 못했습니다.** Windows PC에 C 컴파일러가 없었기 때문입니다. `01_tutorial_workorder.md` 3-4절에 따라 Mac의 AI가 구간마다 먼저 컴파일해 확인합니다.
- `03_stage2_to_9_curriculum.md`는 설계서입니다. 함수 모양과 꼭 지킬 동작은 적혀 있지만 칠 줄은 적혀 있지 않습니다. Mac의 AI가 구간마다 참고 구현을 만들어 검사한 뒤 줄 단위로 안내합니다.
- 실험의 "기대 결과"는 예상입니다. 실제 출력과 다르면 실제 출력이 맞습니다. 다른 곳은 `docs/TUTORIAL_LOG.md`의 "대본 · 설계서 수정 요청 모음"에 모입니다.
- 통합 아키텍처 지도(HTML)와 학습 노트는 9단계가 끝난 뒤 Windows PC 쪽에서 만듭니다. Mac의 AI는 9-4구간에서 필요한 자료를 `docs/HANDOFF_FOR_MAP.md`에 모아 줍니다.
