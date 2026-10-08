// SPDX-License-Identifier: GPL-3.0-or-later
//
// Posts planned events on one serial queue (CLAUDE.md §4.4, principle 4). Each call to `send` is one
// transaction; a transaction never interleaves with another. Every event is marked with
// `EventSender.marker` so KeyTap lets it through untouched.

import CoreGraphics
import Foundation

final class EventSender {
    /// "VISK", stored in eventSourceUserData of every event VisKey posts.
    static let marker: Int64 = 0x5649_534B

    private static let backspaceKeyCode: CGKeyCode = 51

    private let queue = DispatchQueue(label: "work.1vision.viskey.event-sender", qos: .userInteractive)
    private let source = CGEventSource(stateID: .privateState)
    private let lock = NSLock()
    private var pending = 0

    /// True while a transaction is queued or posting. Keys that arrive meanwhile must be queued behind it
    /// (via `send(_:thenRepost:)`), never passed straight to the app, or they would overtake it.
    var isBusy: Bool {
        lock.lock()
        defer { lock.unlock() }
        return pending > 0
    }

    /// Queues `events`, then a marked copy of `original` (the user's key event) if given.
    /// Call from the tap callback: it copies `original` before returning.
    func send(_ events: [OutputEvent], thenRepost original: CGEvent? = nil) {
        let repost = original?.copy()
        repost?.setIntegerValueField(.eventSourceUserData, value: Self.marker)
        guard !events.isEmpty || repost != nil else { return }

        lock.lock()
        pending += 1
        lock.unlock()

        queue.async { [self] in
            for event in events { post(event) }
            repost?.post(tap: .cgSessionEventTap)
            lock.lock()
            pending -= 1
            lock.unlock()
        }
    }

    // MARK: Private (sender queue)

    private func post(_ event: OutputEvent) {
        switch event {
        case .backspace:
            postKey(Self.backspaceKeyCode, flags: [])
        case .key(let code, let shift):
            postKey(code, flags: shift ? .maskShift : [])
        case .text(let units):
            postText(units)
        }
    }

    private func postKey(_ code: CGKeyCode, flags: CGEventFlags) {
        for down in [true, false] {
            guard let e = CGEvent(keyboardEventSource: source, virtualKey: code, keyDown: down) else { continue }
            e.flags = flags
            e.setIntegerValueField(.eventSourceUserData, value: Self.marker)
            e.post(tap: .cgSessionEventTap)
        }
    }

    private func postText(_ units: [UInt16]) {
        for down in [true, false] {
            guard let e = CGEvent(keyboardEventSource: source, virtualKey: 0, keyDown: down) else { continue }
            e.flags = []
            units.withUnsafeBufferPointer { e.keyboardSetUnicodeString(stringLength: $0.count, unicodeString: $0.baseAddress) }
            e.setIntegerValueField(.eventSourceUserData, value: Self.marker)
            e.post(tap: .cgSessionEventTap)
        }
    }
}
