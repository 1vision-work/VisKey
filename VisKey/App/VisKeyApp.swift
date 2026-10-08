// SPDX-License-Identifier: GPL-3.0-or-later
//
// VisKey — Vietnamese input for macOS. Based on OpenKey by Mai Vũ Tuyên.

import SwiftUI

@main
struct VisKeyApp: App {
    @NSApplicationDelegateAdaptor(AppDelegate.self) private var appDelegate

    var body: some Scene {
        // Menu bar app: no window at launch. The Settings window arrives in M2.
        SwiftUI.Settings { EmptyView() }
    }
}
