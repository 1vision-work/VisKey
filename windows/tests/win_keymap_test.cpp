// SPDX-License-Identifier: GPL-3.0-or-later
//
// VisKey for Windows (by 1VISION) — based on OpenKey by Mai Vũ Tuyên.
//
// Tests the non-UI logic of the Windows app without installing hooks: config.json round trip, key
// classification / modifier mapping / the Ctrl+Shift hotkey, and engine result -> keystrokes, including an
// end-to-end run of the real engine with Windows virtual-key codes against a simulated text field.
// Plain console program (no test framework): prints failures and returns non-zero.

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "config.h"
#include "json.h"
#include "keymap.h"
#include "output_planner.h"
#include "viskey_engine.h"

#ifdef _WIN32
#include "win_util.h"
#endif

using namespace viskey;

static int g_checks = 0;
static int g_failures = 0;

#define CHECK(cond)                                                                 \
    do {                                                                            \
        g_checks++;                                                                 \
        if (!(cond)) {                                                              \
            g_failures++;                                                           \
            std::fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
        }                                                                           \
    } while (0)

static std::string ToUtf8(const std::u16string& s) {
    std::string out;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned cp = s[i];
        if (cp >= 0xD800 && cp <= 0xDBFF && i + 1 < s.size()) cp = 0x10000 + ((cp - 0xD800) << 10) + (s[++i] - 0xDC00);
        if (cp < 0x80) out += char(cp);
        else if (cp < 0x800) { out += char(0xC0 | (cp >> 6)); out += char(0x80 | (cp & 0x3F)); }
        else if (cp < 0x10000) {
            out += char(0xE0 | (cp >> 12)); out += char(0x80 | ((cp >> 6) & 0x3F)); out += char(0x80 | (cp & 0x3F));
        } else {
            out += char(0xF0 | (cp >> 18)); out += char(0x80 | ((cp >> 12) & 0x3F));
            out += char(0x80 | ((cp >> 6) & 0x3F)); out += char(0x80 | (cp & 0x3F));
        }
    }
    return out;
}

#define CHECK_EQ_STR(actual, expected)                                                               \
    do {                                                                                             \
        const std::string a_ = (actual), e_ = (expected);                                            \
        g_checks++;                                                                                  \
        if (a_ != e_) {                                                                              \
            g_failures++;                                                                            \
            std::fprintf(stderr, "%s:%d: expected \"%s\", got \"%s\"\n", __FILE__, __LINE__, e_.c_str(), a_.c_str()); \
        }                                                                                            \
    } while (0)

// ---- JSON / config ------------------------------------------------------------------------------

static void TestJson() {
    json::Value v;
    CHECK(json::Parse(u8"{\"a\": [1, 2.5, -3e2, true, false, null], \"s\": \"x\\\"\\n\\u00e0\\ud83d\\ude00 ố\"}", v));
    CHECK(v.IsObject());
    const json::Value* a = v.Find("a");
    CHECK(a && a->IsArray() && a->array.size() == 6);
    if (a && a->array.size() == 6) {
        CHECK(a->array[1].number == 2.5);
        CHECK(a->array[2].number == -300);
        CHECK(a->array[3].IsBool() && a->array[3].boolean);
        CHECK(a->array[5].type == json::Value::Type::Null);
    }
    const json::Value* s = v.Find("s");
    CHECK(s && s->string == u8"x\"\nà😀 ố");

    json::Value back;
    CHECK(json::Parse(json::Serialize(v), back));
    CHECK(back.Find("s") && back.Find("s")->string == s->string);

    std::string err;
    CHECK(!json::Parse("{\"a\": }", v, &err) && !err.empty());
    CHECK(!json::Parse("[1, 2", v));
    CHECK(!json::Parse("\"\\ud83d\"", v));  // lone surrogate
    CHECK(!json::Parse("{} x", v));
    CHECK(json::Parse("\xEF\xBB\xBF{}", v));  // UTF-8 BOM (Notepad)
}

