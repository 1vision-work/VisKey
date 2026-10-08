//
//  viskey_engine.cpp — C API over vk_engine.
//
//  Based on OpenKey by Mai Vũ Tuyên. Modified by VisKey contributors.
//  SPDX-License-Identifier: GPL-3.0-or-later
//

#include "viskey_engine.h"
#include "Engine.h"
#include <algorithm>
#include <new>
#include <string.h>

static_assert(VK_MAX_RESULT_CHARS >= MAX_BUFF, "result must hold a full engine word buffer");


static int* optionSlot(vk_engine* e, vk_option key) {
    switch (key) {
    case VK_OPT_LANGUAGE: return &e->vLanguage;
    case VK_OPT_INPUT_TYPE: return &e->vInputType;
    case VK_OPT_CODE_TABLE: return &e->vCodeTable;
    case VK_OPT_FREE_MARK: return &e->vFreeMark;
    case VK_OPT_CHECK_SPELLING: return &e->vCheckSpelling;
    case VK_OPT_MODERN_ORTHOGRAPHY: return &e->vUseModernOrthography;
    case VK_OPT_QUICK_TELEX: return &e->vQuickTelex;
    case VK_OPT_RESTORE_IF_WRONG_SPELLING: return &e->vRestoreIfWrongSpelling;
    case VK_OPT_USE_MACRO: return &e->vUseMacro;
    case VK_OPT_USE_MACRO_IN_ENGLISH_MODE: return &e->vUseMacroInEnglishMode;
    case VK_OPT_AUTO_CAPS_MACRO: return &e->vAutoCapsMacro;
    case VK_OPT_UPPERCASE_FIRST_CHAR: return &e->vUpperCaseFirstChar;
    case VK_OPT_ALLOW_CONSONANT_ZFWJ: return &e->vAllowConsonantZFWJ;
    case VK_OPT_QUICK_START_CONSONANT: return &e->vQuickStartConsonant;
    case VK_OPT_QUICK_END_CONSONANT: return &e->vQuickEndConsonant;
    }
    return nullptr;
}

