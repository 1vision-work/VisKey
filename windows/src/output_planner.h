// SPDX-License-Identifier: GPL-3.0-or-later
//
// VisKey for Windows (by 1VISION) — based on OpenKey by Mai Vũ Tuyên.
//
// Turns an engine result into keystrokes to inject. Pure C++ (no Win32), unit-tested in
// windows/tests/win_keymap_test.cpp. Port of VisKey/Core/ReplacementPlanner.swift, which mirrors
// OpenKey's SendBackspace / SendNewCharString / handleMacro and its `_syncKey` bookkeeping:
// with VNI Windows and Unicode compound one visible character can be two UTF-16 units, so the planner
// remembers how many units each character of the current word took and sends a second backspace.
//
// Windows-specific choices:
//  - The key that was typed is re-sent after a restored word or a macro as a REAL key press (its virtual-key
//    code), not as a character: the app then applies the current keyboard layout and Shift state. This also
//    avoids the swapped names of ` \ ' in Engine/src/platforms/win32.h (inherited from OpenKey).
//  - OpenKey's "fix recommend browser" (U+202F typed then deleted before the backspaces) is optional.

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

#include "viskey_engine.h"

namespace viskey {

struct OutputAction {
    enum class Kind { Backspace, Text, Key };
    Kind kind = Kind::Backspace;
    std::vector<uint16_t> units;  // Text: UTF-16 units, sent with KEYEVENTF_UNICODE
    uint16_t vk = 0;              // Key: virtual-key code

    static OutputAction MakeBackspace() { return OutputAction(); }
    static OutputAction MakeText(std::vector<uint16_t> u) {
        OutputAction a; a.kind = Kind::Text; a.units = std::move(u); return a;
    }
    static OutputAction MakeKey(uint16_t vk) { OutputAction a; a.kind = Kind::Key; a.vk = vk; return a; }
    bool operator==(const OutputAction& o) const { return kind == o.kind && units == o.units && vk == o.vk; }
};

struct OutputPlan {
    std::vector<OutputAction> actions;
    // The original key event still reaches the app, AFTER `actions` (e.g. an extra backspace before it).
    bool passThrough = true;
};

// Decodes one engine word into 1-2 UTF-16 units (vk_word_decode). Injected for tests.
using WordDecoder = std::function<size_t(uint32_t word, uint16_t out[2])>;

class OutputPlanner {
public:
    static constexpr uint16_t kEmptyChar = 0x202F;  // OpenKey's invisible "empty character"

    void SetCodeTable(int codeTable) { codeTable_ = codeTable; reset(); }
    void SetFixBrowserAutocomplete(bool on) { fixAutocomplete_ = on; }
    // The app deletes a base letter and its combining mark with one backspace (OpenKey `_unicodeCompoundApp`).
    void SetBackspaceDeletesCluster(bool on) { backspaceDeletesCluster_ = on; }
    void reset() { syncKey_.clear(); }

    // `words`/`count`: chars[] of the result, or for VK_REPLACE_MACRO the full macro content
    // (vk_last_macro_words). `typedVk`: the key that produced the result.
    OutputPlan Plan(const vk_result& r, const uint32_t* words, size_t count, uint16_t typedVk,
                    const WordDecoder& decode);

    const std::vector<int>& syncKey() const { return syncKey_; }

private:
    bool IsDoubleCode() const { return codeTable_ == VK_TABLE_VNI_WINDOWS || codeTable_ == VK_TABLE_UNICODE_COMPOUND; }
    OutputPlan PassThrough(uint8_t extCode);
    void AppendBackspace(std::vector<OutputAction>& out);
    void AppendText(std::vector<OutputAction>& out, const uint32_t* words, size_t count, const WordDecoder& decode);
    void AppendKey(std::vector<OutputAction>& out, uint16_t vk);
    void AppendAutocompleteFix(std::vector<OutputAction>& out, const vk_result& r);

    int codeTable_ = VK_TABLE_UNICODE;
    bool fixAutocomplete_ = false;
    bool backspaceDeletesCluster_ = false;
    std::vector<int> syncKey_;
};

// One injected key event, before it becomes a Win32 INPUT (see win_input.h).
struct KeyStroke {
    uint16_t vk = 0;       // 0 for unicode strokes
    uint16_t unit = 0;     // UTF-16 unit for unicode strokes
    bool unicode = false;
    bool up = false;
    bool operator==(const KeyStroke& o) const {
        return vk == o.vk && unit == o.unit && unicode == o.unicode && up == o.up;
    }
};

// Flattens actions into down/up strokes: Backspace -> VK_BACK down/up; Text -> per UTF-16 unit a unicode
// down/up (a surrogate pair is sent as two consecutive units); Key -> vk down/up.
std::vector<KeyStroke> ToKeyStrokes(const std::vector<OutputAction>& actions);

}  // namespace viskey
