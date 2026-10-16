#include "../core/viskey_engine.hpp"
#include <iostream>
#include <utility>
#include <vector>

using namespace viskey;

int main() {
    EngineConfig cfg;
    cfg.input_method = "telex";
    cfg.modern_orthography = false;
    cfg.auto_restore_english = true;
    VisKeyEngine engine(cfg);

    std::vector<std::pair<std::string, std::string>> vietnamese_cases = {
        {"Tieengs Vieetj ", "Tiếng Việt "},
        {"chuowng trinhf ", "chương trình "},
        {"thuowngs ", "thướng "},
        {"thusowng ", "thướng "},
        {"thuoswng ", "thướng "},
        {"nguowif ", "người "},
        {"nguoiwf ", "người "},
        {"huowu ", "hươu "},
        {"khuysu ", "khuýu "},
        {"khuyur ", "khuỷu "},
        {"ngoaif ", "ngoài "},
        {"xoays ", "xoáy "},
        {"ngoanwf ", "ngoằn "},
        {"ngoeof ", "ngoèo "},
        {"nghieengs ", "nghiếng "},
        {"nghieeng ", "nghiêng "},
        {"quoocs gia ", "quốc gia "},
        {"giuwx ginf ", "giữ gìn "},
        {"gif ", "gì "},
        {"quar ", "quả "},
        {"quawpj ", "quặp "},
        {"muaw ", "mưa "},
        {"chuaw ", "chưa "},
        {"luarw ", "lửa "},
        {"ddawcj bieetj ", "đặc biệt "},
        {"DDaif Loan ", "Đài Loan "},
        {"hoaf binhf ", "hòa bình "},
        {"thuys kieeuf ", "thúy kiều "},
        {"khoer manhj ", "khỏe mạnh "},
        {"toans hocj ", "toán học "},
        {"hoas ", "hóa "},
        {"hoasn ", "hoán "},
        {"tuas ", "túa "},
        {"tuasn ", "tuán "},
        {"tuasna ", "tuấn "},
        {"xooocj ", "xoọc "},
        {"booong ", "boong "},
        {"tets ", "tét "}
    };

    int failed = 0;
    for (const auto& p : vietnamese_cases) {
        std::string actual = engine.feed_sequence(p.first);
        if (actual != p.second) {
            std::cerr << "[FAIL C++ Vietnamese] typed=" << p.first
                      << " -> expected=" << p.second << ", got=" << actual << "\n";
            failed++;
        }
    }

    // Modern orthography tests
    EngineConfig mod_cfg = cfg;
    mod_cfg.modern_orthography = true;
    VisKeyEngine mod_engine(mod_cfg);
    std::vector<std::pair<std::string, std::string>> modern_cases = {
        {"hoaf binhf ", "hoà bình "},
        {"thuys kieeuf ", "thuý kiều "},
        {"khoer manhj ", "khoẻ mạnh "},
        {"toans hocj ", "toán học "}
    };
    for (const auto& p : modern_cases) {
        std::string actual = mod_engine.feed_sequence(p.first);
        if (actual != p.second) {
            std::cerr << "[FAIL C++ Modern] typed=" << p.first
                      << " -> expected=" << p.second << ", got=" << actual << "\n";
            failed++;
        }
    }

    // English Auto-Restore tests
    std::vector<std::pair<std::string, std::string>> english_cases = {
        {"window ", "window "},
        {"Window ", "Window "},
        {"software ", "software "},
        {"refresh ", "refresh "},
        {"express ", "express "},
        {"book ", "book "},
        {"food ", "food "},
        {"good ", "good "},
        {"feel ", "feel "},
        {"school ", "school "},
        {"keep ", "keep "},
        {"root ", "root "},
        {"loop ", "loop "},
        {"raw ", "raw "},
        {"saw ", "saw "},
        {"law ", "law "},
        {"draw ", "draw "},
        {"view ", "view "},
        {"review ", "review "},
        {"new ", "new "},
        {"true ", "true "},
        {"issue ", "issue "},
        {"service ", "service "},
        {"source ", "source "},
        {"course ", "course "},
        {"house ", "house "},
        {"user ", "user "},
        {"server ", "server "},
        {"update ", "update "},
        {"create ", "create "},
        {"delete ", "delete "},
        {"test ", "test "},
        {"first ", "first "},
        {"world ", "world "},
        {"github ", "github "},
        {"macos ", "macos "},
        {"linux ", "linux "},
        {"soon ", "soon "},
        {"moon ", "moon "},
        {"room ", "room "},
        {"beer ", "beer "},
        {"door ", "door "},
        {"poor ", "poor "},
        {"more ", "more "},
        {"core ", "core "},
        {"store ", "store "},
        {"restore ", "restore "},
        {"pure ", "pure "},
        {"sure ", "sure "},
        {"nature ", "nature "},
        {"future ", "future "},
        {"feature ", "feature "}
    };
    for (const auto& p : english_cases) {
        std::string actual = engine.feed_sequence(p.first);
        if (actual != p.second) {
            std::cerr << "[FAIL C++ English] typed=" << p.first
                      << " -> expected=" << p.second << ", got=" << actual << "\n";
            failed++;
        }
    }

    // Mixed bilingual sentence
    std::string mixed = "Tooi ddang code app treen macos vaf linux vowis window mode trong browser ";
    std::string expected_mixed = "Tôi đang code app trên macos và linux với window mode trong browser ";
    if (engine.feed_sequence(mixed) != expected_mixed) {
        std::cerr << "[FAIL C++ Mixed Sentence] got=" << engine.feed_sequence(mixed) << "\n";
        failed++;
    }

    // VNI tests
    EngineConfig vni_cfg = cfg;
    vni_cfg.input_method = "vni";
    VisKeyEngine vni_engine(vni_cfg);
    std::vector<std::pair<std::string, std::string>> vni_cases = {
        {"Tie61ng Vie65t ", "Tiếng Việt "},
        {"chu7o7ng trinh2 ", "chương trình "},
        {"d9a85c bie65t ", "đặc biệt "},
        {"hoa2 binh2 ", "hòa bình "}
    };
    for (const auto& p : vni_cases) {
        std::string actual = vni_engine.feed_sequence(p.first);
        if (actual != p.second) {
            std::cerr << "[FAIL C++ VNI] typed=" << p.first
                      << " -> expected=" << p.second << ", got=" << actual << "\n";
            failed++;
        }
    }

    // Macros & Quick Telex
    EngineConfig m_cfg = cfg;
    m_cfg.quick_telex = true;
    m_cfg.enable_macros = true;
    VisKeyEngine m_engine(m_cfg);
    std::vector<std::pair<std::string, std::string>> macro_cases = {
        {"vn ", "Việt Nam "},
        {"ko ", "không "},
        {"KO ", "KHÔNG "},
        {"Ko ", "Không "},
        {"dc ", "được "},
        {"ttieengs ", "thiếng "}
    };
    for (const auto& p : macro_cases) {
        std::string actual = m_engine.feed_sequence(p.first);
        if (actual != p.second) {
            std::cerr << "[FAIL C++ Macro] typed=" << p.first
                      << " -> expected=" << p.second << ", got=" << actual << "\n";
            failed++;
        }
    }

    // Backspace tests
    std::vector<std::pair<std::string, std::string>> bs_cases = {
        {"tieengs\b ", "tiến "},
        {"tieengs\b\b ", "tiế "},
        {"tieengs\b\b\b ", "ti "},
        {"hoasn\b ", "hóa "}
    };
    for (const auto& p : bs_cases) {
        std::string actual = engine.feed_sequence(p.first);
        if (actual != p.second) {
            std::cerr << "[FAIL C++ Backspace] typed=" << p.first
                      << " -> expected=" << p.second << ", got=" << actual << "\n";
            failed++;
        }
    }

    int total = static_cast<int>(vietnamese_cases.size() + modern_cases.size() +
                                 english_cases.size() + 1 + vni_cases.size() +
                                 macro_cases.size() + bs_cases.size());
    if (failed == 0) {
        std::cout << "ALL " << total << " C++17 ENGINE TESTS PASSED SUCCESSFULLY!\n";
        return 0;
    } else {
        std::cerr << "TOTAL C++ FAILURES: " << failed << "\n";
        return 1;
    }
}