static void TestConfig() {
    Config c;
    c.vietnamese = false;
    c.inputType = VK_INPUT_VNI;
    c.codeTable = VK_TABLE_UNICODE_COMPOUND;
    c.quickTelex = true;
    c.fixBrowserAutocomplete = false;
    c.hotkey = ToggleHotkey::AltZ;
    c.macros = {{"vk", "VisKey"}, {"ko", u8"không"}, {"q", "say \"hi\"\\\n"}};

    Config back;
    CHECK(ConfigFromJson(ConfigToJson(c), back));
    CHECK(back == c);

    // Defaults for missing keys, unknown keys and wrong types ignored.
    Config d;
    CHECK(ConfigFromJson(u8"{\"inputType\": \"simple_telex\", \"codeTable\": \"tcvn3\", \"checkSpelling\": 0, "
                         u8"\"future\": {\"x\": 1}, \"macros\": [{\"abbreviation\": \"hs\", \"expansion\": \"học sinh\"}, "
                         u8"{\"abbreviation\": \"\", \"expansion\": \"x\"}, 5]}",
                         d));
    CHECK(d.inputType == VK_INPUT_SIMPLE_TELEX_1);
    CHECK(d.codeTable == VK_TABLE_TCVN3);
    CHECK(d.checkSpelling == true);  // 0 is not a bool: default kept
    CHECK(d.vietnamese && d.useMacro && d.hotkey == ToggleHotkey::CtrlShift);
    CHECK(d.macros.size() == 1 && d.macros[0].expansion == u8"học sinh");

    Config bad;
    bad.vietnamese = false;
    CHECK(!ConfigFromJson("not json", bad));
    CHECK(bad == Config());

    for (int t = 0; t <= 4; t++) CHECK(CodeTableFromName(CodeTableName(t), -1) == t);
    for (int t = 0; t <= 3; t++) CHECK(InputTypeFromName(InputTypeName(t), -1) == t);

    std::vector<Macro> m;
    CHECK(UpsertMacro(m, {"a", "1"}));
    CHECK(UpsertMacro(m, {"a", "2"}));
    CHECK(m.size() == 1 && m[0].expansion == "2");
    CHECK(!UpsertMacro(m, {"", "x"}));
    CHECK(!UpsertMacro(m, {"b", ""}));
    CHECK(RemoveMacro(m, "a") && m.empty());
    CHECK(!RemoveMacro(m, "a"));

    CHECK(ChooseConfigPath(L"C:\\Tools\\VisKey", true, L"C:\\Users\\u\\AppData\\Roaming") ==
          L"C:\\Tools\\VisKey\\config.json");
    CHECK(ChooseConfigPath(L"C:\\Program Files\\VisKey", false, L"C:\\Users\\u\\AppData\\Roaming") ==
          L"C:\\Users\\u\\AppData\\Roaming\\VisKey\\config.json");
    CHECK(ChooseConfigPath(L"D:\\", false, L"") == L"D:\\config.json");
}

// ---- keys ---------------------------------------------------------------------------------------

