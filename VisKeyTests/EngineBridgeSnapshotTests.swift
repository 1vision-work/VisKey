// SPDX-License-Identifier: GPL-3.0-or-later
//
// Replays every key case of Engine/tests/snapshots/engine.snap (G1 oracle, recorded from the original
// OpenKey engine) through EngineBridge and re-renders each block exactly like Engine/tests/common/cases.cpp.
// The Swift side must produce the same engine words, counts and screen text, byte for byte.

import Carbon.HIToolbox
import VisKeyEngine
import XCTest
@testable import VisKey

final class EngineBridgeSnapshotTests: XCTestCase {
    func testAllKeyCasesMatchSnapshot() throws {
        let url = try XCTUnwrap(Bundle(for: Self.self).url(forResource: "engine", withExtension: "snap"))
        let blocks = try String(contentsOf: url, encoding: .utf8)
            .components(separatedBy: "\n\n")
            .map { $0.trimmingCharacters(in: CharacterSet(charactersIn: "\n")) }
            .compactMap(SnapshotBlock.init)
        XCTAssertGreaterThanOrEqual(blocks.count, 500, "expected the full G1 snapshot")

        var failures = 0
        for block in blocks {
            let rendered = try render(block)
            if rendered != block.text {
                failures += 1
                if failures <= 10 { XCTFail("\(block.id) differs\n--- snapshot\n\(block.text)\n--- swift\n\(rendered)") }
            }
        }
        XCTAssertEqual(failures, 0, "\(failures) of \(blocks.count) cases differ")
    }

    func testDecodeMatchesKnownWords() {
        let engine = EngineBridge()
        XCTAssertEqual(engine.decode(0x2001EDF), [0x1EDF])                      // ở
        XCTAssertEqual(engine.decode(UInt32(kVK_ANSI_A)), [UInt16(UInt8(ascii: "a"))]) // plain key code
        XCTAssertEqual(engine.decode(UInt32(kVK_ANSI_A) | 0x10000), [UInt16(UInt8(ascii: "A"))])
        engine.codeTable = .unicodeCompound
        _ = engine.handleKey(UInt16(kVK_ANSI_A), modifiers: [])
        let accented = engine.handleKey(UInt16(kVK_ANSI_S), modifiers: []) // "as" -> "á"
        XCTAssertEqual(accented.words.map(engine.decode), [[0x61, 0x301]], "compound: base + combining mark")
    }

    func testMacroLongerThanResultBufferIsComplete() {
        let engine = EngineBridge()
        engine.setOption(VK_OPT_USE_MACRO, 1)
        let content = String(repeating: "0123456789", count: 20)
        XCTAssertTrue(engine.addMacro("long", content: content))
        for key in [kVK_ANSI_L, kVK_ANSI_O, kVK_ANSI_N, kVK_ANSI_G] { _ = engine.handleKey(UInt16(key), modifiers: []) }
        let result = engine.handleKey(UInt16(kVK_Space), modifiers: [])
        XCTAssertEqual(result.code, .replaceMacro)
        XCTAssertEqual(result.backspaceCount, 4)
        let text = result.words.flatMap(engine.decode)
        XCTAssertEqual(String(decoding: text, as: UTF16.self), content)
    }

    // MARK: Replay

    private func render(_ block: SnapshotBlock) throws -> String {
        let engine = EngineBridge()
        for (option, value) in block.options { engine.setOption(option, value) }
        for (text, content) in Self.macros(for: block.id) { engine.addMacro(text, content: content) }
        let english = block.id.hasPrefix("macro-en/") // cases.cpp routes only these through englishKey()

        var out = block.header
        var screen: [[UInt16]] = []
        for token in try KeyToken.parse(block.keys) {
            switch token.kind {
            case .mouse:
                if !english { engine.mouseDown() }
                out += "  \(token.text) mouse\n"
                continue
            case .tempSpell:
                engine.tempOffSpelling()
                out += "  \(token.text) tempspell\n"
                continue
            case .tempOff:
                engine.tempOffEngine()
                out += "  \(token.text) tempoff\n"
                continue
            case .key:
                break
            }
            var modifiers: KeyModifiers = token.caps ? .shift : []
            if token.other { modifiers.insert(.other) }
            let r = engine.handleKey(token.code, modifiers: modifiers)
            let isMacro = r.code == .replaceMacro
            out += "  " + token.text.padding(toLength: max(8, token.text.count), withPad: " ", startingAt: 0)
            out += " c=\(r.code.rawValue) b=\(r.backspaceCount) n=\(r.words.count) e=\(r.extCode)"
            out += " w=\(hex(isMacro ? [] : r.words)) m=\(hex(isMacro ? r.words : []))\n"

            switch r.code {
            case .doNothing:
                if english { if !token.other { passThrough(token, &screen) } }
                else if r.extCode == 2 { _ = screen.popLast() }
                else if !token.other { passThrough(token, &screen) }
            case .willProcess:
                screen.removeLast(min(r.backspaceCount, screen.count))
                screen += r.words.map(engine.decode).filter { !$0.isEmpty }
            case .restore, .restoreAndStartNewSession, .replaceMacro:
                screen.removeLast(min(r.backspaceCount, screen.count))
                screen += r.words.map(engine.decode).filter { !$0.isEmpty }
                passThrough(token, &screen)
            case .breakWord:
                break
            }
        }
        out += "=> " + renderScreen(screen, table: block.codeTable)
        return out
    }

