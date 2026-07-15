#include "Json.h"
#include <sstream>
#include <cctype>
#include <cmath>

JsonValue JsonValue::makeObject() {
    JsonValue v;
    v.type = Type::Object;
    return v;
}

JsonValue JsonValue::makeArray() {
    JsonValue v;
    v.type = Type::Array;
    return v;
}

JsonValue JsonValue::makeString(const std::string& s) {
    JsonValue v;
    v.type = Type::String;
    v.stringValue = s;
    return v;
}

JsonValue JsonValue::makeInt(long long n) {
    JsonValue v;
    v.type = Type::Number;
    v.numberValue = static_cast<double>(n);
    return v;
}

const JsonValue* JsonValue::find(const std::string& key) const {
    for (const auto& kv : objectValue) {
        if (kv.first == key) return &kv.second;
    }
    return nullptr;
}

void JsonValue::set(const std::string& key, JsonValue value) {
    for (auto& kv : objectValue) {
        if (kv.first == key) {
            kv.second = std::move(value);
            return;
        }
    }
    objectValue.emplace_back(key, std::move(value));
}

void JsonValue::push_back(JsonValue value) {
    arrayValue.push_back(std::move(value));
}

std::string JsonValue::asString(const std::string& defaultValue) const {
    if (type == Type::String) return stringValue;
    return defaultValue;
}

long long JsonValue::asInt(long long defaultValue) const {
    if (type == Type::Number) return static_cast<long long>(std::llround(numberValue));
    return defaultValue;
}

namespace {
    void escapeStringInto(std::string& out, const std::string& s) {
        out += '"';
        for (char c : s) {
            switch (c) {
                case '"': out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default:
                    if (static_cast<unsigned char>(c) < 0x20) {
                        char buf[8];
                        std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                        out += buf;
                    } else {
                        out += c;
                    }
            }
        }
        out += '"';
    }
}

void JsonValue::dumpInternal(std::string& out, int indentSize, int currentIndent) const {
    std::string pad(currentIndent, ' ');
    std::string childPad(currentIndent + indentSize, ' ');

    switch (type) {
        case Type::Null:
            out += "null";
            break;
        case Type::Bool:
            out += boolValue ? "true" : "false";
            break;
        case Type::Number: {
            // 정수인 경우 소수점 없이 출력
            if (numberValue == std::floor(numberValue)) {
                out += std::to_string(static_cast<long long>(numberValue));
            } else {
                std::ostringstream oss;
                oss << numberValue;
                out += oss.str();
            }
            break;
        }
        case Type::String:
            escapeStringInto(out, stringValue);
            break;
        case Type::Array: {
            if (arrayValue.empty()) {
                out += "[]";
                break;
            }
            out += "[\n";
            for (size_t i = 0; i < arrayValue.size(); ++i) {
                out += childPad;
                arrayValue[i].dumpInternal(out, indentSize, currentIndent + indentSize);
                if (i + 1 < arrayValue.size()) out += ",";
                out += "\n";
            }
            out += pad + "]";
            break;
        }
        case Type::Object: {
            if (objectValue.empty()) {
                out += "{}";
                break;
            }
            out += "{\n";
            for (size_t i = 0; i < objectValue.size(); ++i) {
                out += childPad;
                escapeStringInto(out, objectValue[i].first);
                out += ": ";
                objectValue[i].second.dumpInternal(out, indentSize, currentIndent + indentSize);
                if (i + 1 < objectValue.size()) out += ",";
                out += "\n";
            }
            out += pad + "}";
            break;
        }
    }
}

std::string JsonValue::dump(int indentSize) const {
    std::string out;
    dumpInternal(out, indentSize, 0);
    return out;
}

namespace {
    class Parser {
    public:
        explicit Parser(const std::string& text) : text_(text), pos_(0) {}

        JsonValue parse() {
            skipWhitespace();
            JsonValue v = parseValue();
            skipWhitespace();
            if (pos_ != text_.size()) {
                throw std::runtime_error("JSON 파싱 오류: 문자열 끝에 불필요한 데이터가 있습니다.");
            }
            return v;
        }

    private:
        const std::string& text_;
        size_t pos_;

        char peek() {
            if (pos_ >= text_.size()) throw std::runtime_error("JSON 파싱 오류: 예상치 못한 문자열 끝");
            return text_[pos_];
        }

        char next() {
            char c = peek();
            ++pos_;
            return c;
        }

