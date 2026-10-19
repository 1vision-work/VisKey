// VisKeyWin.cpp
// Native Windows (Win32 C++17) Vietnamese Input Method Application
// Features:
//  - Low-Level Keyboard & Mouse Hook (WH_KEYBOARD_LL, WH_MOUSE_LL) - Zero Preedit Underline
//  - Atomic SendInput() with KEYEVENTF_UNICODE & synthetic event marker (kVisKeyEventMagic)
//  - Browser / Windows Search / Excel Autocomplete Fix (U+202F injection)
//  - Per-app rules (VI / EN / remember, persisted), defaults EN in cmd/powershell/WindowsTerminal
//  - Configurable hotkeys (primary / secondary VI-EN toggle, Telex <-> VNI)
//  - System tray icon with a Fluent flyout (left click) and a small classic menu (right click)
//  - Custom-drawn Direct2D settings window (windows/ui/*), registry persistence, launch at startup
//  - Update check: manual button + optional automatic check (at most once per 24 h, tray timer,
//    quiet notice: nav dot, flyout line, one balloon per new version; never downloads anything)
//  - "--ui-snapshot <dir>" renders every screen to PNG without hooks, tray or registry access
//
// Build: see CMakeLists.txt (MSVC) or windows/README-Windows.txt (MinGW-w64).

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <objbase.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include "../core/viskey_engine.hpp"
#include "ui/common.h"
#include "ui/views.h"
#include "ui/update_policy.h"

static const ULONG_PTR kVisKeyEventMagic = 0x564B5821;
static const UINT kTrayId = 1;
static const UINT_PTR kSaveTimer = 1;
static const UINT_PTR kUpdateTimer = 2;
static const UINT kUpdateFirstDelayMs = 30 * 1000;             // after startup, off the critical path
static const UINT kUpdateRecheckMs = 3 * 60 * 60 * 1000;       // re-evaluate; the 24 h rule decides
#ifndef NIIF_RESPECT_QUIET_TIME
#define NIIF_RESPECT_QUIET_TIME 0x00000080
#endif
#ifndef NIN_BALLOONUSERCLICK
#define NIN_BALLOONUSERCLICK (WM_USER + 5)
#endif

enum ControlID {
    ID_TRAY_TOGGLE = 1001,
    ID_TRAY_SETTINGS,
    ID_TRAY_EXIT
};

static HINSTANCE g_hInst = nullptr;
static HWND g_hHostWnd = nullptr;  // hidden window owning the tray icon
static HHOOK g_hKeyboardHook = nullptr;
static HHOOK g_hMouseHook = nullptr;
static NOTIFYICONDATAW g_nid = {};
static HICON g_iconVi = nullptr;
static HICON g_iconEn = nullptr;
static UINT g_taskbarCreatedMsg = 0;
static bool g_uiAvailable = false;
static bool g_ignoreNextTrayUp = false;

static viskey::VisKeyEngine g_engine;
static vk::Model g_model;
static std::wstring g_lastExeName;     // lower-case
static std::wstring g_lastExeDisplay;  // as on disk
static std::string g_lastRawWord;      // raw keys of the word being typed (macro usage counters)
static HWND g_recordTarget = nullptr;  // settings window while recording a hotkey
static unsigned g_pendingSave = 0;

static const std::set<std::wstring> kDefaultEnglishExes = {
    L"cmd.exe", L"powershell.exe", L"pwsh.exe", L"windowsterminal.exe", L"mintty.exe", L"conhost.exe"
};

static const std::set<std::wstring> kAutocompleteExes = {
    L"chrome.exe", L"msedge.exe", L"brave.exe", L"firefox.exe", L"arc.exe", L"searchhost.exe", L"excel.exe"
};

