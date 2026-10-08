# Architecture Decision Records

Short ADRs for decisions not covered by CLAUDE.md. Newest last.

## ADR-001 — Where CLAUDE.md and the design doc disagree, CLAUDE.md wins
Status: accepted. Example: paced delay defaults are 3 ms (terminal) / 8 ms (terminal inside IDE).

## ADR-002 — Policy schema gains `axDescriptionContains`
Status: accepted. The design doc uses it for terminals embedded in IDEs; CLAUDE.md §4.5 omits it.
Rule order is kept: terminal-in-JetBrains (paced) is evaluated before generic `com.jetbrains.*` (selection).
Terminal-in-IDE rules are finalised only after `Tools/axdump` output (M1).

## ADR-003 — Input Monitoring permission
Status: accepted. Check with `CGPreflightListenEventAccess()`; only call
`CGRequestListenEventAccess()` when it is actually missing; onboarding shows this step only when needed.

## ADR-004 — G1 snapshots cover the 5 existing code tables only
Status: accepted. VIQR/VISCII/NCR arrive in M4.

## ADR-005 — Snapshots record original engine behaviour, bugs included
Status: accepted. Known engine bugs are tagged `[known-bug]` in the snapshot and fixed
in a later, separate commit that updates the snapshot with a reason.

## ADR-006 — Work split: Linux session does M0 steps 1–5, macOS machine does the rest
Status: accepted. The Linux session delivers engine + C API + Catch2 + ubuntu CI (PR "M0a").
Swift/Xcode work (M0 step 6 onward) is done on a Mac. The macOS CI job is committed with `if: false`
until Swift code exists.

## ADR-007 — Guard an out-of-bounds read in `checkForStandaloneChar`
Status: accepted. The original reads `TypingWord[_index - 1]` with `_index == 0` (index −1, found with
UBSan) when the first key of a word is `w`/`[`/`]`. The value is whatever sits before the array, so the
result depended on memory layout. Added `_index > 0 &&`. This is the only logic-adjacent change of the
refactor; the snapshot is unchanged (garbage never matched in the original either).

## ADR-008 — C API deviations from CLAUDE.md §3
Status: accepted. Same shape as the spec, with these precisions:
- `chars[]` holds **engine words** in display order, not UTF-32. A word is a key code or a character code of
  the *current code table* (VNI/TCVN3/CP1258 use two bytes, compound uses base + mark). Decoding needs the
  table, so `vk_word_decode()` does it exactly like OpenKey.mm `SendNewCharString`; the Swift bridge only
  handles `_syncKey`/chunking. `vk_keycode_to_char()` covers plain keys.
- Macros can be longer than 64 words (spec requires 200 characters): `vk_result.macro_total` + `vk_last_macro_words()`.
- `backspace_count`/`char_count` are 0 for `VK_DO_NOTHING` (the engine leaves stale values there).
  `ext_code` keeps OpenKey's meaning (1 break, 2 delete, 3 normal key, 4 no empty char first).
- English mode is handled inside `vk_handle_key` (macro-only path, like OpenKey.mm). Mouse down =
  `VK_EVENT_MOUSE_DOWN`. Modifiers are a small bitmask (`VK_MOD_*`).
- `vk_set_option` resets the typing state (OpenKey re-ran `vKeyInit` after option changes) and re-encodes
  macros when the code table changes.
- Platform-only globals (`vSwitchKeyStatus`, `vTempOffSpelling`, `vFixRecommendBrowser`, `vUseSmartSwitchKey`,
  `vRememberCode`, `vOtherLanguage`, `vTempOffOpenKey`, convert hotkey/alert flags) are not engine state and
  were dropped; Swift owns them.
- `vk_convert_ex` adds the case/mark flags the convert tool needs; `vk_convert` is the spec signature.

## ADR-009 — Static language tables stay global
Status: accepted. `_vowel`, `_codeTable`, … are initialised once and treated as read-only data; the original
code uses `operator[]` on them, so they are not `const`. Making them const would touch dozens of lines of
engine logic for no behavioural gain; instead no code inserts into them (verified by the snapshot and
ASan/UBSan). The only lazily-built table (`keyCodeToCharacter`) is now a function-local static
(thread-safe). Engine *state* has no globals; two engines are independent (`api_test.cpp`).
Revisit when hardening in M4.

