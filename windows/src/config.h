// SPDX-License-Identifier: GPL-3.0-or-later
//
// VisKey for Windows (by 1VISION) — based on OpenKey by Mai Vũ Tuyên.
//
// User settings and their config.json form. Pure C++: no Win32 here, so it is unit-tested on any OS.
// "Run at startup" is not stored here: the HKCU Run registry value is the single source of truth.

#pragma once

#include <string>
#include <vector>

namespace viskey {

struct Macro {
    std::string abbreviation;  // UTF-8
    std::string expansion;     // UTF-8
    bool operator==(const Macro& o) const { return abbreviation == o.abbreviation && expansion == o.expansion; }
};

enum class ToggleHotkey {
    CtrlShift = 0,  // modifier-only: press and release Ctrl+Shift with no other key (OpenKey/UniKey default)
    AltZ = 1,
};

struct Config {
    bool vietnamese = true;
    int inputType = 0;                // vk_input_type: 0 Telex, 1 VNI, 2 Simple Telex
    int codeTable = 0;                // vk_code_table: 0 Unicode, 1 TCVN3, 2 VNI Windows, 3 Unicode compound, 4 CP1258
    bool checkSpelling = true;
    bool modernOrthography = true;    // oà, uý
    bool quickTelex = false;
    bool restoreIfWrongSpelling = true;
    bool freeMark = false;
    bool upperCaseFirstChar = false;
    bool allowConsonantZFWJ = false;
    bool quickStartConsonant = false;
    bool quickEndConsonant = false;
    bool useMacro = true;
    bool macroInEnglishMode = false;
    bool autoCapsMacro = true;
    // OpenKey "fix recommend browser": type an invisible U+202F and delete it first, so an inline
    // autocomplete suggestion (address bars, search boxes) is dismissed before the word is replaced.
    bool fixBrowserAutocomplete = true;
    ToggleHotkey hotkey = ToggleHotkey::CtrlShift;
    std::vector<Macro> macros;

    bool operator==(const Config& o) const;
    bool operator!=(const Config& o) const { return !(*this == o); }
};

// Name <-> value tables used in config.json ("telex", "unicode_compound", ...).
const char* InputTypeName(int inputType);
const char* CodeTableName(int codeTable);
int InputTypeFromName(const std::string& name, int fallback);
int CodeTableFromName(const std::string& name, int fallback);

std::string ConfigToJson(const Config& c);

// Reads config.json text. Unknown keys are ignored, missing or invalid keys keep their defaults.
// Returns false (and leaves `out` at defaults) only if the text is not valid JSON.
bool ConfigFromJson(const std::string& text, Config& out, std::string* error = nullptr);

// Adds or replaces (by abbreviation) a macro. Returns false if the abbreviation or expansion is empty.
bool UpsertMacro(std::vector<Macro>& macros, const Macro& m);
bool RemoveMacro(std::vector<Macro>& macros, const std::string& abbreviation);

// Where config.json lives (see windows/README.md):
//   - next to VisKey.exe if that folder is writable (portable mode, like UniKey);
//   - otherwise %APPDATA%\VisKey\config.json (e.g. installed under Program Files).
std::wstring ChooseConfigPath(const std::wstring& exeDir, bool exeDirWritable, const std::wstring& appDataDir);

}  // namespace viskey