// ---------------------------------------------------------------------------------------------
// Model <-> engine
namespace vk {

Model& M() { return g_model; }

std::map<std::string, std::string> DefaultMacros() { return viskey::EngineConfig().macros; }

std::vector<AppRule> DefaultRules() {
    static const wchar_t* display[] = {L"cmd.exe", L"powershell.exe", L"pwsh.exe", L"WindowsTerminal.exe", L"mintty.exe", L"conhost.exe"};
    std::vector<AppRule> rules;
    for (const wchar_t* d : display) {
        AppRule r;
        r.display = d;
        r.exe = LowerW(d);
        if (!kDefaultEnglishExes.count(r.exe)) continue;
        r.mode = RuleMode::EN;
        r.source = RuleSource::Default;
        r.lastVi = false;
        rules.push_back(r);
    }
    return rules;
}

void ResetModelToDefaults(Model& m) {
    viskey::EngineConfig def;
    m.vietnamese = true;
    m.vni = def.input_method == "vni";
    m.modernOrthography = def.modern_orthography;
    m.autoRestore = def.auto_restore_english;
    m.fixAutocomplete = def.fix_autocomplete;
    m.smartPerApp = true;
    m.macrosEnabled = def.enable_macros;
    // Fresh install: engine default. (Existing installs without a "QuickTelex" value keep
    // following "EnableMacros", see LoadModel.)
    m.quickTelex = def.quick_telex;
    m.launchAtStartup = false;
    m.policy = NewAppPolicy::RememberLast;
    Hotkey primary;
    primary.vk = VK_SPACE;
    primary.ctrl = true;
    Hotkey secondary;
    secondary.vk = 'Z';
    secondary.alt = true;
    m.hotkeys[kHotkeyPrimary] = primary;
    m.hotkeys[kHotkeySecondary] = secondary;
    m.hotkeys[kHotkeyMethod] = Hotkey();
    m.macros = def.macros;
    m.usage.clear();
    m.rules = DefaultRules();
    m.autoUpdateCheck = true;  // LastUpdateCheck / LastNotifiedVersion are bookkeeping, not options: kept
}

}  // namespace vk

static void ApplyModelToEngine() {
    viskey::EngineConfig& cfg = g_engine.config();
    cfg.input_method = g_model.vni ? "vni" : "telex";
    cfg.modern_orthography = g_model.modernOrthography;
    cfg.auto_restore_english = g_model.autoRestore;
    cfg.fix_autocomplete = g_model.fixAutocomplete;
    cfg.enable_macros = g_model.macrosEnabled;
    cfg.quick_telex = g_model.quickTelex;
    cfg.macros = g_model.macros;
}

static void ResetEngine() {
    g_engine.reset();
    g_lastRawWord.clear();
}

