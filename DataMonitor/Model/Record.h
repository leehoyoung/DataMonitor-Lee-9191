#pragma once
#include <string>
#include "Json.h"

// 콘솔 CRUD 앱이 다루는 데이터 모델
struct Record {
    int id = 0;
    std::string name;
    std::string description;
    std::string createdAt; // "YYYY-MM-DD HH:MM:SS"

    JsonValue toJson() const {
        JsonValue obj = JsonValue::makeObject();
        obj.set("id", JsonValue::makeInt(id));
        obj.set("name", JsonValue::makeString(name));
        obj.set("description", JsonValue::makeString(description));
        obj.set("createdAt", JsonValue::makeString(createdAt));
        return obj;
    }

    static Record fromJson(const JsonValue& obj) {
        Record r;
        if (const JsonValue* v = obj.find("id")) r.id = static_cast<int>(v->asInt());
        if (const JsonValue* v = obj.find("name")) r.name = v->asString();
        if (const JsonValue* v = obj.find("description")) r.description = v->asString();
        if (const JsonValue* v = obj.find("createdAt")) r.createdAt = v->asString();
        return r;
    }
};
