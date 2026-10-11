// SPDX-License-Identifier: GPL-3.0-or-later

import Carbon.HIToolbox
import XCTest
@testable import VisKey

/// Words 0x200–0x2FF decode to two units (a VNI/compound "double" character); any other word to itself.
private struct FakeDecoder: WordDecoding {
    func decode(_ word: UInt32) -> [UInt16] {
        (0x200..<0x300).contains(word) ? [UInt16(word & 0xFF), 0x301] : [UInt16(truncatingIfNeeded: word)]
    }
}

final class ReplacementPlannerTests: XCTestCase {
    private let decoder = FakeDecoder()
    private let keyS = TypedKey(keyCode: UInt16(kVK_ANSI_S), shift: false, capsLock: false)

    private func result(_ code: EngineResult.Code, bs: Int = 0, _ words: [UInt32] = [], ext: UInt8 = 3) -> EngineResult {
        EngineResult(code: code, backspaceCount: bs, words: words, extCode: ext)
    }

    func testBackspaceThenTextWithRealEngine() {
        // "quet" + "s" -> "quét": engine asks to delete "et" and type "ét".
        let engine = EngineBridge()
        var planner = ReplacementPlanner()
        for key in [kVK_ANSI_Q, kVK_ANSI_U, kVK_ANSI_E, kVK_ANSI_T] {
            XCTAssertEqual(planner.plan(engine.handleKey(UInt16(key), modifiers: []), key: keyS, decoder: engine), .passThrough)
        }
        let r = planner.plan(engine.handleKey(UInt16(kVK_ANSI_S), modifiers: []), key: keyS, decoder: engine)
        XCTAssertEqual(r, Replacement(events: [.backspace, .backspace, .text(Array("ét".utf16))], passThrough: false))
    }

    func testPassThroughKeepsKeyForUnicode() {
        var planner = ReplacementPlanner()
        XCTAssertEqual(planner.plan(result(.doNothing, ext: 2), key: keyS, decoder: decoder), .passThrough)
        XCTAssertTrue(planner.syncKey.isEmpty, "Unicode does not track lengths")
    }

    func testDoubleCodeCharacterNeedsTwoBackspaces() {
        var planner = ReplacementPlanner()
        planner.codeTable = .vniWindows
        _ = planner.plan(result(.willProcess, [0x241]), key: keyS, decoder: decoder)
        XCTAssertEqual(planner.syncKey, [2])
        let r = planner.plan(result(.willProcess, bs: 1, [0x42]), key: keyS, decoder: decoder)
        XCTAssertEqual(r.events, [.backspace, .backspace, .text([0x42])])
        XCTAssertEqual(planner.syncKey, [1])
    }

    func testCompoundBackspaceInClusterAwareApp() {
        var planner = ReplacementPlanner()
        planner.codeTable = .unicodeCompound
        planner.backspaceDeletesCluster = true
        _ = planner.plan(result(.willProcess, [0x241]), key: keyS, decoder: decoder)
        let r = planner.plan(result(.willProcess, bs: 1, [0x42]), key: keyS, decoder: decoder)
        XCTAssertEqual(r.events, [.backspace, .text([0x42])])
    }

    func testDeleteKeyOnDoubleCharacterSendsExtraBackspace() {
        var planner = ReplacementPlanner()
        planner.codeTable = .vniWindows
        _ = planner.plan(result(.willProcess, [0x241]), key: keyS, decoder: decoder)
        let r = planner.plan(result(.doNothing, ext: 2), key: keyS, decoder: decoder)
        XCTAssertEqual(r, Replacement(events: [.backspace], passThrough: true))
        XCTAssertTrue(planner.syncKey.isEmpty)
    }

    func testDoubleCodeTracksPlainKeysAndWordBreaks() {
        var planner = ReplacementPlanner()
        planner.codeTable = .unicodeCompound
        _ = planner.plan(result(.doNothing, ext: 3), key: keyS, decoder: decoder)
        _ = planner.plan(result(.doNothing, ext: 3), key: keyS, decoder: decoder)
        XCTAssertEqual(planner.syncKey, [1, 1])
        _ = planner.plan(result(.doNothing, ext: 1), key: keyS, decoder: decoder)
        XCTAssertTrue(planner.syncKey.isEmpty)
    }

    func testRestoreAppendsTypedCharacter() {
        var planner = ReplacementPlanner()
        let shiftedS = TypedKey(keyCode: UInt16(kVK_ANSI_S), shift: true, capsLock: false)
        let r = planner.plan(result(.restore, bs: 2, [0x61, 0x62]), key: shiftedS, decoder: decoder)
        XCTAssertEqual(r.events, [.backspace, .backspace, .text([0x61, 0x62, UInt16(UInt8(ascii: "S"))])])
    }

    func testRestoreWithControlKeySendsKeyAfterText() {
        var planner = ReplacementPlanner()
        let tab = TypedKey(keyCode: UInt16(kVK_Tab), shift: false, capsLock: false)
        let r = planner.plan(result(.restoreAndStartNewSession, bs: 1, [0x61]), key: tab, decoder: decoder)
        XCTAssertEqual(r.events, [.backspace, .text([0x61]), .key(code: UInt16(kVK_Tab), shift: false)])
    }

    func testMacroIsChunkedAndEndsWithTypedKey() {
        var planner = ReplacementPlanner()
        let space = TypedKey(keyCode: UInt16(kVK_Space), shift: true, capsLock: false)
        let words = (0..<40).map { UInt32(0x41 + $0 % 26) }
        let r = planner.plan(result(.replaceMacro, bs: 3, words), key: space, decoder: decoder)
        let texts = r.events.compactMap { if case .text(let u) = $0 { return u.count } else { return nil } }
        XCTAssertEqual(Array(r.events.prefix(3)), [.backspace, .backspace, .backspace])
        XCTAssertEqual(texts, [16, 16, 8])
        XCTAssertEqual(r.events.last, .key(code: UInt16(kVK_Space), shift: true))
        XCTAssertFalse(r.passThrough)
    }

    func testChunkNeverSplitsSurrogatePair() {
        var planner = ReplacementPlanner()
        let emoji = Array("😀".utf16).map(UInt32.init) // macro content: one word per UTF-16 unit
        let words = Array(repeating: UInt32(0x61), count: 15) + emoji
        let r = planner.plan(result(.replaceMacro, words), key: keyS, decoder: decoder)
        XCTAssertEqual(r.events.prefix(2), [.text(Array(repeating: 0x61, count: 15)), .text(Array("😀".utf16))])
    }

    func testOversizedBackspaceCountIsIgnoredLikeOpenKey() {
        var planner = ReplacementPlanner()
        let r = planner.plan(result(.willProcess, bs: 32, [0x61]), key: keyS, decoder: decoder)
        XCTAssertEqual(r.events, [.text([0x61])])
    }
}
