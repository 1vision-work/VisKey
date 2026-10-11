// SPDX-License-Identifier: GPL-3.0-or-later
//
// Accessibility permission (CLAUDE.md §6): poll AXIsProcessTrusted() every second so the tap starts as
// soon as the user grants it (no restart) and stops if it is revoked while running.

import AppKit
import ApplicationServices

final class Permissions {
    private static let settingsURL =
        URL(string: "x-apple.systempreferences:com.apple.preference.security?Privacy_Accessibility")!

    private var timer: Timer?

    var isTrusted: Bool { AXIsProcessTrusted() }

    func openAccessibilitySettings() { NSWorkspace.shared.open(Self.settingsURL) }

    /// Calls `check` with the current permission now and then every second, on the main thread.
    func poll(_ check: @escaping (Bool) -> Void) {
        timer?.invalidate()
        check(isTrusted)
        let timer = Timer(timeInterval: 1, repeats: true) { [weak self] _ in
            guard let self else { return }
            check(self.isTrusted)
        }
        timer.tolerance = 0.2
        RunLoop.main.add(timer, forMode: .common)
        self.timer = timer
    }
}
