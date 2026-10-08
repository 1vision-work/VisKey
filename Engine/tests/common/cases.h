// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "driver.h"
#include <string>
#include <utility>
#include <vector>

namespace vktest {

struct Case {
    std::string id;
    Config cfg;
    std::string keys;                                          // see cases.cpp for the key DSL
    std::vector<std::pair<std::string, std::string>> macros;   // text -> content
    bool english = false;                                      // route keys through englishKey()
    std::string tag;                                           // e.g. "[known-bug]"
};

struct ConvertCase {
    std::string id;
    ConvertOptions opt;
    std::string input;
};

const std::vector<Case>& allCases();
const std::vector<ConvertCase>& allConvertCases();

// Render one typing case / convert case / misc case to snapshot text. Needs a fresh driver state.
std::string renderCase(Driver&, const Case&);
std::string renderConvertCase(Driver&, const ConvertCase&);
std::string renderSmartSwitch(Driver&);
std::string renderMacroBlob(Driver&);

} // namespace vktest
