#include <iostream>
#include "Model/Repository.h"
#include "View/MonitorView.h"

int main() {
    RecordRepository repo("../../DataPersistence/CrudApp/data/records.json");
    repo.load();
    renderDashboard(repo.listAll(), "(초기 로드)", false);
    return 0;
}
