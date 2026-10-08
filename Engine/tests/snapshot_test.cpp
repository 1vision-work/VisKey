// SPDX-License-Identifier: GPL-3.0-or-later
// Gate G1: the refactored engine, driven through the C API, must reproduce the snapshot recorded from
// the original OpenKey engine byte for byte.
#include <catch2/catch_test_macros.hpp>
#include "common/cases.h"
#include "vk/driver_vk.h"
#include <fstream>
#include <map>
#include <sstream>

using namespace vktest;

namespace {

// Split the snapshot into "## id" blocks (header comment lines are ignored).
void readBlocks(const char* path, bool required, std::map<std::string, std::string>& blocks, std::vector<std::string>& order) {
    std::ifstream f(path, std::ios::binary);
    if (!f.good()) { REQUIRE(!required); return; }
    std::string line, id, cur;
    auto flush = [&] { if (!id.empty()) { if (!blocks.count(id)) order.push_back(id); blocks[id] = cur; } };
    while (std::getline(f, line)) {
        if (line.rfind("## ", 0) == 0) {
            flush();
            id = line.substr(3);
            cur = line + "\n";
        } else if (!id.empty()) {
            cur += line + "\n";
        }
    }
    flush();
}

// engine.snap is pinned to the original engine. Intentional behaviour changes of the VisKey engine live in
// overrides.snap (optional): same block format, each block replaces the original one and is preceded in the
// commit message / ADR by the reason.
std::map<std::string, std::string> loadSnapshot(std::vector<std::string>& order) {
    std::map<std::string, std::string> blocks;
    readBlocks(VK_SNAPSHOT_PATH, true, blocks, order);
    readBlocks(VK_OVERRIDES_PATH, false, blocks, order);
    return blocks;
}

std::string headerId(const Case& c) { return c.id + (c.tag.empty() ? "" : " " + c.tag); }

} // namespace

TEST_CASE("snapshot has at least 300 typing cases with unique ids", "[snapshot]") {
    CHECK(allCases().size() >= 300);
    std::map<std::string, int> seen;
    for (const auto& c : allCases()) CHECK(++seen[c.id] == 1);
    for (const auto& c : allConvertCases()) CHECK(++seen[c.id] == 1);
}

TEST_CASE("C API reproduces the original engine snapshot", "[snapshot]") {
    std::vector<std::string> order;
    auto blocks = loadSnapshot(order);
    size_t checked = 0;

    for (const auto& c : allCases()) {
        VkDriver d;
        std::string got = renderCase(d, c);
        INFO("case " << c.id);
        auto it = blocks.find(headerId(c));
        REQUIRE(it != blocks.end());
        CHECK(got == it->second);
        checked++;
    }
    for (const auto& c : allConvertCases()) {
        VkDriver d;
        d.begin(Config{});
        std::string got = renderConvertCase(d, c);
        INFO("case " << c.id);
        auto it = blocks.find(c.id);
        REQUIRE(it != blocks.end());
        CHECK(got == it->second);
        checked++;
    }
    {
        VkDriver d; d.begin(Config{});
        auto it = blocks.find("smartswitch/basic"); REQUIRE(it != blocks.end());
        CHECK(renderSmartSwitch(d) == it->second);
        checked++;
    }
    {
        VkDriver d; d.begin(Config{});
        auto it = blocks.find("macro/blob"); REQUIRE(it != blocks.end());
        CHECK(renderMacroBlob(d) == it->second);
        checked++;
    }
    CHECK(checked == blocks.size()); // no stale snapshot entries, no missing cases
}
