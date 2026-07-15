# DataMonitor

반도체 시료 생산주문관리 시스템 개인과제의 **[미션1] PoC** 중 "데이터 모니터링 Tool" 항목입니다.

`../DataPersistence/CrudApp` 이 JSON 파일(`data/records.json`)로 저장하고 있는 데이터를,
콘솔에서 실시간으로 조회할 수 있는 읽기 전용(read-only) 관리자 도구입니다.

## 설계

### 데이터 모델

CrudApp이 현재 저장 중인 범용 `Record` 구조(id / name / description / createdAt)를 그대로 모니터링 대상으로 삼습니다.
PoC 단계이므로 SampleOrderSystem의 도메인 모델(시료/주문 등)은 아직 반영하지 않으며,
추후 본 프로젝트 단계에서 도메인 모델로 교체됩니다.

### 코드 공유 방식

각 PoC는 개별 Repository로 제출해야 하므로, CrudApp의 `Json.h/.cpp`, `Record.h`, `Repository.h/.cpp`를
**파일로 복사**하여 DataMonitor 레포 안에 독립적으로 포함합니다. (상대경로 include로 두 레포를 결합하지 않음)

### 아키텍처 (역할 분리)

| 계층 | 파일 | 역할 |
|---|---|---|
| Model | `Model/Json.h`, `Model/Json.cpp` | 범용 JSON 파서 & 직렬화 (CrudApp에서 복사) |
| Model | `Model/Record.h` | 데이터 모델 정의 (CrudApp에서 복사) |
| Model | `Model/Repository.h`, `Model/Repository.cpp` | JSON 파일 기반 저장소. 쓰기 메서드(`create/updateFields/removeById`)는 호출하지 않고 조회 전용으로만 사용 |
| View | `View/MonitorView.h`, `View/MonitorView.cpp` | 대시보드/검색 결과를 콘솔에 렌더링하는 순수 출력 함수 |
| Controller | `main.cpp` | 폴링 루프 + 비동기 키 입력 처리 |

### 데이터 흐름 / 동작 방식

1. 시작 시 `RecordRepository`가 `data/records.json` (CrudApp과 동일한 파일)을 읽어 메모리에 적재합니다.
2. **자동 대시보드 루프**: 2초 간격으로 파일의 마지막 수정 시각(`std::filesystem::last_write_time`)을 확인합니다.
   - 변경이 감지되면 `load()`를 재호출하고 화면을 다시 그립니다.
   - 변경이 없으면 화면을 다시 그리지 않고 대기합니다 (깜빡임 최소화).
   - 화면에는 전체 레코드 목록(ID/이름/설명/생성일시), 총 건수, 마지막 변경 감지 시각을 표시합니다.
3. **비동기 키 입력** (`_kbhit`/`_getch`)을 매 틱마다 확인합니다.
   - `s` → 검색 모드로 전환(루프 일시정지) → `[1] ID 검색 [2] 이름 키워드 검색 [0] 뒤로가기` → 결과 출력 후 아무 키나 누르면 대시보드로 복귀.
   - `q` → 프로그램 종료.
4. 실행 위치와 무관하게 데이터 파일을 찾을 수 있도록, `data/records.json` 경로는 `../DataPersistence/CrudApp/data/records.json`을 가리키도록 설정합니다 (`.vcxproj.user`의 디버그 작업 디렉터리로 지정).

### 에러 처리

- JSON 파일이 없거나 파싱에 실패하면 CrudApp의 기존 방어 로직을 그대로 재사용합니다: 오류 메시지를 출력한 뒤 빈 목록으로 표시하되, 프로그램은 계속 폴링합니다 (파일이 나중에 생성/복구될 수 있으므로).
- 데이터 파일 경로를 찾을 수 없는 경우 대시보드 상단에 경고를 표시합니다.

### 테스트 (수동 검증 시나리오)

- CrudApp을 별도로 실행해 레코드를 추가/수정/삭제하면서, DataMonitor 화면이 2초 내 자동 갱신되는지 확인.
- ID 검색 / 이름 키워드 검색이 정확한 결과를 반환하는지 확인.
- `records.json`이 없는 상태에서 DataMonitor를 실행해도 크래시 없이 경고만 표시하는지 확인.

## 빌드 및 실행

- Visual Studio (MSVC v145, C++20) 프로젝트로 구성되어 있습니다 (`DataMonitor.slnx`).
- `DataMonitor` 프로젝트를 시작 프로젝트로 지정하고 F5로 실행합니다.
