#pragma once
// 최소 기능의 범용 JSON 파서/직렬화 라이브러리 (외부 의존성 없음)

#include <string>
#include <vector>
#include <utility>
#include <stdexcept>

class JsonValue {
public:
    enum class Type { Null, Bool, Number, String, Array, Object };

    Type type = Type::Null;
    bool boolValue = false;
    double numberValue = 0.0;
    std::string stringValue;
    std::vector<JsonValue> arrayValue;
    std::vector<std::pair<std::string, JsonValue>> objectValue; // 입력 순서 보존

    JsonValue() = default;

    static JsonValue makeObject();
    static JsonValue makeArray();
    static JsonValue makeString(const std::string& s);
    static JsonValue makeInt(long long v);

    bool isNull() const { return type == Type::Null; }
    bool isObject() const { return type == Type::Object; }
    bool isArray() const { return type == Type::Array; }

    // Object 전용: 키로 값 찾기 (없으면 nullptr)
    const JsonValue* find(const std::string& key) const;

    // Object 전용: 키에 값 설정(없으면 추가, 있으면 갱신)
    void set(const std::string& key, JsonValue value);

    // Array 전용: 원소 추가
    void push_back(JsonValue value);

    std::string asString(const std::string& defaultValue = "") const;
    long long asInt(long long defaultValue = 0) const;

    // 들여쓰기가 적용된 JSON 문자열로 직렬화
    std::string dump(int indentSize = 2) const;

    // JSON 문자열을 파싱하여 JsonValue로 변환 (실패 시 std::runtime_error)
    static JsonValue parse(const std::string& text);

private:
    void dumpInternal(std::string& out, int indentSize, int currentIndent) const;
};
