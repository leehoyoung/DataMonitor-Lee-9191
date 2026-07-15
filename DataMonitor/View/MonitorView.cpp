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