// ---------------------------------------------------------------------------------------------
// Tray icon
static HICON MakeGrayIcon(HICON src) {
    ICONINFO ii = {};
    if (!src || !GetIconInfo(src, &ii)) return nullptr;
    HICON result = nullptr;
    BITMAP bm = {};
    if (ii.hbmColor && GetObjectW(ii.hbmColor, sizeof(bm), &bm)) {
        int w = bm.bmWidth, h = bm.bmHeight;
        BITMAPINFO bi = {};
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = w;
        bi.bmiHeader.biHeight = -h;
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 32;
        bi.bmiHeader.biCompression = BI_RGB;
        std::vector<DWORD> px((size_t)w * h);
        HDC dc = GetDC(nullptr);
        if (GetDIBits(dc, ii.hbmColor, 0, (UINT)h, px.data(), &bi, DIB_RGB_COLORS)) {
            for (DWORD& p : px) {
                DWORD a = p >> 24, r = (p >> 16) & 0xFF, g = (p >> 8) & 0xFF, b = p & 0xFF;
                DWORD gray = (r * 30 + g * 59 + b * 11) / 100;
                gray = (gray + 0x90) / 2;  // lighter grey reads as "off" on light and dark taskbars
                p = (a << 24) | (gray << 16) | (gray << 8) | gray;
            }
            void* bits = nullptr;
            HBITMAP color = CreateDIBSection(dc, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
            if (color && bits) {
                memcpy(bits, px.data(), px.size() * sizeof(DWORD));
                ICONINFO ni = {};
                ni.fIcon = TRUE;
                ni.hbmMask = ii.hbmMask;
                ni.hbmColor = color;
                result = CreateIconIndirect(&ni);
            }
            if (color) DeleteObject(color);
        }
        ReleaseDC(nullptr, dc);
    }
    if (ii.hbmColor) DeleteObject(ii.hbmColor);
    if (ii.hbmMask) DeleteObject(ii.hbmMask);
    return result;
}

static void LoadTrayIcons() {
    int cx = GetSystemMetrics(SM_CXSMICON), cy = GetSystemMetrics(SM_CYSMICON);
    g_iconVi = static_cast<HICON>(LoadImageW(g_hInst, MAKEINTRESOURCEW(1), IMAGE_ICON, cx, cy, 0));
    if (!g_iconVi) g_iconVi = LoadIconW(nullptr, IDI_APPLICATION);
    g_iconEn = MakeGrayIcon(g_iconVi);
    if (!g_iconEn) g_iconEn = g_iconVi;
}

static void UpdateTrayTooltip() {
    if (g_model.snapshot || !g_nid.hWnd) return;
    std::wstring tip = g_model.vietnamese ? (g_model.vni ? L"VisKey — Tiếng Việt (VNI)" : L"VisKey — Tiếng Việt (Telex)") : L"VisKey — Tiếng Anh";
    wcsncpy_s(g_nid.szTip, tip.c_str(), _TRUNCATE);
    g_nid.hIcon = g_model.vietnamese ? g_iconVi : g_iconEn;
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
}

static void AddTrayIcon() {
    g_nid.cbSize = sizeof(NOTIFYICONDATAW);
    g_nid.hWnd = g_hHostWnd;
    g_nid.uID = kTrayId;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = vk::WM_VK_TRAYICON;
    g_nid.hIcon = g_model.vietnamese ? g_iconVi : g_iconEn;
    wcscpy_s(g_nid.szTip, L"VisKey");
    Shell_NotifyIconW(NIM_ADD, &g_nid);
    UpdateTrayTooltip();
}

static void RefreshWindows() {
    vk::RefreshSettingsWindow();
    vk::RefreshFlyout();
}

// Persist later (used from the keyboard hook so it never waits on the registry).
static void ScheduleSave(unsigned flags) {
    g_pendingSave |= flags;
    if (g_hHostWnd) SetTimer(g_hHostWnd, kSaveTimer, 400, nullptr);
}

static void FlushSaves() {
    unsigned f = g_pendingSave;
    g_pendingSave = 0;
    if (f & (vk::kChangeSettings | vk::kChangeHotkeys)) vk::SaveSettings(g_model);
    if (f & vk::kChangeMacros) vk::SaveMacros(g_model);
    if (f & vk::kChangeUsage) vk::SaveUsage(g_model);
    if (f & vk::kChangeRules) vk::SaveRules(g_model);
    if (f & vk::kChangeUpdateState) vk::SaveUpdateState(g_model);
}

// ---------------------------------------------------------------------------------------------
// Per-app memory
static vk::AppRule* FindRule(const std::wstring& exe) {
    for (auto& r : g_model.rules) {
        if (r.exe == exe) return &r;
    }
    return nullptr;
}

// Records the current VI/EN state for the app that was last in the foreground.
static void RememberModeForCurrentApp() {
    if (!g_model.smartPerApp || g_lastExeName.empty()) return;
    if (vk::AppRule* r = FindRule(g_lastExeName)) {
        r->lastVi = g_model.vietnamese;
        return;
    }
    vk::AppRule r;
    r.exe = g_lastExeName;
    r.display = g_lastExeDisplay;
    r.mode = vk::RuleMode::Remember;
    r.source = vk::RuleSource::Auto;
    r.lastVi = g_model.vietnamese;
    g_model.rules.push_back(r);
}

static bool GetForegroundExe(std::wstring& lower, std::wstring& display) {
    HWND hwnd = GetForegroundWindow();
    if (!hwnd) return false;
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (!pid || pid == GetCurrentProcessId()) return false;  // VisKey's own windows keep the previous app

    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProc) return false;
    wchar_t buf[MAX_PATH] = {0};
    DWORD sz = MAX_PATH;
    bool ok = false;
    if (QueryFullProcessImageNameW(hProc, 0, buf, &sz)) {
        std::wstring full(buf);
        size_t pos = full.find_last_of(L"\\/");
        display = (pos == std::wstring::npos) ? full : full.substr(pos + 1);
        lower = vk::LowerW(display);
        ok = !lower.empty();
    }
    CloseHandle(hProc);
    return ok;
}