## ADR-010 — How the original is kept as the test oracle
Status: accepted. A frozen, unmodified copy lives in `Engine/tests/original/`. `gen_snapshot_original`
runs each case in a forked process (the original keeps state in globals) and `ctest` verifies the committed
`engine.snap` equals its output, so the oracle cannot silently drift. Intentional behaviour changes go to
`snapshots/overrides.snap`, never into `engine.snap`. `backspace`/`char_count` of non-processing results
are normalised in the harness (not part of the contract, see ADR-008).

## ADR-011 — Catch2 via CMake FetchContent
Status: accepted. Catch2 v3.7.1, test targets only, pinned tag; no vendoring. Not linked into the app.

## ADR-012 — Engine builds as a CMake static library; Xcode consumes it separately
Status: accepted. `Engine/CMakeLists.txt` builds `viskey_engine` + tests on Linux and macOS. The Xcode
project (XcodeGen, done on the Mac) compiles `Engine/src/*.cpp` directly with `Engine/include` on the
header path and a module map in `VisKey/Bridge/`. The CMake build is the source of truth for engine
tests only.

## ADR-013 — Xcode project from XcodeGen, generated project not committed
Status: accepted. `project.yml` is the source of truth; `VisKey.xcodeproj` is generated (`xcodegen generate`) and
git-ignored, CI generates it too. The engine sources are compiled into the app target with `-w` (OpenKey code,
unchanged per ADR-009/012); Swift imports the C API through `VisKey/Bridge/module.modulemap` (`import VisKeyEngine`).
Unit tests are hosted in the app (`@testable import VisKey`); the app skips tap and permission setup when
`XCTestConfigurationFilePath` is set. The test bundle targets macOS 14 because XCTest itself does; the app targets 13.
Swift language mode 5 (CLAUDE.md §1: Swift 5.9+) to keep the C callback and AppKit code free of strict-concurrency noise.

## ADR-014 — M0 typing pipeline details
Status: accepted (M0); revisit in M1 with ContextResolver/AppPolicyStore.
- The tap runs on the main run loop and the engine is only used there (like OpenKey). EventSender posts on its own serial queue.
- Key ordering: while a replacement is still being posted, later keyDown/keyUp events are swallowed and re-posted
  (marked) behind it on the same queue, so a fast key can never overtake a pending replacement. Events with ⌘ are
  never swallowed or re-posted (⌘Space, shortcuts).
- The modifier-only hotkey passes its flagsChanged events through; OpenKey swallowed the release, which can leave
  apps believing a modifier is still down.
- `_syncKey` is kept in `ReplacementPlanner`. OpenKey's list of apps that delete a base letter + combining mark with
  one backspace (`_unicodeCompoundApp`) is not hard-coded: `backspaceDeletesCluster` stays false until it becomes
  policy data in M1. Only matters for the Unicode compound table, which the M0 menu does not offer.
- The empty-character autocomplete workaround (`vFixRecommendBrowser`) and Shift+← selection are M1 strategies.

## ADR-015 — App assets generated from design/ with system tools only
Status: accepted. `Tools/gen-assets.sh` runs `gen-colors.py`, copies `design/icons/menubar-*.svg` into template
image sets as vectors (`preserves-vector-representation`), and renders `AppIcon.appiconset` with Quick Look
(`qlmanage`, WebKit's SVG renderer, which supports the icon's gradient and drop shadow) plus `sips`: 16 pt @1x from
`appicon-hinted-16.svg`, 16 @2x and 32 @1x from `appicon-small.svg`, larger sizes from `appicon-1024.svg`
(brand.md §3). No new dependency. Generated PNGs are committed.

## ADR-016 — Local signing through an xcconfig with an optional override
Status: accepted. Ad-hoc signing changes the code hash on every build, so macOS drops the Accessibility
permission after each rebuild. `Config/Signing.xcconfig` keeps ad-hoc as the default (CI and machines without a
certificate) and includes an optional, git-ignored `Config/Signing.local.xcconfig` where a developer sets
`CODE_SIGN_IDENTITY = Apple Development` and `DEVELOPMENT_TEAM`; the permission then survives rebuilds.
`project.yml` must not set `CODE_SIGN_*` itself, because target/project settings override xcconfig values.
