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
    std::cout.flush();

    while (true) {
        std::this_thread::sleep_for(POLL_INTERVAL);

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
