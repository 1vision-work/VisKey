// SPDX-License-Identifier: GPL-3.0-or-later
//
// VisKey for Windows (by 1VISION) — based on OpenKey by Mai Vũ Tuyên.

#include "output_planner.h"

#include "keymap.h"

namespace viskey {

namespace {
// Mirrors the engine's own limit on deletions for typing results (OpenKey MAX_BUFF).
constexpr int kMaxBackspaces = 32;
}  // namespace

OutputPlan OutputPlanner::Plan(const vk_result& r, const uint32_t* words, size_t count, uint16_t typedVk,
                               const WordDecoder& decode) {
    OutputPlan plan;
    switch (r.code) {
    case VK_WILL_PROCESS:
    case VK_RESTORE:
    case VK_RESTORE_AND_START_NEW_SESSION:
        plan.passThrough = false;
        if (r.backspace_count > 0 && r.backspace_count < kMaxBackspaces) {
            AppendAutocompleteFix(plan.actions, r);
            for (int i = 0; i < r.backspace_count; i++) AppendBackspace(plan.actions);
        }
        AppendText(plan.actions, words, count, decode);
        // A restored word is followed by the key that was typed (letter, Space, Tab, punctuation...).
        if (r.code != VK_WILL_PROCESS) AppendKey(plan.actions, typedVk);
        return plan;

    case VK_REPLACE_MACRO:
        plan.passThrough = false;
        if (r.backspace_count > 0) AppendAutocompleteFix(plan.actions, r);
        for (int i = 0; i < r.backspace_count; i++) AppendBackspace(plan.actions);
        AppendText(plan.actions, words, count, decode);
        AppendKey(plan.actions, typedVk);  // the key that ended the macro (Space, punctuation...)
        return plan;

    default:  // VK_DO_NOTHING, VK_BREAK_WORD
        return PassThrough(r.ext_code);
    }
}

OutputPlan OutputPlanner::PassThrough(uint8_t extCode) {
    OutputPlan plan;
    if (!IsDoubleCode()) return plan;
    switch (extCode) {
    case 1:  // word break
        syncKey_.clear();
        break;
    case 2:  // delete key: the app deletes one unit, a two-unit character needs one more
        if (!syncKey_.empty()) {
            const int last = syncKey_.back();
            syncKey_.pop_back();
            if (last > 1 && (codeTable_ == VK_TABLE_VNI_WINDOWS || !backspaceDeletesCluster_))
                plan.actions.push_back(OutputAction::MakeBackspace());
        }
        break;
    case 3:  // normal key typed as is
        syncKey_.push_back(1);
        break;
    default:
        break;
    }
    return plan;
}

void OutputPlanner::AppendBackspace(std::vector<OutputAction>& out) {
    out.push_back(OutputAction::MakeBackspace());
    if (!IsDoubleCode() || syncKey_.empty()) return;
    const int last = syncKey_.back();
    syncKey_.pop_back();
    if (last > 1 && !(codeTable_ == VK_TABLE_UNICODE_COMPOUND && backspaceDeletesCluster_))
        out.push_back(OutputAction::MakeBackspace());
}

void OutputPlanner::AppendText(std::vector<OutputAction>& out, const uint32_t* words, size_t count,
                               const WordDecoder& decode) {
    std::vector<uint16_t> units;
    for (size_t i = 0; i < count; i++) {
        uint16_t u[2] = {0, 0};
        const size_t n = decode(words[i], u);
        if (n == 0) continue;
        units.insert(units.end(), u, u + n);
        if (IsDoubleCode()) syncKey_.push_back(int(n));
    }
    if (!units.empty()) out.push_back(OutputAction::MakeText(std::move(units)));
}

void OutputPlanner::AppendKey(std::vector<OutputAction>& out, uint16_t vk) {
    if (vk == 0) return;
    if (IsDoubleCode()) syncKey_.push_back(1);
    out.push_back(OutputAction::MakeKey(vk));
}

// OpenKey: type an invisible character first and delete it with one extra backspace, so an inline
// autocomplete suggestion is replaced by it instead of swallowing our first backspace.
// ext_code 4 means "should not send an empty character".
void OutputPlanner::AppendAutocompleteFix(std::vector<OutputAction>& out, const vk_result& r) {
    if (!fixAutocomplete_ || r.ext_code == 4) return;
    out.push_back(OutputAction::MakeText({kEmptyChar}));
    out.push_back(OutputAction::MakeBackspace());
}

std::vector<KeyStroke> ToKeyStrokes(const std::vector<OutputAction>& actions) {
    std::vector<KeyStroke> out;
    auto key = [&](uint16_t vk) {
        out.push_back(KeyStroke{vk, 0, false, false});
        out.push_back(KeyStroke{vk, 0, false, true});
    };
    for (const auto& a : actions) {
        switch (a.kind) {
        case OutputAction::Kind::Backspace: key(vkey::Back); break;
        case OutputAction::Kind::Key: key(a.vk); break;
        case OutputAction::Kind::Text:
            for (const uint16_t u : a.units) {
                out.push_back(KeyStroke{0, u, true, false});
                out.push_back(KeyStroke{0, u, true, true});
            }
            break;
        }
    }
    return out;
}

}  // namespace viskey
