// SPDX-License-Identifier: GPL-3.0-or-later

import CoreGraphics
import XCTest
@testable import VisKey

final class ModifierHotkeyTests: XCTestCase {
    func testControlShiftReleaseFires() {
        var hotkey = ModifierHotkey()
        XCTAssertFalse(hotkey.flagsChanged(.maskControl))
        XCTAssertFalse(hotkey.flagsChanged([.maskControl, .maskShift]))
        XCTAssertTrue(hotkey.flagsChanged(.maskControl))
        XCTAssertFalse(hotkey.flagsChanged([]), "fires once per press")
    }

    func testReleaseInEitherOrderFires() {
        var hotkey = ModifierHotkey()
        _ = hotkey.flagsChanged(.maskShift)
        _ = hotkey.flagsChanged([.maskShift, .maskControl])
        XCTAssertTrue(hotkey.flagsChanged(.maskShift))
    }

    func testKeyPressedInBetweenCancels() {
        var hotkey = ModifierHotkey()
        _ = hotkey.flagsChanged([.maskControl, .maskShift])
        hotkey.keyDown() // ⌃⇧T
        XCTAssertFalse(hotkey.flagsChanged(.maskControl))
        XCTAssertFalse(hotkey.flagsChanged([]))
    }

    func testExtraModifierDoesNotFire() {
        var hotkey = ModifierHotkey()
        _ = hotkey.flagsChanged([.maskControl, .maskShift, .maskCommand])
        XCTAssertFalse(hotkey.flagsChanged([.maskControl, .maskShift]))
        XCTAssertFalse(hotkey.flagsChanged([]))
    }

    func testSingleModifierDoesNotFire() {
        var hotkey = ModifierHotkey()
        _ = hotkey.flagsChanged(.maskControl)
        XCTAssertFalse(hotkey.flagsChanged([]))
    }

    func testIrrelevantFlagsAreIgnored() {
        var hotkey = ModifierHotkey()
        _ = hotkey.flagsChanged([.maskControl, .maskShift])
        XCTAssertFalse(hotkey.flagsChanged([.maskControl, .maskShift, .maskAlphaShift]))
        XCTAssertTrue(hotkey.flagsChanged(.maskShift))
    }
}