static void CheckForegroundAppChange() {
    std::wstring curExe, curDisplay;
    if (!GetForegroundExe(curExe, curDisplay) || curExe == g_lastExeName) return;
    g_lastExeName = curExe;
    g_lastExeDisplay = curDisplay;
    ResetEngine();

    if (!g_model.smartPerApp) return;
    bool before = g_model.vietnamese;
    if (const vk::AppRule* r = FindRule(curExe)) {
        if (r->mode == vk::RuleMode::VI) g_model.vietnamese = true;
        else if (r->mode == vk::RuleMode::EN) g_model.vietnamese = false;
        else g_model.vietnamese = r->lastVi;
    } else if (g_model.policy == vk::NewAppPolicy::AlwaysVI) {
        g_model.vietnamese = true;
    } else if (g_model.policy == vk::NewAppPolicy::AlwaysEN) {
        g_model.vietnamese = false;
    }
    UpdateTrayTooltip();
    if (before != g_model.vietnamese) {
        ScheduleSave(vk::kChangeSettings);
        RefreshWindows();
    }
}

// ---------------------------------------------------------------------------------------------
// Atomic SendInput for Backspaces + UTF-16 Unicode string
static void SendVisKeyReplacement(int backspaces, const std::string& commitText, bool needAutocompleteFix) {
    std::vector<INPUT> inputs;

    auto addKeyVk = [&](WORD vkey, bool down) {
        INPUT in = {};
        in.type = INPUT_KEYBOARD;
        in.ki.wVk = vkey;
        in.ki.dwFlags = down ? 0 : KEYEVENTF_KEYUP;
        in.ki.dwExtraInfo = kVisKeyEventMagic;
        inputs.push_back(in);
    };

    auto addUnicodeChar = [&](wchar_t wc, bool down) {
        INPUT in = {};
        in.type = INPUT_KEYBOARD;
        in.ki.wVk = 0;
        in.ki.wScan = wc;
        in.ki.dwFlags = KEYEVENTF_UNICODE | (down ? 0 : KEYEVENTF_KEYUP);
        in.ki.dwExtraInfo = kVisKeyEventMagic;
        inputs.push_back(in);
    };

    if (needAutocompleteFix && backspaces > 0) {
        addUnicodeChar(0x202F, true);
        addUnicodeChar(0x202F, false);
        addKeyVk(VK_BACK, true);
        addKeyVk(VK_BACK, false);
    }

    for (int i = 0; i < backspaces; ++i) {
        addKeyVk(VK_BACK, true);
        addKeyVk(VK_BACK, false);
    }

    std::wstring wcommit = vk::Utf8ToWide(commitText);
    for (wchar_t wc : wcommit) {
        addUnicodeChar(wc, true);
        addUnicodeChar(wc, false);
    }

    if (!inputs.empty()) {
        SendInput((UINT)inputs.size(), inputs.data(), sizeof(INPUT));
    }
}

static LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        if (wParam == WM_LBUTTONDOWN || wParam == WM_RBUTTONDOWN || wParam == WM_MBUTTONDOWN) {
            ResetEngine();
        }
    }
    return CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);
}

static bool IsModifierVk(DWORD v) {
    return v == VK_SHIFT || v == VK_LSHIFT || v == VK_RSHIFT || v == VK_CONTROL || v == VK_LCONTROL || v == VK_RCONTROL ||
           v == VK_MENU || v == VK_LMENU || v == VK_RMENU || v == VK_LWIN || v == VK_RWIN;
}

