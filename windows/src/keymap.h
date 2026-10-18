// SPDX-License-Identifier: GPL-3.0-or-later
//
// VisKey for Windows (by 1VISION) — based on OpenKey by Mai Vũ Tuyên.
//
// Keyboard decisions that do not need Win32: which virtual keys go to the engine, the modifier bits
// passed to vk_handle_key, and the modifier-only toggle hotkey. On Windows the engine key codes ARE
// Windows virtual-key codes (Engine/src/platforms/win32.h), so no key translation table is needed.

#pragma once

#include <cstdint>

namespace viskey {

// Windows virtual-key codes used here (same values as <winuser.h>; repeated so this file builds anywhere).
namespace vkey {
constexpr uint16_t Back = 0x08, Tab = 0x09, Return = 0x0D, Shift = 0x10, Control = 0x11, Menu = 0x12,
                   Pause = 0x13, Capital = 0x14, Escape = 0x1B, Space = 0x20, Prior = 0x21, Next = 0x22,
                   End = 0x23, Home = 0x24, Left = 0x25, Up = 0x26, Right = 0x27, Down = 0x28,
                   Insert = 0x2D, Delete = 0x2E, LWin = 0x5B, RWin = 0x5C, LShift = 0xA0, RShift = 0xA1,
                   LControl = 0xA2, RControl = 0xA3, LMenu = 0xA4, RMenu = 0xA5, Packet = 0xE7;
constexpr uint16_t Z = 0x5A;
}  // namespace vkey

// Modifier bits tracked by the hook (not the engine's VK_MOD_* bits).
enum : uint32_t { kModShift = 1u << 0, kModCtrl = 1u << 1, kModAlt = 1u << 2, kModWin = 1u << 3 };

// kMod* bit of a modifier key (left/right/generic), 0 for any other key. Caps Lock is not a modifier here.
uint32_t ModifierBit(uint16_t vk);

enum class KeyClass {
    Typing,    // goes to the engine: letters, digits, OEM punctuation, space, Backspace, Enter, Tab, Esc, navigation
    Modifier,  // Shift/Ctrl/Alt/Win/Caps Lock: only tracked, never sent to the engine
    Other,     // F-keys, numpad, media, IME, VK_PACKET...: passed through and end the current word
};
KeyClass ClassifyKey(uint16_t vk);

// Engine modifiers (VK_MOD_SHIFT / VK_MOD_CAPS_LOCK / VK_MOD_OTHER) for a key press.
// Ctrl, Alt (incl. AltGr = Ctrl+Alt) and Win count as "other": the key is a shortcut, not typing.
uint32_t EngineModifiers(uint32_t heldMods, bool capsLockOn);

// True if the key press is a shortcut that VisKey must pass through untouched (Ctrl/Alt/Win held).
inline bool IsShortcut(uint32_t heldMods) { return (heldMods & (kModCtrl | kModAlt | kModWin)) != 0; }

// The optional Alt+Z toggle: Z with Alt and no other modifier.
inline bool IsAltZ(uint16_t vk, uint32_t heldMods) { return vk == vkey::Z && heldMods == kModAlt; }

// Modifier-only hotkey (default Ctrl+Shift), OpenKey's `_lastFlag` logic as in VisKey/Features/Hotkey:
// remember the largest set of modifiers held and fire when one of them is released if that set is exactly
// the hotkey and no other key was pressed meanwhile. So Ctrl+Shift+T never fires, nor does Win+Shift.
class ModifierHotkey {
public:
    explicit ModifierHotkey(uint32_t hotkey = kModCtrl | kModShift) : hotkey_(hotkey) {}

    // Feed the set of modifiers held after every modifier key down/up. Returns true when the hotkey fires.
    bool ModifiersChanged(uint32_t now);
    // Feed every non-modifier key down: it cancels the pending hotkey.
    void KeyDown() { held_ = 0; }

private:
    uint32_t hotkey_;
    uint32_t held_ = 0;
};

}  // namespace viskey