        void skipWhitespace() {
            while (pos_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[pos_]))) {
                ++pos_;
            }
        }

        void expect(char c) {
            if (next() != c) {
                throw std::runtime_error(std::string("JSON 파싱 오류: '") + c + "' 문자가 필요합니다.");
            }
        }

        JsonValue parseValue() {
            skipWhitespace();
            char c = peek();
            if (c == '{') return parseObject();
            if (c == '[') return parseArray();
            if (c == '"') return parseString();
            if (c == 't' || c == 'f') return parseBool();
            if (c == 'n') return parseNull();
            return parseNumber();
        }

        JsonValue parseObject() {
            JsonValue v = JsonValue::makeObject();
            expect('{');
            skipWhitespace();
            if (peek() == '}') { next(); return v; }
            while (true) {
                skipWhitespace();
                JsonValue key = parseString();
                skipWhitespace();
                expect(':');
                JsonValue value = parseValue();
                v.set(key.stringValue, value);
                skipWhitespace();
                char c = next();
                if (c == ',') continue;
                if (c == '}') break;
                throw std::runtime_error("JSON 파싱 오류: 객체 형식이 올바르지 않습니다.");
            }
            return v;
        }

        JsonValue parseArray() {
            JsonValue v = JsonValue::makeArray();
            expect('[');
            skipWhitespace();
            if (peek() == ']') { next(); return v; }
            while (true) {
                JsonValue value = parseValue();
                v.push_back(value);
                skipWhitespace();
                char c = next();
                if (c == ',') continue;
                if (c == ']') break;
                throw std::runtime_error("JSON 파싱 오류: 배열 형식이 올바르지 않습니다.");
            }
            return v;
        }

        JsonValue parseString() {
            expect('"');
            std::string result;
            while (true) {
                char c = next();
                if (c == '"') break;
                if (c == '\\') {
                    char esc = next();
                    switch (esc) {
                        case '"': result += '"'; break;
                        case '\\': result += '\\'; break;
                        case '/': result += '/'; break;
                        case 'n': result += '\n'; break;
                        case 't': result += '\t'; break;
                        case 'r': result += '\r'; break;
                        case 'b': result += '\b'; break;
                        case 'f': result += '\f'; break;
                        case 'u': {
                            // 유니코드 이스케이프는 그대로 4자리 hex를 UTF-8로 단순 변환하지 않고
                            // 원문 코드포인트를 최대한 보존하기 위해 raw 그대로 남김(간단 구현 한계).
                            std::string hex = text_.substr(pos_, 4);
                            pos_ += 4;
                            unsigned int code = static_cast<unsigned int>(std::stoul(hex, nullptr, 16));
                            // 기본 다국어 평면(BMP) 범위를 UTF-8로 인코딩
                            if (code < 0x80) {
                                result += static_cast<char>(code);
                            } else if (code < 0x800) {
                                result += static_cast<char>(0xC0 | (code >> 6));
                                result += static_cast<char>(0x80 | (code & 0x3F));
                            } else {
                                result += static_cast<char>(0xE0 | (code >> 12));
                                result += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
                                result += static_cast<char>(0x80 | (code & 0x3F));
                            }
                            break;
                        }
                        default:
                            throw std::runtime_error("JSON 파싱 오류: 알 수 없는 이스케이프 문자");
                    }
                } else {
                    result += c;
                }
            }
            return JsonValue::makeString(result);
        }

        JsonValue parseBool() {
            if (text_.compare(pos_, 4, "true") == 0) {
                pos_ += 4;
                JsonValue v;
                v.type = JsonValue::Type::Bool;
                v.boolValue = true;
                return v;
            }
            if (text_.compare(pos_, 5, "false") == 0) {
                pos_ += 5;
                JsonValue v;
                v.type = JsonValue::Type::Bool;
                v.boolValue = false;
                return v;
            }
            throw std::runtime_error("JSON 파싱 오류: bool 값이 올바르지 않습니다.");
        }

        JsonValue parseNull() {
            if (text_.compare(pos_, 4, "null") == 0) {
                pos_ += 4;
                return JsonValue();
            }
            throw std::runtime_error("JSON 파싱 오류: null 값이 올바르지 않습니다.");
        }

        JsonValue parseNumber() {
            size_t start = pos_;
            if (peek() == '-') next();
            while (pos_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[pos_]))) next();
            if (pos_ < text_.size() && text_[pos_] == '.') {
                next();
                while (pos_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[pos_]))) next();
            }
            if (pos_ < text_.size() && (text_[pos_] == 'e' || text_[pos_] == 'E')) {
                next();
                if (pos_ < text_.size() && (text_[pos_] == '+' || text_[pos_] == '-')) next();
                while (pos_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[pos_]))) next();
            }
            std::string numStr = text_.substr(start, pos_ - start);
            if (numStr.empty()) throw std::runtime_error("JSON 파싱 오류: 숫자 형식이 올바르지 않습니다.");
            JsonValue v;
            v.type = JsonValue::Type::Number;
            v.numberValue = std::stod(numStr);
            return v;
        }
    };
}

JsonValue JsonValue::parse(const std::string& text) {
    Parser parser(text);
    return parser.parse();
}
