// SPDX-License-Identifier: GPL-3.0-or-later
//
// VisKey for Windows (by 1VISION) — based on OpenKey by Mai Vũ Tuyên.
//
// The typing pipeline: a dedicated thread owns the engine and installs WH_KEYBOARD_LL, WH_MOUSE_LL and an
// EVENT_SYSTEM_FOREGROUND WinEvent hook, then runs its own message loop (low-level hook callbacks run on
// the installing thread). The UI thread never touches the engine: it posts config changes here, and this
// thread posts language changes (hotkey) back. No state is shared between the two threads.

#pragma once

#include <windows.h>

#include "config.h"

namespace viskey {

// Messages posted to the notify window.
constexpr UINT WM_APP_LANGUAGE_CHANGED = WM_APP + 1;  // wParam: 1 Vietnamese, 0 English (hotkey toggle)
constexpr UINT WM_APP_HOOK_STATUS = WM_APP + 2;       // wParam: 1 hooks installed, 0 failed

class TypingService {
public:
    TypingService() = default;
    ~TypingService() { Stop(); }
    TypingService(const TypingService&) = delete;
    TypingService& operator=(const TypingService&) = delete;

    // Starts the hook thread and waits until the hooks are installed. Returns false if they could not be.
    bool Start(HWND notifyWindow, const Config& config);
    void Stop();
    // Hands a copy of the settings to the hook thread (options, macros, hotkey, language).
    void Apply(const Config& config);

private:
    HANDLE thread_ = nullptr;
    DWORD threadId_ = 0;
};

}  // namespace viskey
