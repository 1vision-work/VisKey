# Tài liệu thiết kế — Bộ gõ tiếng Việt native macOS (fork OpenKey)

Oct 8, 2026 · @Richard Ngo

## 1. Tổng quan

**Quyết định:** fork [OpenKey](https://github.com/tuyenvm/OpenKey) (GPL-3.0, C++ engine + Objective-C++ macOS layer), giữ engine gõ, **viết lại hoàn toàn lớp platform macOS** bằng Swift với chiến lược thay thế text theo ngữ cảnh (context-aware text replacement). Đây là cách duy nhất sửa tận gốc lỗi Spotlight và Terminal mà không phải viết lại engine 3.300 dòng đã ổn định 6 năm.

**Tên làm việc:** VisKey (theo tên Project). Bundle ID dự kiến `work.1vision.viskey`.

**Vấn đề cần giải quyết (theo thứ tự ưu tiên):**

1. **Spotlight bị đúp chữ** — issue [#315](https://github.com/tuyenvm/OpenKey/issues/315) (Dec 2025, còn mở). Spotlight autocomplete inline chiếm selection, backspace của OpenKey xoá phần gợi ý thay vì ký tự vừa gõ → chữ bị lặp.
2. **Terminal / Claude Code mất chữ, mất dấu cách** — issue [#319](https://github.com/tuyenvm/OpenKey/issues/319) (Mar 2026, còn mở). Backspace và ký tự thay thế tới cùng một stdin chunk; TUI (Ink) xử lý backspace rồi bỏ phần còn lại.
3. **OpenKey gốc ngừng phát triển** — bản 2.0.5 (Homebrew), commit cuối Jun 2026, 139 issue mở, tác giả không phản hồi. Mã platform macOS dùng API 2019, chưa có notarization, chưa hỗ trợ macOS 26 Tahoe (Spotlight mới).
4. **Thiếu tính năng Unikey** mà người dùng quen: VIQR, bảng mã VISCII/VPS/BK HCM, chuyển mã clipboard bằng hotkey, bỏ dấu tự do, loại trừ app thủ công (issue [#321](https://github.com/tuyenvm/OpenKey/issues/321)).

**Phạm vi v1.0:**

- macOS 13 Ventura trở lên, Apple Silicon + Intel (universal binary), build native Swift 5.9 / SwiftUI, không Electron, không dependency ngoài.
- Gõ đúng trên: Spotlight, Terminal.app, iTerm2, Ghostty, Warp, Claude Code / Codex CLI trong terminal, VS Code / Cursor terminal, Chrome/Safari/Arc address bar, Excel, JetBrains, Slack, Notion, Zalo, Messages.
- Đủ tính năng OpenKey hiện có + nhóm tính năng Unikey ở mục 4.
- Phân phối: DMG notarized + Homebrew cask, auto-update bằng Sparkle.

**Ngoài phạm vi v1.0:** Windows/Linux, App Store (event tap không qua được sandbox), gõ tiếng Việt trong game full-screen, đồng bộ cài đặt qua iCloud.

**Ràng buộc license:** OpenKey là GPL-3.0 → VisKey bắt buộc open source GPL-3.0 nếu phân phối, ghi rõ nguồn gốc OpenKey. Không thể bán closed-source; mô hình kinh doanh (nếu có) là donate / sponsor / dịch vụ, không phải bán license.

## 2. OpenKey hiện tại — kết quả đọc source

OpenKey tách rõ hai tầng: **engine C++ thuần** (không phụ thuộc OS, 3.259 dòng) và **lớp platform macOS** (Objective-C++, file chính `OpenKey.mm` 795 dòng). Toàn bộ lỗi Spotlight/Terminal nằm ở lớp platform; engine không cần đụng.

| Thành phần | File | Dòng | Vai trò | Giữ lại? |
| --- | --- | --- | --- | --- |
| Engine gõ | `engine/Engine.cpp` | 1.558 | State machine Telex/VNI/Simple Telex, kiểm tra chính tả, bỏ dấu, restore từ sai | Giữ nguyên, bọc C API |
| Bảng tiếng Việt | `engine/Vietnamese.cpp` | 576 | Bảng nguyên âm, phụ âm, 5 bảng mã (Unicode, TCVN3, VNI, Unicode tổ hợp, CP1258) | Giữ, mở rộng thêm VIQR/VISCII/VPS |
| Macro | `engine/Macro.cpp` | 293 | Gõ tắt không giới hạn độ dài | Giữ |
| Smart switch | `engine/SmartSwitchKey.cpp` | 73 | Nhớ VI/EN + bảng mã theo bundle ID | Giữ |
| Convert tool | `engine/ConvertTool.cpp` | 180 | Chuyển mã văn bản | Giữ |
| Hook macOS | `macOS/ModernKey/OpenKey.mm` | 795 | CGEventTap callback, gửi backspace + unicode, workaround từng app | **Viết lại** |
| Quản lý tap | `macOS/ModernKey/OpenKeyManager.m` | — | `CGEventTapCreate(kCGSessionEventTap, ...)`, xin quyền Accessibility | **Viết lại** |
| UI | `ViewController.m`, `MacroViewController.mm`, xib | — | AppKit + Interface Builder, giao diện 2019 | **Viết lại** bằng SwiftUI |

**Cơ chế gõ (“kỹ thuật Backspace”):** mỗi keyDown đi qua `OpenKeyCallback` → engine `vKeyHandleEvent` trả về `pData` gồm `backspaceCount`, `newCharCount`, `charData[]`. Lớp platform post thẳng vào tap bằng `CGEventTapPostEvent(proxy, ...)`: N lần Backspace, rồi một event `CGEventKeyboardSetUnicodeString` chứa tối đa 16 ký tự (chunk 16 nếu dài hơn). Event của chính mình được lọc bằng `kCGEventSourceStateID` so với `myEventSource` (private source). Không có bất kỳ delay nào giữa backspace và chuỗi mới.

**Workaround theo app hiện có (OpenKey.mm dòng 178–197, 740–775):**

- `isSpotlightVisible()` quét `CGWindowListCopyWindowInfo` tìm cửa sổ có owner name `"Spotlight"` → nếu thấy thì thay backspace bằng `Shift+Left` × N (selection replacement). Đây là điểm gãy trên macOS 26: Spotlight được thiết kế lại, cửa sổ không còn khớp tên cũ, hàm trả `false`, OpenKey rơi về backspace và xoá nhầm phần autocomplete.
- `vFixRecommendBrowser` (“Sửa lỗi autocorrect”): gửi một ký tự rỗng rồi backspace thêm 1 để phá gợi ý — hack theo tên app, không theo loại ô nhập.
- `vSendKeyStepByStep`: gửi từng ký tự thay vì chuỗi — là cách duy nhất để người dùng “chữa” app không tương thích, phải bật tay, áp dụng toàn cục.
- Danh sách bundle ID hard-code: `_unicodeCompoundApp` (Chromium), `_recommendWorkaroundDisabledApp` (`com.apple.Spotlight`).

**Những gì còn tốt và nên kế thừa:** engine đã xử lý đúng các ca khó (“quởn”, “tuyết”, khôi phục dấu khi xoá ký tự, VNI đa byte), Quick Telex, viết hoa đầu câu, cơ chế hotkey bằng `kCGEventFlagsChanged` (cho phép hotkey chỉ gồm modifier như Ctrl+Shift), tạm tắt bằng Cmd/Alt, Smart switch theo app.

**Nợ kỹ thuật phải trả:** deployment target 10.14, project Xcode cũ, không có test cho engine, biến global C++ dùng chung giữa các file (`vLanguage`, `vCodeTable`, `pData`), không notarize, auto-update tự viết (tải `version.json` từ GitHub rồi mở link), helper app để chạy cùng hệ thống theo cách cũ (`SMLoginItemSetEnabled`, đã deprecated từ macOS 13).

## 3. Nguyên nhân lỗi Spotlight, Terminal và các app đặc biệt

Cả ba nhóm lỗi có chung một gốc: **kỹ thuật backspace giả định app nhận đúng N backspace rồi đúng chuỗi thay thế, theo đúng thứ tự, và mỗi backspace xoá đúng một ký tự vừa gõ**. Ba loại app phá giả định đó theo ba cách khác nhau, nên cần ba chiến lược gửi khác nhau — không có một cách gửi nào đúng cho mọi app.

| Ngữ cảnh | Triệu chứng | Nguyên nhân kỹ thuật | Chiến lược đúng |
| --- | --- | --- | --- |
| Spotlight (macOS 26), Chrome/Safari/Arc address bar, Excel cell, JetBrains, Alfred/Raycast | Đúp chữ: `hộ` → `hoộ`, dính chữ với phần gợi ý | Ô nhập có **inline autocomplete**: sau khi gõ `ho`, app tự chèn phần gợi ý dưới dạng text đã select. Backspace đầu tiên chỉ xoá selection gợi ý, không xoá `o`; chuỗi `ộ` được chèn thêm → `hoộ`. OpenKey nhận dạng Spotlight bằng tên cửa sổ nên trượt trên Spotlight mới | **Selection replacement**: `Shift+Left` × N rồi chèn chuỗi (chuỗi mới ghi đè cả selection); nhận dạng bằng **AX role** của ô nhập (`AXComboBox`, `AXSearchField`), không bằng tên app |
| Terminal.app, iTerm2, Ghostty, Warp, VS Code/Cursor terminal **khi chạy TUI** (Claude Code, Codex CLI, lazygit, vim insert mode) | Mất chữ, mất dấu cách, chữ dính vào từ trước | Terminal dịch keyDown thành byte stdin: backspace = `0x7F`, ký tự = UTF-8. Vì IME post backspace và chuỗi trong cùng micro-giây, chúng tới TUI trong **cùng một `read()` chunk**: `"\x7f\x7fộ "`. Ink/readline của Claude Code (trước v2.1.108) đếm số `\x7f`, gọi `backspace()` N lần rồi **bỏ toàn bộ phần còn lại của chunk** (đọc từ [patcher.py](https://github.com/manhit96/claude-code-vietnamese-fix) — fix là chèn lại phần sau `\x7f`). Shell thường (zsh line editor) không lỗi vì xử lý từng byte | **Paced backspace**: gửi từng backspace và chuỗi mới thành các event riêng, **cách nhau delay có thể chỉnh (mặc định 1–3 ms, terminal 5–15 ms)** để mỗi thứ rơi vào `read()` khác nhau; post qua `CGEvent.post(tap:)` thay vì `CGEventTapPostEvent` để có thể định thời; không dùng Shift+Left (terminal không có selection) |
| Chromium/Electron (Slack, Discord, Notion), Google Docs, Figma canvas | Thỉnh thoảng mất 1 ký tự khi gõ nhanh | Renderer xử lý input bất đồng bộ qua IPC; backspace và chuỗi unicode có thể bị đảo thứ tự hoặc gộp; Chromium coi chuỗi unicode dài trong một keyDown là “IME commit” và đôi khi drop | Backspace mặc định nhưng **chunk chuỗi ngắn (≤ 4 ký tự/event)**, delay 0.5–1 ms; cho phép user override per-app |
| App dùng Unicode tổ hợp hoặc font cũ (Photoshop, AutoCAD, Word font Palatino) | Dấu rời khỏi chữ, backspace xoá nhầm 2 code point | Ký tự tổ hợp = base + combining mark; app đếm 2 code point nhưng cursor nhảy 1 | Đã có trong OpenKey (`_syncKey` đếm độ dài thật từng ký tự) — giữ nguyên, gắn vào bảng mã per-app |

**Vì sao không chọn Input Method Kit (IMK) để hết hẳn backspace?** IMK là cơ chế chính thống: app nhận composition qua `NSTextInputClient`, không cần backspace, Spotlight và Terminal đều hỗ trợ. Nhưng IMK hiển thị **marked text gạch chân** khi đang gõ — chính là “lỗi gạch chân” mà OpenKey sinh ra để tránh; commit sớm từng ký tự rồi dùng `insertText:replacementRange:` để sửa ngược chỉ hoạt động với Cocoa text view; Chromium, Electron, terminal và Java bỏ qua `replacementRange` (`NSNotFound`), nên vẫn phải có fallback backspace cho chính các app đang lỗi. Kết luận: **v1.0 dùng CGEventTap với chiến lược gửi theo ngữ cảnh**; IMK là chế độ tuỳ chọn ở v2 (mục 8).

**Bằng chứng cách tiếp cận này chạy được:** [Gõ Nhanh](https://github.com/khaphanspace/gonhanh.org) (BSD-3, Rust + Swift, 1.0.159, macOS ≥ 13) dùng đúng mô hình AX-role detection + selection replacement + per-app delay và ghi nhận đã fix Spotlight, Chrome, Arc, Claude Code, JetBrains; latency 0,3–0,5 ms, RAM \~20 MB. VisKey không copy code (khác license, khác engine) nhưng lấy đó làm baseline tương thích.

## 4. Benchmark tính năng — danh sách cần có

VisKey v1.0 = toàn bộ cột OpenKey + các dòng đánh dấu **P1**. P2 làm sau khi ổn định. Unikey (Windows, bản 4.3) là chuẩn tham chiếu vì người dùng Việt đều quen; EVKey đã ngừng phát triển, Gõ Nhanh là đối thủ trực tiếp trên macOS.

| Tính năng | Unikey | OpenKey | Gõ Nhanh | VisKey |
| --- | --- | --- | --- | --- |
| Kiểu gõ Telex, VNI | Có | Có | Có | v1.0 |
| Simple Telex | — | Có | — | v1.0 |
| VIQR | Có | — | — | **P1** |
| Kiểu gõ tự định nghĩa (user-defined) | Có | — | — | P2 |
| Bảng mã Unicode, TCVN3, VNI, Unicode tổ hợp, CP1258 | Có | Có | Unicode | v1.0 |
| Bảng mã VIQR, VISCII, VPS, BK HCM 1/2, UTF-8 literal, NCR decimal/hex | Có | — | — | **P1** (VIQR, VISCII, NCR), P2 còn lại |
| Bỏ dấu oà/uý (chính tả mới) | Có | Có | Có | v1.0 |
| Bỏ dấu tự do (free tone placement) | Có | Kiểm tra chính tả on/off | — | **P1** |
| Kiểm tra chính tả + khôi phục từ sai | Có | Có | Auto-restore tiếng Anh khi Space | v1.0, thêm **ESC restore** (P1) |
| Quick Telex (cc=ch, nn=ng…), phụ âm đầu/cuối gõ tắt | — | Có | — | v1.0 |
| Cho phép f z w j đầu từ | — | Có | — | v1.0 |
| Macro / gõ tắt, import/export | Có (.txt) | Có (không giới hạn độ dài) | Có | v1.0, import file Unikey/EVKey (**P1**) |
| Viết hoa đầu câu | — | Có | Có | v1.0 |
| Chuyển mã văn bản (convert tool) | Có | Có | — | v1.0 |
| Chuyển mã nhanh clipboard bằng hotkey (Ctrl+Shift+F6 kiểu Unikey) | Có | Có (hotkey tùy chọn) | — | v1.0 |
| Công cụ clipboard: chuyển HOA/thường, bỏ dấu, chuyển sang không dấu | Có | — | — | **P1** |
| Hotkey bật/tắt tùy chọn (kể cả modifier-only) | Có | Có | Có (+ hotkey phụ) | v1.0 |
| Tạm tắt bằng Cmd/Alt, tạm tắt chính tả bằng Ctrl | — | Có | — | v1.0 |
| Smart switch VI/EN theo app | — | Có | Có | v1.0 |
| Nhớ bảng mã theo app | — | Có | — | v1.0 |
| Tự tắt khi input source không phải Latin (Nhật, Hàn, Trung) | — | Có (vOtherLanguage) | Có | v1.0 |
| Loại trừ app thủ công (blacklist) | — | — (issue #321) | — | **P1** |
| Tùy chỉnh per-app: chiến lược gửi, delay, bảng mã | — | — | Delay per-app | **P1** (điểm khác biệt chính) |
| Sửa lỗi Spotlight / address bar / Terminal TUI | n/a | Lỗi | Đã fix | **P1** (mục tiêu số 1) |
| Menu bar icon, dark mode, Dock icon on/off | n/a | Có | Có | v1.0 |
| Chạy cùng hệ thống | Có | Có (API cũ) | Có | v1.0 (`SMAppService`) |
| Auto-update | — | Tự viết | Có (24h) | v1.0 (Sparkle 2, EdDSA) |
| Homebrew cask | n/a | Có | Có | v1.0 |
| Notarized, universal binary | n/a | Không | Có | v1.0 |
| Onboarding xin quyền Accessibility + Input Monitoring | n/a | Hướng dẫn tĩnh | Có | v1.0 |
| Export/import toàn bộ cài đặt (JSON) | — | — | — | P2 |
| CLI `viskey --toggle` cho Raycast/Alfred/scripts | — | — | — | P2 |
| Chế độ IMK (input source thật) tuỳ chọn | n/a | — | — | v2 |

## 5. Kiến trúc đề xuất

**Chọn: CGEventTap + chiến lược gửi theo ngữ cảnh (context-aware replacement), engine OpenKey giữ nguyên sau lớp C API.** Hai thay đổi quyết định so với OpenKey: (a) nhận dạng *ô nhập* bằng Accessibility API thay vì nhận dạng *app* bằng tên cửa sổ; (b) tách việc *gửi event* thành module riêng có hàng đợi tuần tự và delay, thay vì post thẳng trong callback.

&#91;embedded content: kiến trúc VisKey · 5 bước xử lý phím, 3 thành phần hỗ trợ\]

Phím đi từ trên xuống cột trái; bước 4 là nơi duy nhất quyết định cách gửi, dựa trên context ở bước 2 và policy ở cột phải. Engine C++ chỉ trả lời câu hỏi “xoá mấy ký tự, chèn chuỗi gì”, không biết gì về app.

**So sánh ba lựa chọn đã cân nhắc:**

| Tiêu chí | CGEventTap + context (chọn) | Input Method Kit | Hybrid IMK + tap |
| --- | --- | --- | --- |
| Spotlight, address bar | Selection replacement, cần AX detection | Native, không hack | Native |
| Terminal TUI (Claude Code) | Paced backspace, cần delay | Native qua marked text, nhưng gạch chân khi đang gõ | Native |
| Chromium/Electron | Backspace chunk nhỏ | `replacementRange` bị bỏ qua → vẫn cần backspace | Vẫn cần tap |
| Gạch chân khi gõ | Không | Có (lý do OpenKey/EVKey ra đời) | Có ở app IMK |
| Quyền cần xin | Accessibility + Input Monitoring | Không (cài như input source) | Cả hai |
| Xung đột với bộ gõ khác | Có nếu chạy 2 tap | Không | Có |
| Công sức | Thấp nhất, tái dùng OpenKey | Viết lại toàn bộ lớp platform, engine phải expose composition state | Gấp đôi |
| Rủi ro | Apple siết event tap (chưa có dấu hiệu, Gõ Nhanh/EVKey vẫn chạy trên macOS 26) | Người dùng ghét gạch chân | Phức tạp, khó debug |

**Nguyên tắc thiết kế:**

1. Engine không biết platform: mọi thứ liên quan app, AX, delay nằm ở Swift.
2. Mặc định đúng cho 90% app (backspace, không delay), chỉ bật chiến lược đặc biệt khi context yêu cầu — selection làm cursor nháy, delay làm chậm; không bật bừa.
3. Policy per-app là dữ liệu (JSON), không phải code: thêm app mới = thêm dòng, user tự override được, có thể cập nhật rules mà không cần release.
4. Mọi event gửi đi phải đi qua một serial queue duy nhất để thứ tự backspace → chuỗi mới không bao giờ bị đảo.
5. Không có global C++ dùng chung với ObjC; engine state gói trong struct, cho phép test unit bằng XCTest + C++ test.

## 6. Thiết kế chi tiết

### 6.1 Cấu trúc repo

```
viskey/
├─ Engine/                 # C++ từ OpenKey, GPL-3.0, sửa tối thiểu
│  ├─ src/ Engine.cpp Vietnamese.cpp Macro.cpp SmartSwitchKey.cpp ConvertTool.cpp
│  ├─ include/viskey_engine.h   # C API mới, không global, opaque handle
│  └─ tests/                    # Catch2: 300+ ca gõ từ issue tracker OpenKey
├─ VisKey/                 # Swift package + Xcode app
│  ├─ Core/      KeyTap.swift ContextResolver.swift ReplacementPlanner.swift EventSender.swift
│  ├─ Policy/    AppPolicyStore.swift builtin-policies.json
│  ├─ Bridge/    EngineBridge.swift (module map trỏ vào viskey_engine.h)
│  ├─ Features/  Macro/ ConvertTool/ SmartSwitch/ Hotkey/ Clipboard/
│  ├─ UI/        MenuBar/ Settings/ Onboarding/ (SwiftUI)
│  └─ App/       VisKeyApp.swift AppDelegate.swift LoginItem.swift Updater.swift
├─ Tools/     policy-lint, build-dmg.sh, notarize.sh
└─ docs/
```

### 6.2 C API của engine (thay cho global `vLanguage`, `pData`)

```c
typedef struct vk_engine vk_engine;
typedef struct {
    uint8_t  code;            // vDoNothing / vWillProcess / vRestore / vReplaceMacro ...
    uint8_t  backspace_count;
    uint8_t  char_count;
    uint32_t chars[64];       // UTF-32, thứ tự gốc (engine OpenKey trả ngược, bridge đảo lại)
    uint8_t  ext_code;        // 4 = đã dùng macro, 1–3 = khôi phục
} vk_result;

vk_engine* vk_create(void);
void       vk_destroy(vk_engine*);
void       vk_set_option(vk_engine*, vk_option key, int32_t value); // input method, code table, spell check, quick telex ...
vk_result  vk_handle_key(vk_engine*, uint16_t keycode, uint32_t modifiers, vk_event_kind kind);
void       vk_new_session(vk_engine*);      // click chuột, đổi app, Esc
void       vk_macro_load(vk_engine*, const uint8_t* blob, size_t len);
size_t     vk_convert(vk_engine*, const char* utf8, size_t len, vk_code_table from, vk_code_table to, char* out, size_t cap);
```

Engine OpenKey hiện dùng biến global C++; bước đầu là gói chúng vào `struct vk_engine` bằng refactor cơ học (không đổi logic), có test chạy trước/sau để đảm bảo kết quả giống hệt.

### 6.3 ContextResolver — nhận dạng ô nhập

Mỗi keyDown (sau khi cache 100 ms hết hạn hoặc app đổi) hỏi `AXUIElementCopyAttributeValue(systemWide, kAXFocusedUIElementAttribute)` rồi đọc `AXRole`, `AXSubrole`, bundle ID của process sở hữu element (`AXUIElementGetPid` → `NSRunningApplication`). Không dùng `frontmostApplication` vì Spotlight, Raycast, Alfred là overlay không đổi frontmost app. Thứ tự ưu tiên:

1. User override cho bundle ID (nếu có) → dùng ngay.
2. `AXRole == AXComboBox` hoặc `AXSubrole == AXSearchField` → **Selection**.
3. Bundle ID khớp nhóm terminal (`com.apple.Terminal`, `com.googlecode.iterm2`, `com.mitchellh.ghostty`, `dev.warp.Warp-Stable`, `com.github.wez.wezterm`, `net.kovidgoyal.kitty`, `org.alacritty`) hoặc AX element là terminal view trong IDE (`com.microsoft.VSCode`, `com.todesktop.230313mzl4w4u92` Cursor, `com.jetbrains.*` với role `AXTextArea` subrole rỗng và `AXDescription` chứa “terminal”) → **Paced**.
4. Bundle ID khớp `com.jetbrains.*`, `com.microsoft.Excel`, `com.raycast.macos`, `com.runningwithcrayons.Alfred` → **Selection**.
5. Còn lại → **Backspace**.

Nếu AX trả lỗi (app không expose accessibility, ví dụ game, Java cũ) → fallback `CGWindowListCopyWindowInfo` kiểm tra cửa sổ Spotlight như OpenKey, rồi Backspace.

### 6.4 Ba chiến lược gửi

| Chiến lược | Chuỗi event gửi | Delay mặc định | Chunk | Dùng cho |
| --- | --- | --- | --- | --- |
| Backspace | BS×N → unicode(string) | 0 ms | 16 ký tự (Cocoa), 4 ký tự (Chromium/Electron) | 90% app |
| Selection | (Shift+←)×N → unicode(string) | 0 ms | 16 | Ô có inline autocomplete |
| Paced | BS, sleep, BS, sleep … → unicode(chữ 1), sleep, unicode(chữ 2) … | 3 ms giữa event; terminal trong IDE 8 ms; user chỉnh 0–30 ms | 1 ký tự | Terminal, TUI |

Paced gửi **từng ký tự một** vì terminal chuyển mỗi keyDown thành một `write()` riêng; delay đảm bảo TUI `read()` từng mảnh. 3 ms × (2 BS + 1 chữ) = 9 ms cho một dấu — dưới ngưỡng cảm nhận 20 ms. Với Claude Code ≥ 2.1.108 (đã fix upstream) Paced vẫn an toàn, chỉ chậm hơn không đáng kể.

### 6.5 EventSender

- Một `DispatchQueue(label: "viskey.sender", qos: .userInteractive)` serial; callback của tap chỉ enqueue rồi `return nil` ngay → callback luôn dưới 1 ms, tránh macOS tự disable tap vì timeout (`kCGEventTapDisabledByTimeout` — vẫn bắt và re-enable).
- `CGEventSource(stateID: .privateState)`; mọi event gắn `sourceUserData = 0x5649534B` (“VISK”) để KeyTap bỏ qua, không phụ thuộc source state ID như OpenKey (bị nhầm khi app khác cũng dùng private state).
- Post bằng `event.post(tap: .cgSessionEventTap)` thay vì `CGEventTapPostEvent(proxy)` — cho phép post ngoài callback (cần cho delay); giữ modifier flags của event gốc = 0 để không dính Shift/Caps.
- Mỗi chuỗi thay thế là một transaction: nếu user gõ phím mới khi transaction chưa xong, phím mới xếp hàng sau — không xử lý song song.

### 6.6 AppPolicyStore

```json
{ "version": 3,
  "rules": [
    { "match": { "axRole": "AXComboBox" }, "strategy": "selection" },
    { "match": { "axSubrole": "AXSearchField" }, "strategy": "selection" },
    { "match": { "bundlePrefix": "com.jetbrains." }, "strategy": "selection" },
    { "match": { "bundle": "com.apple.Terminal" }, "strategy": "paced", "delayMs": 3 },
    { "match": { "bundle": "com.microsoft.VSCode", "axDescriptionContains": "terminal" }, "strategy": "paced", "delayMs": 8 },
    { "match": { "bundle": "com.google.Chrome" }, "strategy": "backspace", "chunk": 4 },
    { "match": { "bundle": "com.adobe.Photoshop" }, "codeTable": "unicodeCompound" }
  ] }
```

File built-in nằm trong bundle; user override lưu `~/Library/Application Support/VisKey/policies.json` và merge đè lên. Settings có tab “Ứng dụng”: chọn app → chọn chiến lược, delay (slider 0–30 ms), bảng mã, bật/tắt tiếng Việt, loại trừ hoàn toàn. Nút “Báo lỗi app này” copy sẵn bundle ID + AX role + strategy đang dùng vào clipboard để dán vào GitHub issue.

### 6.7 Hotkey và chế độ tạm

- Hotkey bật/tắt mặc định Ctrl+Shift (modifier-only, kế thừa cơ chế `flagsChanged` của OpenKey); hotkey phụ tuỳ chọn (ví dụ Ctrl+Space, tránh Cmd+Space của Spotlight).
- Esc khi từ vừa gõ bị biến dạng (`user` → `úẻ`) → khôi phục nguyên văn (engine đã có `vRestore`, chỉ cần map phím).
- Tạm tắt khi giữ Cmd/Alt, tạm tắt chính tả khi giữ Ctrl: giữ nguyên.
- Tự tắt khi input source hiện tại không phải Latin (`TISCopyCurrentKeyboardInputSource`, lắng nghe `kTISNotifySelectedKeyboardInputSourceChanged` thay vì hỏi mỗi phím).

### 6.8 Quyền và onboarding

Event tap `.defaultTap` (có sửa event) trên macOS ≥ 10.15 cần **Accessibility**; đọc keyDown cần thêm **Input Monitoring** trên một số cấu hình. Onboarding 3 bước: giải thích vì sao cần quyền → nút mở đúng pane (`x-apple.systempreferences:com.apple.preference.security?Privacy_Accessibility`) → poll `AXIsProcessTrusted()` mỗi 1 s, tự khởi động tap khi được cấp, không bắt restart. Phát hiện bộ gõ khác đang chạy (OpenKey, EVKey, Gõ Nhanh, Unikey-port) qua `NSRunningApplication` và cảnh báo xung đột.

### 6.9 Dữ liệu người dùng

- Cài đặt: `UserDefaults` (suite `work.1vision.viskey`), một struct `Settings: Codable` duy nhất.
- Macro: SQLite hoặc JSON trong Application Support, format import được từ file `.txt` của Unikey (`từ_tắt:nội_dung` mỗi dòng) và EVKey.
- Smart switch: dictionary bundleID → (VI/EN, codeTable), giới hạn 500 entry, LRU.
- Không telemetry, không mạng ngoài Sparkle appcast và GitHub release.

## 7. Tech stack, build và phân phối

| Hạng mục | Lựa chọn | Lý do |
| --- | --- | --- |
| Ngôn ngữ platform | Swift 5.9, SwiftUI cho UI, AppKit cho `NSStatusItem`/`NSEvent` | Native, không runtime ngoài; ObjC++ của OpenKey khó tuyển người contribute |
| Engine | C++17, giữ từ OpenKey, biên dịch thành static lib qua SwiftPM C target | Độ ổn định 6 năm; viết lại bằng Swift/Rust không đem lại giá trị cho người dùng |
| Build | Xcode 16, SwiftPM, `xcodebuild` trong GitHub Actions `macos-15` runner | Universal binary (arm64 + x86\_64), một lệnh `make release` |
| Deployment target | macOS 13 Ventura | Cần `SMAppService` (login item mới), SwiftUI `MenuBarExtra`, `Settings` scene; cùng mốc với Gõ Nhanh |
| Code signing | Developer ID Application + Hardened Runtime; entitlement không sandbox | Event tap không chạy trong App Sandbox → không lên App Store, phải notarize |
| Notarization | `notarytool` trong CI, `stapler staple` vào `.app` và `.dmg` | Không notarize = Gatekeeper chặn, user phải `xattr -cr` như Gõ Nhanh — rào cản lớn nhất với người không kỹ thuật |
| Auto-update | Sparkle 2 (EdDSA key, appcast trên GitHub Pages) | Chuẩn de-facto macOS, delta update, không tự viết như OpenKey |
| Đóng gói | `create-dmg` → `VisKey.dmg`; Homebrew cask `viskey` (tự PR vào homebrew-cask sau khi có ≥ 30 star và notarized) | Hai kênh cài quen thuộc của user OpenKey |
| Test | Catch2 cho engine (chạy cả Linux CI), XCTest cho Planner/Policy, UI test thủ công theo checklist mục 8 | Engine test chạy 5 s, không cần Mac → contributor Linux vẫn sửa engine được |
| Chi phí cố định | Apple Developer Program 99 USD/năm | Bắt buộc cho Developer ID + notarize |
| License | GPL-3.0 (kế thừa OpenKey), header mỗi file ghi “Based on OpenKey by Mai Vũ Tuyên” | Nghĩa vụ GPL; Sparkle (MIT) tương thích |

**Pipeline CI (GitHub Actions):** push tag `v*` → build universal → chạy test → sign → notarize → staple → tạo DMG → ký appcast Sparkle → GitHub Release + cập nhật `appcast.xml` trên branch `gh-pages`. Secrets: `DEVELOPER_ID_P12`, `NOTARY_APPLE_ID`, `NOTARY_TEAM_ID`, `NOTARY_PASSWORD` (app-specific), `SPARKLE_ED_KEY`.

**Kích thước và hiệu năng mục tiêu:** app < 15 MB, RAM < 30 MB, callback tap < 1 ms (p99), latency phím-đến-ký-tự < 5 ms với Backspace, < 15 ms với Paced; CPU idle 0%.

## 8. Roadmap, rủi ro và test plan

### 8.1 Roadmap

Lỗi Spotlight và Terminal được fix xong ở cuối M1 (tuần 5) — trước khi làm bất kỳ UI nào, vì đó là lý do dự án tồn tại. 12 tuần × 10 giờ ≈ 120 giờ; nếu trượt, cắt M4 (P1) sang 1.1 chứ không cắt M3.

&#91;embedded content: roadmap VisKey · 5 milestone, 4 gate, 12 tuần\]

Mỗi gate là điều kiện để sang milestone sau; không qua G2 thì không bắt đầu UI.

### 8.2 Rủi ro

| Rủi ro | Khả năng | Tác động | Giảm thiểu |
| --- | --- | --- | --- |
| AX role của Spotlight macOS 26 không phải `AXComboBox`/`AXSearchField` | Trung bình | Mục tiêu số 1 không đạt | Tuần 3: viết tool `viskey-axdump` in role/subrole của ô đang focus, chạy trên macOS 15 và 26 trước khi code rule; fallback nhận dạng bằng PID của process `Spotlight` sở hữu element |
| Terminal trong IDE (VS Code, Cursor) không phân biệt được với editor qua AX | Cao | Paced áp dụng cho cả editor → chậm 9 ms, không sai | Chấp nhận Paced toàn IDE nếu không tách được; user chỉnh delay = 0 cho editor |
| Refactor global engine làm đổi hành vi gõ | Trung bình | Regression khó thấy | G1: test snapshot 300 ca chạy trên OpenKey gốc trước, so kết quả byte-by-byte |
| Apple đổi hành vi event tap / quyền ở macOS 27 | Thấp | Toàn bộ bộ gõ ngoài cùng chết | Theo dõi beta WWDC; giữ thiết kế để thêm IMK mode ở v2 |
| Xung đột với bộ gõ khác đang chạy | Cao ở user mới | Đúp chữ, báo lỗi sai | Onboarding phát hiện và yêu cầu tắt; FAQ |
| 10 giờ/tuần không đủ | Cao | Trượt 1.0 | Thứ tự milestone đã xếp theo giá trị; M1 xong đã dùng được cho bản thân |
| GPL hạn chế mô hình kinh doanh | Chắc chắn | Không bán được license | Xác nhận từ đầu: sản phẩm cộng đồng, kênh thu là sponsor + thương hiệu 1VISION |

### 8.3 Test plan

**Engine (tự động, Catch2):** 300 ca từ issue tracker OpenKey và CHANGELOG (“quởn”, “tuyết”, “dui9”, “chưa”+“a” issue #312, Quick Telex, VNI đa byte, restore khi xoá), mỗi ca: chuỗi phím → kỳ vọng (backspaceCount, chars). Chạy trên mọi commit.

**Planner/Policy (tự động, XCTest):** cho context giả (role, subrole, bundle) → kỳ vọng strategy, delay, chunk; merge override của user; JSON policy không hợp lệ bị từ chối.

**Tương thích app (thủ công, checklist 40 ca, chạy trước mỗi release trên macOS 15 và 26):**

- [ ] Spotlight: gõ `hộ chiếu`, `trường đại học` khi có gợi ý inline → không đúp, không mất chữ
- [ ] Raycast, Alfred: như trên
- [ ] Chrome, Safari, Arc, Firefox address bar và ô tìm kiếm Google
- [ ] Terminal.app, iTerm2, Ghostty, Warp ở zsh prompt
- [ ] Claude Code (bản < 2.1.108 và bản mới nhất), Codex CLI, vim insert mode, lazygit commit message
- [ ] VS Code editor + terminal tích hợp, Cursor, Zed, Xcode, IntelliJ (autocomplete đang mở)
- [ ] Excel cell có autocomplete, Word, PowerPoint, Pages, Numbers
- [ ] Slack, Discord, Notion, Obsidian, Telegram, Zalo, Messages, Mail
- [ ] Google Docs, Figma text, Canva
- [ ] Gõ nhanh 120 wpm 2 phút vào TextEdit: 0 ký tự sai
- [ ] Macro 200 ký tự, macro trong chế độ tiếng Anh
- [ ] Hotkey modifier-only, Esc restore, tạm tắt Cmd/Alt, đổi input source Nhật rồi quay lại
- [ ] Thu hồi quyền Accessibility khi đang chạy → app báo, không crash, tự phục hồi khi cấp lại
- [ ] Ngủ / thức máy, đổi user nhanh → tap vẫn sống

**Hiệu năng:** đo bằng `os_signpost` quanh callback và EventSender; p99 callback < 1 ms, không có `kCGEventTapDisabledByTimeout` trong 8 giờ dùng thật.

### 8.4 Việc làm ngay tuần này

- [ ] Tạo repo `1vision/viskey`, import `Sources/OpenKey/engine` từ OpenKey commit mới nhất, giữ lịch sử git bằng `git subtree`
- [ ] Viết `viskey-axdump` (50 dòng Swift) và ghi lại AX role/subrole của Spotlight, Chrome address bar, Terminal, VS Code terminal trên máy thật → chốt rule ở mục 6.3
- [ ] Đăng ký Apple Developer Program (nếu chưa) để kịp notarize ở M3

### Nguồn

- [tuyenvm/OpenKey](https://github.com/tuyenvm/OpenKey) — README, `Sources/OpenKey/engine`, `macOS/ModernKey/OpenKey.mm`, `OpenKeyManager.m` (đọc trực tiếp từ clone, commit 15/06/2026)
- [OpenKey issue #315 — đúp chữ Spotlight](https://github.com/tuyenvm/OpenKey/issues/315), [#319 — Terminal/Claude Code](https://github.com/tuyenvm/OpenKey/issues/319), [#321 — loại trừ app](https://github.com/tuyenvm/OpenKey/issues/321), [#312 — chưâ](https://github.com/tuyenvm/OpenKey/issues/312)
- [Homebrew cask openkey](https://formulae.brew.sh/cask/openkey) — phiên bản 2.0.5; [cask gonhanh](https://formulae.brew.sh/cask/gonhanh) — 1.0.159, macOS ≥ 13
- [khaphanspace/gonhanh.org](https://github.com/khaphanspace/gonhanh.org) và [system-architecture.md](https://github.com/khaphanspace/gonhanh.org/blob/main/docs/system-architecture.md) — AX detection, selection replacement, latency
- [manhit96/claude-code-vietnamese-fix](https://github.com/manhit96/claude-code-vietnamese-fix) (`patcher.py`) và [0x0a0d/fix-vietnamese-claude-code](https://github.com/0x0a0d/fix-vietnamese-claude-code) — cơ chế lỗi `\x7f` chunk, fix upstream Claude Code 2.1.108
- [Claude Code issue #7989](https://claudeissues.com/issue/7989-bug-error-typing-vietnamese-telex) — báo cáo cộng đồng
