// VisKey for Windows - settings persistence (HKCU\Software\VisKey), macro import/export,
// version / system information and the update check (MIT License, 1VISION).
#include "common.h"
#include "update_policy.h"

#include <objbase.h>
#include <shellapi.h>
#include <shobjidl.h>
#include <winhttp.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cwchar>
#include <cwctype>
#include <set>

namespace vk {

namespace {

const wchar_t* kRoot = L"Software\\VisKey";
const wchar_t* kMacrosKey = L"Software\\VisKey\\Macros";
const wchar_t* kUsageKey = L"Software\\VisKey\\MacroUsage";
const wchar_t* kRulesKey = L"Software\\VisKey\\AppRules";
const wchar_t* kRunKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";

const GUID kClsidFileOpenDialog = {0xdc1c5a9c, 0xe88a, 0x4dde, {0xa5, 0xa1, 0x60, 0xf8, 0x2a, 0x20, 0xae, 0xf7}};
const GUID kClsidFileSaveDialog = {0xc0b4e2f3, 0xba21, 0x4773, {0x8d, 0xba, 0x33, 0x5e, 0xc9, 0x46, 0xeb, 0x8b}};

template <class T>
T LoadProc(HMODULE mod, const char* name) {
    FARPROC p = mod ? GetProcAddress(mod, name) : nullptr;
    T f = nullptr;
    std::memcpy(&f, &p, sizeof(f));
    return f;
}

bool ReadDword(HKEY key, const wchar_t* name, DWORD& out) {
    DWORD type = 0, v = 0, size = sizeof(v);
    if (RegQueryValueExW(key, name, nullptr, &type, reinterpret_cast<LPBYTE>(&v), &size) != ERROR_SUCCESS || type != REG_DWORD) return false;
    out = v;
    return true;
}

void WriteDword(HKEY key, const wchar_t* name, DWORD v) {
    RegSetValueExW(key, name, 0, REG_DWORD, reinterpret_cast<const BYTE*>(&v), sizeof(v));
}

void WriteString(HKEY key, const wchar_t* name, const std::wstring& v) {
    RegSetValueExW(key, name, 0, REG_SZ, reinterpret_cast<const BYTE*>(v.c_str()), (DWORD)((v.size() + 1) * sizeof(wchar_t)));
}

// Calls fn(name, type, data bytes) for each value of HKCU\<path>. Returns false if the key is absent.
template <class Fn>
bool EnumValues(const wchar_t* path, Fn fn) {
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, path, 0, KEY_READ, &key) != ERROR_SUCCESS) return false;
    std::vector<wchar_t> name(512);
    std::vector<BYTE> data(16384);
    for (DWORD i = 0;; ++i) {
        DWORD nameLen = (DWORD)name.size(), dataLen = (DWORD)data.size(), type = 0;
        LONG r = RegEnumValueW(key, i, name.data(), &nameLen, nullptr, &type, data.data(), &dataLen);
        if (r == ERROR_NO_MORE_ITEMS) break;
        if (r != ERROR_SUCCESS) continue;
        fn(std::wstring(name.data(), nameLen), type, data.data(), dataLen);
    }
    RegCloseKey(key);
    return true;
}

std::wstring RegString(const BYTE* data, DWORD len) {
    std::wstring s(reinterpret_cast<const wchar_t*>(data), len / sizeof(wchar_t));
    while (!s.empty() && s.back() == 0) s.pop_back();
    return s;
}

// Recreates HKCU\<path> empty and returns it open for writing.
HKEY RecreateKey(const wchar_t* path) {
    RegDeleteTreeW(HKEY_CURRENT_USER, path);
    HKEY key = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, path, 0, nullptr, 0, KEY_WRITE, nullptr, &key, nullptr) != ERROR_SUCCESS) return nullptr;
    return key;
}

const wchar_t* ModeName(RuleMode m) { return m == RuleMode::VI ? L"vi" : m == RuleMode::EN ? L"en" : L"remember"; }
const wchar_t* SourceName(RuleSource s) { return s == RuleSource::Default ? L"default" : s == RuleSource::User ? L"user" : L"auto"; }

std::vector<std::wstring> Split(const std::wstring& s, wchar_t sep) {
    std::vector<std::wstring> out;
    size_t start = 0;
    for (;;) {
        size_t p = s.find(sep, start);
        out.push_back(s.substr(start, p == std::wstring::npos ? std::wstring::npos : p - start));
        if (p == std::wstring::npos) break;
        start = p + 1;
    }
    return out;
}

}  // namespace

// ---------------------------------------------------------------------------------------------
// Utilities
std::wstring Utf8ToWide(const std::string& s) {
    if (s.empty()) return std::wstring();
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
    std::wstring out((size_t)std::max(n, 0), L'\0');
    if (n > 0) MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), &out[0], n);
    return out;
}

std::string WideToUtf8(const std::wstring& s) {
    if (s.empty()) return std::string();
    int n = WideCharToMultiByte(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0, nullptr, nullptr);
    std::string out((size_t)std::max(n, 0), '\0');
    if (n > 0) WideCharToMultiByte(CP_UTF8, 0, s.data(), (int)s.size(), &out[0], n, nullptr, nullptr);
    return out;
}

std::wstring LowerW(std::wstring s) {
    if (!s.empty()) CharLowerBuffW(&s[0], (DWORD)s.size());
    return s;
}

std::wstring FoldForSearch(const std::wstring& s) {
    if (s.empty()) return s;
    std::wstring decomposed;
    int n = FoldStringW(MAP_COMPOSITE, s.c_str(), (int)s.size(), nullptr, 0);
    if (n > 0) {
        decomposed.resize((size_t)n);
        FoldStringW(MAP_COMPOSITE, s.c_str(), (int)s.size(), &decomposed[0], n);
    } else {
        decomposed = s;
    }
    std::wstring out;
    out.reserve(decomposed.size());
    for (wchar_t c : decomposed) {
        if (c >= 0x0300 && c <= 0x036F) continue;  // combining marks
        if (c == 0x0111 || c == 0x0110) c = L'd';  // đ Đ
        out.push_back(c);
    }
    return LowerW(out);
}

