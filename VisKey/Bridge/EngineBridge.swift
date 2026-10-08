// SPDX-License-Identifier: GPL-3.0-or-later
//
// Swift face of the C engine (Engine/include/viskey_engine.h). Converts vk_result into a value type and
// decodes engine words exactly like OpenKey's SendNewCharString (ADR-008). Knows nothing about events.

import VisKeyEngine

enum InputMethod: Int32, Codable, CaseIterable {
    case telex = 0
    case vni = 1
    case simpleTelex1 = 2
    case simpleTelex2 = 3
}

enum CodeTable: Int32, Codable, CaseIterable {
    case unicode = 0
    case tcvn3 = 1
    case vniWindows = 2
    case unicodeCompound = 3
    case cp1258 = 4

    /// Tables where one visible character may be two UTF-16 units (OpenKey's IS_DOUBLE_CODE).
    var isDoubleCode: Bool { self == .vniWindows || self == .unicodeCompound }
}

struct KeyModifiers: OptionSet {
    let rawValue: UInt32
    static let shift = KeyModifiers(rawValue: UInt32(VK_MOD_SHIFT))
    static let capsLock = KeyModifiers(rawValue: UInt32(VK_MOD_CAPS_LOCK))
    /// Cmd, Ctrl, Option, Fn: anything that is not a typing modifier.
    static let other = KeyModifiers(rawValue: UInt32(VK_MOD_OTHER))
}

struct EngineResult: Equatable {
    enum Code: UInt8 {
        case doNothing = 0
        case willProcess = 1
        case breakWord = 2
        case restore = 3
        case replaceMacro = 4
        case restoreAndStartNewSession = 5
    }

    var code: Code
    var backspaceCount: Int
    /// Engine words in display order. For `.replaceMacro` this is the full macro content, not only the first 64.
    var words: [UInt32]
    /// OpenKey meaning: 1 word break, 2 delete key, 3 normal key, 4 no empty character first.
    var extCode: UInt8

    static let nothing = EngineResult(code: .doNothing, backspaceCount: 0, words: [], extCode: 0)
}

/// Turns an engine word into the UTF-16 units to type, using the current code table.
protocol WordDecoding {
    func decode(_ word: UInt32) -> [UInt16]
}

/// Owns one engine. Not thread-safe: use it from one thread (the key tap's run loop).
final class EngineBridge: WordDecoding {
    private let engine: OpaquePointer

    init() {
        guard let engine = vk_create() else { fatalError("vk_create failed: out of memory") }
        self.engine = engine
    }

    deinit { vk_destroy(engine) }

    // MARK: Options (every change resets the typing state)

    func setOption(_ option: vk_option, _ value: Int32) { vk_set_option(engine, option, value) }
    func option(_ option: vk_option) -> Int32 { vk_get_option(engine, option) }

    var isVietnamese: Bool {
        get { option(VK_OPT_LANGUAGE) != 0 }
        set { setOption(VK_OPT_LANGUAGE, newValue ? 1 : 0) }
    }

    var inputMethod: InputMethod {
        get { InputMethod(rawValue: option(VK_OPT_INPUT_TYPE)) ?? .telex }
        set { setOption(VK_OPT_INPUT_TYPE, newValue.rawValue) }
    }

    var codeTable: CodeTable {
        get { CodeTable(rawValue: option(VK_OPT_CODE_TABLE)) ?? .unicode }
        set { setOption(VK_OPT_CODE_TABLE, newValue.rawValue) }
    }

    // MARK: Typing

    func handleKey(_ keyCode: UInt16, modifiers: KeyModifiers) -> EngineResult {
        convert(vk_handle_key(engine, keyCode, modifiers.rawValue, VK_EVENT_KEY_DOWN))
    }

    func mouseDown() { _ = vk_handle_key(engine, 0, 0, VK_EVENT_MOUSE_DOWN) }

    func newSession() { vk_new_session(engine) }

    func tempOffSpelling() { vk_temp_off_spelling(engine) }

    func tempOffEngine() { vk_temp_off_engine(engine, 1) }

    func decode(_ word: UInt32) -> [UInt16] {
        var out: (UInt16, UInt16) = (0, 0)
        let count = withUnsafeMutableBytes(of: &out) { raw in
            vk_word_decode(engine, word, raw.baseAddress!.assumingMemoryBound(to: UInt16.self))
        }
        switch count {
        case 1: return [out.0]
        case 2: return [out.0, out.1]
        default: return []
        }
    }

    /// Character a plain key types (US layout), or nil for keys like Tab or the arrows.
    static func character(keyCode: UInt16, caps: Bool) -> UInt16? {
        let ch = vk_keycode_to_char(UInt32(keyCode) | (caps ? 0x10000 : 0))
        return ch == 0 ? nil : ch
    }

    // MARK: Macro

    @discardableResult
    func addMacro(_ text: String, content: String) -> Bool { vk_macro_add(engine, text, content) != 0 }

    // MARK: Private

    private func convert(_ r: vk_result) -> EngineResult {
        guard let code = EngineResult.Code(rawValue: r.code) else { return .nothing }
        var words: [UInt32]
        if code == .replaceMacro {
            let total = Int(r.macro_total)
            words = [UInt32](repeating: 0, count: total)
            let got = words.withUnsafeMutableBufferPointer { vk_last_macro_words(engine, 0, $0.baseAddress, total) }
            words.removeSubrange(got...)
        } else {
            words = withUnsafeBytes(of: r.chars) { Array($0.bindMemory(to: UInt32.self).prefix(Int(r.char_count))) }
        }
        return EngineResult(code: code, backspaceCount: Int(r.backspace_count), words: words, extCode: r.ext_code)
    }
}
