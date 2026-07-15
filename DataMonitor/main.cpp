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