std::wstring FormatCount(uint32_t n) {
    std::wstring digits = std::to_wstring(n), out;
    int count = 0;
    for (auto it = digits.rbegin(); it != digits.rend(); ++it) {
        if (count && count % 3 == 0) out.insert(out.begin(), L'.');
        out.insert(out.begin(), *it);
        ++count;
    }
    return out;
}

static std::wstring KeyName(UINT vk) {
    if ((vk >= 'A' && vk <= 'Z') || (vk >= '0' && vk <= '9')) return std::wstring(1, (wchar_t)vk);
    if (vk >= VK_F1 && vk <= VK_F24) return L"F" + std::to_wstring(vk - VK_F1 + 1);
    if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9) return L"Num " + std::to_wstring(vk - VK_NUMPAD0);
    switch (vk) {
    case VK_SPACE: return L"Space";
    case VK_TAB: return L"Tab";
    case VK_RETURN: return L"Enter";
    case VK_ESCAPE: return L"Esc";
    case VK_BACK: return L"Backspace";
    case VK_DELETE: return L"Delete";
    case VK_INSERT: return L"Insert";
    case VK_HOME: return L"Home";
    case VK_END: return L"End";
    case VK_PRIOR: return L"PgUp";
    case VK_NEXT: return L"PgDn";
    case VK_LEFT: return L"←";
    case VK_RIGHT: return L"→";
    case VK_UP: return L"↑";
    case VK_DOWN: return L"↓";
    case VK_CAPITAL: return L"Caps Lock";
    case VK_PAUSE: return L"Pause";
    case VK_SCROLL: return L"Scroll Lock";
    case VK_SNAPSHOT: return L"PrtSc";
    case VK_APPS: return L"Menu";
    default: break;
    }
    UINT ch = MapVirtualKeyW(vk, MAPVK_VK_TO_CHAR) & 0x7FFF;
    if (ch > 32) return std::wstring(1, (wchar_t)ch);
    wchar_t buf[64] = {0};
    LONG scan = (LONG)MapVirtualKeyW(vk, MAPVK_VK_TO_VSC) << 16;
    if (GetKeyNameTextW(scan, buf, 64) > 0) return buf;
    wchar_t hex[16];
    swprintf(hex, 16, L"0x%02X", vk);
    return hex;
}

std::vector<std::wstring> HotkeyParts(const Hotkey& h) {
    std::vector<std::wstring> parts;
    if (h.Empty()) return parts;
    if (h.ctrl) parts.push_back(L"Ctrl");
    if (h.alt) parts.push_back(L"Alt");
    if (h.shift) parts.push_back(L"Shift");
    if (h.win) parts.push_back(L"Win");
    parts.push_back(KeyName(h.vk));
    return parts;
}

std::wstring HotkeyText(const Hotkey& h) {
    std::wstring out;
    for (const auto& p : HotkeyParts(h)) {
        if (!out.empty()) out += L" + ";
        out += p;
    }
    return out;
}

bool IsConsoleExe(const std::wstring& exe) {
    static const std::set<std::wstring> k = {L"cmd.exe", L"powershell.exe", L"pwsh.exe", L"windowsterminal.exe", L"mintty.exe",
                                             L"conhost.exe", L"wsl.exe", L"bash.exe", L"alacritty.exe", L"wezterm-gui.exe"};
    return k.count(exe) > 0;
}

void OpenUrl(const wchar_t* url) { ShellExecuteW(nullptr, L"open", url, nullptr, nullptr, SW_SHOWNORMAL); }

bool CopyToClipboard(HWND owner, const std::wstring& text) {
    if (!OpenClipboard(owner)) return false;
    EmptyClipboard();
    size_t bytes = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, bytes);
    bool ok = false;
    if (mem) {
        void* p = GlobalLock(mem);
        if (p) {
            std::memcpy(p, text.c_str(), bytes);
            GlobalUnlock(mem);
            ok = SetClipboardData(CF_UNICODETEXT, mem) != nullptr;
        }
        if (!ok) GlobalFree(mem);
    }
    CloseClipboard();
    return ok;
}

