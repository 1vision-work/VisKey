// VisKeyWin.cpp
// Native Windows (Win32 C++17) Vietnamese Input Method Application
// Features:
//  - Low-Level Keyboard & Mouse Hook (WH_KEYBOARD_LL, WH_MOUSE_LL) - Zero Preedit Underline
//  - Atomic SendInput() with KEYEVENTF_UNICODE & synthetic event marker (kVisKeyEventMagic)
//  - Browser / Windows Search / Excel Autocomplete Fix (U+202F injection)
//  - Smart Per-App Mode (Remembers VI/EN per process .exe, defaults EN in cmd/powershell/WindowsTerminal)
//  - System Tray Icon (Shell_NotifyIconW) with context menu & live VI/EN indicator
//  - Native Win32 Settings & Macro Manager Window + Windows Registry persistence + Launch at Startup
//
// Compile with MinGW-w64 (g++):
//   g++ -std=c++17 -O2 -mwindows -municode windows/VisKeyWin.cpp -luser32 -lgdi32 -lshell32 -ladvapi32 -lcomctl32 -o VisKey64.exe
// Compile with MSVC (Developer Command Prompt):
//   cl /std:c++17 /O2 /EHsc /utf-8 /DUNICODE /D_UNICODE windows\VisKeyWin.cpp user32.lib gdi32.lib shell32.lib advapi32.lib comctl32.lib /link /SUBSYSTEM:WINDOWS /OUT:VisKey64.exe

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
#include <shellapi.h>
#include <commctrl.h>
#include <psapi.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include "../core/viskey_engine.hpp"

static const ULONG_PTR kVisKeyEventMagic = 0x564B5821;
static const UINT WM_TRAYICON = WM_USER + 100;

enum ControlID {
    ID_TRAY_TOGGLE = 1001,
    ID_TRAY_SETTINGS,
    ID_TRAY_TELEX,
    ID_TRAY_VNI,
    ID_TRAY_ORTHO,
    ID_TRAY_RESTORE,
    ID_TRAY_ACFIX,
    ID_TRAY_PERAPP,
    ID_TRAY_MACROS,
    ID_TRAY_EXIT,

    ID_CMB_METHOD = 2001,
    ID_CHK_RESTORE,
    ID_CHK_ACFIX,
    ID_CHK_ORTHO,
    ID_CHK_PERAPP,
    ID_CHK_MACRO,
    ID_CHK_STARTUP,
    ID_LST_MACROS,
    ID_EDT_MACRO_KEY,
    ID_EDT_MACRO_VAL,
    ID_BTN_ADD_MACRO,
    ID_BTN_DEL_MACRO,
    ID_BTN_SAVE
};

static HINSTANCE g_hInst = nullptr;
static HWND g_hMainWnd = nullptr;
static HHOOK g_hKeyboardHook = nullptr;
static HHOOK g_hMouseHook = nullptr;
static NOTIFYICONDATAW g_nid = {};

static viskey::VisKeyEngine g_engine;
static bool g_vietnameseEnabled = true;
static bool g_smartPerAppMode = true;
static bool g_launchAtStartup = false;
static std::map<std::wstring, bool> g_appModes;
static std::wstring g_lastExeName;

// UI Handles
static HWND g_hCmbMethod = nullptr;
static HWND g_hChkRestore = nullptr;
static HWND g_hChkAcFix = nullptr;
static HWND g_hChkOrtho = nullptr;
static HWND g_hChkPerApp = nullptr;
static HWND g_hChkMacro = nullptr;
static HWND g_hChkStartup = nullptr;
static HWND g_hListMacros = nullptr;
static HWND g_hEdtKey = nullptr;
static HWND g_hEdtVal = nullptr;

static const std::set<std::wstring> kDefaultEnglishExes = {
    L"cmd.exe", L"powershell.exe", L"pwsh.exe", L"windowsterminal.exe", L"mintty.exe", L"conhost.exe"
};

static const std::set<std::wstring> kAutocompleteExes = {
    L"chrome.exe", L"msedge.exe", L"brave.exe", L"firefox.exe", L"arc.exe", L"searchhost.exe", L"excel.exe"
};

static std::wstring utf8_to_wide(const std::string& str) {
    if (str.empty()) return L"";
    int sz = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), nullptr, 0);
    std::wstring out(sz, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &out[0], sz);
    return out;
}

