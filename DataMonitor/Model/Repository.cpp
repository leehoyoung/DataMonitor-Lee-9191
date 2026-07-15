#include "Repository.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <ctime>
#include <filesystem>
#include <iostream>

RecordRepository::RecordRepository(std::string filePath) : filePath_(std::move(filePath)) {}

void RecordRepository::load() {
    records_.clear();

    std::ifstream in(filePath_, std::ios::binary);
    if (!in.is_open()) {
        // 파일이 없으면 빈 목록으로 시작 (최초 실행 시 정상적인 상황)
        return;
    }

    std::ostringstream buffer;
    buffer << in.rdbuf();
    std::string content = buffer.str();

    if (content.find_first_not_of(" \t\r\n") == std::string::npos) {
        // 빈 파일
        return;
    }

    try {
        JsonValue root = JsonValue::parse(content);
        if (root.isArray()) {
            for (const auto& item : root.arrayValue) {
                records_.push_back(Record::fromJson(item));
            }
        }
    } catch (const std::exception& ex) {
        std::cerr << "[경고] JSON 파일을 읽는 중 오류가 발생하여 빈 목록으로 시작합니다: "
                  << ex.what() << "\n";
        records_.clear();
    }
}

void RecordRepository::save() const {
    std::filesystem::path path(filePath_);
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path());
    }

    JsonValue root = JsonValue::makeArray();
    for (const auto& r : records_) {
        root.push_back(r.toJson());
    }

    // 저장 도중 프로세스가 중단되어도 기존 파일이 손상되지 않도록
    // 임시 파일에 먼저 쓴 뒤 원자적으로 rename하여 교체한다.
    std::filesystem::path tmpPath = path;
    tmpPath += ".tmp";

    {
        std::ofstream out(tmpPath, std::ios::binary | std::ios::trunc);
        if (!out.is_open()) {
            throw std::runtime_error("임시 파일에 쓸 수 없습니다: " + tmpPath.string());
        }
        out << root.dump(2);
        out.flush();
        if (!out) {
            throw std::runtime_error("임시 파일 쓰기에 실패했습니다: " + tmpPath.string());
        }
    }

    std::error_code ec;
    std::filesystem::rename(tmpPath, path, ec);
    if (ec) {
        std::filesystem::remove(tmpPath);
        throw std::runtime_error("파일 교체(rename)에 실패했습니다: " + path.string() + " (" + ec.message() + ")");
    }
}

const Record* RecordRepository::findById(int id) const {
    for (const auto& r : records_) {
        if (r.id == id) return &r;
    }
    return nullptr;
}

std::vector<Record> RecordRepository::searchByNameKeyword(const std::string& keyword) const {
    std::string lowerKeyword = keyword;
    std::transform(lowerKeyword.begin(), lowerKeyword.end(), lowerKeyword.begin(),
                    [](unsigned char c) { return std::tolower(c); });

    std::vector<Record> result;
    for (const auto& r : records_) {
        std::string lowerName = r.name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(),
                        [](unsigned char c) { return std::tolower(c); });
        if (lowerName.find(lowerKeyword) != std::string::npos) {
            result.push_back(r);
        }
    }
    return result;
}

int RecordRepository::nextId() const {
    int maxId = 0;
    for (const auto& r : records_) {
        maxId = std::max(maxId, r.id);
    }
    return maxId + 1;
}

std::string RecordRepository::currentTimestamp() {
    std::time_t t = std::time(nullptr);
    std::tm tmBuf{};
    localtime_s(&tmBuf, &t);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tmBuf);
    return std::string(buf);
}

Record RecordRepository::create(const std::string& name, const std::string& description) {
    Record r;
    r.id = nextId();
    r.name = name;
    r.description = description;
    r.createdAt = currentTimestamp();
    records_.push_back(r);
    save();
    return r;
}

bool RecordRepository::updateFields(int id, const std::optional<std::string>& newName,
                                     const std::optional<std::string>& newDescription) {
    for (auto& r : records_) {
        if (r.id == id) {
            if (newName.has_value()) r.name = *newName;
            if (newDescription.has_value()) r.description = *newDescription;
            save();
            return true;
        }
    }
    return false;
}

bool RecordRepository::removeById(int id) {
    auto it = std::find_if(records_.begin(), records_.end(),
                            [id](const Record& r) { return r.id == id; });
    if (it == records_.end()) return false;
    records_.erase(it);
    save();
    return true;
}