// ---------------------------------------------------------------------------------------------
// Registry persistence
void LoadModel(Model& m) {
    ResetModelToDefaults(m);
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRoot, 0, KEY_READ, &key) == ERROR_SUCCESS) {
        DWORD v = 0;
        if (ReadDword(key, L"VietnameseEnabled", v)) m.vietnamese = v != 0;
        if (ReadDword(key, L"InputMethodVNI", v)) m.vni = v != 0;
        if (ReadDword(key, L"ModernOrthography", v)) m.modernOrthography = v != 0;
        if (ReadDword(key, L"AutoRestore", v)) m.autoRestore = v != 0;
        if (ReadDword(key, L"FixAutocomplete", v)) m.fixAutocomplete = v != 0;
        if (ReadDword(key, L"SmartPerApp", v)) m.smartPerApp = v != 0;
        if (ReadDword(key, L"EnableMacros", v)) m.macrosEnabled = v != 0;
        // Quick Telex used to follow EnableMacros; keep that value until it has its own setting.
        m.quickTelex = m.macrosEnabled;
        if (ReadDword(key, L"QuickTelex", v)) m.quickTelex = v != 0;
        if (ReadDword(key, L"LaunchAtStartup", v)) m.launchAtStartup = v != 0;
        if (ReadDword(key, L"NewAppPolicy", v) && v <= 2) m.policy = (NewAppPolicy)v;
        if (ReadDword(key, L"HotkeyPrimary", v)) m.hotkeys[kHotkeyPrimary] = Hotkey::Unpack(v);
        if (ReadDword(key, L"HotkeySecondary", v)) m.hotkeys[kHotkeySecondary] = Hotkey::Unpack(v);
        if (ReadDword(key, L"HotkeySwitchMethod", v)) m.hotkeys[kHotkeyMethod] = Hotkey::Unpack(v);
        if (ReadDword(key, L"AutoUpdateCheck", v)) m.autoUpdateCheck = v != 0;
        ULONGLONG q = 0;
        DWORD type = 0, size = sizeof(q);
        if (RegQueryValueExW(key, L"LastUpdateCheck", nullptr, &type, reinterpret_cast<LPBYTE>(&q), &size) == ERROR_SUCCESS && type == REG_QWORD &&
            q < (1ull << 40)) {
            m.lastUpdateCheck = (int64_t)q;
        }
        wchar_t ver[64] = {0};
        size = sizeof(ver) - sizeof(wchar_t);
        if (RegQueryValueExW(key, L"LastNotifiedVersion", nullptr, &type, reinterpret_cast<LPBYTE>(ver), &size) == ERROR_SUCCESS && type == REG_SZ) {
            m.lastNotifiedVersion = ver;
        }
        RegCloseKey(key);
    }

    std::map<std::string, std::string> macros;
    bool haveMacros = EnumValues(kMacrosKey, [&](const std::wstring& name, DWORD type, const BYTE* data, DWORD len) {
        if (type != REG_SZ) return;
        std::string k = WideToUtf8(LowerW(name));
        std::string val = WideToUtf8(RegString(data, len));
        if (IsValidMacroKey(k) && !val.empty()) macros[k] = val;
    });
    if (haveMacros) {
        m.macros = macros;
    } else {
        SaveMacros(m);  // first run: seed the engine defaults
    }

    m.usage.clear();
    EnumValues(kUsageKey, [&](const std::wstring& name, DWORD type, const BYTE* data, DWORD len) {
        if (type != REG_DWORD || len != sizeof(DWORD)) return;
        DWORD v = 0;
        std::memcpy(&v, data, sizeof(v));
        std::string k = WideToUtf8(LowerW(name));
        if (v && m.macros.count(k)) m.usage[k] = v;
    });

    std::vector<AppRule> rules;
    bool haveRules = EnumValues(kRulesKey, [&](const std::wstring& name, DWORD type, const BYTE* data, DWORD len) {
        if (type != REG_SZ || name.empty()) return;
        auto f = Split(RegString(data, len), L';');
        AppRule r;
        r.exe = LowerW(name);
        r.mode = (f[0] == L"vi") ? RuleMode::VI : (f[0] == L"en") ? RuleMode::EN : RuleMode::Remember;
        r.source = RuleSource::Auto;
        if (f.size() > 1) r.source = (f[1] == L"default") ? RuleSource::Default : (f[1] == L"user") ? RuleSource::User : RuleSource::Auto;
        if (f.size() > 2) r.lastVi = (f[2] != L"en");
        if (f.size() > 3 && LowerW(f[3]) == r.exe) r.display = f[3];
        rules.push_back(r);
    });
    if (haveRules) {
        m.rules = rules;
    } else {
        SaveRules(m);
    }
}

void SaveSettings(const Model& m) {
    if (m.snapshot) return;
    HKEY key = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kRoot, 0, nullptr, 0, KEY_WRITE, nullptr, &key, nullptr) == ERROR_SUCCESS) {
        WriteDword(key, L"VietnameseEnabled", m.vietnamese ? 1 : 0);
        WriteDword(key, L"InputMethodVNI", m.vni ? 1 : 0);
        WriteDword(key, L"ModernOrthography", m.modernOrthography ? 1 : 0);
        WriteDword(key, L"AutoRestore", m.autoRestore ? 1 : 0);
        WriteDword(key, L"FixAutocomplete", m.fixAutocomplete ? 1 : 0);
        WriteDword(key, L"SmartPerApp", m.smartPerApp ? 1 : 0);
        WriteDword(key, L"EnableMacros", m.macrosEnabled ? 1 : 0);
        WriteDword(key, L"QuickTelex", m.quickTelex ? 1 : 0);
        WriteDword(key, L"LaunchAtStartup", m.launchAtStartup ? 1 : 0);
        WriteDword(key, L"NewAppPolicy", (DWORD)m.policy);
        WriteDword(key, L"HotkeyPrimary", m.hotkeys[kHotkeyPrimary].Pack());
        WriteDword(key, L"HotkeySecondary", m.hotkeys[kHotkeySecondary].Pack());
        WriteDword(key, L"HotkeySwitchMethod", m.hotkeys[kHotkeyMethod].Pack());
        WriteDword(key, L"AutoUpdateCheck", m.autoUpdateCheck ? 1 : 0);
        RegCloseKey(key);
    }

    HKEY run = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunKey, 0, KEY_WRITE, &run) == ERROR_SUCCESS) {
        if (m.launchAtStartup) {
            wchar_t exePath[MAX_PATH] = {0};
            GetModuleFileNameW(nullptr, exePath, MAX_PATH);
            RegSetValueExW(run, L"VisKey", 0, REG_SZ, reinterpret_cast<const BYTE*>(exePath), (DWORD)((wcslen(exePath) + 1) * sizeof(wchar_t)));
        } else {
            RegDeleteValueW(run, L"VisKey");
        }
        RegCloseKey(run);
    }
}