static void TestKeymap() {
    CHECK(ClassifyKey('A') == KeyClass::Typing);
    CHECK(ClassifyKey('7') == KeyClass::Typing);
    CHECK(ClassifyKey(0xDB) == KeyClass::Typing);  // [ (Telex ư/ơ)
    CHECK(ClassifyKey(0xBE) == KeyClass::Typing);  // .
    CHECK(ClassifyKey(vkey::Space) == KeyClass::Typing);
    CHECK(ClassifyKey(vkey::Back) == KeyClass::Typing);
    CHECK(ClassifyKey(vkey::Left) == KeyClass::Typing);
    CHECK(ClassifyKey(vkey::LShift) == KeyClass::Modifier);
    CHECK(ClassifyKey(vkey::RMenu) == KeyClass::Modifier);
    CHECK(ClassifyKey(vkey::LWin) == KeyClass::Modifier);
    CHECK(ClassifyKey(vkey::Capital) == KeyClass::Modifier);
    CHECK(ClassifyKey(0x70) == KeyClass::Other);  // F1
    CHECK(ClassifyKey(0x60) == KeyClass::Other);  // numpad 0
    CHECK(ClassifyKey(vkey::Packet) == KeyClass::Other);

    CHECK(ModifierBit(vkey::RShift) == kModShift);
    CHECK(ModifierBit(vkey::LControl) == kModCtrl);
    CHECK(ModifierBit(vkey::Menu) == kModAlt);
    CHECK(ModifierBit(vkey::RWin) == kModWin);
    CHECK(ModifierBit('A') == 0);

    CHECK(EngineModifiers(0, false) == 0);
    CHECK(EngineModifiers(kModShift, false) == VK_MOD_SHIFT);
    CHECK(EngineModifiers(0, true) == VK_MOD_CAPS_LOCK);
    CHECK(EngineModifiers(kModShift | kModCtrl, true) == (VK_MOD_SHIFT | VK_MOD_CAPS_LOCK | VK_MOD_OTHER));
    CHECK(EngineModifiers(kModCtrl | kModAlt, false) == VK_MOD_OTHER);  // AltGr
    CHECK(EngineModifiers(kModWin, false) == VK_MOD_OTHER);
    CHECK(IsShortcut(kModAlt) && !IsShortcut(kModShift));
    CHECK(IsAltZ(vkey::Z, kModAlt) && !IsAltZ(vkey::Z, kModAlt | kModShift) && !IsAltZ('X', kModAlt));

    // Ctrl down, Shift down, Shift up -> fires.
    ModifierHotkey h;
    CHECK(!h.ModifiersChanged(kModCtrl));
    CHECK(!h.ModifiersChanged(kModCtrl | kModShift));
    CHECK(h.ModifiersChanged(kModCtrl));
    CHECK(!h.ModifiersChanged(0));
    // Ctrl+Shift+T -> does not fire.
    CHECK(!h.ModifiersChanged(kModShift));
    CHECK(!h.ModifiersChanged(kModShift | kModCtrl));
    h.KeyDown();
    CHECK(!h.ModifiersChanged(kModShift));
    CHECK(!h.ModifiersChanged(0));
    // Ctrl+Shift+Alt -> does not fire; Shift alone -> does not fire.
    CHECK(!h.ModifiersChanged(kModCtrl));
    CHECK(!h.ModifiersChanged(kModCtrl | kModShift));
    CHECK(!h.ModifiersChanged(kModCtrl | kModShift | kModAlt));
    CHECK(!h.ModifiersChanged(kModCtrl | kModShift));
    CHECK(!h.ModifiersChanged(0));
    CHECK(!h.ModifiersChanged(kModShift));
    CHECK(!h.ModifiersChanged(0));
    // Shift first, then Ctrl, release Ctrl first -> fires.
    CHECK(!h.ModifiersChanged(kModShift));
    CHECK(!h.ModifiersChanged(kModShift | kModCtrl));
    CHECK(h.ModifiersChanged(kModShift));
}

// ---- planner (fake decoder) ---------------------------------------------------------------------

static size_t FakeDecode(uint32_t w, uint16_t out[2]) {
    // High half set -> two units (like VNI Windows "base + mark").
    out[0] = uint16_t(w & 0xFFFF);
    if (w >> 16) { out[1] = uint16_t(w >> 16); return 2; }
    return out[0] ? 1 : 0;
}

static vk_result Result(uint8_t code, uint8_t bs, std::vector<uint32_t> words, uint8_t ext = 0) {
    vk_result r;
    std::memset(&r, 0, sizeof r);
    r.code = code;
    r.backspace_count = bs;
    r.char_count = uint8_t(words.size());
    for (size_t i = 0; i < words.size(); i++) r.chars[i] = words[i];
    r.ext_code = ext;
    return r;
}