// exact: Shift must match too. Otherwise a hotkey without Shift also fires with Shift held
// (Ctrl+Shift+Space toggled before hotkeys became configurable, keep that).
static bool HotkeyMatches(const vk::Hotkey& h, DWORD vkCode, bool ctrl, bool alt, bool shift, bool win, bool exact) {
    if (h.Empty() || h.vk != vkCode) return false;
    if (h.ctrl != ctrl || h.alt != alt || h.win != win) return false;
    if (exact || h.shift) return h.shift == shift;
    return true;
}

static void ToggleVietnameseFromHotkey() {
    g_model.vietnamese = !g_model.vietnamese;
    ResetEngine();
    RememberModeForCurrentApp();
    UpdateTrayTooltip();
    ScheduleSave(vk::kChangeSettings | vk::kChangeRules);
    RefreshWindows();
}

static void SwitchInputMethodFromHotkey() {
    g_model.vni = !g_model.vni;
    ApplyModelToEngine();
    ResetEngine();
    UpdateTrayTooltip();
    ScheduleSave(vk::kChangeSettings);
    RefreshWindows();
}

static void CountMacroUse(const std::string& raw) {
    std::string key = raw;
    for (char& c : key) {
        if (c >= 'A' && c <= 'Z') c = (char)(c + 32);
    }
    if (!g_model.macros.count(key)) return;
    uint32_t& n = g_model.usage[key];
    if (n < 0xFFFFFFFFu) ++n;
    ScheduleSave(vk::kChangeUsage);
}

static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode != HC_ACTION) {
        return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
    }

    KBDLLHOOKSTRUCT* kb = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
    if (!kb || (kb->flags & LLKHF_INJECTED) || kb->dwExtraInfo == kVisKeyEventMagic) {
        return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
    }

    if (wParam != WM_KEYDOWN && wParam != WM_SYSKEYDOWN) {
        return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
    }

    bool ctrl  = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    bool alt   = (GetAsyncKeyState(VK_MENU)    & 0x8000) != 0;
    bool win   = ((GetAsyncKeyState(VK_LWIN) | GetAsyncKeyState(VK_RWIN)) & 0x8000) != 0;
    bool shift = (GetAsyncKeyState(VK_SHIFT)   & 0x8000) != 0;
    bool caps  = (GetKeyState(VK_CAPITAL)      & 0x0001) != 0;

    // Hotkey recorder in the settings window: forward the combo, swallow the key.
    if (g_recordTarget) {
        if (IsModifierVk(kb->vkCode)) return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
        DWORD mods = (ctrl ? vk::kModCtrl : 0) | (alt ? vk::kModAlt : 0) | (shift ? vk::kModShift : 0) | (win ? vk::kModWin : 0);
        PostMessageW(g_recordTarget, vk::WM_VK_HOTKEY_RECORDED, (WPARAM)kb->vkCode, (LPARAM)mods);
        return 1;
    }

    CheckForegroundAppChange();

    // Configurable hotkeys (defaults: Ctrl + Space, Alt + Z; Telex <-> VNI unset).
    for (int pass = 0; pass < 2; ++pass) {
        for (int slot = 0; slot < vk::kHotkeyCount; ++slot) {
            if (!HotkeyMatches(g_model.hotkeys[slot], kb->vkCode, ctrl, alt, shift, win, pass == 0)) continue;
            if (slot == vk::kHotkeyMethod) SwitchInputMethodFromHotkey();
            else ToggleVietnameseFromHotkey();
            return 1;
        }
    }

    if (ctrl || alt || win) {
        ResetEngine();
        return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
    }

    if (!g_model.vietnamese) {
        return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
    }

    if (kb->vkCode == VK_BACK) {
        auto res = g_engine.process_backspace();
        g_lastRawWord = res.raw_word;
        if (res.handled) {
            SendVisKeyReplacement(res.backspaces, res.commit_text, res.autocomplete_fix_needed);
            return 1;
        }
        return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
    }

    if (kb->vkCode == VK_LEFT || kb->vkCode == VK_RIGHT ||
        kb->vkCode == VK_UP   || kb->vkCode == VK_DOWN  ||
        kb->vkCode == VK_HOME || kb->vkCode == VK_END   ||
        kb->vkCode == VK_PRIOR|| kb->vkCode == VK_NEXT  ||
        kb->vkCode == VK_ESCAPE || kb->vkCode == VK_TAB ||
        kb->vkCode == VK_RETURN) {
        ResetEngine();
        return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
    }

    char ch = 0;
    if (kb->vkCode >= 'A' && kb->vkCode <= 'Z') {
        bool isUpper = (shift ^ caps);
        ch = isUpper ? (char)kb->vkCode : (char)(kb->vkCode + 32);
    } else if (!shift && kb->vkCode >= '0' && kb->vkCode <= '9') {
        ch = (char)kb->vkCode;
    } else if (kb->vkCode == VK_SPACE) {
        ch = ' ';
    } else {
        BYTE keyState[256] = {0};
        GetKeyboardState(keyState);
        wchar_t wbuf[4] = {0};
        int r = ToUnicode(kb->vkCode, kb->scanCode, keyState, wbuf, 4, 0);
        if (r == 1 && wbuf[0] >= 32 && wbuf[0] <= 126) {
            ch = (char)wbuf[0];
        } else {
            ResetEngine();
            return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
        }
    }

    // The engine expands a macro when a word boundary follows a known key; it then returns a
    // handled result with an empty word. Detect that here to count usage without touching the core.
    std::string rawBefore = g_lastRawWord;
    auto res = g_engine.process_key(ch);
    g_lastRawWord = res.raw_word;
    if (res.handled && res.raw_word.empty() && res.current_word.empty() && !rawBefore.empty() && g_engine.config().enable_macros) {
        CountMacroUse(rawBefore);
    }
    if (!res.handled) {
        return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
    }

    bool isAcApp = kAutocompleteExes.count(g_lastExeName) > 0;
    bool useFix = res.autocomplete_fix_needed || (g_engine.config().fix_autocomplete && isAcApp && res.backspaces == 1);
    SendVisKeyReplacement(res.backspaces, res.commit_text, useFix);
    return 1;
}

