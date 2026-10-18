// SPDX-License-Identifier: GPL-3.0-or-later
//
// VisKey for Windows (by 1VISION) — based on OpenKey by Mai Vũ Tuyên.

#include "win_util.h"

#include <shlobj.h>

namespace viskey {

std::wstring Utf8ToWide(const std::string& s) {
    if (s.empty()) return {};
    const int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), int(s.size()), nullptr, 0);
    std::wstring out(size_t(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), int(s.size()), &out[0], n);
    return out;
}

std::string WideToUtf8(const std::wstring& s) {
    if (s.empty()) return {};
    const int n = WideCharToMultiByte(CP_UTF8, 0, s.data(), int(s.size()), nullptr, 0, nullptr, nullptr);
    std::string out(size_t(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, s.data(), int(s.size()), &out[0], n, nullptr, nullptr);
    return out;
}

std::wstring ExePath() {
    std::wstring buf(MAX_PATH, L'\0');
    while (true) {
        const DWORD n = GetModuleFileNameW(nullptr, &buf[0], DWORD(buf.size()));
        if (n == 0) return {};
        if (n < buf.size()) { buf.resize(n); return buf; }
        buf.resize(buf.size() * 2);
    }
}

std::wstring DirName(const std::wstring& path) {
    const size_t slash = path.find_last_of(L"\\/");
    return slash == std::wstring::npos ? std::wstring() : path.substr(0, slash);
}

std::wstring AppDataDir() {
    PWSTR p = nullptr;
    std::wstring out;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &p))) out = p;
    CoTaskMemFree(p);
    return out;
}

bool IsDirWritable(const std::wstring& dir) {
    const std::wstring probe = dir + L"\\.viskey-write-test";
    HANDLE h = CreateFileW(probe.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    CloseHandle(h);
    return true;
}

bool EnsureDir(const std::wstring& dir) {
    if (dir.empty()) return false;
    const DWORD attr = GetFileAttributesW(dir.c_str());
    if (attr != INVALID_FILE_ATTRIBUTES) return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
    EnsureDir(DirName(dir));
    return CreateDirectoryW(dir.c_str(), nullptr) || GetLastError() == ERROR_ALREADY_EXISTS;
}

bool ReadFileUtf8(const std::wstring& path, std::string& out) {
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER size{};
    bool ok = GetFileSizeEx(h, &size) && size.QuadPart < (16 << 20);
    if (ok) {
        out.resize(size_t(size.QuadPart));
        DWORD read = 0;
        ok = out.empty() || (ReadFile(h, &out[0], DWORD(out.size()), &read, nullptr) && read == out.size());
    }
    CloseHandle(h);
    return ok;
}

bool WriteFileAtomic(const std::wstring& path, const std::string& data) {
    EnsureDir(DirName(path));
    const std::wstring tmp = path + L".tmp";
    HANDLE h = CreateFileW(tmp.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    bool ok = WriteFile(h, data.data(), DWORD(data.size()), &written, nullptr) && written == data.size();
    ok = FlushFileBuffers(h) && ok;
    CloseHandle(h);
    if (ok) ok = MoveFileExW(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
    if (!ok) DeleteFileW(tmp.c_str());
    return ok;
}

namespace {
const wchar_t kRunKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
const wchar_t kRunValue[] = L"VisKey";
}  // namespace

std::wstring RunAtStartupCommand() {
    wchar_t buf[2 * MAX_PATH + 8];
    DWORD size = sizeof buf;
    if (RegGetValueW(HKEY_CURRENT_USER, kRunKey, kRunValue, RRF_RT_REG_SZ, nullptr, buf, &size) != ERROR_SUCCESS)
        return {};
    return buf;
}

bool IsRunAtStartupEnabled() { return !RunAtStartupCommand().empty(); }

bool SetRunAtStartup(bool enabled) {
    HKEY key;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kRunKey, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr) !=
        ERROR_SUCCESS)
        return false;
    LSTATUS st;
    if (enabled) {
        const std::wstring cmd = L"\"" + ExePath() + L"\"";
        st = RegSetValueExW(key, kRunValue, 0, REG_SZ, reinterpret_cast<const BYTE*>(cmd.c_str()),
                            DWORD((cmd.size() + 1) * sizeof(wchar_t)));
    } else {
        st = RegDeleteValueW(key, kRunValue);
        if (st == ERROR_FILE_NOT_FOUND) st = ERROR_SUCCESS;
    }
    RegCloseKey(key);
    return st == ERROR_SUCCESS;
}

static bool IsExtendedKey(WORD vk) {
    switch (vk) {
    case VK_INSERT: case VK_DELETE: case VK_HOME: case VK_END: case VK_PRIOR: case VK_NEXT:
    case VK_LEFT: case VK_RIGHT: case VK_UP: case VK_DOWN: case VK_RCONTROL: case VK_RMENU:
    case VK_DIVIDE: case VK_NUMLOCK: case VK_SNAPSHOT:
        return true;
    default:
        return false;
    }
}

std::vector<INPUT> ToInputs(const std::vector<KeyStroke>& strokes) {
    std::vector<INPUT> out;
    out.reserve(strokes.size());
    for (const auto& s : strokes) {
        INPUT in{};
        in.type = INPUT_KEYBOARD;
        in.ki.dwExtraInfo = kInjectedTag;
        if (s.unicode) {
            in.ki.wVk = 0;
            in.ki.wScan = s.unit;
            in.ki.dwFlags = KEYEVENTF_UNICODE;
        } else {
            in.ki.wVk = s.vk;
            in.ki.wScan = WORD(MapVirtualKeyW(s.vk, MAPVK_VK_TO_VSC));
            if (IsExtendedKey(s.vk)) in.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
        }
        if (s.up) in.ki.dwFlags |= KEYEVENTF_KEYUP;
        out.push_back(in);
    }
    return out;
}

}  // namespace viskey
