#include <iostream>
#include <string>
#include <filesystem>
#include <chrono>
#include <ctime>
#include <thread>
#include <conio.h>
#include "Model/Repository.h"
#include "View/MonitorView.h"

void runSearchMode(RecordRepository& repo); // Task 5에서 구현

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
    bool wasMissing = fileMissing;
    fileMissing = false;
    // 파일이 없어졌다가 mtime이 바뀌지 않은 채로 복구된 경우도 화면을 갱신해야 경고가 사라진다.
    if (currentTime != lastSeen || wasMissing) {
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
    std::cout.flush();

    while (true) {
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
                    checkForChange(lastSeen, fileMissing); // 검색 중 파일이 바뀌었을 수도 있으니 재확인
                    lastChangeTime = nowTimestamp();
                    clearScreen();
                    renderDashboard(repo.listAll(), lastChangeTime, fileMissing);
                    std::cout.flush();
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
            std::cout.flush();
        }
    }

    return 0;
}

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
