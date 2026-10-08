# VisKey — Project spec cho Claude Code

## 0. Tóm tắt
VisKey là bộ gõ tiếng Việt native cho macOS, menu bar app, fork từ OpenKey (github.com/tuyenvm/OpenKey).
Giữ nguyên engine C++ của OpenKey, viết lại toàn bộ lớp platform macOS bằng Swift/SwiftUI.
Mục tiêu số 1: gõ đúng trên Spotlight, Terminal + TUI (Claude Code, Codex CLI, vim), address bar trình duyệt, Excel, JetBrains — những nơi OpenKey đang lỗi (issue #315, #319).

- License: **GPL-3.0** (bắt buộc, kế thừa OpenKey). Mọi file nguồn có header SPDX `GPL-3.0-or-later`. File engine giữ copyright gốc + dòng "Modified by VisKey contributors".
- Ghi công: README, About, LICENSE/NOTICE ghi "Based on OpenKey by Mai Vũ Tuyên".
- Thương hiệu: VisKey, by 1VISION. Bundle ID: `work.1vision.viskey`.
- Thiết kế UI + brand: `design/` (nguồn sự thật cho giao diện; spec này là nguồn sự thật cho hành vi).

## 1. Ràng buộc kỹ thuật
- macOS 13 Ventura+, universal binary (arm64 + x86_64).
- Swift 5.9+, SwiftUI cho UI, AppKit khi SwiftUI không đủ (NSStatusItem, NSEvent). Engine C++17.
- Không App Sandbox (event tap không chạy được trong sandbox). Hardened Runtime bật.
- Dependency cho phép: Sparkle 2 (auto-update), Catch2 (test engine, chỉ trong test target). Không dependency nào khác nếu chưa được tôi duyệt.
- Không telemetry, không network ngoài Sparkle appcast.
- Code, comment, commit, tên biến: tiếng Anh. UI strings: tiếng Việt (mặc định) + tiếng Anh, qua String Catalog (`Localizable.xcstrings`).

## 2. Cấu trúc repo
```
viskey/
├─ Engine/                    # C++ từ OpenKey (import bằng git subtree, giữ lịch sử)
│  ├─ src/                    # Engine.cpp Vietnamese.cpp Macro.cpp SmartSwitchKey.cpp ConvertTool.cpp
│  ├─ include/viskey_engine.h # C API mới
│  └─ tests/                  # Catch2
├─ VisKey/
│  ├─ Core/       KeyTap.swift ContextResolver.swift ReplacementPlanner.swift EventSender.swift
│  ├─ Policy/     AppPolicyStore.swift builtin-policies.json
│  ├─ Bridge/     EngineBridge.swift + module.modulemap
│  ├─ Features/   Macro/ ConvertTool/ SmartSwitch/ Hotkey/ Clipboard/
│  ├─ UI/         MenuBar/ Settings/ Onboarding/ ConvertWindow/
│  ├─ App/        VisKeyApp.swift AppDelegate.swift LoginItem.swift Updater.swift Permissions.swift
│  └─ Resources/  Assets.xcassets Localizable.xcstrings
├─ VisKeyTests/               # XCTest cho Planner, Policy, Bridge
├─ Tools/axdump/              # CLI in AX role/subrole của ô đang focus
├─ Scripts/                   # build-release.sh notarize.sh make-dmg.sh
├─ .github/workflows/         # ci.yml (test mỗi push), release.yml (tag v*)
├─ design/                    # export từ Claude Design
└─ docs/                      # decisions.md, app-compat.md, CONTRIBUTING.md
```
Dùng Xcode project được sinh bằng XcodeGen (`project.yml`) để diff được trong git. Nếu XcodeGen không có, dùng `.xcodeproj` thường và ghi ADR.

## 3. Engine (C++, giữ từ OpenKey)
Nguồn: `Sources/OpenKey/engine/` của OpenKey (Engine.cpp ~1558 dòng, Vietnamese.cpp, Macro.cpp, SmartSwitchKey.cpp, ConvertTool.cpp, DataType.h).
Engine hiện dùng biến global (`vLanguage`, `vCodeTable`, `pData`...). Refactor CƠ HỌC để gói state vào `struct vk_engine`, KHÔNG đổi logic.

### C API (`viskey_engine.h`)
```c
typedef struct vk_engine vk_engine;
typedef struct {
    uint8_t  code;            // DoNothing / WillProcess / Restore / RestoreAndStartNewSession / ReplaceMacro
    uint8_t  backspace_count;
    uint8_t  char_count;
    uint32_t chars[64];       // UTF-32, đúng thứ tự hiển thị (OpenKey trả ngược, bridge đảo lại)
    uint8_t  ext_code;
} vk_result;

vk_engine* vk_create(void);
void       vk_destroy(vk_engine*);
void       vk_set_option(vk_engine*, vk_option key, int32_t value);
vk_result  vk_handle_key(vk_engine*, uint16_t keycode, uint32_t modifiers, vk_event_kind kind);
void       vk_new_session(vk_engine*);
void       vk_macro_load(vk_engine*, const uint8_t* blob, size_t len);
size_t     vk_convert(vk_engine*, const char* utf8, size_t len, vk_code_table from, vk_code_table to, char* out, size_t cap);
```

### Test engine (gate G1)
- Trước khi refactor: viết harness chạy trên engine OpenKey GỐC, ghi snapshot (chuỗi phím → backspace_count, chars) cho ≥ 300 ca.
- Ca bắt buộc: "quởn", "tuyệt", "quét", "dui9", "duoi96", "tuyps", "chưa"+"a" (issue #312), hoà/khoẻ/thuỷ, Quick Telex (cc, gg, kk, nn, qq, pp, tt), phụ âm đầu f/j/w, phụ âm cuối g/h/k, khôi phục từ sai (text, expect, user), khôi phục dấu khi xoá ký tự ("tuỳa" xoá "a"), VNI đa byte, Unicode tổ hợp, macro dài 200 ký tự, viết hoa đầu câu, Simple Telex.
- Sau refactor: snapshot phải giống hệt byte-by-byte. Test chạy được trên Linux (CI ubuntu) và macOS.

## 4. Lớp platform macOS — luồng xử lý phím
```
KeyTap → ContextResolver → EngineBridge → ReplacementPlanner → EventSender → app đích
              ↑                                   ↑
        AppPolicyStore ───────────────────────────┘
```

### 4.1 KeyTap
- `CGEvent.tapCreate(tap: .cgSessionEventTap, place: .headInsertEventTap, options: .defaultTap, ...)`, mask: keyDown, keyUp, flagsChanged, leftMouseDown, rightMouseDown, leftMouseDragged, rightMouseDragged.
- Bỏ qua event của chính mình: kiểm tra `eventSourceUserData == 0x5649534B` ("VISK").
- Xử lý `tapDisabledByTimeout` / `tapDisabledByUserInput` → re-enable ngay, log.
- Hotkey bật/tắt: hỗ trợ modifier-only (mặc định Ctrl+Shift) qua flagsChanged, giữ logic `_lastFlag` của OpenKey; hotkey phụ tuỳ chọn. Không bao giờ nuốt Cmd+Space.
- Click chuột / đổi app → `vk_new_session`.
- Callback phải trả về < 1 ms: chỉ gọi engine + enqueue cho EventSender, rồi `return nil` khi đã xử lý.

### 4.2 ContextResolver
- Lấy element đang focus: `AXUIElementCopyAttributeValue(AXUIElementCreateSystemWide(), kAXFocusedUIElementAttribute)`; đọc `AXRole`, `AXSubrole`, `AXDescription`; lấy PID qua `AXUIElementGetPid` → bundle ID. KHÔNG dùng `frontmostApplication` (Spotlight/Raycast/Alfred là overlay, không đổi frontmost app).
- Cache kết quả 100 ms hoặc đến khi đổi app/click.
- Thứ tự quyết định strategy:
  1. User override cho bundle ID → dùng luôn.
  2. `AXRole == AXComboBox` hoặc `AXSubrole == AXSearchField` → `selection`.
  3. Nhóm terminal (`com.apple.Terminal`, `com.googlecode.iterm2`, `com.mitchellh.ghostty`, `dev.warp.Warp-Stable`, `com.github.wez.wezterm`, `net.kovidgoyal.kitty`, `org.alacritty`) → `paced`. Terminal tích hợp trong VS Code/Cursor/JetBrains (nhận diện qua AX nếu được) → `paced` delay 8 ms.
  4. `com.jetbrains.*`, `com.microsoft.Excel`, `com.raycast.macos`, `com.runningwithcrayons.Alfred` → `selection`.
  5. Chromium/Electron (Chrome, Edge, Arc, Brave, Slack, Discord, Notion, VS Code editor) → `backspace` chunk 4.
  6. Còn lại → `backspace`.
- AX lỗi/không có → fallback kiểm tra cửa sổ Spotlight qua `CGWindowListCopyWindowInfo` (như OpenKey), rồi `backspace`.
- **Việc đầu tiên của M1:** viết `Tools/axdump` (CLI Swift, in role/subrole/description/bundle mỗi 500 ms) để tôi chạy trên Spotlight (macOS 15 và 26), Chrome address bar, Terminal, VS Code terminal, IntelliJ — rule ở trên phải được xác nhận bằng output thật trước khi chốt.

### 4.3 ReplacementPlanner — 3 chiến lược
| Strategy | Chuỗi event | Delay mặc định | Chunk |
|---|---|---|---|
| backspace | BS×N → unicode(string) | 0 ms | 16 (Cocoa), 4 (Chromium/Electron) |
| selection | (Shift+←)×N → unicode(string) | 0 ms | 16 |
| paced | BS, sleep, BS, sleep… → unicode(ký tự 1), sleep, unicode(ký tự 2)… | 3 ms (terminal), 8 ms (terminal trong IDE), user chỉnh 0–30 ms | 1 |

- Giữ logic `_syncKey` của OpenKey cho VNI/Unicode tổ hợp (một ký tự có thể là 2 code point → số backspace thật khác nhau).
- Giữ các case macro, restore, restoreAndStartNewSession, gửi phím gốc sau restore.

### 4.4 EventSender
- Một `DispatchQueue` serial `.userInteractive`. Mỗi lần thay thế là một transaction; phím mới đến khi transaction chưa xong thì xếp hàng sau, không chạy song song.
- `CGEventSource(stateID: .privateState)`; mọi event gán `eventSourceUserData = 0x5649534B`; flags = 0 (không dính Shift/Caps) trừ Shift+← của selection.
- Post bằng `event.post(tap: .cgSessionEventTap)` (cho phép post ngoài callback, có delay).
- Unicode ngoài BMP: tách surrogate pair đúng cách.

### 4.5 AppPolicyStore
- Built-in: `builtin-policies.json` trong bundle. User override: `~/Library/Application Support/VisKey/policies.json`, merge đè lên built-in.
- Schema:
```json
{ "version": 1,
  "rules": [
    { "match": { "axRole": "AXComboBox" }, "strategy": "selection" },
    { "match": { "axSubrole": "AXSearchField" }, "strategy": "selection" },
    { "match": { "bundlePrefix": "com.jetbrains." }, "strategy": "selection" },
    { "match": { "bundle": "com.apple.Terminal" }, "strategy": "paced", "delayMs": 3 },
    { "match": { "bundle": "com.google.Chrome" }, "strategy": "backspace", "chunk": 4 },
    { "match": { "bundle": "com.adobe.Photoshop" }, "codeTable": "unicodeCompound" }
  ] }
```
- Mỗi app có thêm trong override: `vietnamese` (on/off/remember), `excluded` (bool).
- Validate JSON khi load; rule lỗi → bỏ qua + log, không crash.

## 5. Tính năng (v1.0)
Kế thừa OpenKey: Telex, VNI, Simple Telex; bảng mã Unicode, TCVN3, VNI Windows, Unicode tổ hợp, CP1258; đặt dấu oà/uý; kiểm tra chính tả; khôi phục từ sai; Quick Telex; cho phép f z w j; phụ âm đầu/cuối gõ tắt; viết hoa đầu câu; macro không giới hạn (cả trong chế độ tiếng Anh); chuyển mã văn bản + hotkey chuyển mã clipboard; smart switch VI/EN theo app; nhớ bảng mã theo app; tạm tắt bằng Cmd/Alt; tạm tắt chính tả bằng Ctrl; tự tắt khi input source không phải Latin (lắng nghe `kTISNotifySelectedKeyboardInputSourceChanged`).

Mới ở M4 (P1): VIQR (kiểu gõ + bảng mã), VISCII, NCR decimal/hex; bỏ dấu tự do; ESC khôi phục từ vừa gõ; loại trừ app thủ công; công cụ clipboard (HOA/thường, bỏ dấu); import macro từ file Unikey (`.txt` dạng `tắt:nội dung`), EVKey, OpenKey.

## 6. UI (theo design/)
- Menu bar: `NSStatusItem` với template image VI / EN / tạm tắt-thiếu quyền. Menu dropdown theo design.
- Settings (SwiftUI `Settings` scene hoặc `NavigationSplitView` sidebar, ~720×520 pt): Chung · Gõ tiếng Việt · Gõ tắt · Ứng dụng · Chuyển mã · Nâng cao · Giới thiệu.
  - Tab **Ứng dụng** là quan trọng nhất: bảng app (icon, tên, Tiếng Việt, Chiến lược [Tự động/Backspace/Selection/Paced], Độ trễ slider 0–30 ms, Bảng mã), badge "Tối ưu sẵn" cho app có rule built-in, phần nâng cao ẩn sau disclosure, nút "Báo lỗi app này" copy vào clipboard: bundle ID, version app, AX role/subrole, strategy đang dùng, macOS version, VisKey version.
- Onboarding: chào mừng → quyền Accessibility (nút mở `x-apple.systempreferences:com.apple.preference.security?Privacy_Accessibility`, poll `AXIsProcessTrusted()` mỗi 1 s, tự khởi động tap khi được cấp, không bắt restart) → chọn kiểu gõ + hotkey → cảnh báo nếu phát hiện OpenKey/EVKey/Gõ Nhanh đang chạy → ô thử gõ.
- Mất quyền khi đang chạy → icon chuyển trạng thái thiếu quyền, thông báo, không crash, tự phục hồi khi cấp lại.
- Light + dark mode, control native theo HIG, Dynamic Type không bắt buộc. Mọi chuỗi tiếng Việt đúng chính tả.
- Login item: `SMAppService.mainApp`. Dock icon tuỳ chọn (`NSApp.setActivationPolicy`).

## 7. Lưu trữ
- Settings: một struct `Settings: Codable` trong `UserDefaults` suite `work.1vision.viskey`.
- Macro: JSON trong Application Support, ghi atomic.
- Smart switch: dictionary bundleID → (language, codeTable), LRU 500 entry.
- Migration: nếu phát hiện cài đặt OpenKey (`com.tuyenmai.openkey` defaults) → đề nghị import macro + cài đặt khi onboarding.

## 8. Build, sign, phân phối
- `Scripts/build-release.sh`: xcodebuild archive universal → export Developer ID.
- `Scripts/notarize.sh`: `xcrun notarytool submit --wait` + `xcrun stapler staple` cho .app và .dmg.
- `Scripts/make-dmg.sh`: DMG với nền từ `design/`.
- Sparkle 2: EdDSA, appcast trên GitHub Pages (`gh-pages`), kiểm tra mỗi 24 h.
- CI: `ci.yml` (mỗi push: test engine trên ubuntu + macos, build Debug + XCTest trên macos-15); `release.yml` (tag `v*`: build, sign, notarize, DMG, appcast, GitHub Release). Secrets: `DEVELOPER_ID_P12`, `DEVELOPER_ID_P12_PASSWORD`, `NOTARY_APPLE_ID`, `NOTARY_TEAM_ID`, `NOTARY_PASSWORD`, `SPARKLE_ED_KEY`. Claude Code viết workflow, tôi tự thêm secret.
- Homebrew cask: chuẩn bị file `viskey.rb` mẫu trong `docs/`.

## 9. Hiệu năng (phải đo, không đoán)
- Callback tap p99 < 1 ms; latency phím → ký tự < 5 ms (backspace), < 15 ms (paced).
- RAM < 30 MB, CPU idle ~0%, app < 15 MB.
- Đo bằng `os_signpost` (subsystem `work.1vision.viskey`, category `latency`).

## 10. Roadmap + gate (dừng ở mỗi gate chờ tôi)
| Milestone | Nội dung | Gate |
|---|---|---|
| M0 | Repo, git subtree engine, harness snapshot trên engine gốc, refactor C API, Catch2, KeyTap Swift tối thiểu (backspace strategy), menu bar icon VI/EN | **G1**: ≥ 300 snapshot giống hệt; gõ được trên TextEdit, Notes, Safari body |
| M1 | Tools/axdump, ContextResolver, ReplacementPlanner 3 strategy, EventSender queue, AppPolicyStore + builtin rules, XCTest cho Planner/Policy | **G2**: checklist tương thích (mục 11) đạt 100% với app tôi có; tôi dùng hàng ngày 1 tuần |
| M2 | Toàn bộ UI SwiftUI theo design, onboarding, quyền, smart switch, hotkey, macro, convert tool, login item, localization | **G3**: mọi màn hình khớp design; mọi tính năng OpenKey hoạt động |
| M3 | Sign, notarize, DMG, Sparkle, CI release, README + CONTRIBUTING + docs/app-compat.md | **G4**: DMG notarized cài trên máy sạch không cần `xattr`; update Sparkle từ bản trước sang bản sau chạy được |
| M4 | Tính năng P1 (mục 5) | **G5**: test tay P1 + toàn bộ checklist lại → tag v1.0.0 |

## 11. Checklist tương thích (test tay, macOS 15 và 26)
- Spotlight: gõ "hộ chiếu", "trường đại học" khi có gợi ý inline → không đúp, không mất chữ
- Raycast, Alfred
- Address bar + ô tìm Google: Chrome, Safari, Arc, Firefox, Edge
- Terminal.app, iTerm2, Ghostty, Warp ở zsh prompt
- Claude Code (bản < 2.1.108 và bản mới nhất), Codex CLI, vim insert mode, lazygit commit message
- VS Code editor + terminal tích hợp, Cursor, Zed, Xcode, IntelliJ khi popup autocomplete mở
- Excel cell có autocomplete, Word, PowerPoint, Pages, Numbers
- Slack, Discord, Notion, Obsidian, Telegram, Zalo, Messages, Mail
- Google Docs, Figma text, Canva
- Gõ nhanh 2 phút vào TextEdit: 0 ký tự sai
- Macro 200 ký tự; macro trong chế độ tiếng Anh
- Hotkey modifier-only, ESC restore, tạm tắt Cmd/Alt, đổi sang bàn phím Nhật rồi quay lại
- Thu hồi quyền Accessibility khi đang chạy → không crash, tự phục hồi
- Sleep/wake, fast user switching → tap vẫn hoạt động

Kết quả ghi vào `docs/app-compat.md` (app · version · strategy · kết quả · ghi chú).

## 12. Nguyên tắc
1. Engine không biết gì về platform; mọi thứ liên quan app/AX/delay nằm ở Swift.
2. Mặc định đúng cho 90% app (backspace, không delay); chỉ bật selection/paced khi context yêu cầu.
3. Per-app policy là dữ liệu (JSON), không hard-code trong code.
4. Mọi event gửi đi qua một serial queue duy nhất.
5. Không global mutable state chia sẻ giữa C++ và Swift.
6. Ưu tiên đơn giản, đọc được, test được hơn là thông minh.

## 13. Tham chiếu
- OpenKey: github.com/tuyenvm/OpenKey (engine + `Sources/OpenKey/macOS/ModernKey/OpenKey.mm` để hiểu hành vi gốc)
- Gõ Nhanh: github.com/khaphanspace/gonhanh.org — docs/system-architecture.md (tham khảo cách tiếp cận AX detection; KHÔNG copy code, khác license)
- Lỗi Claude Code: github.com/manhit96/claude-code-vietnamese-fix (patcher.py giải thích bug `\x7f` chunk)