void SaveMacros(const Model& m) {
    if (m.snapshot) return;
    HKEY key = RecreateKey(kMacrosKey);
    if (!key) return;
    for (const auto& kv : m.macros) WriteString(key, Utf8ToWide(kv.first).c_str(), Utf8ToWide(kv.second));
    RegCloseKey(key);
}

void SaveUsage(const Model& m) {
    if (m.snapshot) return;
    HKEY key = RecreateKey(kUsageKey);
    if (!key) return;
    for (const auto& kv : m.usage) {
        if (kv.second && m.macros.count(kv.first)) WriteDword(key, Utf8ToWide(kv.first).c_str(), kv.second);
    }
    RegCloseKey(key);
}

void SaveRules(const Model& m) {
    if (m.snapshot) return;
    HKEY key = RecreateKey(kRulesKey);
    if (!key) return;
    for (const auto& r : m.rules) {
        std::wstring data = std::wstring(ModeName(r.mode)) + L";" + SourceName(r.source) + L";" + (r.lastVi ? L"vi" : L"en");
        if (!r.display.empty() && r.display != r.exe) data += L";" + r.display;
        WriteString(key, r.exe.c_str(), data);
    }
    RegCloseKey(key);
}

void SaveUpdateState(const Model& m) {
    if (m.snapshot) return;
    HKEY key = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kRoot, 0, nullptr, 0, KEY_WRITE, nullptr, &key, nullptr) != ERROR_SUCCESS) return;
    ULONGLONG q = (ULONGLONG)std::max<int64_t>(0, m.lastUpdateCheck);
    RegSetValueExW(key, L"LastUpdateCheck", 0, REG_QWORD, reinterpret_cast<const BYTE*>(&q), sizeof(q));
    if (m.lastNotifiedVersion.empty()) RegDeleteValueW(key, L"LastNotifiedVersion");
    else WriteString(key, L"LastNotifiedVersion", m.lastNotifiedVersion);
    RegCloseKey(key);
}

void DeleteAllStoredSettings() {
    if (M().snapshot) return;
    RegDeleteTreeW(HKEY_CURRENT_USER, kRoot);
}

void FillDemoModel(Model& m) {
    ResetModelToDefaults(m);
    m.snapshot = true;
    m.vietnamese = true;
    m.macrosEnabled = true;
    m.quickTelex = false;
    // 24 phrases, mirroring the design mock-ups.
    m.macros = {
        {"bt", "bình thường"}, {"dc", "được"}, {"ko", "không"}, {"mn", "mọi người"}, {"ntn", "như thế nào"},
        {"uh", "ừ"}, {"vn", "Việt Nam"}, {"cty", "công ty"}, {"tks", "cảm ơn nhiều"}, {"hn", "Hà Nội"},
        {"hcm", "Hồ Chí Minh"}, {"sg", "Sài Gòn"}, {"bn", "bao nhiêu"}, {"cx", "cũng"}, {"dt", "điện thoại"},
        {"vs", "với"}, {"j", "gì"}, {"r", "rồi"}, {"ng", "người"}, {"nc", "nước"},
        {"tl", "trả lời"}, {"kq", "kết quả"}, {"hs", "học sinh"}, {"lm", "làm"},
    };
    m.usage = {{"bt", 212}, {"dc", 1038}, {"ko", 2417}, {"mn", 386}, {"ntn", 154}, {"uh", 97}, {"vn", 640}, {"cty", 58}, {"j", 31}};
    m.rules.clear();
    auto add = [&](const wchar_t* display, RuleMode mode, RuleSource src, bool lastVi) {
        AppRule r;
        r.display = display;
        r.exe = LowerW(display);
        r.mode = mode;
        r.source = src;
        r.lastVi = lastVi;
        m.rules.push_back(r);
    };
    add(L"cmd.exe", RuleMode::EN, RuleSource::Default, false);
    add(L"powershell.exe", RuleMode::EN, RuleSource::Default, false);
    add(L"WindowsTerminal.exe", RuleMode::EN, RuleSource::Default, false);
    add(L"zalo.exe", RuleMode::VI, RuleSource::User, true);
    add(L"msedge.exe", RuleMode::Remember, RuleSource::Auto, true);
    add(L"Code.exe", RuleMode::Remember, RuleSource::Auto, false);
    // Update page: automatic check on, last checked this morning, already up to date.
    m.autoUpdateCheck = true;
    m.lastUpdateCheck = 1792403520;  // 2026-10-19 02:52 UTC (fixed so snapshots are reproducible)
    m.lastNotifiedVersion.clear();
    m.haveUpdateResult = true;
    m.updateResultAuto = true;
    m.update = UpdateResult();
    m.update.ok = true;
    m.update.latest = AppVersion();
    m.update.tag = L"v" + AppVersion();
}

// ---------------------------------------------------------------------------------------------
// Macro import / export
bool IsValidMacroKey(const std::string& key) {
    if (key.empty() || key.size() > 32) return false;
    if (!(key[0] >= 'a' && key[0] <= 'z')) return false;
    for (char c : key) {
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))) return false;
    }
    return true;
}

static bool IsValidMacroValue(const std::string& v) {
    if (v.empty() || v.size() > 2000) return false;
    for (unsigned char c : v) {
        if (c < 0x20) return false;
    }
    return true;
}

namespace {

// Minimal JSON reader: enough to walk {"version":1,"macros":{"key":"value",...}}.
struct JsonReader {
    const std::string& s;
    size_t i = 0;
    explicit JsonReader(const std::string& str) : s(str) {}

