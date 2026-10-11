// SPDX-License-Identifier: GPL-3.0-or-later
//
// Session event tap (CLAUDE.md §4.1). Lets VisKey's own events through, re-enables itself when macOS
// disables it, and hands every other event to its handler. The callback runs on the main run loop and
// must stay well under 1 ms: the handler only calls the engine and enqueues work for EventSender.

import CoreGraphics
import Foundation
import os

protocol KeyTapHandler: AnyObject {
    /// Returns true to let the event through unchanged, false to swallow it.
    func keyTap(_ tap: KeyTap, shouldPass event: CGEvent, type: CGEventType) -> Bool
}

final class KeyTap {
    weak var handler: KeyTapHandler?

    private var port: CFMachPort?
    private var source: CFRunLoopSource?
    private let log = Logger(subsystem: "work.1vision.viskey", category: "tap")

    var isRunning: Bool { port != nil }

    private static let eventMask: CGEventMask = [
        CGEventType.keyDown, .keyUp, .flagsChanged,
        .leftMouseDown, .rightMouseDown, .leftMouseDragged, .rightMouseDragged,
    ].reduce(0) { $0 | (1 << $1.rawValue) }

    /// Creates the tap on the main run loop. Fails without Accessibility (or Input Monitoring) permission.
    @discardableResult
    func start() -> Bool {
        guard port == nil else { return true }
        let callback: CGEventTapCallBack = { _, type, event, refcon in
            guard let refcon else { return Unmanaged.passUnretained(event) }
            let tap = Unmanaged<KeyTap>.fromOpaque(refcon).takeUnretainedValue()
            return tap.handle(type, event) ? Unmanaged.passUnretained(event) : nil
        }
        guard let port = CGEvent.tapCreate(
            tap: .cgSessionEventTap, place: .headInsertEventTap, options: .defaultTap,
            eventsOfInterest: Self.eventMask, callback: callback,
            userInfo: Unmanaged.passUnretained(self).toOpaque())
        else {
            log.error("CGEvent.tapCreate failed")
            return false
        }
        let source = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, port, 0)
        CFRunLoopAddSource(CFRunLoopGetMain(), source, .commonModes)
        CGEvent.tapEnable(tap: port, enable: true)
        self.port = port
        self.source = source
        log.info("event tap started")
        return true
    }

    func stop() {
        guard let port else { return }
        CGEvent.tapEnable(tap: port, enable: false)
        if let source { CFRunLoopRemoveSource(CFRunLoopGetMain(), source, .commonModes) }
        CFMachPortInvalidate(port)
        self.port = nil
        self.source = nil
        log.info("event tap stopped")
    }

    deinit { stop() }

    private func handle(_ type: CGEventType, _ event: CGEvent) -> Bool {
        switch type {
        case .tapDisabledByTimeout, .tapDisabledByUserInput:
            log.warning("event tap disabled (\(type == .tapDisabledByTimeout ? "timeout" : "user input", privacy: .public)), re-enabling")
            if let port { CGEvent.tapEnable(tap: port, enable: true) }
            return true
        default:
            if event.getIntegerValueField(.eventSourceUserData) == EventSender.marker { return true }
            return handler?.keyTap(self, shouldPass: event, type: type) ?? true
        }
    }
}