    private func passThrough(_ token: KeyToken, _ screen: inout [[UInt16]]) {
        if token.code == UInt16(kVK_Delete) { _ = screen.popLast(); return }
        var ch = EngineBridge.character(keyCode: token.code, caps: token.caps) ?? 0
        if token.code == UInt16(kVK_Return) { ch = 0x0A }
        if token.code == UInt16(kVK_Tab) { ch = 0x09 }
        if ch != 0 { screen.append([ch]) }
    }

    private func renderScreen(_ screen: [[UInt16]], table: Int32) -> String {
        var s = ""
        for unit in screen.joined() {
            if unit < 0x80 {
                s += unit == 0x0A ? "\\n" : unit == 0x09 ? "\\t" : String(UnicodeScalar(UInt8(unit)))
            } else if table == 0 || table == 3 {
                s += UnicodeScalar(unit).map(String.init) ?? "?"
            } else {
                s += String(format: "<%02X>", unit)
            }
        }
        return s
    }

    private func hex(_ words: [UInt32]) -> String {
        words.isEmpty ? "-" : words.map { String($0, radix: 16, uppercase: true) }.joined(separator: ",")
    }

    /// Macro definitions of Engine/tests/common/cases.cpp (the snapshot records keys and output, not macros).
    private static func macros(for id: String) -> [(String, String)] {
        let long = String(repeating: "0123456789", count: 20)
        let longVn = String(repeating: "tiếng Việt đẹp ", count: 10)
        switch id {
        case "macro/vn-content": return [("tp", "thành phố Hồ Chí Minh")]
        case "macro/vn-key": return [("đường", "đường phố")]
        case "macro/two": return [("btw", "by the way"), ("tp", "thành phố")]
        case "macro/200-chars", "macro-en/200": return [("long", long)]
        case "macro/long-vn": return [("tv", longVn)]
        case _ where id.hasPrefix("macro/table"): return [("tp", "thành phố")]
        case _ where id.hasPrefix("macro/") || id.hasPrefix("macro-en/"): return [("btw", "by the way")]
        default: return []
        }
    }
}

// MARK: - Snapshot parsing

private struct SnapshotBlock {
    let id: String
    let text: String
    let header: String
    let keys: String
    let options: [(vk_option, Int32)]
    let codeTable: Int32

    init?(_ text: String) {
        let lines = text.components(separatedBy: "\n")
        guard lines.count >= 3, lines[0].hasPrefix("## "), lines[1].hasPrefix("cfg: "), lines[2].hasPrefix("keys: ")
        else { return nil }
        self.text = text
        id = String(lines[0].dropFirst(3).split(separator: " ")[0])
        header = lines[0...2].joined(separator: "\n") + "\n"
        keys = String(lines[2].dropFirst("keys: ".count))

        var cfg: [String: Int32] = [:]
        for pair in lines[1].dropFirst("cfg: ".count).split(separator: " ") {
            let kv = pair.split(separator: "=", maxSplits: 1).map(String.init)
            if kv[0] == "macro" {
                let parts = kv[1].split(separator: "/").compactMap { Int32($0) }
                cfg["macro"] = parts[0]
                cfg["macroEn"] = parts[1]
            } else {
                cfg[kv[0]] = Int32(kv[1])
            }
        }
        codeTable = cfg["table"] ?? 0
        // Same order as Engine/tests/vk/driver_vk.cpp (each vk_set_option resets the typing state).
        let order: [(vk_option, String)] = [
            (VK_OPT_LANGUAGE, "lang"), (VK_OPT_INPUT_TYPE, "input"), (VK_OPT_CODE_TABLE, "table"),
            (VK_OPT_FREE_MARK, "free"), (VK_OPT_CHECK_SPELLING, "spell"), (VK_OPT_MODERN_ORTHOGRAPHY, "modern"),
            (VK_OPT_QUICK_TELEX, "quick"), (VK_OPT_RESTORE_IF_WRONG_SPELLING, "restore"), (VK_OPT_USE_MACRO, "macro"),
            (VK_OPT_USE_MACRO_IN_ENGLISH_MODE, "macroEn"), (VK_OPT_AUTO_CAPS_MACRO, "autocaps"),
            (VK_OPT_UPPERCASE_FIRST_CHAR, "upper1"), (VK_OPT_ALLOW_CONSONANT_ZFWJ, "zfwj"),
            (VK_OPT_QUICK_START_CONSONANT, "qstart"), (VK_OPT_QUICK_END_CONSONANT, "qend"),
        ]
        options = order.map { ($0.0, cfg[$0.1] ?? 0) }
    }
}