    void Ws() {
        while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n')) ++i;
    }
    bool Eat(char c) {
        Ws();
        if (i < s.size() && s[i] == c) {
            ++i;
            return true;
        }
        return false;
    }
    static void PutUtf8(std::string& out, uint32_t cp) {
        if (cp < 0x80) {
            out.push_back((char)cp);
        } else if (cp < 0x800) {
            out.push_back((char)(0xC0 | (cp >> 6)));
            out.push_back((char)(0x80 | (cp & 0x3F)));
        } else if (cp < 0x10000) {
            out.push_back((char)(0xE0 | (cp >> 12)));
            out.push_back((char)(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back((char)(0x80 | (cp & 0x3F)));
        } else {
            out.push_back((char)(0xF0 | (cp >> 18)));
            out.push_back((char)(0x80 | ((cp >> 12) & 0x3F)));
            out.push_back((char)(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back((char)(0x80 | (cp & 0x3F)));
        }
    }
    bool Hex4(uint32_t& v) {
        if (i + 4 > s.size()) return false;
        v = 0;
        for (int k = 0; k < 4; ++k) {
            char c = s[i++];
            v <<= 4;
            if (c >= '0' && c <= '9') v |= (uint32_t)(c - '0');
            else if (c >= 'a' && c <= 'f') v |= (uint32_t)(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') v |= (uint32_t)(c - 'A' + 10);
            else return false;
        }
        return true;
    }
    bool String(std::string& out) {
        Ws();
        if (i >= s.size() || s[i] != '"') return false;
        ++i;
        out.clear();
        while (i < s.size()) {
            char c = s[i++];
            if (c == '"') return true;
            if (c != '\\') {
                out.push_back(c);
                continue;
            }
            if (i >= s.size()) return false;
            char e = s[i++];
            switch (e) {
            case '"': out.push_back('"'); break;
            case '\\': out.push_back('\\'); break;
            case '/': out.push_back('/'); break;
            case 'b': out.push_back('\b'); break;
            case 'f': out.push_back('\f'); break;
            case 'n': out.push_back('\n'); break;
            case 'r': out.push_back('\r'); break;
            case 't': out.push_back('\t'); break;
            case 'u': {
                uint32_t cp = 0;
                if (!Hex4(cp)) return false;
                if (cp >= 0xD800 && cp <= 0xDBFF && i + 6 <= s.size() && s[i] == '\\' && s[i + 1] == 'u') {
                    i += 2;
                    uint32_t lo = 0;
                    if (!Hex4(lo)) return false;
                    cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                }
                PutUtf8(out, cp);
                break;
            }
            default: return false;
            }
        }
        return false;
    }
    bool Skip(int depth = 0) {
        if (depth > 32) return false;
        Ws();
        if (i >= s.size()) return false;
        char c = s[i];
        if (c == '"') {
            std::string tmp;
            return String(tmp);
        }
        if (c == '{' || c == '[') {
            char close = (c == '{') ? '}' : ']';
            ++i;
            if (Eat(close)) return true;
            for (;;) {
                if (c == '{') {
                    std::string k;
                    if (!String(k) || !Eat(':')) return false;
                }
                if (!Skip(depth + 1)) return false;
                if (Eat(',')) continue;
                return Eat(close);
            }
        }
        size_t start = i;
        while (i < s.size() && (std::isalnum((unsigned char)s[i]) || s[i] == '-' || s[i] == '+' || s[i] == '.')) ++i;
        return i > start;
    }
};

}  // namespace

bool ParseMacroFile(const std::string& raw, std::map<std::string, std::string>& out, int& skipped) {
    skipped = 0;
    out.clear();
    std::string text = raw;
    if (text.size() >= 2 && (unsigned char)text[0] == 0xFF && (unsigned char)text[1] == 0xFE) {
        std::wstring w((text.size() - 2) / 2, L'\0');
        if (!w.empty()) std::memcpy(&w[0], text.data() + 2, w.size() * 2);
        text = WideToUtf8(w);
    } else if (text.size() >= 3 && (unsigned char)text[0] == 0xEF && (unsigned char)text[1] == 0xBB && (unsigned char)text[2] == 0xBF) {
        text = text.substr(3);
    }
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), (int)text.size(), nullptr, 0) == 0 && !text.empty()) return false;

    size_t first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return false;
    auto accept = [&](std::string k, const std::string& v) {
        for (char& c : k) {
            if (c >= 'A' && c <= 'Z') c = (char)(c + 32);
        }
        if (IsValidMacroKey(k) && IsValidMacroValue(v)) out[k] = v;
        else ++skipped;
    };

    if (text[first] == '{') {
        JsonReader j(text);
        if (!j.Eat('{')) return false;
        bool sawMacros = false;
        if (!j.Eat('}')) {
            for (;;) {
                std::string name;
                if (!j.String(name) || !j.Eat(':')) return false;
                if (name == "macros") {
                    sawMacros = true;
                    if (!j.Eat('{')) return false;
                    if (!j.Eat('}')) {
                        for (;;) {
                            std::string k, v;
                            if (!j.String(k) || !j.Eat(':')) return false;
                            j.Ws();
                            if (j.i < text.size() && text[j.i] == '"') {
                                if (!j.String(v)) return false;
                                accept(k, v);
                            } else {
                                if (!j.Skip()) return false;
                                ++skipped;
                            }
                            if (j.Eat(',')) continue;
                            if (!j.Eat('}')) return false;
                            break;
                        }
                    }
                } else if (!j.Skip()) {
                    return false;
                }
                if (j.Eat(',')) continue;
                if (!j.Eat('}')) return false;
                break;
            }
        }
        return sawMacros;
    }

    // Plain text: one "key<TAB>value" per line; blank lines and lines starting with '#' are ignored.
    size_t pos = 0;
    while (pos <= text.size()) {
        size_t nl = text.find('\n', pos);
        std::string line = text.substr(pos, nl == std::string::npos ? std::string::npos : nl - pos);
        pos = (nl == std::string::npos) ? text.size() + 1 : nl + 1;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.find_first_not_of(" \t") == std::string::npos || line[0] == '#') continue;
        size_t tab = line.find('\t');
        if (tab == std::string::npos) {
            ++skipped;
            continue;
        }
        std::string k = line.substr(0, tab), v = line.substr(tab + 1);
        while (!k.empty() && k.back() == ' ') k.pop_back();
        while (!k.empty() && k.front() == ' ') k.erase(k.begin());
        accept(k, v);
    }
    return !out.empty() || skipped > 0;
}

std::string ExportMacrosJson(const std::map<std::string, std::string>& macros) {
    auto esc = [](const std::string& s) {
        std::string o;
        for (unsigned char c : s) {
            if (c == '"') o += "\\\"";
            else if (c == '\\') o += "\\\\";
            else if (c < 0x20) {
                char buf[8];
                snprintf(buf, sizeof(buf), "\\u%04x", c);
                o += buf;
            } else {
                o.push_back((char)c);
            }
        }
        return o;
    };
    std::string out = "{\n  \"version\": 1,\n  \"macros\": {";
    bool first = true;
    for (const auto& kv : macros) {
        out += first ? "\n" : ",\n";
        first = false;
        out += "    \"" + esc(kv.first) + "\": \"" + esc(kv.second) + "\"";
    }
    out += first ? "}\n}\n" : "\n  }\n}\n";
    return out;
}

// ---------------------------------------------------------------------------------------------
// Files and dialogs
bool ReadFileBytes(const std::wstring& path, std::string& out) {
    HANDLE f = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER size;
    bool ok = GetFileSizeEx(f, &size) && size.QuadPart < (8 << 20);
    if (ok) {
        out.resize((size_t)size.QuadPart);
        DWORD read = 0;
        ok = out.empty() || (ReadFile(f, &out[0], (DWORD)out.size(), &read, nullptr) && read == out.size());
    }
    CloseHandle(f);
    return ok;
}

bool WriteFileBytes(const std::wstring& path, const std::string& data) {
    HANDLE f = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    bool ok = data.empty() || (WriteFile(f, data.data(), (DWORD)data.size(), &written, nullptr) && written == data.size());
    CloseHandle(f);
    return ok;
}

static bool RunFileDialog(bool save, HWND owner, const wchar_t* title, const wchar_t* filterName, const wchar_t* filterSpec,
                          const wchar_t* defaultName, const wchar_t* defaultExt, std::wstring& path) {
    IFileDialog* dlg = nullptr;
    HRESULT hr = save ? CoCreateInstance(kClsidFileSaveDialog, nullptr, CLSCTX_INPROC_SERVER, __uuidof(IFileSaveDialog), reinterpret_cast<void**>(&dlg))
                      : CoCreateInstance(kClsidFileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, __uuidof(IFileOpenDialog), reinterpret_cast<void**>(&dlg));
    if (FAILED(hr) || !dlg) {
        MessageBoxW(owner, L"Không mở được hộp thoại chọn tệp.", L"VisKey", MB_OK | MB_ICONERROR);
        return false;
    }
    COMDLG_FILTERSPEC spec[2];
    spec[0].pszName = filterName;
    spec[0].pszSpec = filterSpec;
    spec[1].pszName = L"Tất cả tệp (*.*)";
    spec[1].pszSpec = L"*.*";
    dlg->SetFileTypes(2, spec);
    dlg->SetTitle(title);
    if (defaultName) dlg->SetFileName(defaultName);
    if (defaultExt) dlg->SetDefaultExtension(defaultExt);
    DWORD opts = 0;
    dlg->GetOptions(&opts);
    dlg->SetOptions(opts | FOS_FORCEFILESYSTEM | (save ? FOS_OVERWRITEPROMPT : FOS_FILEMUSTEXIST));
    bool ok = false;
    if (SUCCEEDED(dlg->Show(owner))) {
        IShellItem* item = nullptr;
        if (SUCCEEDED(dlg->GetResult(&item)) && item) {
            PWSTR p = nullptr;
            if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &p)) && p) {
                path = p;
                CoTaskMemFree(p);
                ok = true;
            }
            item->Release();
        }
    }
    dlg->Release();
    return ok;
}

