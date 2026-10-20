#ifndef VISKEY_ENGINE_HPP
#define VISKEY_ENGINE_HPP

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace viskey {

// UTF-8 <-> UTF-32 conversion utilities (zero external dependencies)
inline std::u32string utf8_to_u32(const std::string& s) {
    std::u32string out;
    size_t i = 0, n = s.size();
    while (i < n) {
        uint32_t cp = 0;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 0x80) {
            cp = c;
            i += 1;
        } else if ((c >> 5) == 0x6 && i + 1 < n) {
            cp = ((c & 0x1F) << 6) | (static_cast<unsigned char>(s[i + 1]) & 0x3F);
            i += 2;
        } else if ((c >> 4) == 0xE && i + 2 < n) {
            cp = ((c & 0x0F) << 12) |
                 ((static_cast<unsigned char>(s[i + 1]) & 0x3F) << 6) |
                 (static_cast<unsigned char>(s[i + 2]) & 0x3F);
            i += 3;
        } else if ((c >> 3) == 0x1E && i + 3 < n) {
            cp = ((c & 0x07) << 18) |
                 ((static_cast<unsigned char>(s[i + 1]) & 0x3F) << 12) |
                 ((static_cast<unsigned char>(s[i + 2]) & 0x3F) << 6) |
                 (static_cast<unsigned char>(s[i + 3]) & 0x3F);
            i += 4;
        } else {
            cp = c;
            i += 1;
        }
        out.push_back(static_cast<char32_t>(cp));
    }
    return out;
}

