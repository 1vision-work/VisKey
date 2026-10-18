// SPDX-License-Identifier: GPL-3.0-or-later
//
// VisKey for Windows (by 1VISION) — based on OpenKey by Mai Vũ Tuyên.
//
// Entry point: single instance, settings file, tray icon + menu, typing service.

#include "app.h"

#include <commctrl.h>
#include <shellapi.h>
#include <windowsx.h>

#include "dialogs.h"
#include "typing_service.h"
#include "viskey_engine.h"
#include "win_util.h"
#include "../res/resource.h"

namespace viskey {
namespace app {

namespace {

constexpr wchar_t kWindowClass[] = L"VisKey.TrayWindow";
constexpr wchar_t kMutexName[] = L"Local\\1VISION.VisKey.SingleInstance";
constexpr UINT WM_APP_TRAY = WM_APP + 10;
constexpr UINT WM_APP_SHOW_SETTINGS = WM_APP + 11;  // posted by a second launch
constexpr UINT kTrayId = 1;

HINSTANCE g_instance = nullptr;
HWND g_window = nullptr;
Config g_config;
std::wstring g_configPath;
TypingService g_typing;
bool g_hookOk = false;
UINT g_taskbarCreated = 0;
HICON g_icons[3] = {};  // VI, EN, OFF

// ---- settings file ------------------------------------------------------------------------------

void LoadConfig() {
    const std::wstring exeDir = DirName(ExePath());
    g_configPath = ChooseConfigPath(exeDir, IsDirWritable(exeDir), AppDataDir());
    std::string text;
    if (!ReadFileUtf8(g_configPath, text)) return;  // first run: defaults, written on the first change
    if (!ConfigFromJson(text, g_config)) {
        // Keep the broken file for the user instead of silently overwriting it.
        MoveFileExW(g_configPath.c_str(), (g_configPath + L".bad").c_str(), MOVEFILE_REPLACE_EXISTING);
        g_config = Config();
    }
}

void SaveConfig() { WriteFileAtomic(g_configPath, ConfigToJson(g_config)); }

// ---- tray ---------------------------------------------------------------------------------------

void LoadTrayIcons() {
    const int ids[3] = {IDI_TRAY_VI, IDI_TRAY_EN, IDI_TRAY_OFF};
    for (int i = 0; i < 3; i++) {
        if (g_icons[i]) DestroyIcon(g_icons[i]);
        g_icons[i] = nullptr;
        LoadIconMetric(g_instance, MAKEINTRESOURCEW(ids[i]), LIM_SMALL, &g_icons[i]);
    }
}

const wchar_t* InputTypeLabel(int t) {
    switch (t) {
    case VK_INPUT_VNI: return L"VNI";
    case VK_INPUT_SIMPLE_TELEX_1: case VK_INPUT_SIMPLE_TELEX_2: return L"Simple Telex";
    default: return L"Telex";
    }
}

const wchar_t* CodeTableLabel(int t) {
    switch (t) {
    case VK_TABLE_TCVN3: return L"TCVN3 (ABC)";
    case VK_TABLE_VNI_WINDOWS: return L"VNI Windows";
    case VK_TABLE_UNICODE_COMPOUND: return L"Unicode tổ hợp";
    case VK_TABLE_CP1258: return L"CP1258";
    default: return L"Unicode";
    }
}

void FillTrayData(NOTIFYICONDATAW& nid) {
    nid.cbSize = sizeof nid;
    nid.hWnd = g_window;
    nid.uID = kTrayId;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_SHOWTIP;
    nid.uCallbackMessage = WM_APP_TRAY;
    nid.hIcon = !g_hookOk ? g_icons[2] : g_config.vietnamese ? g_icons[0] : g_icons[1];
    std::wstring tip;
    if (!g_hookOk)
        tip = L"VisKey — chưa bắt được bàn phím";
    else if (g_config.vietnamese)
        tip = std::wstring(L"VisKey — Tiếng Việt (") + InputTypeLabel(g_config.inputType) + L", " +
              CodeTableLabel(g_config.codeTable) + L")";
    else
        tip = L"VisKey — English";
    wcsncpy_s(nid.szTip, tip.c_str(), _TRUNCATE);
}

void AddTrayIcon() {
    NOTIFYICONDATAW nid{};
    FillTrayData(nid);
    Shell_NotifyIconW(NIM_ADD, &nid);
    nid.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIconW(NIM_SETVERSION, &nid);
}

void UpdateTrayIcon() {
    NOTIFYICONDATAW nid{};
    FillTrayData(nid);
    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

void RemoveTrayIcon() {
    NOTIFYICONDATAW nid{};
    nid.cbSize = sizeof nid;
    nid.hWnd = g_window;
    nid.uID = kTrayId;
    Shell_NotifyIconW(NIM_DELETE, &nid);
}

void ShowBalloon(const wchar_t* title, const wchar_t* text, DWORD icon) {
    NOTIFYICONDATAW nid{};
    FillTrayData(nid);
    nid.uFlags = NIF_INFO;
    nid.dwInfoFlags = icon;
    wcsncpy_s(nid.szInfoTitle, title, _TRUNCATE);
    wcsncpy_s(nid.szInfo, text, _TRUNCATE);
    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

// ---- menu ---------------------------------------------------------------------------------------

void AddItem(HMENU m, UINT id, const wchar_t* text, bool checked = false) {
    AppendMenuW(m, MF_STRING | (checked ? MF_CHECKED : MF_UNCHECKED), id, text);
}

void AddRadio(HMENU m, UINT id, const wchar_t* text, bool checked) {
    MENUITEMINFOW mi{};
    mi.cbSize = sizeof mi;
    mi.fMask = MIIM_FTYPE | MIIM_STATE | MIIM_ID | MIIM_STRING;
    mi.fType = MFT_STRING | MFT_RADIOCHECK;
    mi.fState = checked ? MFS_CHECKED : MFS_UNCHECKED;
    mi.wID = id;
    mi.dwTypeData = const_cast<wchar_t*>(text);
    InsertMenuItemW(m, GetMenuItemCount(m), TRUE, &mi);
}

void ShowTrayMenu(int x, int y) {
    const Config& c = g_config;
    HMENU menu = CreatePopupMenu();
    AddItem(menu, IDM_TOGGLE_VIETNAMESE,
            c.hotkey == ToggleHotkey::AltZ ? L"Tiếng Việt\tAlt+Z" : L"Tiếng Việt\tCtrl+Shift", c.vietnamese);
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);

    HMENU input = CreatePopupMenu();
    AddRadio(input, IDM_INPUT_TELEX, L"Telex", c.inputType == VK_INPUT_TELEX);
    AddRadio(input, IDM_INPUT_VNI, L"VNI", c.inputType == VK_INPUT_VNI);
    AddRadio(input, IDM_INPUT_SIMPLE_TELEX, L"Simple Telex",
             c.inputType == VK_INPUT_SIMPLE_TELEX_1 || c.inputType == VK_INPUT_SIMPLE_TELEX_2);
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(input), L"Kiểu gõ");

    HMENU table = CreatePopupMenu();
    AddRadio(table, IDM_TABLE_UNICODE, L"Unicode", c.codeTable == VK_TABLE_UNICODE);
    AddRadio(table, IDM_TABLE_UNICODE_COMPOUND, L"Unicode tổ hợp", c.codeTable == VK_TABLE_UNICODE_COMPOUND);
    AddRadio(table, IDM_TABLE_TCVN3, L"TCVN3 (ABC)", c.codeTable == VK_TABLE_TCVN3);
    AddRadio(table, IDM_TABLE_VNI_WINDOWS, L"VNI Windows", c.codeTable == VK_TABLE_VNI_WINDOWS);
    AddRadio(table, IDM_TABLE_CP1258, L"CP1258", c.codeTable == VK_TABLE_CP1258);
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(table), L"Bảng mã");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);

