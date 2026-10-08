// SPDX-License-Identifier: GPL-3.0-or-later
//
// User settings (CLAUDE.md §7): one Codable struct stored as JSON in the `work.1vision.viskey` suite.
// M0 holds only what the minimal menu changes; M2 adds the rest of the Settings window.

import Foundation

struct Settings: Codable, Equatable {
    var vietnamese = true
    var inputMethod: InputMethod = .telex
    var codeTable: CodeTable = .unicode

    init() {}

    // Missing keys keep their defaults, so older saved settings still decode after fields are added.
    init(from decoder: Decoder) throws {
        let c = try decoder.container(keyedBy: CodingKeys.self)
        let d = Settings()
        vietnamese = try c.decodeIfPresent(Bool.self, forKey: .vietnamese) ?? d.vietnamese
        inputMethod = try c.decodeIfPresent(InputMethod.self, forKey: .inputMethod) ?? d.inputMethod
        codeTable = try c.decodeIfPresent(CodeTable.self, forKey: .codeTable) ?? d.codeTable
    }
}

final class SettingsStore {
    private let defaults: UserDefaults
    private static let key = "settings"

    private(set) var settings: Settings

    init(defaults: UserDefaults = UserDefaults(suiteName: "work.1vision.viskey") ?? .standard) {
        self.defaults = defaults
        settings = defaults.data(forKey: Self.key).flatMap { try? JSONDecoder().decode(Settings.self, from: $0) } ?? Settings()
    }

    func update(_ change: (inout Settings) -> Void) {
        change(&settings)
        if let data = try? JSONEncoder().encode(settings) { defaults.set(data, forKey: Self.key) }
    }
}
