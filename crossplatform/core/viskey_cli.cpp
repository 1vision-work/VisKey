// viskey_cli.cpp
// Interactive terminal simulator to test VisKey Engine directly in CLI
#include "viskey_engine.hpp"
#include <iostream>
#include <string>

int main() {
    viskey::EngineConfig cfg;
    cfg.input_method = "telex";
    cfg.modern_orthography = false;
    cfg.auto_restore_english = true;
    cfg.enable_macros = true;
    viskey::VisKeyEngine engine(cfg);

    std::cout << "==============================================================\n";
    std::cout << "  VisKey Interactive CLI Simulator (Telex + Auto-Restore)   \n";
    std::cout << "  Nhap chuoi phim go (vi du: Tieengs Vieetj trreen window ):  \n";
    std::cout << "  Go 'q' de thoat, ':vni' hoac ':telex' de doi kieu go.       \n";
    std::cout << "==============================================================\n";

    std::string line;
    while (std::cout << "\n[Nhap phim thô] > " && std::getline(std::cin, line)) {
        if (line == "q" || line == ":q") break;
        if (line == ":vni") {
            engine.config().input_method = "vni";
            std::cout << "-> Da chuyen sang kieu go VNI.\n";
            continue;
        }
        if (line == ":telex") {
            engine.config().input_method = "telex";
            std::cout << "-> Da chuyen sang kieu go Telex.\n";
            continue;
        }
        std::string out = engine.feed_sequence(line + " ");
        if (!out.empty() && out.back() == ' ') out.pop_back();
        std::cout << "[Ket qua VisKey] => " << out << "\n";
    }
    return 0;
}