    AddItem(menu, IDM_CHECK_SPELLING, L"Kiểm tra chính tả", c.checkSpelling);
    AddItem(menu, IDM_MODERN_ORTHO, L"Đặt dấu kiểu mới (oà, uý)", c.modernOrthography);
    AddItem(menu, IDM_QUICK_TELEX, L"Quick Telex", c.quickTelex);
    AddItem(menu, IDM_RESTORE_WRONG, L"Khôi phục từ sai", c.restoreIfWrongSpelling);
    AddItem(menu, IDM_USE_MACRO, L"Gõ tắt", c.useMacro);
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AddItem(menu, IDM_RUN_AT_STARTUP, L"Khởi động cùng Windows", IsRunAtStartupEnabled());
    AddItem(menu, IDM_SETTINGS, L"Cài đặt…");
    AddItem(menu, IDM_ABOUT, L"Giới thiệu…");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AddItem(menu, IDM_EXIT, L"Thoát");
    SetMenuDefaultItem(menu, IDM_SETTINGS, FALSE);

    // Required so the menu closes when the user clicks elsewhere (KB135788).
    SetForegroundWindow(g_window);
    const UINT align = GetSystemMetrics(SM_MENUDROPALIGNMENT) ? TPM_RIGHTALIGN : TPM_LEFTALIGN;
    TrackPopupMenuEx(menu, align | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON, x, y, g_window, nullptr);
    PostMessageW(g_window, WM_NULL, 0, 0);
    DestroyMenu(menu);  // destroys the submenus too
}

void OnCommand(UINT id) {
    Config c = g_config;
    switch (id) {
    case IDM_TOGGLE_VIETNAMESE: c.vietnamese = !c.vietnamese; break;
    case IDM_INPUT_TELEX: c.inputType = VK_INPUT_TELEX; break;
    case IDM_INPUT_VNI: c.inputType = VK_INPUT_VNI; break;
    case IDM_INPUT_SIMPLE_TELEX: c.inputType = VK_INPUT_SIMPLE_TELEX_1; break;
    case IDM_TABLE_UNICODE: c.codeTable = VK_TABLE_UNICODE; break;
    case IDM_TABLE_UNICODE_COMPOUND: c.codeTable = VK_TABLE_UNICODE_COMPOUND; break;
    case IDM_TABLE_TCVN3: c.codeTable = VK_TABLE_TCVN3; break;
    case IDM_TABLE_VNI_WINDOWS: c.codeTable = VK_TABLE_VNI_WINDOWS; break;
    case IDM_TABLE_CP1258: c.codeTable = VK_TABLE_CP1258; break;
    case IDM_CHECK_SPELLING: c.checkSpelling = !c.checkSpelling; break;
    case IDM_MODERN_ORTHO: c.modernOrthography = !c.modernOrthography; break;
    case IDM_QUICK_TELEX: c.quickTelex = !c.quickTelex; break;
    case IDM_RESTORE_WRONG: c.restoreIfWrongSpelling = !c.restoreIfWrongSpelling; break;
    case IDM_USE_MACRO: c.useMacro = !c.useMacro; break;
    case IDM_RUN_AT_STARTUP:
        if (!SetRunAtStartup(!IsRunAtStartupEnabled()))
            ShowBalloon(L"VisKey", L"Không ghi được cài đặt khởi động cùng Windows.", NIIF_WARNING);
        return;
    case IDM_SETTINGS: ShowSettingsDialog(); return;
    case IDM_ABOUT: ShowAboutDialog(); return;
    case IDM_EXIT: DestroyWindow(g_window); return;
    default: return;
    }
    Commit(c);
    RefreshSettingsDialog();
}

// ---- window -------------------------------------------------------------------------------------

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_APP_TRAY:
        switch (LOWORD(lParam)) {
        case NIN_SELECT:  // left click
            OnCommand(IDM_TOGGLE_VIETNAMESE);
            return 0;
        case WM_CONTEXTMENU:
            ShowTrayMenu(GET_X_LPARAM(wParam), GET_Y_LPARAM(wParam));
            return 0;
        }
        return 0;
    case WM_APP_SHOW_SETTINGS:
        ShowSettingsDialog();
        return 0;
    case WM_APP_LANGUAGE_CHANGED:  // the hook thread toggled the language (hotkey); it already applied it
        g_config.vietnamese = wParam != 0;
        SaveConfig();
        UpdateTrayIcon();
        RefreshSettingsDialog();
        return 0;
    case WM_APP_HOOK_STATUS:
        g_hookOk = wParam != 0;
        UpdateTrayIcon();
        if (!g_hookOk)
            ShowBalloon(L"VisKey", L"Không cài được bộ bắt phím (keyboard hook). VisKey sẽ không gõ được tiếng Việt.",
                        NIIF_ERROR);
        return 0;
    case WM_COMMAND:
        OnCommand(LOWORD(wParam));
        return 0;
    case WM_DISPLAYCHANGE:
    case WM_SETTINGCHANGE:
        LoadTrayIcons();
        UpdateTrayIcon();
        break;
    case WM_DESTROY:
        RemoveTrayIcon();
        g_typing.Stop();
        PostQuitMessage(0);
        return 0;
    default:
        if (msg == g_taskbarCreated && msg != 0) {  // Explorer restarted
            AddTrayIcon();
            return 0;
        }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// Asks the running instance to show its settings. Returns true if one was found.
bool ActivateRunningInstance() {
    HWND other = FindWindowW(kWindowClass, nullptr);
    if (!other) return false;
    DWORD pid = 0;
    GetWindowThreadProcessId(other, &pid);
    AllowSetForegroundWindow(pid);
    PostMessageW(other, WM_APP_SHOW_SETTINGS, 0, 0);
    return true;
}

}  // namespace

