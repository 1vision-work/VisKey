// SPDX-License-Identifier: GPL-3.0-or-later
//
// VisKey for Windows (by 1VISION) — based on OpenKey by Mai Vũ Tuyên.

#include "json.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace viskey {
namespace json {

Value Value::Bool(bool b) { Value v; v.type = Type::Bool; v.boolean = b; return v; }
Value Value::Number(double n) { Value v; v.type = Type::Number; v.number = n; return v; }
Value Value::String(std::string s) { Value v; v.type = Type::String; v.string = std::move(s); return v; }
Value Value::Array() { Value v; v.type = Type::Array; return v; }
Value Value::Object() { Value v; v.type = Type::Object; return v; }

const Value* Value::Find(const std::string& key) const {
    if (type != Type::Object) return nullptr;
    for (const auto& kv : object)
        if (kv.first == key) return &kv.second;
    return nullptr;
}

void Value::Set(const std::string& key, Value v) {
    type = Type::Object;
    for (auto& kv : object) {
        if (kv.first == key) { kv.second = std::move(v); return; }
    }
    object.emplace_back(key, std::move(v));
}

// ---- parser ---------------------------------------------------------------------------------

namespace {

constexpr int kMaxDepth = 64;

class Parser {
public:
    explicit Parser(const std::string& s) : s_(s) {}

    bool Document(Value& out) {
        SkipBom();
        SkipSpace();
        if (!ParseValue(out, 0)) return false;
        SkipSpace();
        if (pos_ != s_.size()) return Fail("trailing characters");
        return true;
    }

    std::string error;

private:
    const std::string& s_;
    size_t pos_ = 0;

    bool Fail(const char* what) {
        if (error.empty()) error = std::string(what) + " at offset " + std::to_string(pos_);
        return false;
    }

    void SkipBom() {
        if (s_.size() >= 3 && (unsigned char)s_[0] == 0xEF && (unsigned char)s_[1] == 0xBB &&
            (unsigned char)s_[2] == 0xBF)
            pos_ = 3;
    }

    void SkipSpace() {
        while (pos_ < s_.size() && (s_[pos_] == ' ' || s_[pos_] == '\t' || s_[pos_] == '\n' || s_[pos_] == '\r'))
            pos_++;
    }

    bool Literal(const char* word) {
        size_t n = 0;
        while (word[n]) n++;
        if (s_.compare(pos_, n, word) != 0) return Fail("invalid literal");
        pos_ += n;
        return true;
    }

    bool ParseValue(Value& out, int depth) {
        if (depth > kMaxDepth) return Fail("nesting too deep");
        if (pos_ >= s_.size()) return Fail("unexpected end");
        const char c = s_[pos_];
        switch (c) {
        case '{': return ParseObject(out, depth);
        case '[': return ParseArray(out, depth);
        case '"': out = Value(); out.type = Value::Type::String; return ParseString(out.string);
        case 't': out = Value::Bool(true); return Literal("true");
        case 'f': out = Value::Bool(false); return Literal("false");
        case 'n': out = Value(); return Literal("null");
        default:
            if (c == '-' || (c >= '0' && c <= '9')) return ParseNumber(out);
            return Fail("unexpected character");
        }
    }

    bool ParseNumber(Value& out) {
        const size_t start = pos_;
        if (s_[pos_] == '-') pos_++;
        auto digits = [&] {
            size_t n = 0;
            while (pos_ < s_.size() && s_[pos_] >= '0' && s_[pos_] <= '9') { pos_++; n++; }
            return n;
        };
        if (digits() == 0) return Fail("invalid number");
        if (pos_ < s_.size() && s_[pos_] == '.') {
            pos_++;
            if (digits() == 0) return Fail("invalid number");
        }
        if (pos_ < s_.size() && (s_[pos_] == 'e' || s_[pos_] == 'E')) {
            pos_++;
            if (pos_ < s_.size() && (s_[pos_] == '+' || s_[pos_] == '-')) pos_++;
            if (digits() == 0) return Fail("invalid number");
        }
        // strtod is locale dependent; config.json is written by us with '.' and the C locale is never changed.
        out = Value::Number(std::strtod(s_.substr(start, pos_ - start).c_str(), nullptr));
        return true;
    }

