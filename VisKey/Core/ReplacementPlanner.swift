// SPDX-License-Identifier: GPL-3.0-or-later
//
// Turns an engine result into the events to post. M0 implements the `backspace` strategy only;
// `selection` and `paced` arrive with ContextResolver in M1 (CLAUDE.md §4.3).
//
// Mirrors OpenKey.mm (SendBackspace, SendNewCharString, handleMacro, the vDoNothing branch), including
// `_syncKey`: with VNI Windows and Unicode compound one visible character can be two UTF-16 units, so the
// planner remembers how many units each character of the current word took and sends a second backspace
// where needed.

/// One step for EventSender, posted in order.
enum OutputEvent: Equatable {
    case backspace
    /// One key event carrying a Unicode string.
    case text([UInt16])
    /// A real key press, e.g. the key that ended a macro or a Tab after a restored word.
    case key(code: UInt16, shift: Bool)
}

struct Replacement: Equatable {
    var events: [OutputEvent]
    /// The original key event still reaches the app, after `events`.
    var passThrough: Bool

    static let passThrough = Replacement(events: [], passThrough: true)
}

/// The key being handled, as the planner needs it.
struct TypedKey {
    var keyCode: UInt16
    var shift: Bool
    var capsLock: Bool
}

struct ReplacementPlanner {
    var codeTable: CodeTable = .unicode
    /// The app deletes a base letter and its combining mark with one backspace (OpenKey `_unicodeCompoundApp`).
    var backspaceDeletesCluster = false
    /// Maximum UTF-16 units in one text event (16 for Cocoa apps).
    var chunkSize = 16

    /// OpenKey `_syncKey`: UTF-16 units of each character typed in the current word. Double-code tables only.
    private(set) var syncKey: [Int] = []

    /// Mirrors the engine's own limit on deletions for typing results (OpenKey MAX_BUFF).
    private static let maxBackspaces = 32

    mutating func reset() { syncKey.removeAll() }

    mutating func plan(_ result: EngineResult, key: TypedKey, decoder: WordDecoding) -> Replacement {
        switch result.code {
        case .doNothing, .breakWord:
            return planPassThrough(extCode: result.extCode)

        case .willProcess, .restore, .restoreAndStartNewSession:
            var events: [OutputEvent] = []
            if result.backspaceCount > 0 && result.backspaceCount < Self.maxBackspaces {
                for _ in 0..<result.backspaceCount { appendBackspace(to: &events) }
            }
            var glyphs = result.words.compactMap { glyph(for: $0, decoder: decoder) }
            var controlKey: UInt16?
            if result.code != .willProcess {
                // A restored word is followed by the key that was typed; a key without a character
                // (Tab, arrows…) is sent as a real key press after the text.
                if let ch = EngineBridge.character(keyCode: key.keyCode, caps: key.shift || key.capsLock) {
                    glyphs.append([ch])
                } else {
                    controlKey = key.keyCode
                }
            }
            appendText(glyphs, to: &events)
            if let controlKey { appendKey(controlKey, shift: false, to: &events) }
            return Replacement(events: events, passThrough: false)

        case .replaceMacro:
            var events: [OutputEvent] = []
            for _ in 0..<result.backspaceCount { appendBackspace(to: &events) }
            appendText(result.words.compactMap { glyph(for: $0, decoder: decoder) }, to: &events)
            appendKey(key.keyCode, shift: key.shift, to: &events)
            return Replacement(events: events, passThrough: false)
        }
    }

    // MARK: Private

    private mutating func planPassThrough(extCode: UInt8) -> Replacement {
        guard codeTable.isDoubleCode else { return .passThrough }
        var events: [OutputEvent] = []
        switch extCode {
        case 1: // word break
            syncKey.removeAll()
        case 2: // delete key: the app deletes one unit, a two-unit character needs one more
            if let last = syncKey.popLast(), last > 1, codeTable == .vniWindows || !backspaceDeletesCluster {
                events.append(.backspace)
            }
        case 3: // normal key typed as is
            syncKey.append(1)
        default:
            break
        }
        return Replacement(events: events, passThrough: true)
    }

    private mutating func appendBackspace(to events: inout [OutputEvent]) {
        events.append(.backspace)
        guard codeTable.isDoubleCode, let last = syncKey.popLast() else { return }
        if last > 1 && !(codeTable == .unicodeCompound && backspaceDeletesCluster) {
            events.append(.backspace)
        }
    }

    /// Decodes one engine word and records its length. Restored keys are appended without a record, like OpenKey.
    private mutating func glyph(for word: UInt32, decoder: WordDecoding) -> [UInt16]? {
        let units = decoder.decode(word)
        guard !units.isEmpty else { return nil }
        if codeTable.isDoubleCode { syncKey.append(units.count) }
        return units
    }

    private mutating func appendKey(_ keyCode: UInt16, shift: Bool, to events: inout [OutputEvent]) {
        if codeTable.isDoubleCode { syncKey.append(1) }
        events.append(.key(code: keyCode, shift: shift))
    }

    /// Packs glyphs into text events of at most `chunkSize` units without splitting a glyph or a surrogate
    /// pair (macro content outside the BMP arrives as two words, one per surrogate).
    private func appendText(_ glyphs: [[UInt16]], to events: inout [OutputEvent]) {
        var chunk: [UInt16] = []
        for glyph in glyphs {
            if chunk.count + glyph.count > chunkSize, let last = chunk.last {
                let carry = UTF16.isLeadSurrogate(last) && chunk.count > 1
                if carry { chunk.removeLast() }
                events.append(.text(chunk))
                chunk = carry ? [last] : []
            }
            chunk += glyph
        }
        if !chunk.isEmpty { events.append(.text(chunk)) }
    }
}
