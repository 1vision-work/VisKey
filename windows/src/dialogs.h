// SPDX-License-Identifier: GPL-3.0-or-later
//
// VisKey for Windows (by 1VISION) — based on OpenKey by Mai Vũ Tuyên.
//
// Settings and About windows: modeless dialogs from res/VisKey.rc. Showing one that is already open
// brings it to the front.

#pragma once

#include <windows.h>

namespace viskey {

void ShowSettingsDialog();
void ShowAboutDialog();
// Routes keyboard navigation (Tab, Enter, Esc) to the open dialogs; call from the message loop.
bool HandleDialogMessage(MSG* msg);
// Reloads an open settings dialog from CurrentConfig() after a change made elsewhere (tray menu, hotkey),
// keeping its unsaved macro edits.
void RefreshSettingsDialog();

}  // namespace viskey
