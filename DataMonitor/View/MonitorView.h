#pragma once
#include <string>
#include <vector>
#include "../Model/Record.h"

void clearScreen();
void renderRecordTable(const std::vector<Record>& records);
void renderDashboard(const std::vector<Record>& records, const std::string& lastChangeTime, bool dataFileMissing);