bool PickOpenFile(HWND owner, const wchar_t* title, const wchar_t* filterName, const wchar_t* filterSpec, std::wstring& path) {
    return RunFileDialog(false, owner, title, filterName, filterSpec, nullptr, nullptr, path);
}

bool PickSaveFile(HWND owner, const wchar_t* title, const wchar_t* filterName, const wchar_t* filterSpec, const wchar_t* defaultName,
                  const wchar_t* defaultExt, std::wstring& path) {
    return RunFileDialog(true, owner, title, filterName, filterSpec, defaultName, defaultExt, path);
}

// ---------------------------------------------------------------------------------------------
// Version / system information
std::wstring AppVersion() {
    static std::wstring cached;
    if (!cached.empty()) return cached;
    HRSRC res = FindResourceW(nullptr, MAKEINTRESOURCEW(1), MAKEINTRESOURCEW(16) /* RT_VERSION */);
    if (res) {
        HGLOBAL h = LoadResource(nullptr, res);
        const BYTE* data = h ? static_cast<const BYTE*>(LockResource(h)) : nullptr;
        DWORD size = SizeofResource(nullptr, res);
        for (DWORD off = 0; data && off + sizeof(VS_FIXEDFILEINFO) <= size; off += 4) {
            DWORD sig = 0;
            std::memcpy(&sig, data + off, 4);
            if (sig != 0xFEEF04BD) continue;
            VS_FIXEDFILEINFO fi;
            std::memcpy(&fi, data + off, sizeof(fi));
            wchar_t buf[64];
            swprintf(buf, 64, L"%u.%u.%u", (unsigned)HIWORD(fi.dwProductVersionMS), (unsigned)LOWORD(fi.dwProductVersionMS),
                     (unsigned)HIWORD(fi.dwProductVersionLS));
            cached = buf;
            break;
        }
    }
#ifdef VISKEY_APP_VERSION
    if (cached.empty() || cached == L"0.0.0") cached = Utf8ToWide(VISKEY_APP_VERSION);
#endif
    if (cached.empty()) cached = L"0.0.0";
    return cached;
}

