// SPDX-License-Identifier: GPL-3.0-or-later
// VisKey engine test harness: abstract driver so the same cases run against
// the original OpenKey engine (globals) and the refactored vk_engine C API.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace vktest {

struct Config {
    int inputType = 0;            // 0 Telex, 1 VNI, 2 Simple Telex 1, 3 Simple Telex 2
    int codeTable = 0;            // 0 Unicode, 1 TCVN3, 2 VNI Windows, 3 Unicode compound, 4 CP1258
    int language = 1;             // 1 Vietnamese, 0 English
    int checkSpelling = 1;
    int modernOrthography = 0;    // 0: òa úy, 1: oà uý
    int quickTelex = 0;
    int restoreIfWrongSpelling = 0;
    int useMacro = 0;
    int macroInEnglishMode = 0;
    int autoCapsMacro = 0;
    int upperCaseFirstChar = 0;
    int allowConsonantZFWJ = 0;
    int quickStartConsonant = 0;
    int quickEndConsonant = 0;
    int freeMark = 0;
};

// Result of one key event. `words` and `macro` are engine words in DISPLAY order.
struct Step {
    int code = 0;      // HoolCodeState
    int backspace = 0;
    int newChars = 0;
    int ext = 0;
    std::vector<uint32_t> words;
    std::vector<uint32_t> macro;
};

struct ConvertOptions {
    int from = 0, to = 0;
    bool allCaps = false, allLower = false, capsFirst = false, capsEachWord = false, removeMark = false;
};

class Driver {
public:
    virtual ~Driver() = default;
    // Start a fresh engine with this configuration.
    virtual void begin(const Config&) = 0;
    virtual void addMacro(const std::string& text, const std::string& content) = 0;
    virtual Step key(uint16_t keycode, int caps, bool otherControl) = 0;
    virtual Step englishKey(uint16_t keycode, int caps, bool otherControl) = 0;
    virtual void mouse() = 0;
    virtual void tempOffSpelling() = 0;
    virtual void tempOffEngine() = 0;
    // Same logic as OpenKey.mm SendNewCharString: engine word -> UTF-16 units of the current code table.
    virtual void decodeWord(uint32_t word, std::vector<uint16_t>& units) = 0;
    virtual uint16_t keyChar(uint32_t keycodeWithCaps) = 0;
    virtual std::string convert(const ConvertOptions&, const std::string& utf8) = 0;
    // Smart switch: set then query; returns query results in order.
    virtual std::vector<int> smartSwitch(const std::vector<std::pair<std::string, int>>& sets,
                                         const std::vector<std::string>& queries, int current) = 0;
    // Macro persistence round trip: returns serialized bytes after adding macros.
    virtual std::vector<uint8_t> macroBlobRoundTrip() = 0;
};

} // namespace vktest
