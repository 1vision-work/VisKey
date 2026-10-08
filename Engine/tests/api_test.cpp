// SPDX-License-Identifier: GPL-3.0-or-later
// Tests of the C API contract that the snapshot does not cover.
#include <catch2/catch_test_macros.hpp>
#include "platforms/mac.h"
#include "viskey_engine.h"
#include <string>
#include <vector>

namespace {

// Types one key into `screen` (kept by the caller) and returns the text to ADD, with deletions applied to `screen`.
std::u16string typeKeysDelta(vk_engine* e, std::u16string& screen, const std::pair<int, uint32_t>& k) {
    std::u16string add;
    vk_result r = vk_handle_key(e, (uint16_t)k.first, k.second, VK_EVENT_KEY_DOWN);
    if (r.code == VK_DO_NOTHING) {
        if (uint16_t ch = vk_keycode_to_char(k.first)) add += (char16_t)ch;
        return add;
    }
    screen.resize(screen.size() - std::min<size_t>(screen.size(), r.backspace_count));
    for (int i = 0; i < r.char_count; i++) {
        uint16_t u[2];
        size_t n = vk_word_decode(e, r.chars[i], u);
        for (size_t j = 0; j < n; j++) add += (char16_t)u[j];
    }
    return add;
}

// Telex: "viet" + "j" -> "việt" needs "vieetj"
const std::vector<std::pair<int, uint32_t>> kVietj = {{KEY_V, 0}, {KEY_I, 0}, {KEY_E, 0}, {KEY_E, 0}, {KEY_T, 0}, {KEY_J, 0}};

} // namespace

TEST_CASE("engines are independent", "[api]") {
    // Telex "vieetj" and VNI "vie65t"-style input typed into two engines, key by key, interleaved.
    const std::vector<std::pair<int, uint32_t>> vni = {{KEY_V, 0}, {KEY_I, 0}, {KEY_E, 0}, {KEY_6, 0}, {KEY_T, 0}, {KEY_5, 0}};
    vk_engine* a = vk_create();
    vk_engine* b = vk_create();
    vk_set_option(b, VK_OPT_INPUT_TYPE, VK_INPUT_VNI);

    std::u16string sa, sb;
    for (size_t i = 0; i < kVietj.size(); i++) {
        sa += typeKeysDelta(a, sa, kVietj[i]);
        sb += typeKeysDelta(b, sb, vni[i]);
    }
    CHECK(sa == u"việt");
    CHECK(sb == u"việt");
    vk_destroy(a); vk_destroy(b);
}

TEST_CASE("options round trip", "[api]") {
    vk_engine* e = vk_create();
    CHECK(vk_get_option(e, VK_OPT_LANGUAGE) == 1);
    vk_set_option(e, VK_OPT_QUICK_TELEX, 1);
    CHECK(vk_get_option(e, VK_OPT_QUICK_TELEX) == 1);
    vk_destroy(e);
}

TEST_CASE("English mode passes keys through", "[api]") {
    vk_engine* e = vk_create();
    vk_set_option(e, VK_OPT_LANGUAGE, 0);
    for (auto& k : kVietj) CHECK(vk_handle_key(e, (uint16_t)k.first, 0, VK_EVENT_KEY_DOWN).code == VK_DO_NOTHING);
    vk_destroy(e);
}

TEST_CASE("macro longer than one result is fetched in pieces", "[api][macro]") {
    vk_engine* e = vk_create();
    vk_set_option(e, VK_OPT_USE_MACRO, 1);
    std::string content;
    for (int i = 0; i < 200; i++) content += char('a' + i % 26);
    REQUIRE(vk_macro_add(e, "long", content.c_str()) == 1);
    vk_result r{};
    for (int k : {KEY_L, KEY_O, KEY_N, KEY_G}) r = vk_handle_key(e, (uint16_t)k, 0, VK_EVENT_KEY_DOWN);
    r = vk_handle_key(e, KEY_SPACE, 0, VK_EVENT_KEY_DOWN);
    REQUIRE(r.code == VK_REPLACE_MACRO);
    CHECK(r.backspace_count == 4);
    CHECK(r.macro_total == 200);
    CHECK(r.char_count == VK_MAX_RESULT_CHARS);
    std::vector<uint32_t> rest(200);
    CHECK(vk_last_macro_words(e, 0, rest.data(), rest.size()) == 200);
    CHECK(vk_last_macro_words(e, 190, rest.data(), 100) == 10);
    CHECK(vk_last_macro_words(e, 200, rest.data(), 100) == 0);
    vk_destroy(e);
}

TEST_CASE("macro blob save and load", "[api][macro]") {
    vk_engine* a = vk_create();
    vk_macro_add(a, "btw", "by the way");
    size_t need = vk_macro_save(a, nullptr, 0);
    std::vector<uint8_t> blob(need);
    REQUIRE(vk_macro_save(a, blob.data(), blob.size()) == need);

    vk_engine* b = vk_create();
    vk_macro_load(b, blob.data(), blob.size());
    CHECK(vk_macro_save(b, nullptr, 0) == need);
    CHECK(vk_macro_delete(b, "btw") == 1);
    CHECK(vk_macro_delete(b, "btw") == 0);
    vk_destroy(a); vk_destroy(b);
}

TEST_CASE("convert reports full length when truncated", "[api][convert]") {
    vk_engine* e = vk_create();
    const std::string in = "việt nam"; // lower case: the converter lower-cases unless told otherwise
    char small[4];
    size_t full = vk_convert(e, in.data(), in.size(), VK_TABLE_UNICODE, VK_TABLE_UNICODE, small, sizeof small);
    CHECK(full == in.size());
    CHECK(std::string(small) == in.substr(0, 3));
    vk_destroy(e);
}

TEST_CASE("null handles are ignored", "[api]") {
    vk_destroy(nullptr);
    vk_set_option(nullptr, VK_OPT_LANGUAGE, 0);
    CHECK(vk_handle_key(nullptr, KEY_A, 0, VK_EVENT_KEY_DOWN).code == VK_DO_NOTHING);
    CHECK(vk_get_option(nullptr, VK_OPT_LANGUAGE) == 0);
}