extern "C" {

vk_engine* vk_create(void) {
    vk_engine* e = new (std::nothrow) vk_engine();
    if (e) e->vKeyInit();
    return e;
}

void vk_destroy(vk_engine* e) { delete e; }

void vk_set_option(vk_engine* e, vk_option key, int32_t value) {
    int* slot = e ? optionSlot(e, key) : nullptr;
    if (!slot) return;
    *slot = value;
    if (key == VK_OPT_CODE_TABLE) e->onTableCodeChange();
    e->vKeyInit(); // reset typing state; also re-captures the spelling option
}

int32_t vk_get_option(const vk_engine* e, vk_option key) {
    int* slot = e ? optionSlot(const_cast<vk_engine*>(e), key) : nullptr;
    return slot ? *slot : 0;
}

vk_result vk_handle_key(vk_engine* e, uint16_t keycode, uint32_t modifiers, vk_event_kind kind) {
    vk_result r;
    memset(&r, 0, sizeof r);
    if (!e) return r;

    const bool shift = (modifiers & VK_MOD_SHIFT) != 0;
    const bool capsLock = (modifiers & VK_MOD_CAPS_LOCK) != 0;
    const bool other = (modifiers & VK_MOD_OTHER) != 0;

    if (e->vLanguage == 0) {
        // English mode: only macros are still handled, on key down.
        if (kind != VK_EVENT_KEY_DOWN || !(e->vUseMacro && e->vUseMacroInEnglishMode)) return r;
        e->vEnglishMode(vKeyEventState::KeyDown, keycode, shift || capsLock, other);
    } else if (kind == VK_EVENT_MOUSE_DOWN) {
        e->vKeyHandleEvent(vKeyEvent::Mouse, vKeyEventState::MouseDown, 0);
    } else {
        e->vKeyHandleEvent(vKeyEvent::Keyboard, vKeyEventState::KeyDown, keycode, shift ? 1 : (capsLock ? 2 : 0), other);
    }

    const vKeyHookState& h = e->HookState;
    r.code = h.code;
    r.ext_code = h.extCode;
    switch (h.code) {
    case vWillProcess:
    case vRestore:
    case vRestoreAndStartNewSession: {
        const int n = std::min<int>(h.newCharCount, MAX_BUFF);
        r.backspace_count = h.backspaceCount;
        r.char_count = (uint8_t)n;
        for (int i = 0; i < n; i++) r.chars[i] = h.charData[n - 1 - i]; // engine returns them reversed
        break;
    }
    case vReplaceMaro: {
        const size_t total = h.macroData.size();
        const size_t n = std::min<size_t>(total, VK_MAX_RESULT_CHARS);
        r.backspace_count = h.backspaceCount;
        r.char_count = (uint8_t)n;
        r.macro_total = (uint16_t)std::min<size_t>(total, 0xFFFF);
        for (size_t i = 0; i < n; i++) r.chars[i] = h.macroData[i];
        break;
    }
    default:
        break; // backspace_count/char_count are only meaningful for the codes above
    }
    return r;
}

void vk_new_session(vk_engine* e) { if (e) e->startNewSession(); }
void vk_temp_off_spelling(vk_engine* e) { if (e) e->vTempOffSpellChecking(); }
void vk_restore_spelling(vk_engine* e) { if (e) e->vSetCheckSpelling(); }
void vk_temp_off_engine(vk_engine* e, int off) { if (e) e->vTempOffEngine(off != 0); }

size_t vk_last_macro_words(const vk_engine* e, size_t offset, uint32_t* out, size_t cap) {
    if (!e || !out) return 0;
    const vector<Uint32>& m = e->HookState.macroData;
    if (offset >= m.size()) return 0;
    const size_t n = std::min(cap, m.size() - offset);
    for (size_t i = 0; i < n; i++) out[i] = m[offset + i];
    return n;
}

// Mirrors OpenKey.mm SendNewCharString for one word.
size_t vk_word_decode(const vk_engine* e, uint32_t word, uint16_t out[2]) {
    if (word & PURE_CHARACTER_MASK) {
        out[0] = (uint16_t)word;
        return 1;
    }
    if (!(word & CHAR_CODE_MASK)) {
        out[0] = keyCodeToCharacter(word);
        return out[0] ? 1 : 0;
    }
    const uint16_t ch = (uint16_t)word;
    switch (e->vCodeTable) {
    case VK_TABLE_UNICODE:
        out[0] = ch;
        return 1;
    case VK_TABLE_TCVN3:
    case VK_TABLE_VNI_WINDOWS:
    case VK_TABLE_CP1258:
        out[0] = LOBYTE(ch);
        if (HIBYTE(ch) > 32) {
            out[1] = HIBYTE(ch);
            return 2;
        }
        return 1;
    case VK_TABLE_UNICODE_COMPOUND: {
        const uint16_t mark = ch >> 13;
        out[0] = ch & 0x1FFF;
        if (mark > 0) {
            out[1] = _unicodeCompoundMark[mark - 1];
            return 2;
        }
        return 1;
    }
    }
    return 0;
}

uint16_t vk_keycode_to_char(uint32_t keycode_with_caps) { return keyCodeToCharacter(keycode_with_caps); }

void vk_macro_load(vk_engine* e, const uint8_t* blob, size_t len) {
    if (!e) return;
    if (!blob || len < 2) { e->initMacroMap(nullptr, 0); return; }
    e->initMacroMap(blob, (int)len);
}

size_t vk_macro_save(const vk_engine* e, uint8_t* out, size_t cap) {
    if (!e) return 0;
    vector<Byte> blob;
    e->getMacroSaveData(blob);
    if (out && cap >= blob.size()) memcpy(out, blob.data(), blob.size());
    return blob.size();
}

int vk_macro_add(vk_engine* e, const char* text, const char* content) {
    return (e && text && content && e->addMacro(text, content)) ? 1 : 0;
}

int vk_macro_delete(vk_engine* e, const char* text) { return (e && text && e->deleteMacro(text)) ? 1 : 0; }
void vk_macro_reload(vk_engine* e) { if (e) e->onTableCodeChange(); }

int vk_smart_switch_get(vk_engine* e, const char* bundle_id, int current) {
    return (e && bundle_id) ? e->smartSwitch.getAppInputMethodStatus(bundle_id, current) : -1;
}

void vk_smart_switch_set(vk_engine* e, const char* bundle_id, int language) {
    if (e && bundle_id) e->smartSwitch.setAppInputMethodStatus(bundle_id, language);
}

size_t vk_convert_ex(vk_engine* e, const char* utf8, size_t len, vk_code_table from, vk_code_table to,
                     uint32_t flags, char* out, size_t cap) {
    (void)e; // conversion is stateless; kept in the signature so the API can grow engine-dependent options
    vk_convert_options opt;
    opt.fromCode = (Uint8)from;
    opt.toCode = (Uint8)to;
    opt.toAllCaps = (flags & VK_CONVERT_ALL_CAPS) != 0;
    opt.toAllNonCaps = (flags & VK_CONVERT_ALL_LOWER) != 0;
    opt.toCapsFirstLetter = (flags & VK_CONVERT_CAPS_FIRST_LETTER) != 0;
    opt.toCapsEachWord = (flags & VK_CONVERT_CAPS_EACH_WORD) != 0;
    opt.removeMark = (flags & VK_CONVERT_REMOVE_MARK) != 0;
    const string result = convertUtil(opt, string(utf8 ? utf8 : "", utf8 ? len : 0));
    if (out && cap > 0) {
        const size_t n = std::min(result.size(), cap - 1);
        memcpy(out, result.data(), n);
        out[n] = '\0';
    }
    return result.size();
}

size_t vk_convert(vk_engine* e, const char* utf8, size_t len, vk_code_table from, vk_code_table to,
                  char* out, size_t cap) {
    return vk_convert_ex(e, utf8, len, from, to, 0, out, cap);
}

} // extern "C"