// ---------------------------------------------------------------------------------------------
// Application core used by the UI (declared in ui/common.h)
namespace vk {

void Commit(unsigned flags) {
    ApplyModelToEngine();
    ResetEngine();
    if (!g_model.snapshot) {
        g_pendingSave |= flags;
        FlushSaves();
    }
    UpdateTrayTooltip();
    RefreshWindows();
}

void SetVietnamese(bool on) {
    g_model.vietnamese = on;
    ResetEngine();
    RememberModeForCurrentApp();
    Commit(kChangeSettings | kChangeRules);
}

void OpenSettings(int page) {
    HideTrayFlyout();
    if (!g_uiAvailable) {
        MessageBoxW(nullptr, L"Không khởi tạo được Direct2D/DirectWrite nên không mở được Bảng điều khiển.\n"
                             L"Bạn vẫn gõ được tiếng Việt và đổi tuỳ chọn qua menu chuột phải ở khay hệ thống.",
                    L"VisKey", MB_OK | MB_ICONWARNING);
        return;
    }
    ShowSettingsWindow(page);
}

void QuitApp() {
    if (g_nid.hWnd) Shell_NotifyIconW(NIM_DELETE, &g_nid);
    g_nid.hWnd = nullptr;
    PostQuitMessage(0);
}

void BeginHotkeyRecord(HWND target) { g_recordTarget = target; }
void EndHotkeyRecord() { g_recordTarget = nullptr; }

}  // namespace vk

// ---------------------------------------------------------------------------------------------
// Update check: manual button and automatic daily check share vk::StartUpdateCheck (one GET on a
// worker thread); the result comes back to the tray host as WM_VK_UPDATE.
static void BeginUpdateCheck(bool automatic) {
    g_model.updateChecking = true;
    g_model.lastUpdateCheck = vk::UnixNow();  // attempt time: failures count for the 24 h rule too
    vk::SaveUpdateState(g_model);
    vk::StartUpdateCheck(g_hHostWnd, automatic);
    RefreshWindows();
}