static std::string wide_to_utf8(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int sz = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    std::string out(sz, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &out[0], sz, nullptr, nullptr);
    return out;
}

static std::wstring GetForegroundExeName() {
    HWND hwnd = GetForegroundWindow();
    if (!hwnd) return L"";
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (!pid) return L"";

    std::wstring exeName;
    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (hProc) {
        wchar_t buf[MAX_PATH] = {0};
        DWORD sz = MAX_PATH;
        if (QueryFullProcessImageNameW(hProc, 0, buf, &sz)) {
            std::wstring full(buf);
            size_t pos = full.find_last_of(L"\\/");
            exeName = (pos == std::wstring::npos) ? full : full.substr(pos + 1);
            for (auto& c : exeName) c = (wchar_t)towlower(c);
        }
        CloseHandle(hProc);
    }
    return exeName;
}

static void UpdateTrayTooltip() {
    wcscpy_s(g_nid.szTip, g_vietnameseEnabled ? L"VisKey - Tiếng Việt (VI) [Ctrl+Space]" : L"VisKey - Tiếng Anh (EN) [Ctrl+Space]");
    g_nid.hIcon = LoadIconW(nullptr, g_vietnameseEnabled ? IDI_APPLICATION : IDI_INFORMATION);
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
}

static void CheckForegroundAppChange() {
    std::wstring curExe = GetForegroundExeName();
    if (curExe.empty() || curExe == g_lastExeName) return;
    g_lastExeName = curExe;
    g_engine.reset();

    if (!g_smartPerAppMode) return;
    auto it = g_appModes.find(curExe);
    if (it != g_appModes.end()) {
        g_vietnameseEnabled = it->second;
    } else if (kDefaultEnglishExes.count(curExe)) {
        g_vietnameseEnabled = false;
        g_appModes[curExe] = false;
    }
    UpdateTrayTooltip();
}

// Atomic SendInput for Backspaces + UTF-16 Unicode string
static void SendVisKeyReplacement(int backspaces, const std::string& commitText, bool needAutocompleteFix) {
    std::vector<INPUT> inputs;

    auto addKeyVk = [&](WORD vk, bool down) {
        INPUT in = {};
        in.type = INPUT_KEYBOARD;
        in.ki.wVk = vk;
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

    std::wstring wcommit = utf8_to_wide(commitText);
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
            g_engine.reset();
        }
    }
    return CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);
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

    CheckForegroundAppChange();

    bool ctrl  = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    bool alt   = (GetAsyncKeyState(VK_MENU)    & 0x8000) != 0;
    bool win   = ((GetAsyncKeyState(VK_LWIN) | GetAsyncKeyState(VK_RWIN)) & 0x8000) != 0;
    bool shift = (GetAsyncKeyState(VK_SHIFT)   & 0x8000) != 0;
    bool caps  = (GetKeyState(VK_CAPITAL)      & 0x0001) != 0;

    // Hotkey: Ctrl + Space or Alt + Z
    if ((ctrl && !alt && !win && kb->vkCode == VK_SPACE) ||
        (alt && !ctrl && !win && kb->vkCode == 'Z')) {
        g_vietnameseEnabled = !g_vietnameseEnabled;
        g_engine.reset();
        if (g_smartPerAppMode && !g_lastExeName.empty()) {
            g_appModes[g_lastExeName] = g_vietnameseEnabled;
        }
        UpdateTrayTooltip();
        return 1;
    }

    if (ctrl || alt || win) {
        g_engine.reset();
        return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
    }

    if (!g_vietnameseEnabled) {
        return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
    }

    if (kb->vkCode == VK_BACK) {
        auto res = g_engine.process_backspace();
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
        g_engine.reset();
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
            g_engine.reset();
            return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
        }
    }

    auto res = g_engine.process_key(ch);
    if (!res.handled) {
        return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
    }

    bool isAcApp = kAutocompleteExes.count(g_lastExeName) > 0;
    bool useFix = res.autocomplete_fix_needed || (g_engine.config().fix_autocomplete && isAcApp && res.backspaces == 1);
    SendVisKeyReplacement(res.backspaces, res.commit_text, useFix);
    return 1;
}