static void TestPlanner() {
    using K = OutputAction;
    OutputPlanner p;

    vk_result r = Result(VK_DO_NOTHING, 0, {});
    OutputPlan plan = p.Plan(r, r.chars, r.char_count, 'A', FakeDecode);
    CHECK(plan.passThrough && plan.actions.empty());

    r = Result(VK_WILL_PROCESS, 2, {'x', 'y'});
    plan = p.Plan(r, r.chars, r.char_count, 'S', FakeDecode);
    CHECK(!plan.passThrough);
    CHECK(plan.actions == (std::vector<K>{K::MakeBackspace(), K::MakeBackspace(), K::MakeText({'x', 'y'})}));

    r = Result(VK_RESTORE, 1, {'a'});
    plan = p.Plan(r, r.chars, r.char_count, 'S', FakeDecode);
    CHECK(plan.actions == (std::vector<K>{K::MakeBackspace(), K::MakeText({'a'}), K::MakeKey('S')}));

    r = Result(VK_REPLACE_MACRO, 2, {'V', 'K'});
    plan = p.Plan(r, r.chars, r.char_count, vkey::Space, FakeDecode);
    CHECK(plan.actions ==
          (std::vector<K>{K::MakeBackspace(), K::MakeBackspace(), K::MakeText({'V', 'K'}), K::MakeKey(vkey::Space)}));

    // Browser autocomplete fix: U+202F + one more backspace first, not with ext_code 4 or nothing to delete.
    p.SetFixBrowserAutocomplete(true);
    r = Result(VK_WILL_PROCESS, 1, {'b'});
    plan = p.Plan(r, r.chars, r.char_count, 'S', FakeDecode);
    CHECK(plan.actions == (std::vector<K>{K::MakeText({OutputPlanner::kEmptyChar}), K::MakeBackspace(),
                                          K::MakeBackspace(), K::MakeText({'b'})}));
    r = Result(VK_WILL_PROCESS, 1, {'b'}, 4);
    plan = p.Plan(r, r.chars, r.char_count, 'S', FakeDecode);
    CHECK(plan.actions.size() == 2);
    r = Result(VK_WILL_PROCESS, 0, {'b'});
    plan = p.Plan(r, r.chars, r.char_count, 'S', FakeDecode);
    CHECK(plan.actions == (std::vector<K>{K::MakeText({'b'})}));
    p.SetFixBrowserAutocomplete(false);

    // Double-code table: a two-unit character needs two backspaces (OpenKey _syncKey).
    p.SetCodeTable(VK_TABLE_VNI_WINDOWS);
    r = Result(VK_DO_NOTHING, 0, {}, 3);  // "t" typed as is
    p.Plan(r, r.chars, r.char_count, 'T', FakeDecode);
    r = Result(VK_WILL_PROCESS, 0, {0x00650000u | 'e', 'n'});  // e + mark (2 units), n
    p.Plan(r, r.chars, r.char_count, 'N', FakeDecode);
    CHECK(p.syncKey() == (std::vector<int>{1, 2, 1}));
    r = Result(VK_WILL_PROCESS, 2, {'x', 'y'});
    plan = p.Plan(r, r.chars, r.char_count, 'S', FakeDecode);
    CHECK(plan.actions.size() == 4);  // BS (n) + BS BS (e+mark) + text
    CHECK(p.syncKey() == (std::vector<int>{1, 1, 1}));
    // Backspace key passed through on a two-unit char: one extra backspace.
    r = Result(VK_WILL_PROCESS, 0, {0x00010000u | 'o'});
    p.Plan(r, r.chars, r.char_count, 'O', FakeDecode);
    r = Result(VK_DO_NOTHING, 0, {}, 2);
    plan = p.Plan(r, r.chars, r.char_count, vkey::Back, FakeDecode);
    CHECK(plan.passThrough && plan.actions == (std::vector<K>{K::MakeBackspace()}));
    r = Result(VK_DO_NOTHING, 0, {}, 1);  // word break clears
    p.Plan(r, r.chars, r.char_count, vkey::Space, FakeDecode);
    CHECK(p.syncKey().empty());

    const std::vector<KeyStroke> ks =
        ToKeyStrokes({K::MakeBackspace(), K::MakeText({0xD83D, 0xDE00}), K::MakeKey(vkey::Space)});
    CHECK(ks.size() == 8);
    CHECK((ks[0] == KeyStroke{vkey::Back, 0, false, false}) && (ks[1] == KeyStroke{vkey::Back, 0, false, true}));
    CHECK((ks[2] == KeyStroke{0, 0xD83D, true, false}) && (ks[3] == KeyStroke{0, 0xD83D, true, true}));
    CHECK((ks[4] == KeyStroke{0, 0xDE00, true, false}) && (ks[7] == KeyStroke{vkey::Space, 0, false, true}));

#ifdef _WIN32
    const std::vector<INPUT> in = ToInputs(ks);
    CHECK(in.size() == ks.size());
    for (const INPUT& i : in) CHECK(i.type == INPUT_KEYBOARD && i.ki.dwExtraInfo == kInjectedTag);
    CHECK(in[0].ki.wVk == VK_BACK && in[0].ki.dwFlags == 0 && in[1].ki.dwFlags == KEYEVENTF_KEYUP);
    CHECK(in[2].ki.wVk == 0 && in[2].ki.wScan == 0xD83D && in[2].ki.dwFlags == KEYEVENTF_UNICODE);
    CHECK(in[3].ki.dwFlags == (KEYEVENTF_UNICODE | KEYEVENTF_KEYUP));
    CHECK(Utf8ToWide(u8"Tiếng Việt") == L"Ti\u1EBFng Vi\u1EC7t");
    CHECK(WideToUtf8(L"\u0111\U0001F600") == u8"đ😀");
#endif
}

