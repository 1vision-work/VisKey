// Typing regression suite (C++ engine, used by macOS + Windows).
//
// Replays every case of tests/data/typing_regression.tsv key by key and applies the
// engine results to a text buffer exactly like VisKeyApp.mm / VisKeyWin.cpp do:
//   handled     -> [autocomplete fix: insert U+202F, delete 1] + delete `backspaces`
//                  + insert `commit_text`; the original key is swallowed
//   not handled -> the original key reaches the app (Backspace deletes one char)
// Each case is typed followed by a space; the final buffer must equal expected + " ".
// After every key the buffer must equal committed text + the engine's displayed word,
// which catches doubled / lost characters mid-word.
//
// Usage: test_typing_regression [path/to/typing_regression.tsv]
// Without an argument the TSV is looked up under VISKEY_SOURCE_DIR (set by CMake),
// then relative to the current directory (repo root or tests/).
#include "../core/viskey_engine.hpp"

#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace viskey;

namespace {

struct Case {
    int lineno;
    std::string method;
    std::string keys;
    std::string expected;
};

const char kBackspace = '\b';

bool load_cases(const std::string& path, std::vector<Case>& cases) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    std::string line;
    int lineno = 0;
    while (std::getline(in, line)) {
        ++lineno;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        size_t t1 = line.find('\t');
        size_t t2 = (t1 == std::string::npos) ? std::string::npos : line.find('\t', t1 + 1);
        if (t2 == std::string::npos) {
            std::cerr << path << ":" << lineno << ": malformed line\n";
            return false;
        }
        Case c;
        c.lineno = lineno;
        c.method = line.substr(0, t1);
        std::string keys = line.substr(t1 + 1, t2 - t1 - 1);
        for (size_t i = 0; i < keys.size(); ++i) {
            if (keys[i] == '\\' && i + 1 < keys.size() && keys[i + 1] == 'b') {
                c.keys.push_back(kBackspace);
                ++i;
            } else {
                c.keys.push_back(keys[i]);
            }
        }
        c.expected = line.substr(t2 + 1);
        cases.push_back(c);
    }
    return true;
}

bool make_config(const std::string& method, EngineConfig& cfg) {
    cfg = EngineConfig();
    if (method == "telex") return true;
    if (method == "vni") {
        cfg.input_method = "vni";
        return true;
    }
    if (method == "telex_norestore") {
        cfg.auto_restore_english = false;
        return true;
    }
    return false;
}

std::string printable(const std::string& keys) {
    std::string out;
    for (char c : keys) {
        if (c == kBackspace) out += "\\b";
        else out.push_back(c);
    }
    return out;
}

// Returns an empty string on success, otherwise a description of the per-step failure.
std::string simulate(VisKeyEngine& engine, const std::string& keys, std::u32string& buf) {
    engine.reset();
    buf.clear();
    std::u32string committed;
    for (size_t step = 0; step < keys.size(); ++step) {
        const char key = keys[step];
        KeyProcessResult res = (key == kBackspace) ? engine.process_backspace() : engine.process_key(key);
        if (res.handled) {
            if (res.autocomplete_fix_needed && res.backspaces > 0) {
                buf.push_back(U' ');
                buf.pop_back();
            }
            if (res.backspaces < 0 || static_cast<size_t>(res.backspaces) > buf.size()) {
                return "step " + std::to_string(step) + ": " + std::to_string(res.backspaces) +
                       " backspaces > buffer " + std::to_string(buf.size());
            }
            buf.erase(buf.size() - static_cast<size_t>(res.backspaces));
            buf += utf8_to_u32(res.commit_text);
        } else if (key == kBackspace) {
            if (buf.empty()) return "step " + std::to_string(step) + ": Backspace on empty buffer";
            buf.pop_back();
        } else {
            buf.push_back(static_cast<unsigned char>(key));
        }

        std::u32string word = utf8_to_u32(engine.get_displayed_word());
        if (word.empty()) {
            committed = buf;
        } else if (buf != committed + word) {
            return "step " + std::to_string(step) + ": screen '" + u32_to_utf8(buf) + "' != committed '" +
                   u32_to_utf8(committed) + "' + engine word '" + u32_to_utf8(word) + "'";
        }
    }
    return "";
}

std::string find_tsv(int argc, char** argv) {
    if (argc > 1) return argv[1];
    std::vector<std::string> candidates;
#ifdef VISKEY_SOURCE_DIR
    candidates.push_back(std::string(VISKEY_SOURCE_DIR) + "/tests/data/typing_regression.tsv");
#endif
    candidates.push_back("tests/data/typing_regression.tsv");
    candidates.push_back("data/typing_regression.tsv");
    candidates.push_back("../tests/data/typing_regression.tsv");
    for (const auto& p : candidates) {
        if (std::ifstream(p)) return p;
    }
    return candidates.front();
}

}  // namespace

int main(int argc, char** argv) {
    const std::string path = find_tsv(argc, argv);
    std::vector<Case> cases;
    if (!load_cases(path, cases) || cases.empty()) {
        std::cerr << "Cannot read typing regression cases from " << path << "\n";
        return 1;
    }

    std::map<std::string, VisKeyEngine> engines;
    std::map<std::string, std::pair<int, int>> per_method;  // method -> (total, failed)
    std::vector<std::string> failures;

    for (const Case& c : cases) {
        auto it = engines.find(c.method);
        if (it == engines.end()) {
            EngineConfig cfg;
            if (!make_config(c.method, cfg)) {
                std::cerr << path << ":" << c.lineno << ": unknown method '" << c.method << "'\n";
                return 1;
            }
            it = engines.emplace(c.method, VisKeyEngine(cfg)).first;
        }
        std::u32string buf;
        std::string err = simulate(it->second, c.keys + " ", buf);
        std::string actual = u32_to_utf8(buf);
        auto& stats = per_method[c.method];
        stats.first++;
        if (!err.empty() || actual != c.expected + " ") {
            stats.second++;
            std::ostringstream os;
            os << "  line " << c.lineno << " [" << c.method << "] keys='" << printable(c.keys)
               << "' expected='" << c.expected << " ' got='" << actual << "'";
            if (!err.empty()) os << "  (" << err << ")";
            failures.push_back(os.str());
        }
    }

    for (const auto& kv : per_method) {
        std::cout << "  " << kv.first << ": " << (kv.second.first - kv.second.second) << "/"
                  << kv.second.first << " passed\n";
    }
    if (!failures.empty()) {
        std::cerr << failures.size() << " FAILURES:\n";
        for (const auto& f : failures) std::cerr << f << "\n";
        std::cerr << "C++ TYPING REGRESSION: " << failures.size() << "/" << cases.size() << " FAILED\n";
        return 1;
    }
    std::cout << "ALL " << cases.size() << " C++ TYPING REGRESSION CASES PASSED\n";
    return 0;
}