static void SaveRegistrySettings() {
    HKEY hKey = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\VisKey", 0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) == ERROR_SUCCESS) {
        DWORD vEn = g_vietnameseEnabled ? 1 : 0;
        DWORD vIm = (g_engine.config().input_method == "vni") ? 1 : 0;
        DWORD vOr = g_engine.config().modern_orthography ? 1 : 0;
        DWORD vRe = g_engine.config().auto_restore_english ? 1 : 0;
        DWORD vAc = g_engine.config().fix_autocomplete ? 1 : 0;
        DWORD vPa = g_smartPerAppMode ? 1 : 0;
        DWORD vMa = g_engine.config().enable_macros ? 1 : 0;
        DWORD vSt = g_launchAtStartup ? 1 : 0;

        RegSetValueExW(hKey, L"VietnameseEnabled", 0, REG_DWORD, (const BYTE*)&vEn, sizeof(DWORD));
        RegSetValueExW(hKey, L"InputMethodVNI",    0, REG_DWORD, (const BYTE*)&vIm, sizeof(DWORD));
        RegSetValueExW(hKey, L"ModernOrthography", 0, REG_DWORD, (const BYTE*)&vOr, sizeof(DWORD));
        RegSetValueExW(hKey, L"AutoRestore",       0, REG_DWORD, (const BYTE*)&vRe, sizeof(DWORD));
        RegSetValueExW(hKey, L"FixAutocomplete",   0, REG_DWORD, (const BYTE*)&vAc, sizeof(DWORD));
        RegSetValueExW(hKey, L"SmartPerApp",       0, REG_DWORD, (const BYTE*)&vPa, sizeof(DWORD));
        RegSetValueExW(hKey, L"EnableMacros",      0, REG_DWORD, (const BYTE*)&vMa, sizeof(DWORD));
        RegSetValueExW(hKey, L"LaunchAtStartup",   0, REG_DWORD, (const BYTE*)&vSt, sizeof(DWORD));
        RegCloseKey(hKey);
    }

    HKEY hRun = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_WRITE, &hRun) == ERROR_SUCCESS) {
        if (g_launchAtStartup) {
            wchar_t exePath[MAX_PATH] = {0};
            GetModuleFileNameW(nullptr, exePath, MAX_PATH);
            RegSetValueExW(hRun, L"VisKey", 0, REG_SZ, (const BYTE*)exePath, (DWORD)((wcslen(exePath) + 1) * sizeof(wchar_t)));
        } else {
            RegDeleteValueW(hRun, L"VisKey");
        }
        RegCloseKey(hRun);
    }
}

static void LoadRegistrySettings() {
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\VisKey", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        auto readDw = [&](const wchar_t* name, DWORD defVal) -> DWORD {
            DWORD val = defVal, sz = sizeof(DWORD);
            RegQueryValueExW(hKey, name, nullptr, nullptr, (LPBYTE)&val, &sz);
            return val;
        };
        g_vietnameseEnabled = readDw(L"VietnameseEnabled", 1) != 0;
        g_engine.config().input_method = (readDw(L"InputMethodVNI", 0) != 0) ? "vni" : "telex";
        g_engine.config().modern_orthography = readDw(L"ModernOrthography", 0) != 0;
        g_engine.config().auto_restore_english = readDw(L"AutoRestore", 1) != 0;
        g_engine.config().fix_autocomplete = readDw(L"FixAutocomplete", 1) != 0;
        g_smartPerAppMode = readDw(L"SmartPerApp", 1) != 0;
        g_engine.config().enable_macros = readDw(L"EnableMacros", 1) != 0;
        g_engine.config().quick_telex = g_engine.config().enable_macros;
        g_launchAtStartup = readDw(L"LaunchAtStartup", 0) != 0;
        RegCloseKey(hKey);
    }
}

static void RefreshMacroListUI() {
    if (!g_hListMacros) return;
    SendMessageW(g_hListMacros, LB_RESETCONTENT, 0, 0);
    for (const auto& kv : g_engine.config().macros) {
        std::wstring item = utf8_to_wide(kv.first + "  =>  " + kv.second);
        SendMessageW(g_hListMacros, LB_ADDSTRING, 0, (LPARAM)item.c_str());
    }
}

