// SPDX-License-Identifier: GPL-3.0-or-later
//
// VisKey for Windows (by 1VISION) — based on OpenKey by Mai Vũ Tuyên.
//
// Behaviour notes (see also windows/README.md):
//  - Events carrying our dwExtraInfo tag are ignored. Other injected events (on-screen keyboard, remote
//    desktop tools, AutoHotkey...) are processed like real typing so they get Vietnamese too.
//  - Ctrl/Alt/Win held: the key is a shortcut. The engine is told (VK_MOD_OTHER ends the word, as in
//    OpenKey) and the key always passes through untouched; Win combos are never swallowed.
//  - Replacement keystrokes are sent with SendInput directly from the hook callback. Injected input is
//    queued behind the event being handled, so the app sees keys in the order they were typed; SendInput of
//    a few dozen events takes microseconds, far below the low-level hook timeout.
//  - If the foreground window belongs to a process with a higher integrity level (an elevated app while
//    VisKey is not elevated), UIPI drops our SendInput, so keys are passed through unchanged there instead
//    of being swallowed and lost.

#include "typing_service.h"

#include <process.h>

#include <vector>

#include "keymap.h"
#include "output_planner.h"
#include "viskey_engine.h"
#include "win_util.h"

namespace viskey {

namespace {

constexpr UINT WM_HOOK_APPLY = WM_APP + 100;  // lParam: Config* (owned by the receiver)
constexpr WORD kMenuMaskKey = 0xE8;           // unassigned VK: stops a lone Alt release from opening menus

DWORD IntegrityLevel(HANDLE process) {
    HANDLE token = nullptr;
    if (!OpenProcessToken(process, TOKEN_QUERY, &token)) return MAXDWORD;
    DWORD level = MAXDWORD;
    BYTE buf[64];
    DWORD size = 0;
    if (GetTokenInformation(token, TokenIntegrityLevel, buf, sizeof buf, &size)) {
        auto* label = reinterpret_cast<TOKEN_MANDATORY_LABEL*>(buf);
        PSID sid = label->Label.Sid;
        level = *GetSidSubAuthority(sid, DWORD(*GetSidSubAuthorityCount(sid) - 1));
    }
    CloseHandle(token);
    return level;
}

// The hook thread. Lives entirely on that thread; reached from the static callbacks through `self`.
class Worker {
public:
    Worker(HWND notify, const Config& c) : notify_(notify) {
        engine_ = vk_create();
        ownIntegrity_ = IntegrityLevel(GetCurrentProcess());
        Apply(c);
    }
    ~Worker() { vk_destroy(engine_); }

    static Worker* self;

    void Apply(const Config& c) {
        config_ = c;
        vk_set_option(engine_, VK_OPT_LANGUAGE, c.vietnamese ? 1 : 0);
        vk_set_option(engine_, VK_OPT_INPUT_TYPE, c.inputType);
        vk_set_option(engine_, VK_OPT_CODE_TABLE, c.codeTable);
        vk_set_option(engine_, VK_OPT_FREE_MARK, c.freeMark);
        vk_set_option(engine_, VK_OPT_CHECK_SPELLING, c.checkSpelling);
        vk_set_option(engine_, VK_OPT_MODERN_ORTHOGRAPHY, c.modernOrthography);
        vk_set_option(engine_, VK_OPT_QUICK_TELEX, c.quickTelex);
        vk_set_option(engine_, VK_OPT_RESTORE_IF_WRONG_SPELLING, c.restoreIfWrongSpelling);
        vk_set_option(engine_, VK_OPT_USE_MACRO, c.useMacro);
        vk_set_option(engine_, VK_OPT_USE_MACRO_IN_ENGLISH_MODE, c.macroInEnglishMode);
        vk_set_option(engine_, VK_OPT_AUTO_CAPS_MACRO, c.autoCapsMacro);
        vk_set_option(engine_, VK_OPT_UPPERCASE_FIRST_CHAR, c.upperCaseFirstChar);
        vk_set_option(engine_, VK_OPT_ALLOW_CONSONANT_ZFWJ, c.allowConsonantZFWJ);
        vk_set_option(engine_, VK_OPT_QUICK_START_CONSONANT, c.quickStartConsonant);
        vk_set_option(engine_, VK_OPT_QUICK_END_CONSONANT, c.quickEndConsonant);
        vk_restore_spelling(engine_);
        vk_macro_load(engine_, nullptr, 0);
        for (const auto& m : c.macros) vk_macro_add(engine_, m.abbreviation.c_str(), m.expansion.c_str());
        planner_.SetCodeTable(c.codeTable);
        planner_.SetFixBrowserAutocomplete(c.fixBrowserAutocomplete);
        hotkey_ = ModifierHotkey(kModCtrl | kModShift);
    }

