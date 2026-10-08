// SPDX-License-Identifier: GPL-3.0-or-later
#include "driver_vk.h"

namespace vktest {

VkDriver::~VkDriver() { vk_destroy(e_); }

void VkDriver::begin(const Config& c) {
    vk_destroy(e_);
    e_ = vk_create();
    vk_set_option(e_, VK_OPT_LANGUAGE, c.language);
    vk_set_option(e_, VK_OPT_INPUT_TYPE, c.inputType);
    vk_set_option(e_, VK_OPT_CODE_TABLE, c.codeTable);
    vk_set_option(e_, VK_OPT_FREE_MARK, c.freeMark);
    vk_set_option(e_, VK_OPT_CHECK_SPELLING, c.checkSpelling);
    vk_set_option(e_, VK_OPT_MODERN_ORTHOGRAPHY, c.modernOrthography);
    vk_set_option(e_, VK_OPT_QUICK_TELEX, c.quickTelex);
    vk_set_option(e_, VK_OPT_RESTORE_IF_WRONG_SPELLING, c.restoreIfWrongSpelling);
    vk_set_option(e_, VK_OPT_USE_MACRO, c.useMacro);
    vk_set_option(e_, VK_OPT_USE_MACRO_IN_ENGLISH_MODE, c.macroInEnglishMode);
    vk_set_option(e_, VK_OPT_AUTO_CAPS_MACRO, c.autoCapsMacro);
    vk_set_option(e_, VK_OPT_UPPERCASE_FIRST_CHAR, c.upperCaseFirstChar);
    vk_set_option(e_, VK_OPT_ALLOW_CONSONANT_ZFWJ, c.allowConsonantZFWJ);
    vk_set_option(e_, VK_OPT_QUICK_START_CONSONANT, c.quickStartConsonant);
    vk_set_option(e_, VK_OPT_QUICK_END_CONSONANT, c.quickEndConsonant);
}

void VkDriver::addMacro(const std::string& t, const std::string& c) { vk_macro_add(e_, t.c_str(), c.c_str()); }

Step VkDriver::toStep(const vk_result& r) {
    Step s;
    s.code = r.code;
    s.backspace = r.backspace_count;
    s.ext = r.ext_code;
    if (r.code == VK_REPLACE_MACRO) {
        s.newChars = r.macro_total;
        s.macro.resize(r.macro_total);
        size_t got = vk_last_macro_words(e_, 0, s.macro.data(), s.macro.size());
        s.macro.resize(got);
    } else {
        s.newChars = r.char_count;
        s.words.assign(r.chars, r.chars + r.char_count);
    }
    return s;
}

Step VkDriver::key(uint16_t k, int caps, bool other) {
    return toStep(vk_handle_key(e_, k, (caps ? VK_MOD_SHIFT : 0) | (other ? VK_MOD_OTHER : 0), VK_EVENT_KEY_DOWN));
}

Step VkDriver::englishKey(uint16_t k, int caps, bool other) { return key(k, caps, other); }
void VkDriver::mouse() { vk_handle_key(e_, 0, 0, VK_EVENT_MOUSE_DOWN); }
void VkDriver::tempOffSpelling() { vk_temp_off_spelling(e_); }
void VkDriver::tempOffEngine() { vk_temp_off_engine(e_, 1); }

void VkDriver::decodeWord(uint32_t w, std::vector<uint16_t>& units) {
    uint16_t out[2] = {0, 0};
    size_t n = vk_word_decode(e_, w, out);
    units.assign(out, out + n);
}

uint16_t VkDriver::keyChar(uint32_t w) { return vk_keycode_to_char(w); }

std::string VkDriver::convert(const ConvertOptions& o, const std::string& in) {
    uint32_t flags = (o.allCaps ? VK_CONVERT_ALL_CAPS : 0) | (o.allLower ? VK_CONVERT_ALL_LOWER : 0) |
                     (o.capsFirst ? VK_CONVERT_CAPS_FIRST_LETTER : 0) | (o.capsEachWord ? VK_CONVERT_CAPS_EACH_WORD : 0) |
                     (o.removeMark ? VK_CONVERT_REMOVE_MARK : 0);
    size_t need = vk_convert_ex(e_, in.data(), in.size(), (vk_code_table)o.from, (vk_code_table)o.to, flags, nullptr, 0);
    std::string out(need + 1, '\0');
    vk_convert_ex(e_, in.data(), in.size(), (vk_code_table)o.from, (vk_code_table)o.to, flags, &out[0], out.size());
    out.resize(need);
    return out;
}

std::vector<int> VkDriver::smartSwitch(const std::vector<std::pair<std::string, int>>& sets,
                                       const std::vector<std::string>& queries, int current) {
    for (const auto& s : sets) vk_smart_switch_set(e_, s.first.c_str(), s.second);
    std::vector<int> r;
    for (const auto& q : queries) r.push_back(vk_smart_switch_get(e_, q.c_str(), current));
    return r;
}

std::vector<uint8_t> VkDriver::macroBlobRoundTrip() {
    vk_macro_add(e_, "btw", "by the way");
    vk_macro_add(e_, "tp", "thành phố");
    size_t need = vk_macro_save(e_, nullptr, 0);
    std::vector<uint8_t> blob(need);
    vk_macro_save(e_, blob.data(), blob.size());
    return blob;
}

} // namespace vktest