    bool Hex4(unsigned& cp) {
        if (pos_ + 4 > s_.size()) return Fail("invalid \\u escape");
        cp = 0;
        for (int i = 0; i < 4; i++) {
            const char c = s_[pos_++];
            cp <<= 4;
            if (c >= '0' && c <= '9') cp |= unsigned(c - '0');
            else if (c >= 'a' && c <= 'f') cp |= unsigned(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') cp |= unsigned(c - 'A' + 10);
            else return Fail("invalid \\u escape");
        }
        return true;
    }

    static void AppendUtf8(std::string& out, unsigned cp) {
        if (cp < 0x80) {
            out += char(cp);
        } else if (cp < 0x800) {
            out += char(0xC0 | (cp >> 6));
            out += char(0x80 | (cp & 0x3F));
        } else if (cp < 0x10000) {
            out += char(0xE0 | (cp >> 12));
            out += char(0x80 | ((cp >> 6) & 0x3F));
            out += char(0x80 | (cp & 0x3F));
        } else {
            out += char(0xF0 | (cp >> 18));
            out += char(0x80 | ((cp >> 12) & 0x3F));
            out += char(0x80 | ((cp >> 6) & 0x3F));
            out += char(0x80 | (cp & 0x3F));
        }
    }

    bool ParseString(std::string& out) {
        pos_++;  // opening quote
        out.clear();
        while (true) {
            if (pos_ >= s_.size()) return Fail("unterminated string");
            const char c = s_[pos_++];
            if (c == '"') return true;
            if ((unsigned char)c < 0x20) return Fail("control character in string");
            if (c != '\\') { out += c; continue; }
            if (pos_ >= s_.size()) return Fail("unterminated string");
            const char e = s_[pos_++];
            switch (e) {
            case '"': out += '"'; break;
            case '\\': out += '\\'; break;
            case '/': out += '/'; break;
            case 'b': out += '\b'; break;
            case 'f': out += '\f'; break;
            case 'n': out += '\n'; break;
            case 'r': out += '\r'; break;
            case 't': out += '\t'; break;
            case 'u': {
                unsigned cp = 0;
                if (!Hex4(cp)) return false;
                if (cp >= 0xD800 && cp <= 0xDBFF) {
                    unsigned lo = 0;
                    if (s_.compare(pos_, 2, "\\u") != 0) return Fail("lone surrogate");
                    pos_ += 2;
                    if (!Hex4(lo)) return false;
                    if (lo < 0xDC00 || lo > 0xDFFF) return Fail("lone surrogate");
                    cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                } else if (cp >= 0xDC00 && cp <= 0xDFFF) {
                    return Fail("lone surrogate");
                }
                AppendUtf8(out, cp);
                break;
            }
            default: return Fail("invalid escape");
            }
        }
    }

    bool ParseArray(Value& out, int depth) {
        pos_++;
        out = Value::Array();
        SkipSpace();
        if (pos_ < s_.size() && s_[pos_] == ']') { pos_++; return true; }
        while (true) {
            Value item;
            SkipSpace();
            if (!ParseValue(item, depth + 1)) return false;
            out.array.push_back(std::move(item));
            SkipSpace();
            if (pos_ >= s_.size()) return Fail("unterminated array");
            if (s_[pos_] == ',') { pos_++; continue; }
            if (s_[pos_] == ']') { pos_++; return true; }
            return Fail("expected ',' or ']'");
        }
    }

    bool ParseObject(Value& out, int depth) {
        pos_++;
        out = Value::Object();
        SkipSpace();
        if (pos_ < s_.size() && s_[pos_] == '}') { pos_++; return true; }
        while (true) {
            SkipSpace();
            if (pos_ >= s_.size() || s_[pos_] != '"') return Fail("expected key");
            std::string key;
            if (!ParseString(key)) return false;
            SkipSpace();
            if (pos_ >= s_.size() || s_[pos_] != ':') return Fail("expected ':'");
            pos_++;
            SkipSpace();
            Value item;
            if (!ParseValue(item, depth + 1)) return false;
            out.Set(key, std::move(item));
            SkipSpace();
            if (pos_ >= s_.size()) return Fail("unterminated object");
            if (s_[pos_] == ',') { pos_++; continue; }
            if (s_[pos_] == '}') { pos_++; return true; }
            return Fail("expected ',' or '}'");
        }
    }
};

// ---- writer ---------------------------------------------------------------------------------

void WriteString(std::string& out, const std::string& s) {
    out += '"';
    for (const char c : s) {
        switch (c) {
        case '"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            if ((unsigned char)c < 0x20) {
                char buf[8];
                std::snprintf(buf, sizeof buf, "\\u%04x", (unsigned char)c);
                out += buf;
            } else {
                out += c;
            }
        }
    }
    out += '"';
}

void WriteValue(std::string& out, const Value& v, int indent) {
    const std::string pad(size_t(indent + 2), ' ');
    const std::string padEnd(size_t(indent), ' ');
    switch (v.type) {
    case Value::Type::Null: out += "null"; break;
    case Value::Type::Bool: out += v.boolean ? "true" : "false"; break;
    case Value::Type::Number: {
        char buf[32];
        if (std::isfinite(v.number) && v.number == std::floor(v.number) && std::fabs(v.number) < 1e15)
            std::snprintf(buf, sizeof buf, "%.0f", v.number);
        else if (std::isfinite(v.number))
            std::snprintf(buf, sizeof buf, "%.17g", v.number);
        else
            std::snprintf(buf, sizeof buf, "null");
        out += buf;
        break;
    }
    case Value::Type::String: WriteString(out, v.string); break;
    case Value::Type::Array:
        if (v.array.empty()) { out += "[]"; break; }
        out += "[\n";
        for (size_t i = 0; i < v.array.size(); i++) {
            out += pad;
            WriteValue(out, v.array[i], indent + 2);
            out += i + 1 < v.array.size() ? ",\n" : "\n";
        }
        out += padEnd + "]";
        break;
    case Value::Type::Object:
        if (v.object.empty()) { out += "{}"; break; }
        out += "{\n";
        for (size_t i = 0; i < v.object.size(); i++) {
            out += pad;
            WriteString(out, v.object[i].first);
            out += ": ";
            WriteValue(out, v.object[i].second, indent + 2);
            out += i + 1 < v.object.size() ? ",\n" : "\n";
        }
        out += padEnd + "}";
        break;
    }
}

}  // namespace

bool Parse(const std::string& text, Value& out, std::string* error) {
    Parser p(text);
    Value v;
    if (!p.Document(v)) {
        if (error) *error = p.error;
        return false;
    }
    out = std::move(v);
    return true;
}

std::string Serialize(const Value& v) {
    std::string out;
    WriteValue(out, v, 0);
    out += '\n';
    return out;
}

}  // namespace json
}  // namespace viskey