    void OnForegroundChanged(HWND hwnd) {
        BreakWord();
        foregroundBlocked_ = IsHigherIntegrity(hwnd);
        capsOn_ = (GetKeyState(VK_CAPITAL) & 1) != 0;
    }

    void BreakWord() {
        vk_handle_key(engine_, 0, 0, VK_EVENT_MOUSE_DOWN);
        vk_new_session(engine_);
        planner_.reset();
    }

    // Returns true to swallow the event.
    bool OnKey(WPARAM msg, const KBDLLHOOKSTRUCT& k) {
        if (k.dwExtraInfo == kInjectedTag) return false;
        const bool down = msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN;
        const uint16_t vk = uint16_t(k.vkCode);

        if (ModifierBit(vk)) {
            UpdateModifier(vk, down);
            if (config_.hotkey == ToggleHotkey::CtrlShift && hotkey_.ModifiersChanged(mods_)) ToggleLanguage();
            return false;
        }
        if (vk == VK_CAPITAL) {
            if (down && !capsDown_) capsOn_ = !capsOn_;
            capsDown_ = down;
            return false;
        }
        if (!down) return false;

        ResyncModifiers();
        hotkey_.KeyDown();
        if (config_.hotkey == ToggleHotkey::AltZ && IsAltZ(vk, mods_)) {
            ToggleLanguage();
            SendStrokes({KeyStroke{kMenuMaskKey, 0, false, false}, KeyStroke{kMenuMaskKey, 0, false, true}});
            return true;
        }
        if (foregroundBlocked_) return false;

        switch (ClassifyKey(vk)) {
        case KeyClass::Modifier: return false;
        case KeyClass::Other: BreakWord(); return false;
        case KeyClass::Typing: break;
        }

        const vk_result r = vk_handle_key(engine_, vk, EngineModifiers(mods_, capsOn_), VK_EVENT_KEY_DOWN);
        if (IsShortcut(mods_)) {  // Ctrl/Alt/Win: never rewrite a shortcut
            planner_.reset();
            return false;
        }

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

        // With passThrough the extra keys (only ever a Backspace for a two-unit character) reach the app after
        // the original key; both are deletions, so the order does not matter.
        if (!plan.actions.empty()) SendStrokes(ToKeyStrokes(plan.actions));
        return !plan.passThrough;
    }

private:
    HWND notify_;
    vk_engine* engine_ = nullptr;
    Config config_;
    OutputPlanner planner_;
    ModifierHotkey hotkey_;
    std::vector<uint32_t> macroWords_;
    uint32_t mods_ = 0;
    bool keyDown_[4][2] = {};  // [shift, ctrl, alt, win][left, right]
    bool capsOn_ = (GetKeyState(VK_CAPITAL) & 1) != 0;
    bool capsDown_ = false;
    bool foregroundBlocked_ = false;
    DWORD ownIntegrity_ = 0;

    static int ModIndex(uint32_t bit) {
        switch (bit) { case kModShift: return 0; case kModCtrl: return 1; case kModAlt: return 2; default: return 3; }
    }

    void RecomputeMods() {
        mods_ = 0;
        const uint32_t bits[4] = {kModShift, kModCtrl, kModAlt, kModWin};
        for (int i = 0; i < 4; i++)
            if (keyDown_[i][0] || keyDown_[i][1]) mods_ |= bits[i];
    }

    void UpdateModifier(uint16_t vk, bool down) {
        const bool right = vk == VK_RSHIFT || vk == VK_RCONTROL || vk == VK_RMENU || vk == VK_RWIN;
        keyDown_[ModIndex(ModifierBit(vk))][right ? 1 : 0] = down;
        RecomputeMods();
    }

    // A key-up can be missed (secure desktop, Win+L, a hook timeout): trust the async state when it says "up".
    void ResyncModifiers() {
        const int vks[4][2] = {{VK_LSHIFT, VK_RSHIFT}, {VK_LCONTROL, VK_RCONTROL}, {VK_LMENU, VK_RMENU}, {VK_LWIN, VK_RWIN}};
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 2; j++)
                if (keyDown_[i][j] && !(GetAsyncKeyState(vks[i][j]) & 0x8000)) keyDown_[i][j] = false;
        RecomputeMods();
    }

    void ToggleLanguage() {
        config_.vietnamese = !config_.vietnamese;
        vk_set_option(engine_, VK_OPT_LANGUAGE, config_.vietnamese ? 1 : 0);
        planner_.reset();
        PostMessageW(notify_, WM_APP_LANGUAGE_CHANGED, config_.vietnamese ? 1 : 0, 0);
    }