// ---- end to end: engine with Windows virtual-key codes -> planner -> simulated text field -------

class Field {
public:
    explicit Field(const Config& c) : engine_(vk_create()) {
        vk_set_option(engine_, VK_OPT_LANGUAGE, c.vietnamese);
        vk_set_option(engine_, VK_OPT_INPUT_TYPE, c.inputType);
        vk_set_option(engine_, VK_OPT_CODE_TABLE, c.codeTable);
        vk_set_option(engine_, VK_OPT_CHECK_SPELLING, c.checkSpelling);
        vk_set_option(engine_, VK_OPT_MODERN_ORTHOGRAPHY, c.modernOrthography);
        vk_set_option(engine_, VK_OPT_QUICK_TELEX, c.quickTelex);
        vk_set_option(engine_, VK_OPT_RESTORE_IF_WRONG_SPELLING, c.restoreIfWrongSpelling);
        vk_set_option(engine_, VK_OPT_USE_MACRO, c.useMacro);
        vk_set_option(engine_, VK_OPT_USE_MACRO_IN_ENGLISH_MODE, c.macroInEnglishMode);
        vk_set_option(engine_, VK_OPT_AUTO_CAPS_MACRO, c.autoCapsMacro);
        vk_macro_load(engine_, nullptr, 0);
        for (const auto& m : c.macros) vk_macro_add(engine_, m.abbreviation.c_str(), m.expansion.c_str());
        planner_.SetCodeTable(c.codeTable);
        planner_.SetFixBrowserAutocomplete(c.fixBrowserAutocomplete);
    }
    ~Field() { vk_destroy(engine_); }

    // Types ASCII text; '\b' is Backspace. Mirrors TypingService's key path.
    Field& Type(const std::string& keys) {
        for (const char ch : keys) Key(ch);
        return *this;
    }
    std::string Text() const { return ToUtf8(text_); }

private:
    vk_engine* engine_;
    OutputPlanner planner_;
    std::vector<uint32_t> macroWords_;
    std::u16string text_;

    static void VkOf(char ch, uint16_t& vk, bool& shift) {
        shift = false;
        if (ch >= 'a' && ch <= 'z') vk = uint16_t(ch - 'a' + 'A');
        else if (ch >= 'A' && ch <= 'Z') { vk = uint16_t(ch); shift = true; }
        else if (ch >= '0' && ch <= '9') vk = uint16_t(ch);
        else if (ch == ' ') vk = vkey::Space;
        else if (ch == '\b') vk = vkey::Back;
        else if (ch == '.') vk = 0xBE;
        else if (ch == ',') vk = 0xBC;
        else if (ch == '[') vk = 0xDB;
        else if (ch == ']') vk = 0xDD;
        else vk = 0;
    }

    // What the app does with a real key press (US layout).
    void Press(uint16_t vk, bool shift) {
        if (vk == vkey::Back) { if (!text_.empty()) text_.pop_back(); return; }
        if (vk == vkey::Space) { text_ += u' '; return; }
        if (vk >= 'A' && vk <= 'Z') { text_ += char16_t(shift ? vk : vk - 'A' + 'a'); return; }
        if (vk >= '0' && vk <= '9') { text_ += char16_t(vk); return; }
        if (vk == 0xBE) text_ += u'.';
        if (vk == 0xBC) text_ += u',';
        if (vk == 0xDB) text_ += u'[';
        if (vk == 0xDD) text_ += u']';
    }

    void Apply(const std::vector<OutputAction>& actions, bool shift) {
        for (const auto& a : actions) {
            switch (a.kind) {
            case OutputAction::Kind::Backspace: Press(vkey::Back, false); break;
            case OutputAction::Kind::Text: text_.append(a.units.begin(), a.units.end()); break;
            case OutputAction::Kind::Key: Press(a.vk, shift); break;
            }
        }
    }

