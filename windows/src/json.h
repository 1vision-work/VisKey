// SPDX-License-Identifier: GPL-3.0-or-later
//
// VisKey for Windows (by 1VISION) — based on OpenKey by Mai Vũ Tuyên.
//
// A tiny JSON reader/writer, just enough for config.json: UTF-8 text in and out, objects keep their
// key order, numbers are doubles. No dependency beyond the standard library, no platform code.

#pragma once

#include <string>
#include <utility>
#include <vector>

namespace viskey {
namespace json {

struct Value {
    enum class Type { Null, Bool, Number, String, Array, Object };

    Type type = Type::Null;
    bool boolean = false;
    double number = 0;
    std::string string;
    std::vector<Value> array;
    std::vector<std::pair<std::string, Value>> object;

    static Value Bool(bool b);
    static Value Number(double n);
    static Value String(std::string s);
    static Value Array();
    static Value Object();

    // Object access. Find returns nullptr if the key is missing or this is not an object.
    const Value* Find(const std::string& key) const;
    // Sets (or replaces) a key of an object.
    void Set(const std::string& key, Value v);
    void Push(Value v) { array.push_back(std::move(v)); }

    bool IsBool() const { return type == Type::Bool; }
    bool IsNumber() const { return type == Type::Number; }
    bool IsString() const { return type == Type::String; }
    bool IsArray() const { return type == Type::Array; }
    bool IsObject() const { return type == Type::Object; }
};

// Parses a complete JSON document. On failure returns false and, if error is given, a short reason
// with the byte offset.
bool Parse(const std::string& text, Value& out, std::string* error = nullptr);

// Pretty-prints with two-space indentation and a trailing newline. Non-ASCII stays UTF-8.
std::string Serialize(const Value& v);

}  // namespace json
}  // namespace viskey