static void SyncControlsFromState() {
    if (!g_hMainWnd) return;
    SendMessageW(g_hCmbMethod, CB_SETCURSEL, (g_engine.config().input_method == "vni") ? 1 : 0, 0);
    SendMessageW(g_hChkRestore, BM_SETCHECK, g_engine.config().auto_restore_english ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(g_hChkAcFix,   BM_SETCHECK, g_engine.config().fix_autocomplete ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(g_hChkOrtho,   BM_SETCHECK, g_engine.config().modern_orthography ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(g_hChkPerApp,  BM_SETCHECK, g_smartPerAppMode ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(g_hChkMacro,   BM_SETCHECK, g_engine.config().enable_macros ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(g_hChkStartup, BM_SETCHECK, g_launchAtStartup ? BST_CHECKED : BST_UNCHECKED, 0);
    RefreshMacroListUI();
}

static void ShowTrayMenu(HWND hWnd) {
    POINT pt;
    GetCursorPos(&pt);
    HMENU hMenu = CreatePopupMenu();

    AppendMenuW(hMenu, MF_STRING | (g_vietnameseEnabled ? MF_CHECKED : 0), ID_TRAY_TOGGLE, L"Bật Tiếng Việt (Ctrl+Space / Alt+Z)");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_SETTINGS, L"Mở Bảng điều khiển VisKey...");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);

    bool isTelex = (g_engine.config().input_method == "telex");
    AppendMenuW(hMenu, MF_STRING | (isTelex ? MF_CHECKED : 0), ID_TRAY_TELEX, L"Kiểu gõ Telex");
    AppendMenuW(hMenu, MF_STRING | (!isTelex ? MF_CHECKED : 0), ID_TRAY_VNI, L"Kiểu gõ VNI");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);

    AppendMenuW(hMenu, MF_STRING | (g_engine.config().auto_restore_english ? MF_CHECKED : 0), ID_TRAY_RESTORE, L"Tự khôi phục từ tiếng Anh");
    AppendMenuW(hMenu, MF_STRING | (g_engine.config().fix_autocomplete ? MF_CHECKED : 0), ID_TRAY_ACFIX, L"Sửa lỗi Autocomplete Trình duyệt");
    AppendMenuW(hMenu, MF_STRING | (g_engine.config().modern_orthography ? MF_CHECKED : 0), ID_TRAY_ORTHO, L"Đặt dấu kiểu mới (hoà, thuý)");
    AppendMenuW(hMenu, MF_STRING | (g_smartPerAppMode ? MF_CHECKED : 0), ID_TRAY_PERAPP, L"Ghi nhớ chế độ theo từng ứng dụng");
    AppendMenuW(hMenu, MF_STRING | (g_engine.config().enable_macros ? MF_CHECKED : 0), ID_TRAY_MACROS, L"Bật Gõ tắt (Macro & Quick Telex)");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"Thoát VisKey");

    SetForegroundWindow(hWnd);
    TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, nullptr);
    DestroyMenu(hMenu);
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        auto addCtrl = [&](const wchar_t* cls, const wchar_t* txt, DWORD style, int x, int y, int w, int h, int id) -> HWND {
            HWND hc = CreateWindowExW(0, cls, txt, WS_CHILD | WS_VISIBLE | style, x, y, w, h, hWnd, (HMENU)(INT_PTR)id, g_hInst, nullptr);
            SendMessageW(hc, WM_SETFONT, (WPARAM)hFont, TRUE);
            return hc;
        };

        addCtrl(L"STATIC", L"Kiểu gõ Tiếng Việt:", 0, 20, 18, 130, 22, 0);
        g_hCmbMethod = addCtrl(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL, 155, 14, 180, 100, ID_CMB_METHOD);
        SendMessageW(g_hCmbMethod, CB_ADDSTRING, 0, (LPARAM)L"Telex (Chuẩn)");
        SendMessageW(g_hCmbMethod, CB_ADDSTRING, 0, (LPARAM)L"VNI");

        g_hChkRestore = addCtrl(L"BUTTON", L"Tự động khôi phục từ tiếng Anh khi gõ song ngữ (window, software...)", BS_AUTOCHECKBOX, 20, 50, 440, 22, ID_CHK_RESTORE);
        g_hChkAcFix   = addCtrl(L"BUTTON", L"Sửa lỗi mất/nhân đôi chữ trên Chrome, Edge, Windows Search, Excel", BS_AUTOCHECKBOX, 20, 78, 440, 22, ID_CHK_ACFIX);
        g_hChkOrtho   = addCtrl(L"BUTTON", L"Đặt dấu kiểu mới (hoà, thuý thay vì hòa, thúy)", BS_AUTOCHECKBOX, 20, 106, 440, 22, ID_CHK_ORTHO);
        g_hChkPerApp  = addCtrl(L"BUTTON", L"Ghi nhớ chế độ Anh/Việt theo từng ứng dụng (Tự tắt ở CMD/PowerShell)", BS_AUTOCHECKBOX, 20, 134, 440, 22, ID_CHK_PERAPP);
        g_hChkMacro   = addCtrl(L"BUTTON", L"Bật bảng Gõ tắt (Macro) & Quick Telex (cc=ch, gg=gi, nn=ng...)", BS_AUTOCHECKBOX, 20, 162, 440, 22, ID_CHK_MACRO);
        g_hChkStartup = addCtrl(L"BUTTON", L"Tự khởi động VisKey cùng hệ điều hành Windows", BS_AUTOCHECKBOX, 20, 190, 440, 22, ID_CHK_STARTUP);

        addCtrl(L"STATIC", L"Danh sách từ Gõ tắt (Macro):", 0, 20, 224, 240, 20, 0);
        g_hListMacros = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY, 20, 246, 440, 110, hWnd, (HMENU)ID_LST_MACROS, g_hInst, nullptr);
        SendMessageW(g_hListMacros, WM_SETFONT, (WPARAM)hFont, TRUE);

        g_hEdtKey = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 20, 366, 90, 26, hWnd, (HMENU)ID_EDT_MACRO_KEY, g_hInst, nullptr);
        SendMessageW(g_hEdtKey, WM_SETFONT, (WPARAM)hFont, TRUE);

        g_hEdtVal = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 118, 366, 190, 26, hWnd, (HMENU)ID_EDT_MACRO_VAL, g_hInst, nullptr);
        SendMessageW(g_hEdtVal, WM_SETFONT, (WPARAM)hFont, TRUE);

        addCtrl(L"BUTTON", L"Thêm / Sửa", BS_PUSHBUTTON, 316, 365, 70, 28, ID_BTN_ADD_MACRO);
        addCtrl(L"BUTTON", L"Xóa", BS_PUSHBUTTON, 390, 365, 70, 28, ID_BTN_DEL_MACRO);
        addCtrl(L"BUTTON", L"Lưu cấu hình & Ẩn xuống Khay hệ thống", BS_DEFPUSHBUTTON, 180, 408, 280, 32, ID_BTN_SAVE);

        SyncControlsFromState();
        return 0;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        switch (id) {
        case ID_TRAY_TOGGLE:
            g_vietnameseEnabled = !g_vietnameseEnabled;
            g_engine.reset();
            UpdateTrayTooltip();
            SaveRegistrySettings();
            break;
        case ID_TRAY_SETTINGS:
            SyncControlsFromState();
            ShowWindow(hWnd, SW_SHOW);
            SetForegroundWindow(hWnd);
            break;
        case ID_TRAY_TELEX:
            g_engine.config().input_method = "telex";
            g_engine.reset();
            SyncControlsFromState();
            SaveRegistrySettings();
            break;
        case ID_TRAY_VNI:
            g_engine.config().input_method = "vni";
            g_engine.reset();
            SyncControlsFromState();
            SaveRegistrySettings();
            break;
        case ID_TRAY_RESTORE:
            g_engine.config().auto_restore_english = !g_engine.config().auto_restore_english;
            SyncControlsFromState();
            SaveRegistrySettings();
            break;
        case ID_TRAY_ACFIX:
            g_engine.config().fix_autocomplete = !g_engine.config().fix_autocomplete;
            SyncControlsFromState();
            SaveRegistrySettings();
            break;
        case ID_TRAY_ORTHO:
            g_engine.config().modern_orthography = !g_engine.config().modern_orthography;
            SyncControlsFromState();
            SaveRegistrySettings();
            break;
        case ID_TRAY_PERAPP:
            g_smartPerAppMode = !g_smartPerAppMode;
            SyncControlsFromState();
            SaveRegistrySettings();
            break;
        case ID_TRAY_MACROS:
            g_engine.config().enable_macros = !g_engine.config().enable_macros;
            g_engine.config().quick_telex = g_engine.config().enable_macros;
            SyncControlsFromState();
            SaveRegistrySettings();
            break;
        case ID_TRAY_EXIT:
            Shell_NotifyIconW(NIM_DELETE, &g_nid);
            PostQuitMessage(0);
            break;
        case ID_BTN_ADD_MACRO: {
            wchar_t kbuf[64] = {0}, vbuf[256] = {0};
            GetWindowTextW(g_hEdtKey, kbuf, 64);
            GetWindowTextW(g_hEdtVal, vbuf, 256);
            std::string k = wide_to_utf8(kbuf), v = wide_to_utf8(vbuf);
            if (!k.empty() && !v.empty()) {
                for (char& c : k) if (c >= 'A' && c <= 'Z') c += 32;
                g_engine.config().macros[k] = v;
                SetWindowTextW(g_hEdtKey, L"");
                SetWindowTextW(g_hEdtVal, L"");
                RefreshMacroListUI();
            }
            break;
        }
        case ID_BTN_DEL_MACRO: {
            int sel = (int)SendMessageW(g_hListMacros, LB_GETCURSEL, 0, 0);
            if (sel != LB_ERR) {
                wchar_t buf[320] = {0};
                SendMessageW(g_hListMacros, LB_GETTEXT, sel, (LPARAM)buf);
                std::string s = wide_to_utf8(buf);
                size_t p = s.find("  =>  ");
                if (p != std::string::npos) {
                    g_engine.config().macros.erase(s.substr(0, p));
                    RefreshMacroListUI();
                }
            }
            break;
        }
        case ID_BTN_SAVE: {
            int selMethod = (int)SendMessageW(g_hCmbMethod, CB_GETCURSEL, 0, 0);
            g_engine.config().input_method = (selMethod == 1) ? "vni" : "telex";
            g_engine.config().auto_restore_english = (SendMessageW(g_hChkRestore, BM_GETCHECK, 0, 0) == BST_CHECKED);
            g_engine.config().fix_autocomplete     = (SendMessageW(g_hChkAcFix,   BM_GETCHECK, 0, 0) == BST_CHECKED);
            g_engine.config().modern_orthography   = (SendMessageW(g_hChkOrtho,   BM_GETCHECK, 0, 0) == BST_CHECKED);
            g_smartPerAppMode                      = (SendMessageW(g_hChkPerApp,  BM_GETCHECK, 0, 0) == BST_CHECKED);
            g_engine.config().enable_macros        = (SendMessageW(g_hChkMacro,   BM_GETCHECK, 0, 0) == BST_CHECKED);
            g_engine.config().quick_telex          = g_engine.config().enable_macros;
            g_launchAtStartup                      = (SendMessageW(g_hChkStartup, BM_GETCHECK, 0, 0) == BST_CHECKED);
            SaveRegistrySettings();
            ShowWindow(hWnd, SW_HIDE);
            break;
        }
        }
        return 0;
    }
    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP) {
            ShowTrayMenu(hWnd);
        } else if (lParam == WM_LBUTTONDBLCLK) {
            SyncControlsFromState();
            ShowWindow(hWnd, SW_SHOW);
            SetForegroundWindow(hWnd);
        } else if (lParam == WM_LBUTTONUP) {
            g_vietnameseEnabled = !g_vietnameseEnabled;
            g_engine.reset();
            UpdateTrayTooltip();
            SaveRegistrySettings();
        }
        return 0;
    case WM_CLOSE:
        ShowWindow(hWnd, SW_HIDE);
        return 0;
    case WM_DESTROY:
        Shell_NotifyIconW(NIM_DELETE, &g_nid);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int) {
    g_hInst = hInstance;
    LoadRegistrySettings();

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"VisKeyWin32Class";
    RegisterClassExW(&wc);

    g_hMainWnd = CreateWindowExW(
        0, L"VisKeyWin32Class", L"VisKey — Bảng Điều Khiển Bộ Gõ Tiếng Việt (Windows)",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 495, 490,
        nullptr, nullptr, hInstance, nullptr
    );

    g_nid.cbSize = sizeof(NOTIFYICONDATAW);
    g_nid.hWnd = g_hMainWnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wcscpy_s(g_nid.szTip, L"VisKey - Tiếng Việt (VI) [Ctrl+Space]");
    Shell_NotifyIconW(NIM_ADD, &g_nid);
    UpdateTrayTooltip();

    g_hKeyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandleW(nullptr), 0);
    g_hMouseHook    = SetWindowsHookExW(WH_MOUSE_LL,    LowLevelMouseProc,    GetModuleHandleW(nullptr), 0);

    // Show control panel on launch so user knows VisKey is active
    ShowWindow(g_hMainWnd, SW_SHOW);
    UpdateWindow(g_hMainWnd);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (g_hKeyboardHook) UnhookWindowsHookEx(g_hKeyboardHook);
    if (g_hMouseHook)    UnhookWindowsHookEx(g_hMouseHook);
    return 0;
}