    void Key(char ch) {
        uint16_t vk;
        bool shift;
        VkOf(ch, vk, shift);
        const vk_result r = vk_handle_key(engine_, vk, EngineModifiers(shift ? uint32_t(kModShift) : 0u, false), VK_EVENT_KEY_DOWN);
        const uint32_t* words = r.chars;
        size_t count = r.char_count;
        if (r.code == VK_REPLACE_MACRO && r.macro_total > r.char_count) {
            macroWords_.resize(r.macro_total);
            count = vk_last_macro_words(engine_, 0, macroWords_.data(), macroWords_.size());
            words = macroWords_.data();
        }
        const OutputPlan plan = planner_.Plan(r, words, count, vk, [this](uint32_t w, uint16_t out[2]) {
            return vk_word_decode(engine_, w, out);
        });
        if (r.code == VK_RESTORE_AND_START_NEW_SESSION) vk_new_session(engine_);
        if (plan.passThrough) Press(vk, shift);  // the original key reaches the app first
        Apply(plan.actions, shift);
    }
};

static void TestEndToEnd() {
    Config c;
    CHECK_EQ_STR(Field(c).Type("Tieengs Vieetj ").Text(), u8"Tiếng Việt ");
    CHECK_EQ_STR(Field(c).Type("dduowcj nguwowif ").Text(), u8"được người ");
    CHECK_EQ_STR(Field(c).Type("hoaf khoer thuyr ").Text(), u8"hoà khoẻ thuỷ ");
    CHECK_EQ_STR(Field(c).Type("quowr tuyeetj ").Text(), u8"quở tuyệt ");
    CHECK_EQ_STR(Field(c).Type("text ").Text(), "text ");  // restored English word
    CHECK_EQ_STR(Field(c).Type("tieengs\b\b ").Text(), u8"tiế ");
    {
        Config old = c;
        old.modernOrthography = false;
        CHECK_EQ_STR(Field(old).Type("hoaf ").Text(), u8"hòa ");
    }
    {
        Config vni = c;
        vni.inputType = VK_INPUT_VNI;
        CHECK_EQ_STR(Field(vni).Type("Tie61ng Vie65t d9u7o7c5 ").Text(), u8"Tiếng Việt được ");
    }
    {
        Config fix = c;
        fix.fixBrowserAutocomplete = true;
        CHECK_EQ_STR(Field(fix).Type("Tieengs Vieetj ").Text(), u8"Tiếng Việt ");
    }
    {
        Config compound = c;
        compound.codeTable = VK_TABLE_UNICODE_COMPOUND;
        // Unicode compound: precomposed base (ê) + combining tone mark (U+0323), see vk_word_decode.
        const std::string got = Field(compound).Type("vieetj ").Text();
        CHECK_EQ_STR(got, u8"vi\u00EA\u0323t ");
        CHECK_EQ_STR(Field(compound).Type("vieetj\b\b ").Text(), "vi ");  // _syncKey: 2 units deleted
    }
    {
        Config vniWin = c;
        vniWin.codeTable = VK_TABLE_VNI_WINDOWS;
        const std::string got = Field(vniWin).Type("vieetj ").Text();
        CHECK(got.size() > 4 && got.substr(0, 2) == "vi" && got.back() == ' ');
        CHECK_EQ_STR(Field(vniWin).Type("vieetj\b\b ").Text(), "vi ");
    }
    {
        Config m = c;
        m.macros = {{"vk", "VisKey"}, {"ko", u8"không"}, {"cuoi", u8"😀"}};
        CHECK_EQ_STR(Field(m).Type("vk ko Ko ").Text(), u8"VisKey không Không ");
        CHECK_EQ_STR(Field(m).Type("cuoi ").Text(), u8"😀 ");
        std::string longText;
        for (int i = 0; i < 20; i++) longText += u8"Tiếng Việt";  // 200 characters
        m.macros = {{"dai", longText}};
        CHECK_EQ_STR(Field(m).Type("dai.").Text(), longText + ".");
    }
    {
        Config en = c;
        en.vietnamese = false;
        CHECK_EQ_STR(Field(en).Type("Tieengs Vieetj ").Text(), "Tieengs Vieetj ");
        en.macroInEnglishMode = true;
        en.macros = {{"vk", "VisKey"}};
        CHECK_EQ_STR(Field(en).Type("vk ").Text(), "VisKey ");
    }
}

int main() {
    TestJson();
    TestConfig();
    TestKeymap();
    TestPlanner();
    TestEndToEnd();
    if (g_failures) {
        std::fprintf(stderr, "%d of %d checks FAILED\n", g_failures, g_checks);
        return 1;
    }
    std::printf("win_keymap_test: all %d checks passed\n", g_checks);
    return 0;
}