void vk::RequestManualUpdateCheck() {
    if (g_model.snapshot || g_model.updateChecking || !g_hHostWnd) return;
    BeginUpdateCheck(false);
}

static void MaybeAutoUpdateCheck() {
    if (g_model.snapshot || g_model.updateChecking) return;
    if (!vk::update::AutoCheckDue(g_model.autoUpdateCheck, vk::UnixNow(), g_model.lastUpdateCheck)) return;
    BeginUpdateCheck(true);
}

// Quiet one-off balloon (no sound, respects quiet time); clicking it opens the update page.
static void ShowUpdateBalloon(const std::wstring& version) {
    if (!g_nid.hWnd) return;
    NOTIFYICONDATAW n = g_nid;
    n.uFlags = NIF_INFO;
    n.dwInfoFlags = NIIF_INFO | NIIF_NOSOUND | NIIF_RESPECT_QUIET_TIME;
    std::wstring title = L"Có bản mới VisKey " + version;
    wcsncpy_s(n.szInfoTitle, title.c_str(), _TRUNCATE);
    wcsncpy_s(n.szInfo, L"Bấm để mở trang Cập nhật. VisKey không tự tải hay cài đặt gì.", _TRUNCATE);
    Shell_NotifyIconW(NIM_MODIFY, &n);
}

static void OnUpdateResult(bool automatic, vk::UpdateResult* r) {
    g_model.updateChecking = false;
    if (!r) return;
    g_model.update = *r;
    g_model.haveUpdateResult = true;
    g_model.updateResultAuto = automatic;
    delete r;
    const vk::UpdateResult& u = g_model.update;
    if (!u.ok) {
        // Automatic failures stay silent: only the "Lần kiểm tra cuối" line on the update page.
        std::wstring log = L"VisKey: update check failed (" + std::wstring(automatic ? L"auto" : L"manual") + L"): " + u.error + L"\n";
        OutputDebugStringW(log.c_str());
    } else if (u.newer && vk::update::ShouldNotify(u.latest, vk::AppVersion(), g_model.lastNotifiedVersion)) {
        // Once per version: a manual check that finds it counts as seen, so no balloon later.
        g_model.lastNotifiedVersion = u.latest;
        vk::SaveUpdateState(g_model);
        if (automatic) ShowUpdateBalloon(u.latest);
    }
    RefreshWindows();
}

// ---------------------------------------------------------------------------------------------
// Tray host window
static void ShowTrayMenu(HWND hWnd) {
    POINT pt;
    GetCursorPos(&pt);
    HMENU hMenu = CreatePopupMenu();
    std::wstring hk = vk::HotkeyText(g_model.hotkeys[vk::kHotkeyPrimary]);
    std::wstring toggle = g_model.vietnamese ? L"Tắt tiếng Việt" : L"Bật tiếng Việt";
    if (!hk.empty()) toggle += L"\t" + hk;

    AppendMenuW(hMenu, MF_STRING, ID_TRAY_SETTINGS, L"Mở Bảng điều khiển");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_TOGGLE, toggle.c_str());
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"Thoát VisKey");
    SetMenuDefaultItem(hMenu, ID_TRAY_SETTINGS, FALSE);

    SetForegroundWindow(hWnd);
    TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, nullptr);
    PostMessageW(hWnd, WM_NULL, 0, 0);
    DestroyMenu(hMenu);
}

