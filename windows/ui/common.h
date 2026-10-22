// VisKey for Windows - shared declarations for the native UI (MIT License, 1VISION).
// Everything here is written from scratch for VisKey.
#pragma once

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace vk {

// ---------------------------------------------------------------------------------------------
// Window messages shared between the tray host, the settings window and the flyout.
constexpr UINT WM_VK_TRAYICON = WM_USER + 100;
constexpr UINT WM_VK_REFRESH = WM_APP + 1;          // model changed: repaint
constexpr UINT WM_VK_HOTKEY_RECORDED = WM_APP + 2;  // wParam = vk, lParam = modifier bits
constexpr UINT WM_VK_POPUP = WM_APP + 3;            // wParam = tag, lParam = chosen index (-1 = none)
constexpr UINT WM_VK_UPDATE = WM_APP + 4;           // wParam = 1 if automatic, lParam = UpdateResult* (receiver deletes)
constexpr UINT WM_VK_FIELD = WM_APP + 5;            // wParam = field id, lParam = FieldKey

enum FieldKey { kFieldTab = 1, kFieldShiftTab, kFieldEnter, kFieldEscape };

// Hotkey modifier bits (also the registry encoding: vk | bits).
constexpr DWORD kModCtrl = 1u << 16;
constexpr DWORD kModAlt = 1u << 17;
constexpr DWORD kModShift = 1u << 18;
constexpr DWORD kModWin = 1u << 19;

struct Hotkey {
    UINT vk = 0;
    bool ctrl = false, alt = false, shift = false, win = false;
    bool Empty() const { return vk == 0; }
    DWORD Pack() const {
        if (!vk) return 0;
        return (vk & 0xFFFFu) | (ctrl ? kModCtrl : 0) | (alt ? kModAlt : 0) | (shift ? kModShift : 0) | (win ? kModWin : 0);
    }
    static Hotkey Unpack(DWORD v) {
        Hotkey h;
        h.vk = v & 0xFFFFu;
        h.ctrl = (v & kModCtrl) != 0;
        h.alt = (v & kModAlt) != 0;
        h.shift = (v & kModShift) != 0;
        h.win = (v & kModWin) != 0;
        return h;
    }
    bool operator==(const Hotkey& o) const { return Pack() == o.Pack(); }
    bool operator!=(const Hotkey& o) const { return !(*this == o); }
};

enum HotkeySlot { kHotkeyPrimary = 0, kHotkeySecondary = 1, kHotkeyMethod = 2, kHotkeyCount = 3 };

enum class RuleMode { VI = 0, EN = 1, Remember = 2 };
enum class RuleSource { Default = 0, User = 1, Auto = 2 };
enum class NewAppPolicy { RememberLast = 0, AlwaysVI = 1, AlwaysEN = 2 };

struct AppRule {
    std::wstring exe;      // lower-case file name, e.g. "cmd.exe" (registry value name)
    std::wstring display;  // original spelling for display, e.g. "WindowsTerminal.exe"
    RuleMode mode = RuleMode::Remember;
    RuleSource source = RuleSource::Auto;
    bool lastVi = true;
    const std::wstring& Name() const { return display.empty() ? exe : display; }
};

struct UpdateResult {
    bool ok = false;
    bool newer = false;
    std::wstring latest;  // "1.2.0"
    std::wstring tag;     // "v1.2.0"
    std::wstring error;
};

// The whole user-visible state. The keyboard pipeline copies the typing options into the engine
// (see ApplyModelToEngine in VisKeyWin.cpp); the UI only reads and mutates this struct and then
// calls Commit().
struct Model {
    bool vietnamese = true;
    bool vni = false;
    bool modernOrthography = false;
    bool autoRestore = true;
    bool fixAutocomplete = true;
    bool smartPerApp = true;
    bool macrosEnabled = true;
    bool quickTelex = false;
    bool launchAtStartup = false;
    NewAppPolicy policy = NewAppPolicy::RememberLast;
    Hotkey hotkeys[kHotkeyCount];
    std::map<std::string, std::string> macros;   // lower-case ASCII key -> UTF-8 text
    std::map<std::string, uint32_t> usage;       // key -> times expanded
    std::vector<AppRule> rules;
    bool snapshot = false;                       // --ui-snapshot: never touch registry/hooks
    bool hooksActive = false;

    // Update check. Persisted: autoUpdateCheck, lastUpdateCheck, lastNotifiedVersion.
    bool autoUpdateCheck = true;
    int64_t lastUpdateCheck = 0;       // Unix seconds of the last attempt (manual or automatic), 0 = never
    std::wstring lastNotifiedVersion;  // newest version already announced (tray balloon / seen on the page)
    bool updateChecking = false;       // session state below
    bool haveUpdateResult = false;
    bool updateResultAuto = false;     // the last result came from the automatic check
    UpdateResult update;
};

enum ChangeFlags : unsigned {
    kChangeSettings = 1u << 0,
    kChangeMacros = 1u << 1,
    kChangeUsage = 1u << 2,
    kChangeRules = 1u << 3,
    kChangeHotkeys = 1u << 4,
    kChangeUpdateState = 1u << 5,  // LastUpdateCheck / LastNotifiedVersion
    kChangeAll = 0xFFu,
};

// ---- Implemented in VisKeyWin.cpp (application core) ----------------------------------------
Model& M();
void Commit(unsigned flags);              // apply to engine, persist, refresh tray + windows
void SetVietnamese(bool on);              // toggle + per-app memory + persist
void OpenSettings(int page);              // page < 0 keeps the current page
void QuitApp();
void BeginHotkeyRecord(HWND target);      // hook forwards the next key combo to target
void EndHotkeyRecord();
std::map<std::string, std::string> DefaultMacros();
std::vector<AppRule> DefaultRules();
void ResetModelToDefaults(Model& m);      // in-memory defaults (no registry)
void RequestManualUpdateCheck();          // "Kiểm tra cập nhật" button; result arrives as WM_VK_UPDATE on the tray host

// ---- store.cpp: persistence, import/export, version, system info, update check ----------------
void LoadModel(Model& m);
void SaveSettings(const Model& m);
void SaveMacros(const Model& m);
void SaveUsage(const Model& m);
void SaveRules(const Model& m);
void SaveUpdateState(const Model& m);
void DeleteAllStoredSettings();
void FillDemoModel(Model& m);

bool IsValidMacroKey(const std::string& key);
bool ParseMacroFile(const std::string& utf8, std::map<std::string, std::string>& out, int& skipped);
std::string ExportMacrosJson(const std::map<std::string, std::string>& macros);

std::wstring AppVersion();          // "1.1.0-beta.5": the build version (VISKEY_VERSION in CMake)
std::wstring SystemInfoText(const Model& m);
std::wstring WindowsVersionText();  // "Windows 10.0.22631"
const wchar_t* ArchText();          // "x64" / "ARM64"

// One anonymous GET of the public GitHub release list on a worker thread, shared by the manual
// button and the automatic check; posts WM_VK_UPDATE(automatic, UpdateResult*) to notify.
void StartUpdateCheck(HWND notify, bool automatic);
int64_t UnixNow();
std::wstring FormatLocalTime(int64_t unixSeconds);  // "19/10/2026 14:05"
// Newest version known to be newer than the running build ("" if none): this session's result,
// else the last announced version.
std::wstring AvailableUpdateVersion(const Model& m);

// ---- small utilities (store.cpp) ------------------------------------------------------------
std::wstring Utf8ToWide(const std::string& s);
std::string WideToUtf8(const std::wstring& s);
std::wstring FoldForSearch(const std::wstring& s);  // lower-case, no diacritics, đ -> d
std::wstring FormatCount(uint32_t n);                // 1038 -> "1.038"
std::wstring LowerW(std::wstring s);
std::vector<std::wstring> HotkeyParts(const Hotkey& h);  // {"Ctrl", "Space"}
std::wstring HotkeyText(const Hotkey& h);               // "Ctrl + Space"
bool IsConsoleExe(const std::wstring& exeLower);
void OpenUrl(const wchar_t* url);
bool CopyToClipboard(HWND owner, const std::wstring& text);

// ---- file dialogs (store.cpp, IFileOpenDialog / IFileSaveDialog through ole32) -------------
bool PickOpenFile(HWND owner, const wchar_t* title, const wchar_t* filterName, const wchar_t* filterSpec, std::wstring& path);
bool PickSaveFile(HWND owner, const wchar_t* title, const wchar_t* filterName, const wchar_t* filterSpec,
                  const wchar_t* defaultName, const wchar_t* defaultExt, std::wstring& path);
bool ReadFileBytes(const std::wstring& path, std::string& out);
bool WriteFileBytes(const std::wstring& path, const std::string& data);

// ---- tray flyout / settings window (flyout.cpp, settings.cpp) ------------------------------
void ShowTrayFlyout(HWND trayHost, UINT trayId);
void HideTrayFlyout();
bool TrayFlyoutRecentlyClosed();
void RefreshFlyout();
HWND SettingsWindow();
void CreateSettingsWindow(HINSTANCE inst);
void ShowSettingsWindow(int page);
void RefreshSettingsWindow();

// ---- snapshot.cpp ----------------------------------------------------------------------------
int RunUiSnapshot(const std::wstring& outDir);

// ---- DPI / theme helpers (gfx.cpp) ----------------------------------------------------------
void EnablePerMonitorDpi();
UINT DpiForWindow(HWND hwnd);
bool SystemPrefersDark();
void ApplyWindowFrameTheme(HWND hwnd, bool dark, bool popup);

}  // namespace vk
