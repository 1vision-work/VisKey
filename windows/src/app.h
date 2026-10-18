// SPDX-License-Identifier: GPL-3.0-or-later
//
// VisKey for Windows (by 1VISION) — based on OpenKey by Mai Vũ Tuyên.
//
// The application on the UI thread: owns the settings, the tray icon and the typing service.

#pragma once

#include <windows.h>

#include <string>

#include "config.h"

namespace viskey {
namespace app {

HINSTANCE Instance();
const Config& CurrentConfig();
const std::wstring& ConfigPath();
// Saves, hands the settings to the typing service and refreshes the tray icon.
void Commit(const Config& c);

}  // namespace app
}  // namespace viskey
