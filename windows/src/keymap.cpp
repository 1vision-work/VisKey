// SPDX-License-Identifier: GPL-3.0-or-later
//
// VisKey for Windows (by 1VISION) — based on OpenKey by Mai Vũ Tuyên.

#include "keymap.h"

#include "viskey_engine.h"

namespace viskey {

uint32_t ModifierBit(uint16_t vk) {
    switch (vk) {
    case vkey::Shift: case vkey::LShift: case vkey::RShift: return kModShift;
    case vkey::Control: case vkey::LControl: case vkey::RControl: return kModCtrl;
    case vkey::Menu: case vkey::LMenu: case vkey::RMenu: return kModAlt;
    case vkey::LWin: case vkey::RWin: return kModWin;
    default: return 0;
    }
}

KeyClass ClassifyKey(uint16_t vk) {
    if (ModifierBit(vk) || vk == vkey::Capital) return KeyClass::Modifier;
    if ((vk >= 'A' && vk <= 'Z') || (vk >= '0' && vk <= '9')) return KeyClass::Typing;
    if (vk >= 0xBA && vk <= 0xC0) return KeyClass::Typing;  // ; = , - . / `
    if (vk >= 0xDB && vk <= 0xDE) return KeyClass::Typing;  // [ \ ] '
    switch (vk) {
    case vkey::Back: case vkey::Tab: case vkey::Return: case vkey::Escape: case vkey::Space:
    case vkey::Prior: case vkey::Next: case vkey::End: case vkey::Home:
    case vkey::Left: case vkey::Up: case vkey::Right: case vkey::Down:
    case vkey::Insert: case vkey::Delete:
        return KeyClass::Typing;  // word breaks the engine knows on _WIN32
    default:
        return KeyClass::Other;
    }
}

uint32_t EngineModifiers(uint32_t heldMods, bool capsLockOn) {
    uint32_t m = 0;
    if (heldMods & kModShift) m |= VK_MOD_SHIFT;
    if (capsLockOn) m |= VK_MOD_CAPS_LOCK;
    if (IsShortcut(heldMods)) m |= VK_MOD_OTHER;
    return m;
}

bool ModifierHotkey::ModifiersChanged(uint32_t now) {
    if (held_ == 0 || (now & held_) == held_) {
        held_ = now;  // first modifier, or more modifiers added to the held set
        return false;
    }
    // A modifier was released: decide on the set that was held.
    const bool fired = held_ == hotkey_;
    held_ = 0;
    return fired;
}

}  // namespace viskey