static LRESULT CALLBACK HostWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (g_taskbarCreatedMsg && msg == g_taskbarCreatedMsg) {
        AddTrayIcon();  // Explorer restarted
        return 0;
    }
    switch (msg) {
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_TRAY_TOGGLE: vk::SetVietnamese(!g_model.vietnamese); break;
        case ID_TRAY_SETTINGS: vk::OpenSettings(-1); break;
        case ID_TRAY_EXIT: vk::QuitApp(); break;
        default: break;
        }
        return 0;
    case vk::WM_VK_TRAYICON:
        switch (LOWORD(lParam)) {
        case WM_LBUTTONUP:
            if (g_ignoreNextTrayUp) {
                g_ignoreNextTrayUp = false;
            } else if (!g_uiAvailable) {
                ShowTrayMenu(hWnd);
            } else if (!vk::TrayFlyoutRecentlyClosed()) {
                vk::ShowTrayFlyout(hWnd, kTrayId);
            }
            break;
        case WM_LBUTTONDBLCLK:
            g_ignoreNextTrayUp = true;
            vk::OpenSettings(-1);
            break;
        case NIN_BALLOONUSERCLICK:
            vk::OpenSettings(vk::kPageUpdate);
            break;
        case WM_RBUTTONUP:
        case WM_CONTEXTMENU:
            vk::HideTrayFlyout();
            ShowTrayMenu(hWnd);
            break;
        default: break;
        }
        return 0;
    case WM_TIMER:
        if (wParam == kSaveTimer) {
            KillTimer(hWnd, kSaveTimer);
            FlushSaves();
            RefreshWindows();
        } else if (wParam == kUpdateTimer) {
            SetTimer(hWnd, kUpdateTimer, kUpdateRecheckMs, nullptr);  // first shot after startup, then every 3 h
            MaybeAutoUpdateCheck();
        }
        return 0;
    case vk::WM_VK_UPDATE:
        OnUpdateResult(wParam != 0, reinterpret_cast<vk::UpdateResult*>(lParam));
        return 0;
    case WM_DESTROY:
        if (g_nid.hWnd) Shell_NotifyIconW(NIM_DELETE, &g_nid);
        g_nid.hWnd = nullptr;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

static int RunSnapshot(int argc, wchar_t** argv) {
    if (argc < 3) return 2;
    g_model.snapshot = true;
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    int rc = vk::RunUiSnapshot(argv[2]);
    if (SUCCEEDED(hr)) CoUninitialize();
    return rc;
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int) {
    g_hInst = hInstance;

    int argc = 0;
    wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv && argc >= 2 && lstrcmpiW(argv[1], L"--ui-snapshot") == 0) {
        int rc = RunSnapshot(argc, argv);
        LocalFree(argv);
        return rc;
    }
    if (argv) LocalFree(argv);

    vk::EnablePerMonitorDpi();
    HRESULT coHr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    vk::LoadModel(g_model);
    ApplyModelToEngine();
    g_uiAvailable = vk::GfxStartup();

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = HostWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"VisKeyWin32Class";
    RegisterClassExW(&wc);
    g_hHostWnd = CreateWindowExW(WS_EX_TOOLWINDOW, wc.lpszClassName, L"VisKey", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr, hInstance, nullptr);

    g_taskbarCreatedMsg = RegisterWindowMessageW(L"TaskbarCreated");
    LoadTrayIcons();
    AddTrayIcon();

    g_hKeyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandleW(nullptr), 0);
    g_hMouseHook    = SetWindowsHookExW(WH_MOUSE_LL,    LowLevelMouseProc,    GetModuleHandleW(nullptr), 0);
    g_model.hooksActive = g_hKeyboardHook != nullptr;

    // Automatic update check (if enabled): first look 30 s after startup, never more than once a day.
    if (g_hHostWnd) SetTimer(g_hHostWnd, kUpdateTimer, kUpdateFirstDelayMs, nullptr);

    // Show the control panel on launch so the user knows VisKey is active.
    if (g_uiAvailable) {
        vk::CreateSettingsWindow(hInstance);
        vk::ShowSettingsWindow(-1);
    }

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (g_hKeyboardHook) UnhookWindowsHookEx(g_hKeyboardHook);
    if (g_hMouseHook)    UnhookWindowsHookEx(g_hMouseHook);
    if (g_pendingSave) FlushSaves();
    if (g_nid.hWnd) Shell_NotifyIconW(NIM_DELETE, &g_nid);
    if (SUCCEEDED(coHr)) CoUninitialize();
    return 0;
}