inline std::string u32_to_utf8(const std::u32string& s) {
    std::string out;
    for (char32_t ch : s) {
        uint32_t cp = static_cast<uint32_t>(ch);
        if (cp < 0x80) {
            out.push_back(static_cast<char>(cp));
        } else if (cp < 0x800) {
            out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp < 0x10000) {
            out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    }
    return out;
}

struct EngineConfig {
    std::string input_method = "telex";    // "telex" or "vni"
    bool modern_orthography = false;       // false: hòa, khỏe, thúy; true: hoà, khoẻ, thuý
    bool auto_restore_english = true;      // Auto-restore invalid Vietnamese syllables to English
    bool quick_telex = false;              // cc=ch, gg=gi, kk=kh, nn=ng, qq=qu, pp=ph, tt=th
    bool enable_macros = true;             // Expand shorthand macros on word boundary
    bool fix_autocomplete = true;          // Emit autocomplete fix flag for Spotlight/Browser omnibox
    std::map<std::string, std::string> macros = {
        {"vn", "Việt Nam"},
        {"ko", "không"},
        {"dc", "được"},
        {"uh", "ừ"},
        {"ntn", "như thế nào"},
        {"bt", "bình thường"},
        {"mn", "mọi người"}
    };
};

struct KeyProcessResult {
    bool handled = false;
    int backspaces = 0;
    std::string commit_text;
    std::string current_word;
    std::string raw_word;
    bool autocomplete_fix_needed = false;
};

class VisKeyEngine {
public:
    explicit VisKeyEngine(const EngineConfig& cfg = EngineConfig()) : config_(cfg) {
        init_tables();
        reset();
    }

    void set_config(const EngineConfig& cfg) { config_ = cfg; }
    EngineConfig& config() { return config_; }
    const EngineConfig& config() const { return config_; }

    void reset() {
        raw_keys_.clear();
        chars_.clear();
        tone_ = 0;
        disabled_for_word_ = false;
        has_backspaced_ = false;
        last_modifier_key_ = 0;
    }

    std::u32string get_displayed_u32() const {
        // Invariant: while disabled_for_word_, chars_ holds exactly the literal text on screen
        // and tone_ is 0, so rendering it is a no-op.
        if (disabled_for_word_) {
            return chars_;
        }
        return render_syllable();
    }

    std::string get_displayed_word() const {
        return u32_to_utf8(get_displayed_u32());
    }

    KeyProcessResult process_backspace() {
        if (raw_keys_.empty() && chars_.empty()) {
            return {false, 0, "", "", "", false};
        }
        has_backspaced_ = true;
        std::u32string old_display = get_displayed_u32();
        if (old_display.empty()) {
            reset();
            return {false, 0, "", "", "", false};
        }

        if (disabled_for_word_) {
            if (!raw_keys_.empty()) raw_keys_.pop_back();
            chars_.pop_back();
            std::string new_disp = u32_to_utf8(chars_);
            if (chars_.empty()) reset();
            return {false, 0, "", new_disp, new_disp, false};
        }

        std::string initial, vowel, final_c;
        std::vector<int> vowel_indices;
        split_syllable(initial, vowel, final_c, vowel_indices);
        int target_idx = find_tone_target_index(initial, vowel, final_c, vowel_indices);
        int removed_idx = static_cast<int>(chars_.size()) - 1;

        chars_.pop_back();
        if (!raw_keys_.empty()) raw_keys_.pop_back();

        if (chars_.empty()) {
            reset();
            return {false, 0, "", "", "", false};
        }

        std::string n_init, n_vowel, n_final;
        std::vector<int> n_v_idx;
        split_syllable(n_init, n_vowel, n_final, n_v_idx);
        if (n_v_idx.empty() || removed_idx == target_idx) {
            tone_ = 0;
        }

        std::u32string new_display = get_displayed_u32();
        std::string new_utf8 = u32_to_utf8(new_display);
        std::string raw_utf8 = u32_to_utf8(raw_keys_);

        if (old_display.size() == new_display.size() + 1 &&
            old_display.substr(0, new_display.size()) == new_display) {
            return {false, 0, "", new_utf8, raw_utf8, false};
        }

        return {
            true,
            static_cast<int>(old_display.size()),
            new_utf8,
            new_utf8,
            raw_utf8,
            config_.fix_autocomplete
        };
    }

    KeyProcessResult process_key(char ch) {
        char32_t cp = static_cast<unsigned char>(ch);
        bool is_alpha = std::isalpha(static_cast<unsigned char>(ch)) != 0;
        bool is_digit = std::isdigit(static_cast<unsigned char>(ch)) != 0;

        if (!(is_alpha || (config_.input_method == "vni" && is_digit && !raw_keys_.empty()))) {
            std::u32string old_display = get_displayed_u32();
            std::string raw_utf8 = u32_to_utf8(raw_keys_);
            std::string ch_str(1, ch);

            // 1. Check Macro expansion
            if (config_.enable_macros && !raw_utf8.empty()) {
                std::string raw_lower = to_ascii_lower(raw_utf8);
                auto it = config_.macros.find(raw_lower);
                if (it != config_.macros.end()) {
                    std::string expanded = apply_case_pattern(raw_utf8, it->second);
                    reset();
                    return {
                        true,
                        static_cast<int>(old_display.size()),
                        expanded + ch_str,
                        "",
                        "",
                        config_.fix_autocomplete
                    };
                }
            }

            // 2. Check Word-Boundary Auto-Restore for English words
            //    (after an escape the literal text is kept unless the raw keys spell a known English word)
            if (config_.auto_restore_english &&
                !has_backspaced_ &&
                !raw_keys_.empty() &&
                old_display != raw_keys_ &&
                (disabled_for_word_ ? english_collision_words_.count(to_ascii_lower(raw_utf8)) > 0
                                    : !is_valid_vietnamese(true))) {
                reset();
                return {
                    true,
                    static_cast<int>(old_display.size()),
                    raw_utf8 + ch_str,
                    "",
                    "",
                    config_.fix_autocomplete
                };
            }

            reset();
            return {false, 0, "", "", "", false};
        }

        std::u32string old_display = get_displayed_u32();
        process_char_internal(cp);
        std::u32string new_display = get_displayed_u32();
        std::string new_utf8 = u32_to_utf8(new_display);
        std::string raw_utf8 = u32_to_utf8(raw_keys_);

        if (new_display == old_display + std::u32string(1, cp)) {
            return {false, 0, std::string(1, ch), new_utf8, raw_utf8, false};
        }

        size_t common_len = 0;
        size_t min_len = std::min(old_display.size(), new_display.size());
        while (common_len < min_len && old_display[common_len] == new_display[common_len]) {
            common_len++;
        }

        int backspaces = static_cast<int>(old_display.size() - common_len);
        std::string commit_utf8 = u32_to_utf8(new_display.substr(common_len));
        bool ac_fix = config_.fix_autocomplete && (common_len == 0) && (old_display.size() == 1);

        return {true, backspaces, commit_utf8, new_utf8, raw_utf8, ac_fix};
    }

    std::string feed_sequence(const std::string& text) {
        reset();
        std::u32string screen;
        for (char c : text) {
            if (c == '\b') {
                auto res = process_backspace();
                if (res.handled) {
                    for (int i = 0; i < res.backspaces && !screen.empty(); ++i) screen.pop_back();
                    auto commit_u32 = utf8_to_u32(res.commit_text);
                    screen.append(commit_u32);
                } else {
                    if (!screen.empty()) screen.pop_back();
                }
            } else {
                auto res = process_key(c);
                if (res.handled) {
                    for (int i = 0; i < res.backspaces && !screen.empty(); ++i) screen.pop_back();
                    auto commit_u32 = utf8_to_u32(res.commit_text);
                    screen.append(commit_u32);
                } else {
                    screen.push_back(static_cast<unsigned char>(c));
                }
            }
        }
        return u32_to_utf8(screen);
    }

private:
    EngineConfig config_;
    std::u32string raw_keys_;
    std::u32string chars_;
    int tone_ = 0;
    bool disabled_for_word_ = false;
    bool has_backspaced_ = false;
    char32_t last_modifier_key_ = 0;

    std::unordered_map<char32_t, std::vector<char32_t>> vowel_table_;
    std::unordered_set<char32_t> base_vowels_lower_;
    std::unordered_set<char32_t> modified_vowels_lower_;
    std::unordered_set<std::string> valid_initials_;
    std::unordered_set<std::string> valid_initials_typing_;
    std::unordered_set<std::string> valid_finals_;
    std::unordered_set<std::string> stop_finals_;
    std::unordered_set<std::string> valid_nuclei_committed_;
    std::unordered_set<std::string> valid_nuclei_typing_;
    std::unordered_set<std::string> valid_circumflex_nuclei_;
    std::unordered_set<std::string> forbidden_open_nuclei_;
    std::unordered_set<std::string> no_final_nuclei_;
    std::unordered_set<std::string> english_collision_words_;

    static char32_t to_lower_cp(char32_t c) {
        if (c >= U'A' && c <= U'Z') return c + (U'a' - U'A');
        switch (c) {
            case U'Ă': return U'ă';
            case U'Â': return U'â';
            case U'Đ': return U'đ';
            case U'Ê': return U'ê';
            case U'Ô': return U'ô';
            case U'Ơ': return U'ơ';
            case U'Ư': return U'ư';
            default: return c;
        }
    }

    static bool is_upper_cp(char32_t c) {
        if (c >= U'A' && c <= U'Z') return true;
        return (c == U'Ă' || c == U'Â' || c == U'Đ' || c == U'Ê' || c == U'Ô' || c == U'Ơ' || c == U'Ư');
    }

    static std::string to_ascii_lower(const std::string& s) {
        std::string r = s;
        for (char& c : r) {
            if (c >= 'A' && c <= 'Z') c = static_cast<char>(c + ('a' - 'A'));
        }
        return r;
    }

    static std::string apply_case_pattern(const std::string& source, const std::string& target) {
        bool all_upper = !source.empty();
        for (char c : source) {
            if (std::isalpha(static_cast<unsigned char>(c)) && !std::isupper(static_cast<unsigned char>(c))) {
                all_upper = false;
                break;
            }
        }
        std::u32string t32 = utf8_to_u32(target);
        if (all_upper) {
            for (char32_t& c : t32) {
                if (c >= U'a' && c <= U'z') c -= (U'a' - U'A');
                else if (c == U'đ') c = U'Đ';
                else if (c == U'ư') c = U'Ư';
                else if (c == U'ơ') c = U'Ơ';
                else if (c == U'ô') c = U'Ô';
                else if (c == U'ê') c = U'Ê';
                else if (c == U'â') c = U'Â';
                else if (c == U'ă') c = U'Ă';
                else if (c == U'ờ') c = U'Ờ';
                else if (c == U'ớ') c = U'Ớ';
                else if (c == U'ợ') c = U'Ợ';
                else if (c == U'ừ') c = U'Ừ';
                else if (c == U'ứ') c = U'Ứ';
                else if (c == U'ự') c = U'Ự';
                else if (c == U'ồ') c = U'Ồ';
                else if (c == U'ố') c = U'Ố';
                else if (c == U'ộ') c = U'Ộ';
                else if (c == U'ề') c = U'Ề';
                else if (c == U'ế') c = U'Ế';
                else if (c == U'ệ') c = U'Ệ';
            }
            return u32_to_utf8(t32);
        }
        if (!source.empty() && std::isupper(static_cast<unsigned char>(source[0])) && !t32.empty()) {
            char32_t& c = t32[0];
            if (c >= U'a' && c <= U'z') c -= (U'a' - U'A');
            else if (c == U'đ') c = U'Đ';
            return u32_to_utf8(t32);
        }
        return target;
    }

    void init_tables() {
        auto add_v = [&](char32_t b, const char* s0, const char* s1, const char* s2, const char* s3, const char* s4, const char* s5) {
            vowel_table_[b] = {
                utf8_to_u32(s0)[0], utf8_to_u32(s1)[0], utf8_to_u32(s2)[0],
                utf8_to_u32(s3)[0], utf8_to_u32(s4)[0], utf8_to_u32(s5)[0]
            };
        };
        add_v(U'a', "a", "á", "à", "ả", "ã", "ạ");
        add_v(U'ă', "ă", "ắ", "ằ", "ẳ", "ẵ", "ặ");
        add_v(U'â', "â", "ấ", "ầ", "ẩ", "ẫ", "ậ");
        add_v(U'e', "e", "é", "è", "ẻ", "ẽ", "ẹ");
        add_v(U'ê', "ê", "ế", "ề", "ể", "ễ", "ệ");
        add_v(U'i', "i", "í", "ì", "ỉ", "ĩ", "ị");
        add_v(U'o', "o", "ó", "ò", "ỏ", "õ", "ọ");
        add_v(U'ô', "ô", "ố", "ồ", "ổ", "ỗ", "ộ");
        add_v(U'ơ', "ơ", "ớ", "ờ", "ở", "ỡ", "ợ");
        add_v(U'u', "u", "ú", "ù", "ủ", "ũ", "ụ");
        add_v(U'ư', "ư", "ứ", "ừ", "ử", "ữ", "ự");
        add_v(U'y', "y", "ý", "ỳ", "ỷ", "ỹ", "ỵ");

        add_v(U'A', "A", "Á", "À", "Ả", "Ã", "Ạ");
        add_v(U'Ă', "Ă", "Ắ", "Ằ", "Ẳ", "Ẵ", "Ặ");
        add_v(U'Â', "Â", "Ấ", "Ầ", "Ẩ", "Ẫ", "Ậ");
        add_v(U'E', "E", "É", "È", "Ẻ", "Ẽ", "Ẹ");
        add_v(U'Ê', "Ê", "Ế", "Ề", "Ể", "Ễ", "Ệ");
        add_v(U'I', "I", "Í", "Ì", "Ỉ", "Ĩ", "Ị");
        add_v(U'O', "O", "Ó", "Ò", "Ỏ", "Õ", "Ọ");
        add_v(U'Ô', "Ô", "Ố", "Ồ", "Ổ", "Ỗ", "Ộ");
        add_v(U'Ơ', "Ơ", "Ớ", "Ờ", "Ở", "Ỡ", "Ợ");
        add_v(U'U', "U", "Ú", "Ù", "Ủ", "Ũ", "Ụ");
        add_v(U'Ư', "Ư", "Ứ", "Ừ", "Ử", "Ữ", "Ự");
        add_v(U'Y', "Y", "Ý", "Ỳ", "Ỷ", "Ỹ", "Ỵ");

        base_vowels_lower_ = {U'a', U'ă', U'â', U'e', U'ê', U'i', U'o', U'ô', U'ơ', U'u', U'ư', U'y'};
        modified_vowels_lower_ = {U'ă', U'â', U'ê', U'ô', U'ơ', U'ư'};

        valid_initials_ = {
            "", "b", "c", "ch", "d", "đ", "g", "gh", "gi", "h", "k", "kh",
            "l", "m", "n", "ng", "ngh", "nh", "p", "ph", "qu", "r", "s",
            "t", "th", "tr", "v", "x"
        };
        valid_initials_typing_ = valid_initials_;
        valid_initials_typing_.insert("q");

        valid_finals_ = {"", "c", "ch", "m", "n", "ng", "nh", "p", "t"};
        stop_finals_ = {"c", "ch", "p", "t"};

        valid_nuclei_committed_ = {
            "a", "ă", "â", "e", "ê", "i", "o", "ô", "ơ", "u", "ư", "y",
            "ai", "ao", "au", "âu", "ay", "ây", "eo", "êu", "ia", "iê", "iu",
            "oa", "oă", "oe", "oi", "ôi", "ơi", "oo",
            "ua", "uă", "uâ", "ưa", "uê", "ui", "ưi", "uô", "ươ", "uơ", "ưu", "uy",
            "ya", "yê",
            "iêu", "oai", "oao", "oay", "oeo", "uây", "uôi", "ươi", "ươu", "uya", "uyê", "uyu", "yêu"
        };
        valid_nuclei_typing_ = valid_nuclei_committed_;
        for (const char* s : {"ie", "ue", "uo", "ưo", "ye", "uye", "uoi", "ưoi", "uou", "ưou", "ieu", "yeu", "uay"}) {
            valid_nuclei_typing_.insert(s);
        }

        valid_circumflex_nuclei_ = {
            "â", "ê", "ô", "âu", "ây", "êu", "iê", "ôi", "uâ", "uê", "uô", "yê",
            "iêu", "uây", "uôi", "uyê", "yêu"
        };

        forbidden_open_nuclei_ = {"ă", "â", "iê", "yê", "uô", "ươ", "oă", "uă", "uâ", "uyê"};

        // Nuclei ending in a glide (or ia/ưa/uya) never take a final consonant in committed words (during, music)
        no_final_nuclei_ = {
            "ai", "ao", "au", "âu", "ay", "ây", "eo", "êu", "ia", "iu", "oi", "ôi", "ơi", "ui", "ưi", "ưa", "ưu",
            "iêu", "yêu", "oai", "oao", "oay", "oeo", "uây", "uôi", "ươi", "ươu", "uya", "uyu"
        };

        english_collision_words_ = {
            "soon", "moon", "room", "noon", "spoon", "boom", "zoom", "loom", "bloom", "gloom", "broom", "groom",
            "seen", "keen", "green", "screen", "between",
            "seem", "deem", "teem",
            "beer", "deer", "peer", "cheer", "steer", "sheer", "queer", "veer", "jeer", "sneer", "career", "engineer", "volunteer", "pioneer",
            "door", "poor", "floor", "moor", "boor",
            "tour", "four", "pour", "your", "hour", "sour", "flour", "scour", "devour",
            "user", "uses", "used", "use", "fuse", "muse", "abuse", "excuse", "refuse", "confuse", "accuse", "amuse",
            "true", "blue", "clue", "glue", "due", "sue", "cue", "hue", "issue", "tissue", "value", "statue", "virtue", "rescue", "continue", "avenue", "revenue",
            "more", "core", "bore", "sore", "tore", "wore", "shore", "score", "store", "ignore", "explore", "restore", "adore", "before", "therefore",
            "pure", "sure", "cure", "lure", "endure", "insure", "ensure", "assure", "secure", "obscure", "procure", "mature", "nature", "future", "feature", "creature", "culture", "structure", "picture", "lecture", "architecture", "literature", "temperature", "signature", "furniture", "adventure", "departure", "capture", "manufacture", "measure", "pleasure", "treasure", "pressure", "exposure", "closure", "enclosure", "disclosure", "leisure", "seizure", "failure", "figure", "procedure", "brochure",
            "fire", "wire", "hire", "tire", "entire", "require", "acquire", "inquire", "inspire", "desire", "admire", "retire", "expire", "empire",
            "care", "dare", "rare", "ware", "share", "spare", "square", "stare", "aware", "compare", "prepare", "declare", "software", "hardware", "firmware", "malware", "freeware", "middleware",
            "raw", "saw", "law", "jaw", "paw", "claw", "flaw", "draw", "straw", "thaw",
            "new", "few", "knew", "flew", "grew", "crew", "drew", "screw", "threw", "chew", "stew", "brew", "view", "review", "interview", "preview", "overview",
            // Deliberately absent: words whose Telex reading is a common Vietnamese word
            // (queen=quên, sex=sẽ, six=sĩ, max=mã, mix=mĩ, tax=tã, box=bõ, rest=rét, cast=cát, last=lát, lost=lót,
            // list=lít, host=hót, must=mút, trust=trút, dust=dút, cost=cót, most=mót, been=bên, teen=tên).
            "fox", "wax", "fax", "fix", "text", "next",
            "test", "best", "west", "post", "fast", "past", "just",
            "auto", "audio", "video", "radio", "studio", "portfolio", "scenario", "ratio", "mode", "code", "node", "home", "some", "come", "done", "none", "gone", "tone", "zone", "bone", "cone", "line", "fine", "mine", "nine", "pine", "wine", "time", "lime", "dime", "mime", "name", "same", "game", "fame", "came", "tame", "lame", "frame", "blame", "flame", "shame",
            // Doubled Telex keys (ss/ff/rr/xx/jj, ww/aa/ee/oo, dd) would otherwise escape to a single key: issue -> isue
            "pass", "mass", "boss", "loss", "toss", "moss", "kiss", "miss", "less", "mess", "bless",
            "assist", "assume", "assign", "assert", "asset", "essay", "possible", "lesson", "message", "session", "mission",
            "passion", "passive", "passing", "missing", "massive", "messy", "assess", "dissolve",
            "off", "offer", "office", "effect", "effort", "affect", "afford", "coffee", "differ", "buffer", "suffer", "offset",
            "sorry", "error", "mirror", "carry", "marry", "hurry", "arrow", "narrow", "terror", "horror", "array", "arrive",
            "added", "odd", "add", "diff", "tomorrow",
            "assistant", "assignment", "assets", "assessment", "passed", "password", "missed", "essential", "necessary",
            "lessons", "messages", "bass", "guess", "effective", "official", "officer", "difference", "different", "tariff",
            "toffee", "errors", "worry", "berry", "cherry", "current", "correct", "correctly", "borrow", "sorrow", "arrange",
            "barrel", "carrot", "parrot", "terrible", "referred", "merry", "ress", "isset", "buff"
        };
    }

    void split_syllable(std::string& initial, std::string& vowel, std::string& final_c, std::vector<int>& vowel_indices) const {
        initial.clear();
        vowel.clear();
        final_c.clear();
        vowel_indices.clear();

        int n = static_cast<int>(chars_.size());
        if (n == 0) return;

        std::u32string lower_chars;
        lower_chars.reserve(n);
        for (char32_t c : chars_) lower_chars.push_back(to_lower_cp(c));

        int i = 0;
        if (n >= 2 && lower_chars[0] == U'q' && lower_chars[1] == U'u') {
            i = 2;
        } else if (n >= 2 && lower_chars[0] == U'g' && lower_chars[1] == U'i') {
            if (n >= 3 && base_vowels_lower_.count(lower_chars[2])) {
                i = 2;
            } else {
                i = 1;
            }
        } else {
            while (i < n && !base_vowels_lower_.count(lower_chars[i])) {
                i++;
            }
        }

        initial = u32_to_utf8(lower_chars.substr(0, i));
        std::u32string v32;
        while (i < n && base_vowels_lower_.count(lower_chars[i])) {
            vowel_indices.push_back(i);
            v32.push_back(lower_chars[i]);
            i++;
        }
        vowel = u32_to_utf8(v32);
        final_c = u32_to_utf8(lower_chars.substr(i));
    }

    int find_tone_target_index(const std::string& /*initial*/, const std::string& vowel, const std::string& final_c, const std::vector<int>& vowel_indices) const {
        int k = static_cast<int>(vowel_indices.size());
        if (k == 0) return -1;
        if (k == 1) return vowel_indices[0];

        for (int pos = 0; pos < k - 1; ++pos) {
            char32_t c1 = to_lower_cp(chars_[vowel_indices[pos]]);
            char32_t c2 = to_lower_cp(chars_[vowel_indices[pos + 1]]);
            if (c1 == U'ư' && (c2 == U'ơ' || c2 == U'o')) {
                return vowel_indices[pos + 1];
            }
        }

        for (int idx : vowel_indices) {
            if (modified_vowels_lower_.count(to_lower_cp(chars_[idx]))) {
                return idx;
            }
        }

        if (k == 2) {
            if (!final_c.empty()) return vowel_indices[1];
            if (vowel == "oa" || vowel == "oe" || vowel == "uy") {
                return config_.modern_orthography ? vowel_indices[1] : vowel_indices[0];
            }
            return vowel_indices[0];
        }

        return vowel_indices[1];
    }

    // Escape (double modifier key): freeze the rendered syllable as literal text.
    void disable_as_literal() {
        chars_ = render_syllable();
        tone_ = 0;
        disabled_for_word_ = true;
    }

    // Not Vietnamese (e.g. 'windows', 'express'): fall back to the raw keystrokes.
    void disable_as_raw() {
        chars_ = raw_keys_;
        tone_ = 0;
        disabled_for_word_ = true;
    }

    std::u32string render_syllable() const {
        if (chars_.empty()) return U"";
        if (tone_ == 0) return chars_;

        std::string initial, vowel, final_c;
        std::vector<int> vowel_indices;
        split_syllable(initial, vowel, final_c, vowel_indices);
        int target_idx = find_tone_target_index(initial, vowel, final_c, vowel_indices);
        if (target_idx < 0) return chars_;

        std::u32string out = chars_;
        char32_t base_ch = out[target_idx];
        auto it = vowel_table_.find(base_ch);
        if (it != vowel_table_.end() && tone_ >= 0 && tone_ < 6) {
            out[target_idx] = it->second[tone_];
        }
        return out;
    }

    bool is_valid_vietnamese(bool committed) const {
        if (chars_.empty()) return true;

        std::string raw_str = to_ascii_lower(u32_to_utf8(raw_keys_));
        if (committed && english_collision_words_.count(raw_str)) {
            return false;
        }

        std::u32string lower_chars;
        for (char32_t c : chars_) {
            char32_t lc = to_lower_cp(c);
            if (lc == U'f' || lc == U'j' || lc == U'w' || lc == U'z') return false;
            lower_chars.push_back(lc);
        }

        std::string initial, vowel, final_c;
        std::vector<int> vowel_indices;
        split_syllable(initial, vowel, final_c, vowel_indices);

        const auto& init_set = committed ? valid_initials_ : valid_initials_typing_;
        if (!init_set.count(initial)) return false;

        if (vowel.empty()) {
            if (!final_c.empty()) return false;
            // A lone 'đ' (dd) stays Vietnamese at commit, like UniKey
            return !committed || initial == "đ";
        }

        int last_vowel_idx = vowel_indices.back();
        for (int idx = last_vowel_idx + 1; idx < static_cast<int>(lower_chars.size()); ++idx) {
            if (base_vowels_lower_.count(lower_chars[idx])) return false;
        }

        char32_t first_v = utf8_to_u32(vowel)[0];
        bool is_iey = (first_v == U'i' || first_v == U'e' || first_v == U'ê' || first_v == U'y');
        bool is_ie  = (first_v == U'i' || first_v == U'e' || first_v == U'ê');

        if (initial == "c" && is_iey) return false;
        if ((initial == "gh" || initial == "ngh") && !is_ie) return false;
        if (initial == "k" && !is_iey) return false;

        if (!final_c.empty() && !valid_finals_.count(final_c)) return false;

        const auto& nuclei_set = committed ? valid_nuclei_committed_ : valid_nuclei_typing_;
        if (!nuclei_set.count(vowel)) return false;

        if (vowel == "oo") {
            if (committed) {
                if (final_c != "c" && final_c != "ng") return false;
            } else {
                if (final_c != "" && final_c != "c" && final_c != "n" && final_c != "ng") return false;
            }
        }

        if (stop_finals_.count(final_c)) {
            if (tone_ == 2 || tone_ == 3 || tone_ == 4) return false;
            if (committed && tone_ == 0) return false;
        }

        if (committed && !final_c.empty()) {
            if (no_final_nuclei_.count(vowel)) return false;
            // Vietnamese spells these 'inh' / 'ich' (things, kings)
            if (vowel == "i" && (final_c == "ng" || final_c == "c")) return false;
        }

        // (a bare vowel without initial consonant, e.g. 'ă', 'â', 'ươ', is kept as typed, like UniKey)
        if (committed && !initial.empty() && final_c.empty() && forbidden_open_nuclei_.count(vowel)) {
            return false;
        }

        return true;
    }

    bool process_char_internal(char32_t ch) {
        raw_keys_.push_back(ch);
        if (disabled_for_word_) {
            chars_.push_back(ch);
            return false;
        }

        char32_t ch_lower = to_lower_cp(ch);
        const std::string& im = config_.input_method;

        // 1. Quick Telex
        if (im == "telex" && config_.quick_telex && chars_.size() == 1) {
            char32_t prev_lower = to_lower_cp(chars_[0]);
            std::map<char32_t, char32_t> qmap = {
                {U'c', U'h'}, {U'g', U'i'}, {U'k', U'h'}, {U'n', U'g'},
                {U'q', U'u'}, {U'p', U'h'}, {U't', U'h'}
            };
            if (ch_lower == prev_lower && qmap.count(ch_lower)) {
                char32_t added = qmap[ch_lower];
                if (is_upper_cp(chars_[0]) && is_upper_cp(ch)) {
                    added -= (U'a' - U'A');
                }
                chars_.push_back(added);
                return true;
            }
        }

        // 2. 'dd' or '9' -> 'đ'
        if ((im == "telex" && ch_lower == U'd') || (im == "vni" && ch == U'9')) {
            if (!chars_.empty() && (to_lower_cp(chars_[0]) == U'd' || to_lower_cp(chars_[0]) == U'đ')) {
                if (im == "vni" || chars_.size() == 1 || last_modifier_key_ != U'd') {
                    if (to_lower_cp(chars_[0]) == U'd') {
                        chars_[0] = is_upper_cp(chars_[0]) ? U'Đ' : U'đ';
                        last_modifier_key_ = ch_lower;
                        return true;
                    } else if (to_lower_cp(chars_[0]) == U'đ' && last_modifier_key_ == ch_lower) {
                        chars_[0] = is_upper_cp(chars_[0]) ? U'D' : U'd';
                        chars_.push_back(ch);
                        disable_as_literal();
                        return true;
                    }
                }
            }
        }

        // 3. Tone marks
        std::map<char32_t, int> tone_map;
        if (im == "telex") {
            tone_map = {{U's', 1}, {U'f', 2}, {U'r', 3}, {U'x', 4}, {U'j', 5}, {U'z', 0}};
        } else {
            tone_map = {{U'1', 1}, {U'2', 2}, {U'3', 3}, {U'4', 4}, {U'5', 5}, {U'0', 0}};
        }

        if (tone_map.count(ch_lower)) {
            std::string initial, vowel, final_c;
            std::vector<int> vowel_indices;
            split_syllable(initial, vowel, final_c, vowel_indices);
            if (!vowel_indices.empty()) {
                int new_tone = tone_map[ch_lower];
                if (new_tone == 0) {
                    if (tone_ != 0) {
                        tone_ = 0;
                        last_modifier_key_ = ch_lower;
                        return true;
                    }
                } else {
                    if (tone_ == new_tone && last_modifier_key_ == ch_lower) {
                        tone_ = 0;
                        chars_.push_back(ch);
                        disable_as_literal();
                        return true;
                    }
                    tone_ = new_tone;
                    last_modifier_key_ = ch_lower;
                    if (config_.auto_restore_english && !is_valid_vietnamese(false)) {
                        disable_as_raw();
                    }
                    return true;
                }
            }
        }

        // 4. Vowel Modifiers
        std::string initial, vowel, final_c;
        std::vector<int> vowel_indices;
        split_syllable(initial, vowel, final_c, vowel_indices);

        // 4a. Circumflex (aa->â, ee->ê, oo->ô or VNI '6')
        if ((im == "telex" && (ch_lower == U'a' || ch_lower == U'e' || ch_lower == U'o')) ||
            (im == "vni" && ch == U'6')) {
            std::map<char32_t, char32_t> hat_map = {
                {U'a', U'â'}, {U'A', U'Â'}, {U'e', U'ê'}, {U'E', U'Ê'}, {U'o', U'ô'}, {U'O', U'Ô'}
            };
            std::map<char32_t, char32_t> unhat_map = {
                {U'â', U'a'}, {U'Â', U'A'}, {U'ê', U'e'}, {U'Ê', U'E'}, {U'ô', U'o'}, {U'Ô', U'O'}
            };

            for (auto it = vowel_indices.rbegin(); it != vowel_indices.rend(); ++it) {
                int idx = *it;
                char32_t cur_v = chars_[idx];
                if (unhat_map.count(cur_v) && last_modifier_key_ == ch_lower) {
                    chars_[idx] = unhat_map[cur_v];
                    chars_.push_back(ch);
                    std::string c_init, c_v, c_fin;
                    std::vector<int> c_vi;
                    split_syllable(c_init, c_v, c_fin, c_vi);
                    if (c_v != "oo") {
                        disable_as_literal();
                    } else {
                        // Still composing (boong, xoọc): drop the escape key so raw_keys_
                        // matches the literal text if the word is later reverted to raw.
                        raw_keys_.pop_back();
                    }
                    last_modifier_key_ = 0;
                    return true;
                }
            }

            for (auto it = vowel_indices.rbegin(); it != vowel_indices.rend(); ++it) {
                int idx = *it;
                char32_t cur_v = chars_[idx];
                char32_t cur_low = to_lower_cp(cur_v);
                bool match = (im == "telex") ? (cur_low == ch_lower) : (cur_low == U'a' || cur_low == U'e' || cur_low == U'o');
                if (match && hat_map.count(cur_v)) {
                    char32_t old_v = chars_[idx];
                    chars_[idx] = hat_map[cur_v];
                    std::string n_init, n_vowel, n_fin;
                    std::vector<int> n_vi;
                    split_syllable(n_init, n_vowel, n_fin, n_vi);
                    if (valid_circumflex_nuclei_.count(n_vowel)) {
                        last_modifier_key_ = ch_lower;
                        if (config_.auto_restore_english && !is_valid_vietnamese(false)) {
                            disable_as_raw();
                        }
                        return true;
                    } else {
                        chars_[idx] = old_v;
                    }
                }
            }
        }

        // 4b. Horn/Breve (Telex 'w' or VNI '7', '8')
        if ((im == "telex" && ch_lower == U'w') || (im == "vni" && (ch == U'7' || ch == U'8'))) {
            if (im == "telex" && last_modifier_key_ == U'w') {
                // 'ư' produced by a standalone 'w' (ww, tww) turns back into that literal 'w'
                const size_t nraw = raw_keys_.size();
                const int last = static_cast<int>(chars_.size()) - 1;
                bool standalone_w = vowel_indices.size() == 1 && vowel_indices[0] == last &&
                                    to_lower_cp(chars_[last]) == U'ư' &&
                                    nraw >= 2 && to_lower_cp(raw_keys_[nraw - 2]) == U'w' &&
                                    !(nraw >= 3 && to_lower_cp(raw_keys_[nraw - 3]) == U'u');
                if (standalone_w) {
                    chars_[last] = raw_keys_[nraw - 2];
                } else {
                    std::map<char32_t, char32_t> unw_map = {
                        {U'ă', U'a'}, {U'Ă', U'A'}, {U'ơ', U'o'}, {U'Ơ', U'O'}, {U'ư', U'u'}, {U'Ư', U'U'}
                    };
                    for (auto it = vowel_indices.rbegin(); it != vowel_indices.rend(); ++it) {
                        if (unw_map.count(chars_[*it])) chars_[*it] = unw_map[chars_[*it]];
                    }
                    chars_.push_back(ch);
                }
                disable_as_literal();
                return true;
            }

            if (!vowel_indices.empty()) {
                if (im == "telex" || ch == U'7') {
                    for (size_t pos = 0; pos + 1 < vowel_indices.size(); ++pos) {
                        int i1 = vowel_indices[pos], i2 = vowel_indices[pos + 1];
                        char32_t v1 = to_lower_cp(chars_[i1]), v2 = to_lower_cp(chars_[i2]);
                        if ((v1 == U'u' || v1 == U'ư') && (v2 == U'o' || v2 == U'ô' || v2 == U'ơ')) {
                            chars_[i1] = is_upper_cp(chars_[i1]) ? U'Ư' : U'ư';
                            chars_[i2] = is_upper_cp(chars_[i2]) ? U'Ơ' : U'ơ';
                            last_modifier_key_ = ch_lower;
                            return true;
                        }
                    }
                    if (vowel == "ua" && initial != "qu") {
                        int i1 = vowel_indices[0];
                        chars_[i1] = is_upper_cp(chars_[i1]) ? U'Ư' : U'ư';
                        last_modifier_key_ = ch_lower;
                        return true;
                    }
                }

                for (auto it = vowel_indices.rbegin(); it != vowel_indices.rend(); ++it) {
                    int idx = *it;
                    char32_t v_ch = chars_[idx];
                    char32_t v_low = to_lower_cp(v_ch);
                    if ((v_low == U'a' || v_low == U'ă' || v_low == U'â') && (im == "telex" || ch == U'8')) {
                        chars_[idx] = is_upper_cp(v_ch) ? U'Ă' : U'ă';
                        last_modifier_key_ = ch_lower;
                        if (config_.auto_restore_english && !is_valid_vietnamese(false)) disable_as_raw();
                        return true;
                    }
                    if ((v_low == U'o' || v_low == U'ô' || v_low == U'ơ') && (im == "telex" || ch == U'7')) {
                        chars_[idx] = is_upper_cp(v_ch) ? U'Ơ' : U'ơ';
                        last_modifier_key_ = ch_lower;
                        if (idx > vowel_indices[0]) {
                            auto vit = std::find(vowel_indices.begin(), vowel_indices.end(), idx);
                            if (vit != vowel_indices.begin()) {
                                int prev_idx = *(vit - 1);
                                if (to_lower_cp(chars_[prev_idx]) == U'u' && !final_c.empty()) {
                                    chars_[prev_idx] = is_upper_cp(chars_[prev_idx]) ? U'Ư' : U'ư';
                                }
                            }
                        }
                        if (config_.auto_restore_english && !is_valid_vietnamese(false)) disable_as_raw();
                        return true;
                    }
                    if ((v_low == U'u' || v_low == U'ư') && (im == "telex" || ch == U'7')) {
                        chars_[idx] = is_upper_cp(v_ch) ? U'Ư' : U'ư';
                        last_modifier_key_ = ch_lower;
                        if (config_.auto_restore_english && !is_valid_vietnamese(false)) disable_as_raw();
                        return true;
                    }
                }
            }

            if (im == "telex" && vowel_indices.empty()) {
                std::u32string init32;
                for (char32_t c : chars_) init32.push_back(to_lower_cp(c));
                std::string init_str = u32_to_utf8(init32);
                if (valid_initials_.count(init_str) && init_str != "q") {
                    chars_.push_back(is_upper_cp(ch) ? U'Ư' : U'ư');
                    last_modifier_key_ = U'w';
                    return true;
                }
            }
        }

        // 5. Normal character append
        chars_.push_back(ch);
        last_modifier_key_ = 0;

        // 6. Auto-convert 'uơ'/'ưo' + final consonant/semivowel -> 'ươ'
        split_syllable(initial, vowel, final_c, vowel_indices);
        if (((vowel == "uơ" || vowel == "ưo") && !final_c.empty()) ||
            vowel == "uơi" || vowel == "ưoi" || vowel == "uơu" || vowel == "ưou") {
            int i1 = vowel_indices[0], i2 = vowel_indices[1];
            chars_[i1] = is_upper_cp(chars_[i1]) ? U'Ư' : U'ư';
            chars_[i2] = is_upper_cp(chars_[i2]) ? U'Ơ' : U'ơ';
        }

        // 7. Phonotactics validity check
        if (config_.auto_restore_english && !is_valid_vietnamese(false)) {
            disable_as_raw();
        }
        return false;
    }
};

} // namespace viskey

#endif // VISKEY_ENGINE_HPP
