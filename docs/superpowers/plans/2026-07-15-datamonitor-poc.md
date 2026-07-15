# DataMonitor PoC Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a read-only C++ console tool (`DataMonitor`) that continuously monitors the `records.json` file produced by `../DataPersistence/CrudApp` and lets an operator search it, without modifying the data.

**Architecture:** Model (JSON parser + `Record` + `RecordRepository`, copied verbatim from CrudApp) / View (pure console-rendering functions) / Controller (`main.cpp`: a 2-second poll loop that redraws on file change, plus non-blocking key handling for `s` = search, `q` = quit).

**Tech Stack:** C++20, MSVC v145, Windows Console API (`<conio.h>` for `_kbhit`/`_getch`), `<filesystem>` for mtime polling. No third-party dependencies, no unit-test framework (this repo family has none — CrudApp verifies via manual `cl.exe` build + scenario run, and this plan follows the same convention).

## Global Constraints

- Each PoC must be buildable and runnable independently — no `#include` reaching outside this repo (per the earlier design decision: Model files are **copied**, not shared, from `../DataPersistence/CrudApp`).
- Read-only: never call `Repository::create`/`updateFields`/`removeById` from DataMonitor.
- Data file path: `../../DataPersistence/CrudApp/data/records.json`, relative to the `DataMonitor/DataMonitor/` project directory (matches Visual Studio's default debugger working directory, i.e. `$(ProjectDir)`).
- Source files must compile with `/utf-8` (Korean literals) — already the project default via `CharacterSet=Unicode`; must add `<PreprocessorDefinitions>` unaffected, but must add `/utf-8` compiler flag explicitly since this project (unlike CrudApp) doesn't have `/utf-8` set today (verify in Task 1).
- Poll interval: 2 seconds, constant `POLL_INTERVAL` in `main.cpp`.

---

### Task 1: Copy Model layer and verify it compiles + loads data

**Files:**
- Create: `DataMonitor/Model/Json.h` (verbatim copy of `../DataPersistence/CrudApp/Json.h`)
- Create: `DataMonitor/Model/Json.cpp` (verbatim copy of `../DataPersistence/CrudApp/Json.cpp`)
- Create: `DataMonitor/Model/Record.h` (verbatim copy of `../DataPersistence/CrudApp/Record.h`)
- Create: `DataMonitor/Model/Repository.h` (verbatim copy of `../DataPersistence/CrudApp/Repository.h`)
- Create: `DataMonitor/Model/Repository.cpp` (verbatim copy of `../DataPersistence/CrudApp/Repository.cpp`, with `#include "Repository.h"` unchanged since it stays in the same folder)
- Create: `DataMonitor/main.cpp` (temporary smoke-test body, replaced in Task 3)
- Modify: `DataMonitor/DataMonitor.vcxproj` (add the 5 Model files + `main.cpp` to `ItemGroup`, add `/utf-8` via `<AdditionalOptions>`)
- Modify: `DataMonitor/DataMonitor.vcxproj.filters` (register the new files under "소스 파일"/"헤더 파일" filters)

**Interfaces:**
- Produces: `RecordRepository` (constructor `RecordRepository(std::string filePath)`, `void load()`, `const std::vector<Record>& listAll() const`, `const Record* findById(int id) const`, `std::vector<Record> searchByNameKeyword(const std::string&) const`) — later tasks call these read-only methods.
- Produces: `Record` struct with public fields `id` (int), `name` (std::string), `description` (std::string), `createdAt` (std::string).

- [ ] **Step 1: Copy the four Model files verbatim**

Copy the exact contents already read from `../DataPersistence/CrudApp/Json.h`, `Json.cpp`, `Record.h`, `Repository.h`, `Repository.cpp` into `DataMonitor/Model/` with the same filenames. Do not change a single line — these are proven, working code. (If using an editor/agent without direct file-copy, read each source file and write it byte-for-byte into the new path.)

- [ ] **Step 2: Write a temporary smoke-test `main.cpp`**

```cpp
#include <iostream>
#include "Model/Repository.h"

int main() {
    RecordRepository repo("../../DataPersistence/CrudApp/data/records.json");
    repo.load();
    std::cout << "Loaded " << repo.listAll().size() << " record(s)\n";
    for (const auto& r : repo.listAll()) {
        std::cout << "[" << r.id << "] " << r.name << " | " << r.description << " | " << r.createdAt << "\n";
    }
    return 0;
}
```

- [ ] **Step 3: Add the new files to the vcxproj**

Add before the final `<ItemGroup></ItemGroup>` line (currently empty) in `DataMonitor.vcxproj`:

```xml
  <ItemGroup>
    <ClInclude Include="Model\Json.h" />
    <ClInclude Include="Model\Record.h" />
    <ClInclude Include="Model\Repository.h" />
  </ItemGroup>
  <ItemGroup>
    <ClCompile Include="Model\Json.cpp" />
    <ClCompile Include="Model\Repository.cpp" />
    <ClCompile Include="main.cpp" />
  </ItemGroup>
```

Also add `<AdditionalOptions>/utf-8 %(AdditionalOptions)</AdditionalOptions>` inside each of the four `<ClCompile>` blocks under `<ItemDefinitionGroup>` (Debug|Win32, Release|Win32, Debug|x64, Release|x64), so Korean string literals compile correctly regardless of system code page.

- [ ] **Step 4: Register files in the filters file**

In `DataMonitor.vcxproj.filters`, add:

```xml
  <ItemGroup>
    <ClInclude Include="Model\Json.h"><Filter>헤더 파일</Filter></ClInclude>
    <ClInclude Include="Model\Record.h"><Filter>헤더 파일</Filter></ClInclude>
    <ClInclude Include="Model\Repository.h"><Filter>헤더 파일</Filter></ClInclude>
  </ItemGroup>
  <ItemGroup>
    <ClCompile Include="Model\Json.cpp"><Filter>소스 파일</Filter></ClCompile>
    <ClCompile Include="Model\Repository.cpp"><Filter>소스 파일</Filter></ClCompile>
    <ClCompile Include="main.cpp"><Filter>소스 파일</Filter></ClCompile>
  </ItemGroup>
```
(insert this `<ItemGroup>` block right before the closing `</Project>`, after the existing `<Filter>` definitions block)

- [ ] **Step 5: Build and run via cl.exe to verify (manual test, no unit-test framework in this repo family)**

Run (from `DataMonitor/DataMonitor/` — adjust the `vcvarsall.bat` path to the installed VS version if different):

```bash
cmd //c '"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 && cl /std:c++20 /utf-8 /EHsc /Fe:datamonitor_smoke.exe main.cpp Model\Json.cpp Model\Repository.cpp && datamonitor_smoke.exe'
```

Expected output: `Loaded 1 record(s)` followed by the one record currently in `../DataPersistence/CrudApp/data/records.json` (`[1] 수정된이름 | 첫번째 설명 | 2026-07-15 09:31:46`, or whatever it currently contains).

- [ ] **Step 6: Clean up build artifact and commit**

```bash
rm -f DataMonitor/DataMonitor/datamonitor_smoke.exe DataMonitor/DataMonitor/datamonitor_smoke.obj
git add DataMonitor/DataMonitor/Model DataMonitor/DataMonitor/main.cpp DataMonitor/DataMonitor/DataMonitor.vcxproj DataMonitor/DataMonitor/DataMonitor.vcxproj.filters
git commit -m "Add Model layer copied from CrudApp and verify data loads"
```

---

### Task 2: Build the View layer (dashboard + record table rendering)

**Files:**
- Create: `DataMonitor/View/MonitorView.h`
- Create: `DataMonitor/View/MonitorView.cpp`
- Modify: `DataMonitor/main.cpp` (replace smoke-test body to call the new view once, still no loop yet)
- Modify: `DataMonitor/DataMonitor.vcxproj` / `.filters` (register the 2 new files)

**Interfaces:**
- Consumes: `Record` (from `Model/Record.h`), `std::vector<Record>` from `RecordRepository::listAll()`.
- Produces: `void renderDashboard(const std::vector<Record>& records, const std::string& lastChangeTime, bool dataFileMissing)` — later tasks (Task 3's poll loop) call this every redraw.
- Produces: `void renderRecordTable(const std::vector<Record>& records)` — shared table renderer, also reused by Task 5's search results.
- Produces: `void clearScreen()` — used by the poll loop before each redraw.

- [ ] **Step 1: Write `View/MonitorView.h`**

```cpp
#pragma once
#include <string>
#include <vector>
#include "../Model/Record.h"

void clearScreen();
void renderRecordTable(const std::vector<Record>& records);
void renderDashboard(const std::vector<Record>& records, const std::string& lastChangeTime, bool dataFileMissing);
```

- [ ] **Step 2: Write `View/MonitorView.cpp`**

```cpp
#include "MonitorView.h"
#include <iostream>
#include <cstdlib>

void clearScreen() {
    std::system("cls");
}

void renderRecordTable(const std::vector<Record>& records) {
    if (records.empty()) {
        std::cout << "표시할 데이터가 없습니다.\n";
        return;
    }
    std::cout << "----------------------------------------------------------------\n";
    std::cout << "ID    NAME                 DESCRIPTION               CREATED_AT\n";
    std::cout << "----------------------------------------------------------------\n";
    for (const auto& r : records) {
        std::cout << r.id << "\t" << r.name << "\t" << r.description << "\t" << r.createdAt << "\n";
    }
    std::cout << "----------------------------------------------------------------\n";
    std::cout << "총 " << records.size() << "건\n";
}

void renderDashboard(const std::vector<Record>& records, const std::string& lastChangeTime, bool dataFileMissing) {
    std::cout << "===================== DataMonitor =====================\n";
    if (dataFileMissing) {
        std::cout << "⚠ 데이터 파일을 찾을 수 없습니다. 파일이 생성되면 자동으로 반영됩니다.\n";
    }
    std::cout << "마지막 변경 감지: " << lastChangeTime << "\n\n";
    renderRecordTable(records);
    std::cout << "\n[s] 검색  [q] 종료\n";
}
```

- [ ] **Step 3: Update `main.cpp` to call it once**

```cpp
#include <iostream>
#include "Model/Repository.h"
#include "View/MonitorView.h"

int main() {
    RecordRepository repo("../../DataPersistence/CrudApp/data/records.json");
    repo.load();
    renderDashboard(repo.listAll(), "(초기 로드)", false);
    return 0;
}
```

- [ ] **Step 4: Register new files in vcxproj + filters**

Add to `DataMonitor.vcxproj`'s `ItemGroup`s from Task 1:
```xml
    <ClInclude Include="View\MonitorView.h" />
```
```xml
    <ClCompile Include="View\MonitorView.cpp" />
```

Add to `DataMonitor.vcxproj.filters`:
```xml
    <ClInclude Include="View\MonitorView.h"><Filter>헤더 파일</Filter></ClInclude>
    <ClCompile Include="View\MonitorView.cpp"><Filter>소스 파일</Filter></ClCompile>
```

- [ ] **Step 5: Build and run to verify (manual test)**

```bash
cmd //c '"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 && cl /std:c++20 /utf-8 /EHsc /Fe:datamonitor_smoke.exe main.cpp Model\Json.cpp Model\Repository.cpp View\MonitorView.cpp && datamonitor_smoke.exe'
```

Expected: a dashboard-style screen showing the header, the record table with the one existing record, and the `[s] 검색  [q] 종료` footer.

- [ ] **Step 6: Clean up and commit**

```bash
rm -f DataMonitor/DataMonitor/datamonitor_smoke.exe DataMonitor/DataMonitor/datamonitor_smoke.obj
git add DataMonitor/DataMonitor/View DataMonitor/DataMonitor/main.cpp DataMonitor/DataMonitor/DataMonitor.vcxproj DataMonitor/DataMonitor/DataMonitor.vcxproj.filters
git commit -m "Add MonitorView dashboard rendering"
```

---

### Task 3: Poll loop with file-change detection

**Files:**
- Modify: `DataMonitor/main.cpp` (add the 2-second poll loop, `std::filesystem::last_write_time` comparison, `Sleep`)

**Interfaces:**
- Consumes: `RecordRepository::load()`, `renderDashboard(...)`, `clearScreen()`.
- Produces: a `bool fileExistsAndChanged(const std::string& path, std::filesystem::file_time_type& lastSeen)` helper used by Task 4's loop body too.

- [ ] **Step 1: Rewrite `main.cpp` with the poll loop (no key handling yet — runs until Ctrl+C)**

```cpp
#include <iostream>
#include <string>
#include <filesystem>
#include <chrono>
#include <ctime>
#include <thread>
#include "Model/Repository.h"
#include "View/MonitorView.h"

namespace {

constexpr auto POLL_INTERVAL = std::chrono::seconds(2);
const std::string DATA_FILE_PATH = "../../DataPersistence/CrudApp/data/records.json";

std::string nowTimestamp() {
    std::time_t t = std::time(nullptr);
    std::tm tmBuf{};
    localtime_s(&tmBuf, &t);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tmBuf);
    return std::string(buf);
}

// 파일 존재 여부와 마지막 수정 시각을 확인. 변경이 감지되면 true를 반환하고 lastSeen을 갱신한다.
bool checkForChange(std::filesystem::file_time_type& lastSeen, bool& fileMissing) {
    std::error_code ec;
    auto currentTime = std::filesystem::last_write_time(DATA_FILE_PATH, ec);
    if (ec) {
        bool wasMissing = fileMissing;
        fileMissing = true;
        return !wasMissing; // 방금 사라졌다면 화면을 한 번 갱신
    }
    fileMissing = false;
    if (currentTime != lastSeen) {
        lastSeen = currentTime;
        return true;
    }
    return false;
}

} // namespace

int main() {
    RecordRepository repo(DATA_FILE_PATH);
    repo.load();

    std::filesystem::file_time_type lastSeen{};
    bool fileMissing = false;
    std::string lastChangeTime = nowTimestamp();

    // 최초 1회는 항상 그린다
    checkForChange(lastSeen, fileMissing);
    clearScreen();
    renderDashboard(repo.listAll(), lastChangeTime, fileMissing);

    while (true) {
        std::this_thread::sleep_for(POLL_INTERVAL);

        if (checkForChange(lastSeen, fileMissing)) {
            if (!fileMissing) repo.load();
            lastChangeTime = nowTimestamp();
            clearScreen();
            renderDashboard(repo.listAll(), lastChangeTime, fileMissing);
        }
    }

    return 0;
}
```

- [ ] **Step 2: Build and manually verify auto-refresh**

Build the same way as Task 2's Step 5. Run it, then in a separate terminal edit `../DataPersistence/CrudApp/data/records.json` (e.g. append a record manually or run CrudApp's "새 데이터 추가"). Expected: within ~2 seconds the DataMonitor screen clears and redraws with the updated record list and a new "마지막 변경 감지" timestamp. Stop the program with Ctrl+C (key-based quit comes in Task 4).

- [ ] **Step 3: Commit**

```bash
git add DataMonitor/DataMonitor/main.cpp
git commit -m "Add 2-second poll loop with file-change detection"
```

---

### Task 4: Non-blocking key input (quit / enter search mode)

**Files:**
- Modify: `DataMonitor/main.cpp` (add `_kbhit`/`_getch` check inside the loop; `q` exits cleanly, `s` calls a placeholder `runSearchMode` that will be filled in by Task 5)

**Interfaces:**
- Produces: `void runSearchMode(RecordRepository& repo)` forward-declared here, implemented in Task 5.

- [ ] **Step 1: Add the key-check and quit/search dispatch**

Add `#include <conio.h>` to the includes, forward-declare and stub the search entry point, and change the `while (true)` loop to poll for keys during the sleep instead of one long sleep:

```cpp
#include <conio.h>
// ... (other includes unchanged)

void runSearchMode(RecordRepository& repo); // Task 5 implements this

// replace the sleep_for + checkForChange block inside main()'s while(true) with:
        bool quit = false;
        auto sleepUntil = std::chrono::steady_clock::now() + POLL_INTERVAL;
        while (std::chrono::steady_clock::now() < sleepUntil) {
            if (_kbhit()) {
                int ch = _getch();
                if (ch == 'q' || ch == 'Q') {
                    quit = true;
                    break;
                }
                if (ch == 's' || ch == 'S') {
                    runSearchMode(repo);
                    lastChangeTime = nowTimestamp();
                    checkForChange(lastSeen, fileMissing); // 검색 중 파일이 바뀌었을 수도 있으니 재확인
                    clearScreen();
                    renderDashboard(repo.listAll(), lastChangeTime, fileMissing);
                    sleepUntil = std::chrono::steady_clock::now() + POLL_INTERVAL;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        if (quit) break;

        if (checkForChange(lastSeen, fileMissing)) {
            if (!fileMissing) repo.load();
            lastChangeTime = nowTimestamp();
            clearScreen();
            renderDashboard(repo.listAll(), lastChangeTime, fileMissing);
        }
```

Add a temporary stub above `main()` so it links (Task 5 replaces this with the real implementation in `main.cpp` directly — no separate file needed, this is controller logic, not a view):

```cpp
void runSearchMode(RecordRepository&) {
    std::cout << "\n(검색 모드는 다음 작업에서 구현됩니다. 아무 키나 누르세요...)\n";
    _getch();
}
```

- [ ] **Step 2: Build and verify `q` quits and `s` shows the stub message**

Build per Task 2 Step 5's command. Run it, press `s` — expect the stub message, then any key returns to the dashboard. Press `q` — expect the program to exit immediately (not waiting up to 2 seconds).

- [ ] **Step 3: Commit**

```bash
git add DataMonitor/DataMonitor/main.cpp
git commit -m "Add non-blocking key handling for quit and search entry"
```

---

### Task 5: Implement search mode (ID search + keyword search)

**Files:**
- Modify: `DataMonitor/main.cpp` (replace the `runSearchMode` stub with the real implementation)

**Interfaces:**
- Consumes: `RecordRepository::findById(int) const`, `RecordRepository::searchByNameKeyword(const std::string&) const`, `renderRecordTable(const std::vector<Record>&)`.

- [ ] **Step 1: Replace the stub with the full search menu**

```cpp
void runSearchMode(RecordRepository& repo) {
    std::cout << "\n===================== 검색 =====================\n";
    std::cout << "[1] ID 검색  [2] 이름 키워드 검색  [0] 뒤로가기\n";
    std::cout << "선택 > ";
    std::string choice;
    std::getline(std::cin, choice);

    if (choice == "1") {
        std::cout << "검색할 ID > ";
        std::string idInput;
        std::getline(std::cin, idInput);
        try {
            int id = std::stoi(idInput);
            const Record* r = repo.findById(id);
            if (r) {
                renderRecordTable(std::vector<Record>{*r});
            } else {
                std::cout << "ID " << id << " 에 해당하는 데이터를 찾을 수 없습니다.\n";
            }
        } catch (...) {
            std::cout << "올바른 ID를 입력해주세요.\n";
        }
    } else if (choice == "2") {
        std::cout << "검색어 > ";
        std::string keyword;
        std::getline(std::cin, keyword);
        renderRecordTable(repo.searchByNameKeyword(keyword));
    }

    std::cout << "\n대시보드로 돌아가려면 아무 키나 누르세요...\n";
    _getch();
}
```

Note: mixing `_getch()` (used to detect the `s`/`q` keypress in the main loop) with `std::getline(std::cin, ...)` here works because `_getch()` does not consume a trailing newline into the stream the way line-buffered input would — this function starts a fresh `std::cin` read each time, which is safe since `_getch()` never touched `std::cin`'s buffer.

- [ ] **Step 2: Build and verify both search paths**

Build per Task 2 Step 5's command (same file list). Run it, press `s`, choose `1`, enter the ID `1` (or whatever ID currently exists in `records.json`) — expect that record's row. Choose `2` and search a keyword that matches part of the existing record's name — expect a match; search a keyword that matches nothing — expect "표시할 데이터가 없습니다.".

- [ ] **Step 3: Commit**

```bash
git add DataMonitor/DataMonitor/main.cpp
git commit -m "Implement ID and keyword search mode"
```

---

### Task 6: Final verification pass and cleanup

**Files:** none new — this task is a verification/documentation pass over everything built in Tasks 1–5.

- [ ] **Step 1: End-to-end manual scenario**

With DataMonitor running (built per Task 2 Step 5's command), in a second terminal run CrudApp (`../DataPersistence/CrudApp`) and: add a record, update a record, delete a record. After each action, confirm DataMonitor's dashboard auto-refreshes within ~2 seconds showing the new state and an updated "마지막 변경 감지" timestamp.

- [ ] **Step 2: Missing-file scenario**

Temporarily rename `../DataPersistence/CrudApp/data/records.json` to `records.json.bak`, confirm DataMonitor shows the "⚠ 데이터 파일을 찾을 수 없습니다" warning without crashing, then rename it back and confirm DataMonitor recovers automatically on the next poll.

- [ ] **Step 3: Remove any leftover build artifacts, verify `.gitignore` covers them**

```bash
git status
```

Expected: no untracked `.exe`/`.obj` files (covered by `.gitignore`). If any appear, confirm they match a `.gitignore` pattern; if not, add the pattern.

- [ ] **Step 4: Final commit**

```bash
git add -A
git commit -m "Complete DataMonitor PoC: verified end-to-end against live CrudApp"
```
