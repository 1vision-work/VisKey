// SPDX-License-Identifier: GPL-3.0-or-later
//
// VisKey for Windows (by 1VISION) — based on OpenKey by Mai Vũ Tuyên.

#include "config.h"

#include "json.h"

#include <algorithm>

namespace viskey {

namespace {

struct Named { int value; const char* name; };

// Simple Telex maps to OpenKey's "Simple Telex 1"; "simple_telex_2" is still accepted.
const Named kInputTypes[] = {{0, "telex"}, {1, "vni"}, {2, "simple_telex"}, {3, "simple_telex_2"}};
const Named kCodeTables[] = {
    {0, "unicode"}, {1, "tcvn3"}, {2, "vni_windows"}, {3, "unicode_compound"}, {4, "cp1258"}};

template <size_t N>
const char* NameOf(const Named (&table)[N], int value) {
    for (const auto& n : table)
        if (n.value == value) return n.name;
    return table[0].name;
}

template <size_t N>
int ValueOf(const Named (&table)[N], const std::string& name, int fallback) {
    for (const auto& n : table)
        if (name == n.name) return n.value;
    return fallback;
}

// The bool options, in the order they are written.
struct BoolField { const char* key; bool Config::*field; };
const BoolField kBools[] = {
    {"vietnamese", &Config::vietnamese},
    {"checkSpelling", &Config::checkSpelling},
    {"modernOrthography", &Config::modernOrthography},
    {"quickTelex", &Config::quickTelex},
    {"restoreIfWrongSpelling", &Config::restoreIfWrongSpelling},
    {"freeMark", &Config::freeMark},
    {"upperCaseFirstChar", &Config::upperCaseFirstChar},
    {"allowConsonantZFWJ", &Config::allowConsonantZFWJ},
    {"quickStartConsonant", &Config::quickStartConsonant},
    {"quickEndConsonant", &Config::quickEndConsonant},
    {"useMacro", &Config::useMacro},
    {"macroInEnglishMode", &Config::macroInEnglishMode},
    {"autoCapsMacro", &Config::autoCapsMacro},
    {"fixBrowserAutocomplete", &Config::fixBrowserAutocomplete},
};

}  // namespace

bool Config::operator==(const Config& o) const {
    for (const auto& b : kBools)
        if (this->*b.field != o.*b.field) return false;
    return inputType == o.inputType && codeTable == o.codeTable && hotkey == o.hotkey && macros == o.macros;
}

const char* InputTypeName(int v) { return NameOf(kInputTypes, v); }
const char* CodeTableName(int v) { return NameOf(kCodeTables, v); }
int InputTypeFromName(const std::string& n, int fallback) { return ValueOf(kInputTypes, n, fallback); }
int CodeTableFromName(const std::string& n, int fallback) { return ValueOf(kCodeTables, n, fallback); }

std::string ConfigToJson(const Config& c) {
    json::Value root = json::Value::Object();
    root.Set("version", json::Value::Number(1));
    root.Set("inputType", json::Value::String(InputTypeName(c.inputType)));
    root.Set("codeTable", json::Value::String(CodeTableName(c.codeTable)));
    for (const auto& b : kBools) root.Set(b.key, json::Value::Bool(c.*b.field));
    root.Set("hotkey", json::Value::String(c.hotkey == ToggleHotkey::AltZ ? "alt+z" : "ctrl+shift"));
    json::Value macros = json::Value::Array();
    for (const auto& m : c.macros) {
        json::Value item = json::Value::Object();
        item.Set("abbreviation", json::Value::String(m.abbreviation));
        item.Set("expansion", json::Value::String(m.expansion));
        macros.Push(std::move(item));
    }
    root.Set("macros", std::move(macros));
    return json::Serialize(root);
}

bool ConfigFromJson(const std::string& text, Config& out, std::string* error) {
    out = Config();
    json::Value root;
    if (!json::Parse(text, root, error)) return false;
    if (!root.IsObject()) {
        if (error) *error = "top level is not an object";
        return false;
    }
    if (const auto* v = root.Find("inputType"); v && v->IsString())
        out.inputType = InputTypeFromName(v->string, out.inputType);
    if (const auto* v = root.Find("codeTable"); v && v->IsString())
        out.codeTable = CodeTableFromName(v->string, out.codeTable);
    for (const auto& b : kBools)
        if (const auto* v = root.Find(b.key); v && v->IsBool()) out.*b.field = v->boolean;
    if (const auto* v = root.Find("hotkey"); v && v->IsString())
        out.hotkey = v->string == "alt+z" ? ToggleHotkey::AltZ : ToggleHotkey::CtrlShift;
    if (const auto* v = root.Find("macros"); v && v->IsArray()) {
        for (const auto& item : v->array) {
            const auto* a = item.Find("abbreviation");
            const auto* e = item.Find("expansion");
            if (a && e && a->IsString() && e->IsString()) UpsertMacro(out.macros, Macro{a->string, e->string});
        }
    }
    return true;
}

bool UpsertMacro(std::vector<Macro>& macros, const Macro& m) {
    if (m.abbreviation.empty() || m.expansion.empty()) return false;
    for (auto& existing : macros) {
        if (existing.abbreviation == m.abbreviation) {
            existing.expansion = m.expansion;
            return true;
        }
    }
    macros.push_back(m);
    return true;
}

bool RemoveMacro(std::vector<Macro>& macros, const std::string& abbreviation) {
    const auto it = std::find_if(macros.begin(), macros.end(),
                                 [&](const Macro& m) { return m.abbreviation == abbreviation; });
    if (it == macros.end()) return false;
    macros.erase(it);
    return true;
}

static std::wstring JoinPath(std::wstring dir, const wchar_t* leaf) {
    if (!dir.empty() && dir.back() != L'\\' && dir.back() != L'/') dir += L'\\';
    return dir + leaf;
}

std::wstring ChooseConfigPath(const std::wstring& exeDir, bool exeDirWritable, const std::wstring& appDataDir) {
    if (exeDirWritable || appDataDir.empty()) return JoinPath(exeDir, L"config.json");
    return JoinPath(appDataDir, L"VisKey\\config.json");
}

}  // namespace viskey