HINSTANCE Instance() { return g_instance; }
const Config& CurrentConfig() { return g_config; }
const std::wstring& ConfigPath() { return g_configPath; }

void Commit(const Config& c) {
    g_config = c;
    SaveConfig();
    g_typing.Apply(g_config);
    UpdateTrayIcon();
}

int Run(HINSTANCE instance) {
    g_instance = instance;

    HANDLE mutex = CreateMutexW(nullptr, TRUE, kMutexName);
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        // A second launch opens the settings of the running VisKey (it may still be starting up).
        for (int i = 0; i < 20 && !ActivateRunningInstance(); i++) Sleep(100);
        if (mutex) CloseHandle(mutex);
        return 0;
    }

    INITCOMMONCONTROLSEX icc{sizeof icc, ICC_STANDARD_CLASSES | ICC_LISTVIEW_CLASSES | ICC_LINK_CLASS};
    InitCommonControlsEx(&icc);

    LoadConfig();
    // A portable VisKey.exe may have moved since "run at startup" was enabled: keep the path current.
    if (IsRunAtStartupEnabled() && RunAtStartupCommand() != L"\"" + ExePath() + L"\"") SetRunAtStartup(true);

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof wc;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = instance;
    wc.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_APP));
    wc.lpszClassName = kWindowClass;
    RegisterClassExW(&wc);
    // A hidden top-level window (not message-only) so a second instance can find it and Explorer can
    // broadcast "TaskbarCreated" to it.
    g_window = CreateWindowExW(WS_EX_TOOLWINDOW, kWindowClass, L"VisKey", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr,
                               instance, nullptr);
    if (!g_window) return 1;
    g_taskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");
    ChangeWindowMessageFilterEx(g_window, g_taskbarCreated, MSGFLT_ALLOW, nullptr);
    ChangeWindowMessageFilterEx(g_window, WM_APP_SHOW_SETTINGS, MSGFLT_ALLOW, nullptr);

    LoadTrayIcons();
    g_hookOk = true;  // optimistic until WM_APP_HOOK_STATUS arrives
    AddTrayIcon();
    g_typing.Start(g_window, g_config);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (HandleDialogMessage(&msg)) continue;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    for (HICON& icon : g_icons)
        if (icon) DestroyIcon(icon);
    if (mutex) {
        ReleaseMutex(mutex);
        CloseHandle(mutex);
    }
    return int(msg.wParam);
}

}  // namespace app
}  // namespace viskey

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) { return viskey::app::Run(instance); }
