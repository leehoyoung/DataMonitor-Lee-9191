#pragma once
#include <string>
#include <vector>
#include <optional>
#include "Record.h"

// records.json 파일을 데이터 저장소로 사용하는 CRUD 저장소 클래스
class RecordRepository {
public:
    explicit RecordRepository(std::string filePath);

    // 파일에서 데이터를 읽어 메모리에 적재. 파일이 없으면 빈 목록으로 시작.
    void load();

    // 현재 메모리 상태를 파일에 저장 (pretty-print JSON)
    void save() const;

    const std::vector<Record>& listAll() const { return records_; }

    // id로 검색 (없으면 nullptr)
    const Record* findById(int id) const;

    // 이름에 keyword(대소문자 무시)가 포함된 레코드 검색
    std::vector<Record> searchByNameKeyword(const std::string& keyword) const;

    // 새 레코드 생성 (id, createdAt은 저장소가 자동으로 채움)
    Record create(const std::string& name, const std::string& description);

    // 기존 레코드의 일부 필드 수정. 빈 optional은 해당 필드를 변경하지 않음.
    bool updateFields(int id, const std::optional<std::string>& newName,
                       const std::optional<std::string>& newDescription);

    // id로 레코드 삭제
    bool removeById(int id);

private:
    std::string filePath_;
    std::vector<Record> records_;

    int nextId() const;
    static std::string currentTimestamp();
};
