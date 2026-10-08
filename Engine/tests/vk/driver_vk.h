// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "../common/driver.h"
#include "viskey_engine.h"

namespace vktest {

// Drives the refactored engine through the public C API only.
class VkDriver : public Driver {
public:
    ~VkDriver() override;
    void begin(const Config&) override;
    void addMacro(const std::string&, const std::string&) override;
    Step key(uint16_t, int, bool) override;
    Step englishKey(uint16_t, int, bool) override;
    void mouse() override;
    void tempOffSpelling() override;
    void tempOffEngine() override;
    void decodeWord(uint32_t, std::vector<uint16_t>&) override;
    uint16_t keyChar(uint32_t) override;
    std::string convert(const ConvertOptions&, const std::string&) override;
    std::vector<int> smartSwitch(const std::vector<std::pair<std::string, int>>&, const std::vector<std::string>&, int) override;
    std::vector<uint8_t> macroBlobRoundTrip() override;
    vk_engine* engine() { return e_; }
private:
    Step toStep(const vk_result&);
    vk_engine* e_ = nullptr;
};

} // namespace vktest
