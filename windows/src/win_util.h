// SPDX-License-Identifier: GPL-3.0-or-later
//
// VisKey for Windows (by 1VISION) — based on OpenKey by Mai Vũ Tuyên.
//
// Small Win32 helpers: UTF-8/UTF-16, paths, files, the HKCU Run value, KeyStroke -> INPUT.

#pragma once

#include <windows.h>

#include <string>
#include <vector>

#include "output_planner.h"

namespace viskey {

// dwExtraInfo of every event VisKey injects ("VISK"). The keyboard hook ignores events carrying it.
constexpr ULONG_PTR kInjectedTag = 0x5649534B;

std::wstring Utf8ToWide(const std::string& s);
std::string WideToUtf8(const std::wstring& s);

std::wstring ExePath();
std::wstring DirName(const std::wstring& path);
std::wstring AppDataDir();                         // %APPDATA% (roaming), empty on failure
bool IsDirWritable(const std::wstring& dir);        // creates and deletes a temporary file
bool EnsureDir(const std::wstring& dir);            // creates missing parents

bool ReadFileUtf8(const std::wstring& path, std::string& out);
// Writes to path.tmp then renames over path, so a crash never leaves a half-written config.
bool WriteFileAtomic(const std::wstring& path, const std::string& data);

// HKCU\Software\Microsoft\Windows\CurrentVersion\Run, value "VisKey" = "\"<exe path>\"".
bool IsRunAtStartupEnabled();
std::wstring RunAtStartupCommand();                // the registered command, empty if none
bool SetRunAtStartup(bool enabled);

// KeyStroke -> INPUT, every INPUT tagged with kInjectedTag.
std::vector<INPUT> ToInputs(const std::vector<KeyStroke>& strokes);

}  // namespace viskey