/// Key DSL of Engine/tests/common/cases.cpp: US keyboard, A–Z and shifted symbols typed with Shift,
/// specials <bs> <esc> <enter> <tab> <left> <mouse> <tempspell> <tempoff> <cmd-a>.
private struct KeyToken {
    enum Kind { case key, mouse, tempSpell, tempOff }
    var kind = Kind.key
    var code: UInt16 = 0
    var caps = false
    var other = false
    var text = ""

    struct ParseError: Error { let message: String }

    private static let keys: [Character: Int] = [
        "a": kVK_ANSI_A, "b": kVK_ANSI_B, "c": kVK_ANSI_C, "d": kVK_ANSI_D, "e": kVK_ANSI_E, "f": kVK_ANSI_F,
        "g": kVK_ANSI_G, "h": kVK_ANSI_H, "i": kVK_ANSI_I, "j": kVK_ANSI_J, "k": kVK_ANSI_K, "l": kVK_ANSI_L,
        "m": kVK_ANSI_M, "n": kVK_ANSI_N, "o": kVK_ANSI_O, "p": kVK_ANSI_P, "q": kVK_ANSI_Q, "r": kVK_ANSI_R,
        "s": kVK_ANSI_S, "t": kVK_ANSI_T, "u": kVK_ANSI_U, "v": kVK_ANSI_V, "w": kVK_ANSI_W, "x": kVK_ANSI_X,
        "y": kVK_ANSI_Y, "z": kVK_ANSI_Z, "0": kVK_ANSI_0, "1": kVK_ANSI_1, "2": kVK_ANSI_2, "3": kVK_ANSI_3,
        "4": kVK_ANSI_4, "5": kVK_ANSI_5, "6": kVK_ANSI_6, "7": kVK_ANSI_7, "8": kVK_ANSI_8, "9": kVK_ANSI_9,
        " ": kVK_Space, ",": kVK_ANSI_Comma, ".": kVK_ANSI_Period, "/": kVK_ANSI_Slash, ";": kVK_ANSI_Semicolon,
        "'": kVK_ANSI_Quote, "[": kVK_ANSI_LeftBracket, "]": kVK_ANSI_RightBracket, "\\": kVK_ANSI_Backslash,
        "-": kVK_ANSI_Minus, "=": kVK_ANSI_Equal, "`": kVK_ANSI_Grave,
    ]
    private static let shifted: [Character: Character] = [
        "!": "1", "@": "2", "#": "3", "$": "4", "%": "5", "^": "6", "&": "7", "*": "8", "(": "9", ")": "0",
        "?": "/", ":": ";", "\"": "'", "_": "-", "+": "=",
    ]
    private static let specials: [String: Int] = [
        "bs": kVK_Delete, "esc": kVK_Escape, "enter": kVK_Return, "tab": kVK_Tab, "left": kVK_LeftArrow,
    ]

    static func parse(_ s: String) throws -> [KeyToken] {
        var out: [KeyToken] = []
        var rest = Substring(s)
        while let c = rest.first {
            var t = KeyToken()
            if c == "<" {
                guard let end = rest.firstIndex(of: ">") else { throw ParseError(message: "bad key DSL: \(s)") }
                let name = String(rest[rest.index(after: rest.startIndex)..<end])
                rest = rest[rest.index(after: end)...]
                t.text = "<\(name)>"
                switch name {
                case "mouse": t.kind = .mouse
                case "tempspell": t.kind = .tempSpell
                case "tempoff": t.kind = .tempOff
                case "cmd-a": t.code = UInt16(kVK_ANSI_A); t.other = true
                default:
                    guard let code = specials[name] else { throw ParseError(message: "unknown token \(name)") }
                    t.code = UInt16(code)
                }
            } else {
                rest = rest.dropFirst()
                t.text = c == " " ? "<sp>" : String(c)
                var base = c
                if c.isASCII && c.isUppercase { t.caps = true; base = Character(c.lowercased()) }
                if let unshifted = shifted[c] { t.caps = true; base = unshifted }
                guard let code = keys[base] else { throw ParseError(message: "unmapped char \(c)") }
                t.code = UInt16(code)
            }
            out.append(t)
        }
        return out
    }
}
