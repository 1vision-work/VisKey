// SPDX-License-Identifier: GPL-3.0-or-later
//
// The key pipeline of CLAUDE.md §4 for M0: KeyTap → EngineBridge → ReplacementPlanner → EventSender.
// ContextResolver and AppPolicyStore join in M1; until then every app gets the `backspace` strategy.
// Runs on the main thread (the tap's run loop); the engine is only touched from here.

import CoreGraphics
import Foundation

final class TypingController: KeyTapHandler {
    /// Called when the language hotkey fires. The owner flips the language and calls `apply`.
    var onToggleLanguage: (() -> Void)?

    private let engine = EngineBridge()
    private var planner = ReplacementPlanner()
    private let sender = EventSender()
    private var hotkey = ModifierHotkey()

    private static let otherModifiers: CGEventFlags = [
        .maskCommand, .maskControl, .maskAlternate, .maskSecondaryFn, .maskNumericPad, .maskHelp,
    ]

    func apply(_ settings: Settings) {
        engine.isVietnamese = settings.vietnamese
        engine.inputMethod = settings.inputMethod
        engine.codeTable = settings.codeTable
        planner.codeTable = settings.codeTable
        planner.reset()
    }

    /// Ends the current word: app switch, click, Esc…
    func newSession() {
        engine.newSession()
        planner.reset()
    }

    // MARK: KeyTapHandler

    func keyTap(_ tap: KeyTap, shouldPass event: CGEvent, type: CGEventType) -> Bool {
        switch type {
        case .keyDown:
            hotkey.keyDown()
            return handleKeyDown(event)
        case .keyUp:
            return passOrQueue(event)
        case .flagsChanged:
            if hotkey.flagsChanged(event.flags) { onToggleLanguage?() }
            return true
        case .leftMouseDown, .rightMouseDown, .leftMouseDragged, .rightMouseDragged:
            engine.mouseDown()
            planner.reset()
            return true
        default:
            return true
        }
    }

    // MARK: Private

    private func handleKeyDown(_ event: CGEvent) -> Bool {
        let flags = event.flags
        let key = TypedKey(
            keyCode: UInt16(truncatingIfNeeded: event.getIntegerValueField(.keyboardEventKeycode)),
            shift: flags.contains(.maskShift),
            capsLock: flags.contains(.maskAlphaShift))
        var modifiers: KeyModifiers = []
        if key.shift { modifiers.insert(.shift) }
        if key.capsLock { modifiers.insert(.capsLock) }
        if !flags.isDisjoint(with: Self.otherModifiers) { modifiers.insert(.other) }

        let result = engine.handleKey(key.keyCode, modifiers: modifiers)
        let replacement = planner.plan(result, key: key, decoder: engine)
        if result.code == .restoreAndStartNewSession { engine.newSession() }

        // Shortcuts (⌘Space, ⌘C…) always go straight through: never swallowed, never re-posted.
        if flags.contains(.maskCommand) { return true }

        if replacement.passThrough {
            if replacement.events.isEmpty { return passOrQueue(event) }
            sender.send(replacement.events, thenRepost: event)
            return false
        }
        sender.send(replacement.events)
        return false
    }

    /// Lets the event through, unless an earlier replacement is still being posted: then it is queued behind
    /// it so the app sees keys in the order they were typed.
    private func passOrQueue(_ event: CGEvent) -> Bool {
        guard sender.isBusy else { return true }
        sender.send([], thenRepost: event)
        return false
    }
}
