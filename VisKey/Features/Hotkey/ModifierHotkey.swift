// SPDX-License-Identifier: GPL-3.0-or-later
//
// Modifier-only hotkey (default ⌃⇧), OpenKey's `_lastFlag` logic: remember the largest set of modifiers
// held, and fire when they are released if that set is exactly the hotkey and no other key was pressed
// in between. ⌃⇧T therefore never fires, and ⌘Space is never involved.

import CoreGraphics

struct ModifierHotkey {
    var modifiers: CGEventFlags = [.maskControl, .maskShift]

    private var held: CGEventFlags = []

    private static let relevant: CGEventFlags = [.maskControl, .maskShift, .maskAlternate, .maskCommand]

    /// Feed every flagsChanged event. Returns true when the hotkey fires.
    mutating func flagsChanged(_ flags: CGEventFlags) -> Bool {
        let now = flags.intersection(Self.relevant)
        if held.isEmpty || now.isStrictSuperset(of: held) {
            held = now
            return false
        }
        if now == held { return false } // e.g. Caps Lock or Fn changed
        // A modifier was released: decide on the set that was held.
        let fired = held == modifiers
        held = []
        return fired
    }

    /// Feed every keyDown: a key pressed while modifiers are held cancels the hotkey.
    mutating func keyDown() { held = [] }
}