    static void SendStrokes(const std::vector<KeyStroke>& strokes) {
        std::vector<INPUT> inputs = ToInputs(strokes);
        if (!inputs.empty()) SendInput(UINT(inputs.size()), inputs.data(), sizeof(INPUT));
    }

    bool IsHigherIntegrity(HWND hwnd) const {
        DWORD pid = 0;
        if (!hwnd || !GetWindowThreadProcessId(hwnd, &pid) || pid == GetCurrentProcessId()) return false;
        HANDLE p = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (!p) return true;  // cannot even query it: treat as protected, pass keys through
        const DWORD level = IntegrityLevel(p);  // MAXDWORD if the token is not readable (elevated)
        CloseHandle(p);
        return level > ownIntegrity_;
    }
};

Worker* Worker::self = nullptr;

LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code == HC_ACTION && Worker::self &&
        Worker::self->OnKey(wParam, *reinterpret_cast<const KBDLLHOOKSTRUCT*>(lParam)))
        return 1;
    return CallNextHookEx(nullptr, code, wParam, lParam);
}

LRESULT CALLBACK MouseProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code == HC_ACTION && Worker::self) {
        switch (wParam) {
        case WM_LBUTTONDOWN: case WM_RBUTTONDOWN: case WM_MBUTTONDOWN: case WM_XBUTTONDOWN:
            Worker::self->BreakWord();
            break;
        default:
            break;
        }
    }
    return CallNextHookEx(nullptr, code, wParam, lParam);
}

void CALLBACK ForegroundProc(HWINEVENTHOOK, DWORD event, HWND hwnd, LONG, LONG, DWORD, DWORD) {
    if (event == EVENT_SYSTEM_FOREGROUND && Worker::self) Worker::self->OnForegroundChanged(hwnd);
}

struct StartData {
    HWND notify;
    Config config;
    HANDLE ready;
    bool ok = false;
};

unsigned __stdcall ThreadMain(void* arg) {
    auto* start = static_cast<StartData*>(arg);
    const HWND notify = start->notify;

    // Make sure this thread has a message queue before anyone posts to it.
    MSG msg;
    PeekMessageW(&msg, nullptr, WM_USER, WM_USER, PM_NOREMOVE);

    Worker worker(notify, start->config);
    Worker::self = &worker;
    worker.OnForegroundChanged(GetForegroundWindow());

    const HINSTANCE inst = GetModuleHandleW(nullptr);
    HHOOK kb = SetWindowsHookExW(WH_KEYBOARD_LL, KeyboardProc, inst, 0);
    HHOOK mouse = SetWindowsHookExW(WH_MOUSE_LL, MouseProc, inst, 0);
    HWINEVENTHOOK fg = SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, nullptr, ForegroundProc,
                                       0, 0, WINEVENT_OUTOFCONTEXT);
    start->ok = kb != nullptr;
    SetEvent(start->ready);  // `start` is gone after this
    PostMessageW(notify, WM_APP_HOOK_STATUS, kb ? 1 : 0, 0);

    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (msg.hwnd == nullptr && msg.message == WM_HOOK_APPLY) {
            Config* c = reinterpret_cast<Config*>(msg.lParam);
            worker.Apply(*c);
            delete c;
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (fg) UnhookWinEvent(fg);
    if (mouse) UnhookWindowsHookEx(mouse);
    if (kb) UnhookWindowsHookEx(kb);
    Worker::self = nullptr;
    // Drop configs posted after WM_QUIT.
    while (PeekMessageW(&msg, nullptr, WM_HOOK_APPLY, WM_HOOK_APPLY, PM_REMOVE))
        delete reinterpret_cast<Config*>(msg.lParam);
    return 0;
}

}  // namespace

bool TypingService::Start(HWND notifyWindow, const Config& config) {
    if (thread_) return true;
    StartData start{notifyWindow, config, CreateEventW(nullptr, TRUE, FALSE, nullptr)};
    unsigned id = 0;
    thread_ = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, ThreadMain, &start, 0, &id));
    if (!thread_) {
        CloseHandle(start.ready);
        return false;
    }
    threadId_ = id;
    WaitForSingleObject(start.ready, INFINITE);
    CloseHandle(start.ready);
    return start.ok;
}

void TypingService::Stop() {
    if (!thread_) return;
    PostThreadMessageW(threadId_, WM_QUIT, 0, 0);
    WaitForSingleObject(thread_, 5000);
    CloseHandle(thread_);
    thread_ = nullptr;
    threadId_ = 0;
}

void TypingService::Apply(const Config& config) {
    if (!thread_) return;
    auto* copy = new Config(config);
    if (!PostThreadMessageW(threadId_, WM_HOOK_APPLY, 0, reinterpret_cast<LPARAM>(copy))) delete copy;
}

}  // namespace viskey
