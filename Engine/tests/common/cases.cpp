// SPDX-License-Identifier: GPL-3.0-or-later
#include "cases.h"
#include "platforms/mac.h"
#include <cstdio>
#include <map>
#include <stdexcept>

namespace vktest {
namespace {

// ---- key DSL -------------------------------------------------------------
// Plain characters are typed on a US keyboard; A-Z are typed with Shift.
// Specials: <bs> <esc> <enter> <tab> <left> <mouse> <tempspell> <tempoff>
struct KeyTok { enum Kind { Key, Mouse, TempSpell, TempOff } kind = Key; uint16_t code = 0; int caps = 0; bool other = false; std::string text; };

const std::map<char, int>& charMap() {
    static const std::map<char, int> m = {
        {'a', KEY_A}, {'b', KEY_B}, {'c', KEY_C}, {'d', KEY_D}, {'e', KEY_E}, {'f', KEY_F}, {'g', KEY_G},
        {'h', KEY_H}, {'i', KEY_I}, {'j', KEY_J}, {'k', KEY_K}, {'l', KEY_L}, {'m', KEY_M}, {'n', KEY_N},
        {'o', KEY_O}, {'p', KEY_P}, {'q', KEY_Q}, {'r', KEY_R}, {'s', KEY_S}, {'t', KEY_T}, {'u', KEY_U},
        {'v', KEY_V}, {'w', KEY_W}, {'x', KEY_X}, {'y', KEY_Y}, {'z', KEY_Z},
        {'0', KEY_0}, {'1', KEY_1}, {'2', KEY_2}, {'3', KEY_3}, {'4', KEY_4}, {'5', KEY_5}, {'6', KEY_6},
        {'7', KEY_7}, {'8', KEY_8}, {'9', KEY_9}, {' ', KEY_SPACE}, {',', KEY_COMMA}, {'.', KEY_DOT},
        {'/', KEY_SLASH}, {';', KEY_SEMICOLON}, {'\'', KEY_QUOTE}, {'[', KEY_LEFT_BRACKET},
        {']', KEY_RIGHT_BRACKET}, {'\\', KEY_BACK_SLASH}, {'-', KEY_MINUS}, {'=', KEY_EQUALS},
        {'`', KEY_BACKQUOTE}};
    return m;
}

std::vector<KeyTok> parseKeys(const std::string& s) {
    std::vector<KeyTok> out;
    for (size_t i = 0; i < s.size();) {
        KeyTok t;
        if (s[i] == '<') {
            size_t e = s.find('>', i);
            if (e == std::string::npos) throw std::runtime_error("bad key DSL: " + s);
            std::string n = s.substr(i + 1, e - i - 1);
            i = e + 1;
            t.text = "<" + n + ">";
            if (n == "bs") t.code = KEY_DELETE;
            else if (n == "esc") t.code = KEY_ESC;
            else if (n == "enter") t.code = KEY_RETURN;
            else if (n == "tab") t.code = KEY_TAB;
            else if (n == "left") t.code = KEY_LEFT;
            else if (n == "mouse") t.kind = KeyTok::Mouse;
            else if (n == "tempspell") t.kind = KeyTok::TempSpell;
            else if (n == "tempoff") t.kind = KeyTok::TempOff;
            else if (n == "cmd-a") { t.code = KEY_A; t.other = true; }
            else throw std::runtime_error("unknown token " + n);
        } else {
            char c = s[i++];
            t.text = (c == ' ') ? std::string("<sp>") : std::string(1, c);
            if (c >= 'A' && c <= 'Z') { t.caps = 1; c = char(c - 'A' + 'a'); }
            static const std::map<char, char> shifted = {{'!', '1'}, {'@', '2'}, {'#', '3'}, {'$', '4'}, {'%', '5'}, {'^', '6'},
                {'&', '7'}, {'*', '8'}, {'(', '9'}, {')', '0'}, {'?', '/'}, {':', ';'}, {'"', '\''}, {'_', '-'}, {'+', '='}};
            auto sh = shifted.find(c);
            if (sh != shifted.end()) { t.caps = 1; c = sh->second; }
            auto it = charMap().find(c);
            if (it == charMap().end()) throw std::runtime_error(std::string("unmapped char ") + c);
            t.code = uint16_t(it->second);
        }
        out.push_back(t);
    }
    return out;
}

// ---- screen simulation --------------------------------------------------
using Glyph = std::vector<uint16_t>;

void appendUtf8(std::string& s, uint32_t cp) {
    if (cp < 0x80) s += char(cp);
    else if (cp < 0x800) { s += char(0xC0 | (cp >> 6)); s += char(0x80 | (cp & 0x3F)); }
    else { s += char(0xE0 | (cp >> 12)); s += char(0x80 | ((cp >> 6) & 0x3F)); s += char(0x80 | (cp & 0x3F)); }
}

std::string renderScreen(const std::vector<Glyph>& screen, int codeTable) {
    std::string s;
    for (const auto& g : screen) {
        for (uint16_t u : g) {
            if (u < 0x80) {
                if (u == '\n') s += "\\n"; else if (u == '\t') s += "\\t"; else s += char(u);
            } else if (codeTable == 0 || codeTable == 3) {
                appendUtf8(s, u);
            } else {
                char b[8]; std::snprintf(b, sizeof b, "<%02X>", u & 0xFFFF); s += b;
            }
        }
    }
    return s;
}

std::string hexList(const std::vector<uint32_t>& v) {
    std::string s;
    for (size_t i = 0; i < v.size(); i++) {
        char b[16]; std::snprintf(b, sizeof b, i ? ",%X" : "%X", v[i]); s += b;
    }
    return s.empty() ? "-" : s;
}

void pop(std::vector<Glyph>& screen, int n) {
    while (n-- > 0 && !screen.empty()) screen.pop_back();
}

void passthrough(Driver& d, std::vector<Glyph>& screen, const KeyTok& k) {
    if (k.code == KEY_DELETE) { if (!screen.empty()) screen.pop_back(); return; }
    uint16_t ch = d.keyChar(uint32_t(k.code) | (k.caps ? 0x10000u : 0u));
    if (k.code == KEY_RETURN) ch = '\n';
    if (k.code == KEY_TAB) ch = '\t';
    if (ch) screen.push_back(Glyph{ch});
}

void appendWords(Driver& d, std::vector<Glyph>& screen, const std::vector<uint32_t>& words) {
    for (uint32_t w : words) {
        Glyph g;
        d.decodeWord(w, g);
        if (!g.empty()) screen.push_back(g);
    }
}

} // namespace

std::string renderCase(Driver& d, const Case& c) {
    d.begin(c.cfg);
    for (const auto& m : c.macros) d.addMacro(m.first, m.second);
    std::vector<Glyph> screen;
    std::string out = "## " + c.id + (c.tag.empty() ? "" : " " + c.tag) + "\n";
    char hdr[256];
    std::snprintf(hdr, sizeof hdr, "cfg: input=%d table=%d lang=%d spell=%d modern=%d quick=%d restore=%d macro=%d/%d autocaps=%d upper1=%d zfwj=%d qstart=%d qend=%d free=%d\n",
                  c.cfg.inputType, c.cfg.codeTable, c.cfg.language, c.cfg.checkSpelling, c.cfg.modernOrthography,
                  c.cfg.quickTelex, c.cfg.restoreIfWrongSpelling, c.cfg.useMacro, c.cfg.macroInEnglishMode,
                  c.cfg.autoCapsMacro, c.cfg.upperCaseFirstChar, c.cfg.allowConsonantZFWJ, c.cfg.quickStartConsonant,
                  c.cfg.quickEndConsonant, c.cfg.freeMark);
    out += hdr;
    out += "keys: " + c.keys + "\n";
    for (const auto& k : parseKeys(c.keys)) {
        if (k.kind == KeyTok::Mouse) { if (!c.english) d.mouse(); out += "  " + k.text + " mouse\n"; continue; }
        if (k.kind == KeyTok::TempSpell) { d.tempOffSpelling(); out += "  " + k.text + " tempspell\n"; continue; }
        if (k.kind == KeyTok::TempOff) { d.tempOffEngine(); out += "  " + k.text + " tempoff\n"; continue; }
        Step s = c.english ? d.englishKey(k.code, k.caps, k.other) : d.key(k.code, k.caps, k.other);
        char line[160];
        std::snprintf(line, sizeof line, "  %-8s c=%d b=%d n=%d e=%d w=", k.text.c_str(), s.code, s.backspace, s.newChars, s.ext);
        out += line + hexList(s.words) + " m=" + hexList(s.macro) + "\n";
        // Apply to the simulated screen the way the macOS bridge would.
        switch (s.code) {
        case 0: // vDoNothing: key is passed through to the app
            if (c.english) { if (!k.other) passthrough(d, screen, k); }
            else if (s.ext == 2) pop(screen, 1);
            else if (!k.other) passthrough(d, screen, k);
            break;
        case 1: // vWillProcess
            pop(screen, s.backspace); appendWords(d, screen, s.words); break;
        case 3: // vRestore
        case 5: // vRestoreAndStartNewSession
            pop(screen, s.backspace); appendWords(d, screen, s.words); passthrough(d, screen, k); break;
        case 4: // vReplaceMaro
            pop(screen, s.backspace); appendWords(d, screen, s.macro); passthrough(d, screen, k); break;
        default: break;
        }
    }
    out += "=> " + renderScreen(screen, c.cfg.codeTable) + "\n\n";
    return out;
}

std::string renderConvertCase(Driver& d, const ConvertCase& c) {
    char b[160];
    std::snprintf(b, sizeof b, "## %s\nconvert: from=%d to=%d caps=%d lower=%d first=%d each=%d nomark=%d\n", c.id.c_str(),
                  c.opt.from, c.opt.to, c.opt.allCaps, c.opt.allLower, c.opt.capsFirst, c.opt.capsEachWord, c.opt.removeMark);
    std::string out = b;
    std::string r = d.convert(c.opt, c.input);
    out += "in:  " + c.input + "\nout: " + r + "\nhex: ";
    for (unsigned char ch : r) { std::snprintf(b, sizeof b, "%02X", ch); out += b; }
    return out + "\n\n";
}

std::string renderSmartSwitch(Driver& d) {
    auto r = d.smartSwitch({{"com.apple.Terminal", 0}, {"com.apple.TextEdit", 1}, {"com.google.Chrome", 0}, {"com.apple.Terminal", 1}},
                           {"com.apple.Terminal", "com.apple.TextEdit", "com.google.Chrome", "com.unknown.app"}, 1);
    std::string out = "## smartswitch/basic\nresults:";
    for (int v : r) out += " " + std::to_string(v);
    return out + "\n\n";
}

std::string renderMacroBlob(Driver& d) {
    auto blob = d.macroBlobRoundTrip();
    std::string out = "## macro/blob\nhex: ";
    char b[8];
    for (uint8_t v : blob) { std::snprintf(b, sizeof b, "%02X", v); out += b; }
    return out + "\n\n";
}

// ---- case tables -----------------------------------------------------------
namespace {

Case mk(const std::string& id, const std::string& keys, Config cfg = {}, const std::string& tag = "") {
    Case c; c.id = id; c.keys = keys; c.cfg = cfg; c.tag = tag; return c;
}

std::vector<Case> build() {
    std::vector<Case> v;
    Config telex;                       // defaults: Telex, Unicode, spelling on, old orthography
    Config modern; modern.modernOrthography = 1;

    // 1. Mandatory cases from CLAUDE.md §3 -----------------------------------
    v.push_back(mk("must/quon", "quowrn ", telex));                 // quởn
    v.push_back(mk("must/tuyet", "tuyeetj ", telex));               // tuyệt
    v.push_back(mk("must/quet", "quets ", telex));                  // quét
    { Config c; c.inputType = 1; v.push_back(mk("must/dui9-vni", "dui9 ", c)); }
    { Config c; c.inputType = 1; v.push_back(mk("must/duoi96-vni", "duoi96 ", c)); }
    v.push_back(mk("must/tuyps", "tuyps ", telex));
    v.push_back(mk("must/chua-a-issue312", "chuwaa ", telex, "[known-bug]")); // chưa + a -> chưâ (issue #312)
    v.push_back(mk("must/chua-a-issue312-separate", "chuwa" "a", telex, "[known-bug]"));
    v.push_back(mk("must/hoa-modern", "hoaf ", modern));
    v.push_back(mk("must/khoe-modern", "khoer ", modern));
    v.push_back(mk("must/thuy-modern", "thuyr ", modern));
    v.push_back(mk("must/hoa-old", "hoaf ", telex));
    v.push_back(mk("must/khoe-old", "khoer ", telex));
    v.push_back(mk("must/thuy-old", "thuyr ", telex));
    v.push_back(mk("must/tuy-del-a", "tuyfa<bs>", modern));         // restore mark when deleting a char
    v.push_back(mk("must/tuy-del-a-old", "tuyfa<bs>", telex));
    v.push_back(mk("must/tuy-del-a-noaccent", "tuyfa<bs><bs>", telex));

    // 2. Quick Telex ------------------------------------------------------------
    { Config c; c.quickTelex = 1;
      for (const char* w : {"cc", "gg", "kk", "nn", "qq", "pp", "tt", "uu"})
          v.push_back(mk(std::string("quicktelex/") + w, std::string("a") + w + " ", c));
      v.push_back(mk("quicktelex/cc-start", "cca ", c));
      v.push_back(mk("quicktelex/nhieefu", "nnhieeu ", c));
      v.push_back(mk("quicktelex/words", "ccos ttrasn ggias ", c));
      Config off; v.push_back(mk("quicktelex/off", "acc agg ", off)); }

    // 3. Consonants f j w z, quick start/end ------------------------------------
    { Config c; c.allowConsonantZFWJ = 1;
      for (const char* w : {"fa", "ja", "wa", "za", "fan", "jam", "wen", "zoo", "fanh"})
          v.push_back(mk(std::string("zfwj/") + w, std::string(w) + " ", c));
      Config off; v.push_back(mk("zfwj/off-fa", "fa ", off)); v.push_back(mk("zfwj/off-wen", "wen ", off)); }
    { Config c; c.quickStartConsonant = 1;
      for (const char* w : {"fanh", "jang", "wen", "fai", "jo", "wa", "fim"})
          v.push_back(mk(std::string("qstart/") + w, std::string(w) + " ", c)); }
    { Config c; c.quickEndConsonant = 1;
      for (const char* w : {"hag", "vih", "bak", "taj", "lah", "mak", "dag"})
          v.push_back(mk(std::string("qend/") + w, std::string(w) + " ", c));
      v.push_back(mk("qend/sentence", "tooi dag di, vih danh. ", c)); }
    { Config c; c.quickStartConsonant = 1; c.quickEndConsonant = 1; c.quickTelex = 1;
      v.push_back(mk("qall/mixed", "fanh hag ccos nnhaf ", c)); }

    // 4. Restore wrong words -----------------------------------------------------
    { Config c; c.restoreIfWrongSpelling = 1;
      for (const char* w : {"text", "expect", "user", "window", "service", "first", "worrk", "tesst", "ass", "bass", "kick"})
          v.push_back(mk(std::string("restore/") + w, std::string(w) + " ", c));
      v.push_back(mk("restore/comma", "text, ", c));
      v.push_back(mk("restore/dot", "user.", c));
      v.push_back(mk("restore/enter", "expect<enter>", c));
      v.push_back(mk("restore/tab", "text<tab>", c));
      v.push_back(mk("restore/right-vn", "viet nam ", c));
      Config off; for (const char* w : {"text", "user", "expect"})
          v.push_back(mk(std::string("restore-off/") + w, std::string(w) + " ", off));
      Config nospell; nospell.checkSpelling = 0;
      for (const char* w : {"text", "user", "tesst", "window"})
          v.push_back(mk(std::string("nospell/") + w, std::string(w) + " ", nospell)); }

    // 5. Telex vocabulary (Unicode), old orthography ---------------------------
    const std::vector<std::string> telexWords = {
        "vieetj", "Vieetj", "VIEETJ", "nguwowif", "dduwowngf", "ddaats", "Ddaats", "DDaats", "quoocs", "gias", "giaf",
        "gixa", "giuwx", "huyeenf", "khuyeens", "nghieeng", "nghieeems", "nghiax", "trangw", "traawng", "hoocj",
        "thaayf", "coong", "tooi", "ddi", "ddeens", "mowis", "muwowif", "chuyeenx", "chuyeenj", "xuyeen", "uyeen",
        "oanh", "hoaanh", "loanh", "quanj", "quaan", "quyeets", "quyr", "quyf", "luaatj", "luaan", "tuaans",
        "ngoaif", "ngoaair", "toanf", "hoang", "khoeeo", "boongf", "buoonf", "cuoocj", "ruoouj", "nuowcs",
        "ddiuf", "dduowcj", "uwowng", "ow", "uw", "aw", "aa", "ee", "oo", "dd", "ddd", "aaa", "ooo", "eee",
        "Hanoij", "Hoof", "Chis", "Minh", "Thuyr", "Tuyeenr", "QUOCS", "NGUWOWIF", "Nguwowif",
        "thuowngr", "dduowngs", "ruwowngf", "nhuwx", "tuwj", "suwr", "muwf", "chuwowngg", "kiemr", "khiems",
        "yeeu", "yeeus", "yeenj", "ysj", "yss", "ays", "aiz", "aus", "aur", "eus", "oir", "oif",
        "giuoongf", "quyenf", "quynhf", "nghiej", "ngheej", "nghee", "ngheet", "ngheaj"};
    for (size_t i = 0; i < telexWords.size(); i++)
        v.push_back(mk("telex/" + telexWords[i], telexWords[i] + " ", telex));
    // modern orthography on the oa/oe/uy family
    for (const char* w : {"hoaf", "hoas", "hoar", "hoax", "hoaj", "khoer", "khoes", "khoef", "thuyr", "thuyf", "thuys", "thuyj",
                          "quys", "quyf", "toanf", "loanj", "xoaf", "xoay", "uys", "oes", "oaf"})
        v.push_back(mk(std::string("telex-modern/") + w, std::string(w) + " ", modern));
    // mark placement keys typed late / early
    for (const char* w : {"vietj", "vieetj", "vieejt", "tojan", "toaanj", "huyeenf", "huyfeen", "soos", "sooos", "ddaaats",
                          "chuwa", "chuaw", "chua" "w", "nguoiwf", "nguowif", "dduowcj", "dduocwj"})
        v.push_back(mk(std::string("telex-order/") + w, std::string(w) + " ", telex));

    // 6. Telex words across the four other code tables -------------------------
    const std::vector<std::string> tableWords = {
        "vieetj", "nguwowif", "dduowngf", "ddaats", "quoocs", "hoaf", "khoer", "thuyr", "quowrn", "tuyeetj", "quets",
        "chuwa", "Vieetj", "uwowng", "hoocj", "muwowif", "nghieeng", "traawng", "ddiuf", "yeeu", "Hanoij", "tooi",
        "gias", "xin", "chaof", "ca", "tuw", "nhuwx", "luaatj", "quyeets"};
    for (int table : {1, 2, 3, 4}) {
        Config c = modern; c.codeTable = table;
        for (const auto& w : tableWords)
            v.push_back(mk("table" + std::to_string(table) + "/" + w, w + " ", c));
    }
    // delete in double-byte tables (VNI / compound), key lengths matter
    for (int table : {2, 3}) {
        Config c = modern; c.codeTable = table;
        v.push_back(mk("table" + std::to_string(table) + "/delete-mid", "vieetj<bs><bs>", c));
        v.push_back(mk("table" + std::to_string(table) + "/delete-all", "nguwowif<bs><bs><bs><bs><bs><bs>", c));
        v.push_back(mk("table" + std::to_string(table) + "/two-words", "ddaats nuwowcs ", c));
    }

    // 7. VNI input ----------------------------------------------------------------
    { Config c; c.inputType = 1;
      const std::vector<std::string> vni = {
          "vie6t5", "Vie6t5", "ngu7o7i2", "d9u7o7ng2", "d9a61t", "d9a6t1", "quo6c1", "ho2a", "kho3e", "thu3y",
          "tuye6t5", "quo7r3n", "qu7o7n3", "toi1", "a1", "a2", "a3", "a4", "a5", "a6", "a8", "o6", "o7", "u7",
          "e6", "d9", "D9", "a61", "a81", "u7o7", "uo7", "uo73", "tru7o7ng2", "nghie6ng", "gia1", "gia2", "hoa1",
          "khoe3", "thuy3", "quy2", "xin1", "chao2", "viet65", "9", "a99", "o66", "a88", "e66", "tu7o7i3", "ye6u"};
      for (const auto& w : vni) v.push_back(mk("vni/" + w, w + " ", c));
      Config m = c; m.modernOrthography = 1;
      for (const char* w : {"ho2a", "kho3e", "thu3y", "hoa2", "khoe3", "thuy3"})
          v.push_back(mk(std::string("vni-modern/") + w, std::string(w) + " ", m));
      // digits mean numbers when no word is in progress
      v.push_back(mk("vni/numbers", "123 a1 456 ", c));
      v.push_back(mk("vni/shift-digit", "a!", c));
      for (int table : {1, 2, 3, 4}) { Config t = c; t.codeTable = table;
          for (const char* w : {"vie6t5", "ngu7o7i2", "d9u7o7ng2", "tuye6t5", "quo6c1"})
              v.push_back(mk("vni-table" + std::to_string(table) + "/" + w, std::string(w) + " ", t)); } }

    // 8. Simple Telex -----------------------------------------------------------------
    for (int type : {2, 3}) { Config c; c.inputType = type;
        for (const char* w : {"vieetj", "nguoiwf", "uow", "aw", "ow", "ddaats", "hoaf", "tuwf", "w", "[", "]", "u[", "o]", "ng[", "quoocs"})
            v.push_back(mk("simpletelex" + std::to_string(type) + "/" + w, std::string(w) + " ", c)); }

    // 9. Backspace, mouse, word breaks -------------------------------------------------------
    for (const char* w : {"vieetj<bs>", "vieetj<bs><bs>", "vieetj<bs><bs><bs>", "vieetj<bs><bs><bs><bs><bs>",
                          "viet <bs>", "viet <bs><bs>", "viet <bs>s", "viet<mouse>j", "viet,<bs>j", "viet.<bs><bs>s",
                          "chaof<bs>o", "chaof<bs>s", "xin chaof<bs><bs><bs>", "hoaf<bs>r", "toi <bs><bs><bs>"})
        v.push_back(mk(std::string("edit/") + w, w, modern));
    v.push_back(mk("edit/mouse-resets", "vie<mouse>etj", telex));
    v.push_back(mk("edit/arrow-breaks", "vie<left>etj", telex));
    v.push_back(mk("edit/esc-breaks", "vie<esc>etj", telex));
    v.push_back(mk("edit/cmd-ignores", "vie<cmd-a>etj", telex));
    v.push_back(mk("edit/punct-break", "viet.nam,viet;nam", telex));
    v.push_back(mk("edit/number-start", "12ab ", telex));
    v.push_back(mk("edit/long-word", "nghieeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeng ", telex));
    v.push_back(mk("edit/very-long-ascii", "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz ", telex));
    v.push_back(mk("edit/long-delete", "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz<bs><bs><bs><bs><bs><bs><bs><bs><bs>x", telex));
    // temp off
    v.push_back(mk("temp/spell-off-then-type", "<tempspell>tesst ", telex));
    v.push_back(mk("temp/spell-off-vn", "<tempspell>vieetj ", telex));
    v.push_back(mk("temp/engine-off", "<tempoff>vieetj ", telex));
    v.push_back(mk("temp/engine-off-then-on", "<tempoff>vieetj vieetj ", telex));

    // 10. Upper case first character --------------------------------------------------------------
    { Config c; c.upperCaseFirstChar = 1;
      v.push_back(mk("upper1/after-dot", "xin chaof. toi la minh. ", c));
      v.push_back(mk("upper1/after-enter", "xin chaof<enter>toi ", c));
      v.push_back(mk("upper1/after-dot-vn", "ok. dduowngf ", c));
      v.push_back(mk("upper1/mid", "ok toi ", c)); }

    // 11. Free mark / spelling off ------------------------------------------------------------------
    { Config c; c.freeMark = 1;
      for (const char* w : {"vieetj", "tuyeetj", "tesst", "xyz", "oaf", "ddd", "zzs"})
          v.push_back(mk(std::string("freemark/") + w, std::string(w) + " ", c)); }
    { Config c; c.checkSpelling = 0;
      for (const char* w : {"vieetj", "tuyeetj", "tesst", "xyz", "oaf", "dd", "zzs", "ngh", "aeiou", "bcdfgs"})
          v.push_back(mk(std::string("nospell-vn/") + w, std::string(w) + " ", c)); }

    // 12. Macros -----------------------------------------------------------------------------------------
    { Config c; c.useMacro = 1;
      Case m = mk("macro/basic", "btw ", c); m.macros = {{"btw", "by the way"}}; v.push_back(m);
      m = mk("macro/comma", "btw,", c); m.macros = {{"btw", "by the way"}}; v.push_back(m);
      m = mk("macro/no-match", "bta ", c); m.macros = {{"btw", "by the way"}}; v.push_back(m);
      m = mk("macro/vn-content", "tp ", c); m.macros = {{"tp", "thành phố Hồ Chí Minh"}}; v.push_back(m);
      m = mk("macro/vn-key", "dduowngf ", c); m.macros = {{"đường", "đường phố"}}; v.push_back(m);
      m = mk("macro/two", "btw tp ", c); m.macros = {{"btw", "by the way"}, {"tp", "thành phố"}}; v.push_back(m);
      std::string longContent; for (int i = 0; i < 20; i++) longContent += "0123456789"; // 200 chars
      m = mk("macro/200-chars", "long ", c); m.macros = {{"long", longContent}}; v.push_back(m);
      std::string longVn; for (int i = 0; i < 10; i++) longVn += "tiếng Việt đẹp "; // multi-code-point content
      m = mk("macro/long-vn", "tv ", c); m.macros = {{"tv", longVn}}; v.push_back(m);
      Config ac = c; ac.autoCapsMacro = 1;
      m = mk("macro/autocaps-title", "Btw ", ac); m.macros = {{"btw", "by the way"}}; v.push_back(m);
      m = mk("macro/autocaps-upper", "BTW ", ac); m.macros = {{"btw", "by the way"}}; v.push_back(m);
      Config off; m = mk("macro/off", "btw ", off); m.macros = {{"btw", "by the way"}}; v.push_back(m);
      for (int table : {1, 2, 3, 4}) { Config t = c; t.codeTable = table;
          m = mk("macro/table" + std::to_string(table), "tp ", t); m.macros = {{"tp", "thành phố"}}; v.push_back(m); }
      // macro in English mode
      Config en = c; en.language = 0; en.macroInEnglishMode = 1;
      m = mk("macro-en/basic", "btw ", en); m.macros = {{"btw", "by the way"}}; m.english = true; v.push_back(m);
      m = mk("macro-en/after-word", "ok btw ", en); m.macros = {{"btw", "by the way"}}; m.english = true; v.push_back(m);
      m = mk("macro-en/bs", "btx<bs>w ", en); m.macros = {{"btw", "by the way"}}; m.english = true; v.push_back(m);
      m = mk("macro-en/mouse", "bt<mouse>w ", en); m.macros = {{"btw", "by the way"}}; m.english = true; v.push_back(m);
      m = mk("macro-en/200", "long ", en); m.macros = {{"long", longContent}}; m.english = true; v.push_back(m);
      Config en0 = en; en0.useMacro = 0;
      m = mk("macro-en/off", "btw ", en0); m.macros = {{"btw", "by the way"}}; m.english = true; v.push_back(m); }

    // 13. Sentences ------------------------------------------------------------------------------------------
    for (const char* s : {"Xin chaof cacs banj, tooi laf Minh. ", "Hoom nay trowif ddepj quas! ",
                          "Vieetj Nam laf mootj ddaats nuwowcs xinh ddepj. ", "Chuwowng trinhf hojc tieengs Vieetj ",
                          "Anh ddi ddaau theex? ", "Thuyr thuyr khoer khoer hoaf hoaf "}) {
        v.push_back(mk(std::string("sentence/") + s, s, telex));
        Config c = modern; c.restoreIfWrongSpelling = 1;
        v.push_back(mk(std::string("sentence-modern-restore/") + s, s, c)); }

    return v;
}

std::vector<ConvertCase> buildConvert() {
    std::vector<ConvertCase> v;
    const std::string text = "Tiếng Việt: nguời Hà Nội, đường Nguyễn Huệ, Thuỷ khoẻ hoà. Quốc ngữ.";
    int n = 0;
    auto add = [&](const std::string& what, ConvertOptions o, const std::string& in) {
        v.push_back({"convert/" + std::to_string(n++) + "-" + what, o, in}); };
    for (int to : {0, 1, 2, 3, 4}) {
        ConvertOptions o; o.from = 0; o.to = to; add("uni-to-" + std::to_string(to), o, text);
    }
    // round trip via the reverse conversion of each table's output is covered by the engine tests on their own:
    for (int from : {1, 2, 3, 4}) {
        ConvertOptions o; o.from = 0; o.to = from; ConvertOptions back; back.from = from; back.to = 0;
        add("sample-to-" + std::to_string(from), o, "đường Việt Nam");
    }
    ConvertOptions o;
    o = {}; o.allCaps = true; add("allcaps", o, text);
    o = {}; o.allLower = true; add("alllower", o, text);
    o = {}; o.capsFirst = true; add("capsfirst", o, "xin chào. tôi là minh! bạn khoẻ không? vâng");
    o = {}; o.capsEachWord = true; add("capseach", o, "xin chào các bạn. tôi là minh");
    o = {}; o.removeMark = true; add("removemark", o, text);
    o = {}; o.removeMark = true; o.allCaps = true; add("removemark-caps", o, text);
    o = {}; o.removeMark = true; o.allLower = true; add("removemark-lower", o, text);
    o = {}; o.to = 2; o.allCaps = true; add("vni-caps", o, "đường Việt");
    o = {}; o.to = 3; o.capsEachWord = true; add("compound-each", o, "đường việt nam");
    o = {}; o.to = 1; o.capsFirst = true; add("tcvn-first", o, "đường việt nam. xin chào");
    o = {}; add("ascii-passthrough", o, "hello world 123 !?");
    o = {}; add("empty", o, "");
    o = {}; add("newline", o, "dòng một\ndòng hai\n");
    return v;
}

} // namespace

const std::vector<Case>& allCases() { static const std::vector<Case> v = build(); return v; }
const std::vector<ConvertCase>& allConvertCases() { static const std::vector<ConvertCase> v = buildConvert(); return v; }

} // namespace vktest