const wchar_t* ArchText() {
#if defined(_M_ARM64) || defined(__aarch64__)
    return L"ARM64";
#elif defined(_M_X64) || defined(__x86_64__)
    return L"x64";
#else
    return L"x86";
#endif
}

std::wstring WindowsVersionText() {
    typedef LONG(WINAPI * PfnRtlGetVersion)(OSVERSIONINFOW*);
    auto rtlGetVersion = LoadProc<PfnRtlGetVersion>(GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion");
    OSVERSIONINFOW vi = {};
    vi.dwOSVersionInfoSize = sizeof(vi);
    if (!rtlGetVersion || rtlGetVersion(&vi) != 0) return L"Windows (không rõ phiên bản)";
    wchar_t buf[96];
    const wchar_t* name = (vi.dwMajorVersion == 10 && vi.dwBuildNumber >= 22000) ? L"Windows 11" : L"Windows";
    swprintf(buf, 96, L"%ls (%lu.%lu.%lu)", name, (unsigned long)vi.dwMajorVersion, (unsigned long)vi.dwMinorVersion,
             (unsigned long)vi.dwBuildNumber);
    return buf;
}

std::wstring SystemInfoText(const Model& m) {
    SYSTEM_INFO si = {};
    GetNativeSystemInfo(&si);
    const wchar_t* machine = si.wProcessorArchitecture == 12 ? L"ARM64" : si.wProcessorArchitecture == 9 ? L"x64" : L"x86";
    std::wstring s;
    s += L"VisKey " + AppVersion() + L" (" + ArchText() + L", bản portable)\r\n";
    s += WindowsVersionText() + L" · máy " + machine + L"\r\n";
    s += std::wstring(L"Kiểu gõ: ") + (m.vni ? L"VNI" : L"Telex") + L" · Đặt dấu: " + (m.modernOrthography ? L"kiểu mới" : L"kiểu cũ") + L"\r\n";
    s += std::wstring(L"Gõ tắt: ") + (m.macrosEnabled ? L"bật" : L"tắt") + L" (" + std::to_wstring(m.macros.size()) + L" cụm từ) · Quick Telex: " +
         (m.quickTelex ? L"bật" : L"tắt") + L"\r\n";
    s += std::wstring(L"Theo ứng dụng: ") + (m.smartPerApp ? L"bật" : L"tắt") + L" (" + std::to_wstring(m.rules.size()) + L" ứng dụng)\r\n";
    s += std::wstring(L"Tự khôi phục EN: ") + (m.autoRestore ? L"bật" : L"tắt") + L" · Sửa trình duyệt: " + (m.fixAutocomplete ? L"bật" : L"tắt") + L"\r\n";
    std::wstring hk;
    for (int i = 0; i < kHotkeyCount; ++i) {
        if (i) hk += L" / ";
        hk += m.hotkeys[i].Empty() ? L"—" : HotkeyText(m.hotkeys[i]);
    }
    s += L"Phím tắt: " + hk + L"\r\n";
    s += std::wstring(L"Bộ gõ: ") + (m.hooksActive ? L"đang chạy" : L"chưa chạy") + L" · Cài đặt: HKCU\\Software\\VisKey\r\n";
    return s;
}

// ---------------------------------------------------------------------------------------------
// Update check (WinHTTP, loaded at runtime). The manual button and the automatic daily check
// share FetchLatestRelease(): one anonymous GET of the public release list, nothing else.
int64_t UnixNow() {
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    ULARGE_INTEGER u;
    u.LowPart = ft.dwLowDateTime;
    u.HighPart = ft.dwHighDateTime;
    const ULONGLONG kEpochDiff = 116444736000000000ull;  // 1601-01-01 -> 1970-01-01 in 100 ns
    return u.QuadPart > kEpochDiff ? (int64_t)((u.QuadPart - kEpochDiff) / 10000000ull) : 0;
}

std::wstring FormatLocalTime(int64_t unixSeconds) {
    if (unixSeconds <= 0) return std::wstring();
    ULARGE_INTEGER u;
    u.QuadPart = (ULONGLONG)unixSeconds * 10000000ull + 116444736000000000ull;
    FILETIME ft;
    ft.dwLowDateTime = u.LowPart;
    ft.dwHighDateTime = u.HighPart;
    SYSTEMTIME utc, local;
    if (!FileTimeToSystemTime(&ft, &utc) || !SystemTimeToTzSpecificLocalTime(nullptr, &utc, &local)) return std::wstring();
    wchar_t buf[40];
    swprintf(buf, 40, L"%02u/%02u/%04u %02u:%02u", (unsigned)local.wDay, (unsigned)local.wMonth, (unsigned)local.wYear, (unsigned)local.wHour,
             (unsigned)local.wMinute);
    return buf;
}

std::wstring AvailableUpdateVersion(const Model& m) {
    if (m.haveUpdateResult && m.update.ok) return m.update.newer ? m.update.latest : std::wstring();
    return update::KnownNewerVersion(m.lastNotifiedVersion, AppVersion()) ? m.lastNotifiedVersion : std::wstring();
}

namespace {

typedef HINTERNET(WINAPI* PfnOpen)(LPCWSTR, DWORD, LPCWSTR, LPCWSTR, DWORD);
typedef HINTERNET(WINAPI* PfnConnect)(HINTERNET, LPCWSTR, INTERNET_PORT, DWORD);
typedef HINTERNET(WINAPI* PfnOpenRequest)(HINTERNET, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR*, DWORD);
typedef BOOL(WINAPI* PfnSendRequest)(HINTERNET, LPCWSTR, DWORD, LPVOID, DWORD, DWORD, DWORD_PTR);
typedef BOOL(WINAPI* PfnReceiveResponse)(HINTERNET, LPVOID);
typedef BOOL(WINAPI* PfnQueryHeaders)(HINTERNET, DWORD, LPCWSTR, LPVOID, LPDWORD, LPDWORD);
typedef BOOL(WINAPI* PfnReadData)(HINTERNET, LPVOID, DWORD, LPDWORD);
typedef BOOL(WINAPI* PfnCloseHandle)(HINTERNET);
typedef BOOL(WINAPI* PfnSetTimeouts)(HINTERNET, int, int, int, int);

// The request itself: same endpoint, headers, 8 s timeouts and version rule for both callers.
// Anonymous: default User-Agent "VisKey/<version>", no cookies, no query beyond per_page.
UpdateResult FetchLatestRelease() {
    UpdateResult res;
    HMODULE mod = LoadLibraryW(L"winhttp.dll");
    auto pOpen = LoadProc<PfnOpen>(mod, "WinHttpOpen");
    auto pConnect = LoadProc<PfnConnect>(mod, "WinHttpConnect");
    auto pOpenRequest = LoadProc<PfnOpenRequest>(mod, "WinHttpOpenRequest");
    auto pSend = LoadProc<PfnSendRequest>(mod, "WinHttpSendRequest");
    auto pReceive = LoadProc<PfnReceiveResponse>(mod, "WinHttpReceiveResponse");
    auto pQuery = LoadProc<PfnQueryHeaders>(mod, "WinHttpQueryHeaders");
    auto pRead = LoadProc<PfnReadData>(mod, "WinHttpReadData");
    auto pClose = LoadProc<PfnCloseHandle>(mod, "WinHttpCloseHandle");
    auto pTimeouts = LoadProc<PfnSetTimeouts>(mod, "WinHttpSetTimeouts");
    if (!pOpen || !pConnect || !pOpenRequest || !pSend || !pReceive || !pQuery || !pRead || !pClose || !pTimeouts) {
        res.error = L"Máy này thiếu thư viện WinHTTP của Windows.";
        return res;
    }
    std::wstring agent = L"VisKey/" + AppVersion();
    HINTERNET session = pOpen(agent.c_str(), 0 /* WINHTTP_ACCESS_TYPE_DEFAULT_PROXY */, nullptr, nullptr, 0);
    HINTERNET conn = nullptr, req = nullptr;
    std::string body;
    DWORD status = 0, err = 0;
    if (session) {
        pTimeouts(session, 8000, 8000, 8000, 8000);
        conn = pConnect(session, L"api.github.com", 443, 0);
    }
    if (conn) req = pOpenRequest(conn, L"GET", L"/repos/1vision-work/VisKey/releases?per_page=5", nullptr, nullptr, nullptr, WINHTTP_FLAG_SECURE);
    DWORD start = GetTickCount();
    if (req && pSend(req, L"Accept: application/vnd.github+json\r\n", (DWORD)-1, nullptr, 0, 0, 0) && pReceive(req, nullptr)) {
        DWORD len = sizeof(status);
        pQuery(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, nullptr, &status, &len, nullptr);
        char buf[8192];
        for (;;) {
            DWORD got = 0;
            if (!pRead(req, buf, sizeof(buf), &got)) {
                err = GetLastError();
                break;
            }
            if (!got) break;
            body.append(buf, got);
            if (body.size() > (2u << 20) || GetTickCount() - start > 8000) {
                err = ERROR_TIMEOUT;
                break;
            }
        }
    } else {
        err = GetLastError();
    }
    if (req) pClose(req);
    if (conn) pClose(conn);
    if (session) pClose(session);

    wchar_t msg[160];
    if (err && body.empty()) {
        swprintf(msg, 160, L"Không kết nối được tới GitHub (mã lỗi %lu). Kiểm tra kết nối mạng rồi thử lại.", (unsigned long)err);
        res.error = msg;
    } else if (status != 200) {
        swprintf(msg, 160, L"GitHub trả về mã %lu. Thử lại sau ít phút.", (unsigned long)status);
        res.error = msg;
    } else if (!update::PickLatestRelease(body, res.tag, res.latest)) {
        res.error = L"Chưa tìm thấy bản phát hành nào trên GitHub.";
    } else {
        res.ok = true;
        res.newer = update::IsNewer(res.latest, AppVersion());
    }
    return res;
}

struct UpdateJob {
    HWND notify;
    bool automatic;
};

void PostUpdateResult(HWND notify, bool automatic, UpdateResult* res) {
    if (!PostMessageW(notify, WM_VK_UPDATE, automatic ? 1 : 0, reinterpret_cast<LPARAM>(res))) delete res;
}

DWORD WINAPI UpdateThread(LPVOID param) {
    UpdateJob job = *static_cast<UpdateJob*>(param);
    delete static_cast<UpdateJob*>(param);
    PostUpdateResult(job.notify, job.automatic, new UpdateResult(FetchLatestRelease()));
    return 0;
}

}  // namespace

void StartUpdateCheck(HWND notify, bool automatic) {
    UpdateJob* job = new UpdateJob{notify, automatic};
    HANDLE th = CreateThread(nullptr, 0, UpdateThread, job, 0, nullptr);
    if (th) {
        CloseHandle(th);
    } else {
        delete job;
        UpdateResult* res = new UpdateResult();
        res->error = L"Không khởi động được việc kiểm tra cập nhật.";
        PostUpdateResult(notify, automatic, res);
    }
}

}  // namespace vk
