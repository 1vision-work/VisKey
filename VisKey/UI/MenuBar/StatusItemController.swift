// SPDX-License-Identifier: GPL-3.0-or-later
//
// Menu bar item, M0 version: icon in three states (brand.md §3) and a minimal menu.
// The full dropdown of design/ui A2/A3 arrives in M2. Strings follow design/ui/strings.md.

import AppKit

final class StatusItemController: NSObject {
    struct State: Equatable {
        var hasPermission: Bool
        var vietnamese: Bool
        var inputMethod: InputMethod
    }

    var onSelectLanguage: ((Bool) -> Void)?
    var onSelectInputMethod: ((InputMethod) -> Void)?
    var onOpenPermissionSettings: (() -> Void)?

    private let item = NSStatusBar.system.statusItem(withLength: NSStatusItem.squareLength)

    func update(_ state: State) {
        let (name, description): (String, String) =
            !state.hasPermission ? ("MenuBarOff", String(localized: "VisKey chưa có quyền Trợ năng"))
            : state.vietnamese ? ("MenuBarVI", String(localized: "Tiếng Việt"))
            : ("MenuBarEN", String(localized: "Tiếng Anh"))
        let image = NSImage(named: name)
        image?.isTemplate = true
        image?.accessibilityDescription = description
        item.button?.image = image
        item.button?.toolTip = "VisKey · \(description)"
        item.menu = makeMenu(state)
    }

    func showMenu() { item.button?.performClick(nil) }

    // MARK: Private

    private func makeMenu(_ state: State) -> NSMenu {
        let menu = NSMenu()
        menu.autoenablesItems = false

        if !state.hasPermission {
            let title = NSMenuItem(title: String(localized: "VisKey chưa có quyền Trợ năng"), action: nil, keyEquivalent: "")
            title.isEnabled = false
            menu.addItem(title)
            menu.addItem(action(String(localized: "Mở Cài đặt hệ thống…"), #selector(openPermissionSettings)))
            menu.addItem(.separator())
        }

        let vietnamese = action(String(localized: "Tiếng Việt"), #selector(selectVietnamese))
        vietnamese.state = state.vietnamese ? .on : .off
        let english = action(String(localized: "Tiếng Anh"), #selector(selectEnglish))
        english.state = state.vietnamese ? .off : .on
        let hint = NSMenuItem(title: String(localized: "⌃⇧ để chuyển"), action: nil, keyEquivalent: "")
        hint.isEnabled = false
        menu.addItem(vietnamese)
        menu.addItem(english)
        menu.addItem(hint)
        menu.addItem(.separator())

        let methods = NSMenu()
        for (method, title) in [(InputMethod.telex, "Telex"), (.vni, "VNI")] {
            let entry = action(title, #selector(selectInputMethod(_:)))
            entry.representedObject = method.rawValue
            entry.state = state.inputMethod == method ? .on : .off
            methods.addItem(entry)
        }
        let methodItem = NSMenuItem(title: String(localized: "Kiểu gõ"), action: nil, keyEquivalent: "")
        methodItem.submenu = methods
        menu.addItem(methodItem)
        menu.addItem(.separator())

        menu.addItem(NSMenuItem(title: String(localized: "Thoát VisKey"),
                                action: #selector(NSApplication.terminate(_:)), keyEquivalent: "q"))
        return menu
    }

    private func action(_ title: String, _ selector: Selector) -> NSMenuItem {
        let item = NSMenuItem(title: title, action: selector, keyEquivalent: "")
        item.target = self
        return item
    }

    @objc private func selectVietnamese() { onSelectLanguage?(true) }
    @objc private func selectEnglish() { onSelectLanguage?(false) }
    @objc private func openPermissionSettings() { onOpenPermissionSettings?() }

    @objc private func selectInputMethod(_ sender: NSMenuItem) {
        guard let raw = sender.representedObject as? Int32, let method = InputMethod(rawValue: raw) else { return }
        onSelectInputMethod?(method)
    }
}
