// SPDX-License-Identifier: GPL-3.0-or-later
//
// Wires the M0 pieces together: settings, permission, key tap, typing pipeline, menu bar.

import AppKit
import CoreGraphics
import os

final class AppDelegate: NSObject, NSApplicationDelegate {
    private let store = SettingsStore()
    private let typing = TypingController()
    private let tap = KeyTap()
    private let permissions = Permissions()
    private let statusItem = StatusItemController()
    private let log = Logger(subsystem: "work.1vision.viskey", category: "app")

    /// Permission granted and tap running; nil until the first check.
    private var hasPermission: Bool?
    private var openedSettingsForPermission = false
    private var requestedInputMonitoring = false

    func applicationDidFinishLaunching(_ notification: Notification) {
        // Unit tests are hosted in the app: do not install a tap or ask for permissions there.
        if ProcessInfo.processInfo.environment["XCTestConfigurationFilePath"] != nil { return }

        typing.apply(store.settings)
        typing.onToggleLanguage = { [weak self] in
            guard let self else { return }
            self.setVietnamese(!self.store.settings.vietnamese)
        }
        tap.handler = typing

        statusItem.onSelectLanguage = { [weak self] in self?.setVietnamese($0) }
        statusItem.onSelectInputMethod = { [weak self] method in self?.change { $0.inputMethod = method } }
        statusItem.onOpenPermissionSettings = { [weak self] in self?.permissions.openAccessibilitySettings() }

        NSWorkspace.shared.notificationCenter.addObserver(
            forName: NSWorkspace.didActivateApplicationNotification, object: nil, queue: .main
        ) { [weak self] _ in self?.typing.newSession() }

        permissions.poll { [weak self] trusted in self?.syncTap(trusted: trusted) }
    }

    func applicationWillTerminate(_ notification: Notification) { tap.stop() }

    // MARK: Private

    /// Runs every second: keeps the tap in line with the permission, so granting it needs no restart and
    /// revoking it never leaves a dead tap holding the keyboard.
    private func syncTap(trusted: Bool) {
        if trusted && !tap.isRunning {
            if tap.start() {
                typing.newSession()
            } else if !requestedInputMonitoring && !CGPreflightListenEventAccess() {
                // ADR-003: only ask for Input Monitoring when the tap really cannot be created without it.
                requestedInputMonitoring = true
                CGRequestListenEventAccess()
            }
        } else if !trusted {
            tap.stop()
            if !openedSettingsForPermission {
                openedSettingsForPermission = true
                // A failed tap attempt lists VisKey in the Accessibility pane, so the user only flips the switch.
                tap.start()
                permissions.openAccessibilitySettings()
            }
        }
        let running = trusted && tap.isRunning
        if running != hasPermission {
            hasPermission = running
            log.info("accessibility permission: \(trusted ? "granted" : "missing", privacy: .public), tap \(running ? "running" : "stopped", privacy: .public)")
            refreshMenu()
        }
    }

    private func setVietnamese(_ on: Bool) { change { $0.vietnamese = on } }

    private func change(_ edit: (inout Settings) -> Void) {
        store.update(edit)
        typing.apply(store.settings)
        refreshMenu()
    }

    private func refreshMenu() {
        let s = store.settings
        statusItem.update(.init(hasPermission: hasPermission ?? false, vietnamese: s.vietnamese, inputMethod: s.inputMethod))
    }
}
