// SPDX-License-Identifier: GPL-3.0-or-later
// Driver for the ORIGINAL (unrefactored) OpenKey engine. Defines the host globals the way the
// OpenKey macOS app does, and drives the engine exactly like OpenKey.mm.
// Must be built against the engine sources as imported (see Engine/tests/CMakeLists.txt).
#include "driver_original.h"
#include "Engine.h" // resolved from Engine/tests/original (frozen original engine)

int vLanguage = 1;
int vInputType = 0;
int vFreeMark = 0;
int vCodeTable = 0;
int vSwitchKeyStatus = 0;
int vCheckSpelling = 1;
int vUseModernOrthography = 0;
int vQuickTelex = 0;
int vRestoreIfWrongSpelling = 0;
int vFixRecommendBrowser = 0;
int vUseMacro = 0;
int vUseMacroInEnglishMode = 0;
int vAutoCapsMacro = 0;
int vUseSmartSwitchKey = 0;
int vUpperCaseFirstChar = 0;
int vTempOffSpelling = 0;
int vAllowConsonantZFWJ = 0;
int vQuickStartConsonant = 0;
int vQuickEndConsonant = 0;
int vRememberCode = 0;
int vOtherLanguage = 0;
int vTempOffOpenKey = 0;

namespace vktest {

// backspace/newChars are only meaningful for the codes that process text; the engine leaves stale
// values for the others, which are not part of the contract (and not exposed by the C API).
static Step toStep(const vKeyHookState* h) {
    Step s;
    s.code = h->code;
    s.ext = h->extCode;
    if (h->code == vWillProcess || h->code == vRestore || h->code == vRestoreAndStartNewSession) {
        s.backspace = h->backspaceCount;
        s.newChars = h->newCharCount;
        for (int i = h->newCharCount - 1; i >= 0; i--) s.words.push_back(h->charData[i]);
    } else if (h->code == vReplaceMaro) {
        s.backspace = h->backspaceCount;
        s.newChars = (int)h->macroData.size();
        s.macro.assign(h->macroData.begin(), h->macroData.end());
    }
    return s;
}

void OriginalDriver::begin(const Config& c) {
    vLanguage = c.language;
    vInputType = c.inputType;
    vCodeTable = c.codeTable;
    vFreeMark = c.freeMark;
    vCheckSpelling = c.checkSpelling;
    vUseModernOrthography = c.modernOrthography;
    vQuickTelex = c.quickTelex;
    vRestoreIfWrongSpelling = c.restoreIfWrongSpelling;
    vUseMacro = c.useMacro;
    vUseMacroInEnglishMode = c.macroInEnglishMode;
    vAutoCapsMacro = c.autoCapsMacro;
    vUpperCaseFirstChar = c.upperCaseFirstChar;
    vAllowConsonantZFWJ = c.allowConsonantZFWJ;
    vQuickStartConsonant = c.quickStartConsonant;
    vQuickEndConsonant = c.quickEndConsonant;
    hook_ = (vKeyHookState*)vKeyInit();
}

void OriginalDriver::addMacro(const std::string& text, const std::string& content) { ::addMacro(text, content); }

Step OriginalDriver::key(uint16_t keycode, int caps, bool other) {
    vKeyHandleEvent(vKeyEvent::Keyboard, vKeyEventState::KeyDown, keycode, caps ? 1 : 0, other);
    return toStep(hook_);
}

// Like OpenKey.mm: in English mode only macros are handled, and only when enabled.
Step OriginalDriver::englishKey(uint16_t keycode, int caps, bool other) {
    if (!(vUseMacro && vUseMacroInEnglishMode)) return Step();
    vEnglishMode(vKeyEventState::KeyDown, keycode, caps != 0, other);
    return toStep(hook_);
}

void OriginalDriver::mouse() { vKeyHandleEvent(vKeyEvent::Mouse, vKeyEventState::MouseDown, 0); }
void OriginalDriver::tempOffSpelling() { vTempOffSpellChecking(); }
void OriginalDriver::tempOffEngine() { vTempOffEngine(); }

// Mirrors OpenKey.mm SendNewCharString (units only; _syncKey bookkeeping is not simulated here).
void OriginalDriver::decodeWord(uint32_t word, std::vector<uint16_t>& units) {
    units.clear();
    if (word & PURE_CHARACTER_MASK) {
        units.push_back((uint16_t)word);
    } else if (!(word & CHAR_CODE_MASK)) {
        { uint16_t ch = keyCodeToCharacter(word); if (ch) units.push_back(ch); }
    } else {
        uint16_t ch = (uint16_t)word;
        if (vCodeTable == 0) {
            units.push_back(ch);
        } else if (vCodeTable == 1 || vCodeTable == 2 || vCodeTable == 4) {
            units.push_back(LOBYTE(ch));
            if (HIBYTE(ch) > 32) units.push_back(HIBYTE(ch));
        } else if (vCodeTable == 3) {
            uint16_t hi = ch >> 13;
            units.push_back(ch & 0x1FFF);
            if (hi > 0) units.push_back(_unicodeCompoundMark[hi - 1]);
        }
    }
}

uint16_t OriginalDriver::keyChar(uint32_t w) { return keyCodeToCharacter(w); }

std::string OriginalDriver::convert(const ConvertOptions& o, const std::string& in) {
    convertToolFromCode = (Uint8)o.from;
    convertToolToCode = (Uint8)o.to;
    convertToolToAllCaps = o.allCaps;
    convertToolToAllNonCaps = o.allLower;
    convertToolToCapsFirstLetter = o.capsFirst;
    convertToolToCapsEachWord = o.capsEachWord;
    convertToolRemoveMark = o.removeMark;
    return convertUtil(in);
}

std::vector<int> OriginalDriver::smartSwitch(const std::vector<std::pair<std::string, int>>& sets,
                                             const std::vector<std::string>& queries, int current) {
    for (const auto& s : sets) setAppInputMethodStatus(s.first, s.second);
    std::vector<int> r;
    for (const auto& q : queries) r.push_back(getAppInputMethodStatus(q, current));
    return r;
}

std::vector<uint8_t> OriginalDriver::macroBlobRoundTrip() {
    ::addMacro("btw", "by the way");
    ::addMacro("tp", "thành phố");
    std::vector<Byte> blob;
    getMacroSaveData(blob);
    return std::vector<uint8_t>(blob.begin(), blob.end());
}

} // namespace vktest
