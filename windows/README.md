<!-- SPDX-License-Identifier: GPL-3.0-or-later -->
# VisKey cho Windows

Bộ gõ tiếng Việt native cho Windows 10/11 (x64), **by 1VISION**. Dựa trên OpenKey của Mai Vũ Tuyên — dùng chính engine C++ trong [`Engine/`](../Engine) qua C API [`viskey_engine.h`](../Engine/include/viskey_engine.h). GPL-3.0-or-later.

> ⚠️ **Bản preview.** CI build và chạy test trên `windows-latest`, nhưng nhóm phát triển **chưa thử app trên một máy Windows thật**. Hãy báo lỗi ở [Issues](https://github.com/1vision-work/VisKey/issues).

## Tải về

[GitHub Releases](https://github.com/1vision-work/VisKey/releases) (pre-release, tag `v*-windows-preview`):

| File | Ghi chú |
| :--- | :--- |
| `VisKey-<ver>-Windows-x64-portable.zip` | **Khuyên dùng.** Một file `VisKey.exe` duy nhất (+ LICENSE, NOTICE, README-Windows.txt), không cần cài, không cần DLL/VC++ Redistributable |
| `VisKey-<ver>-Windows-Setup.exe` | Trình cài đặt Inno Setup (tuỳ chọn, chỉ có khi CI build được) — cài cho riêng người dùng, không cần quyền Admin |

Mỗi lần push, artifact `viskey-windows` của workflow [Windows](../.github/workflows/windows.yml) chứa cùng các file đó.

## Sử dụng

- Chạy `VisKey.exe`. Biểu tượng ở khay hệ thống: **ô xanh chữ V** = tiếng Việt, **ô xám chữ E** = tiếng Anh, ô xám V gạch chéo = không cài được keyboard hook.
- **Chuột trái**: chuyển Việt/Anh. **Chuột phải**: Tiếng Việt · Kiểu gõ (Telex / VNI / Simple Telex) · Bảng mã (Unicode, Unicode tổ hợp, TCVN3, VNI Windows, CP1258) · Kiểm tra chính tả · Đặt dấu kiểu mới (oà, uý) · Quick Telex · Khôi phục từ sai · Gõ tắt · Khởi động cùng Windows · Cài đặt… · Giới thiệu… · Thoát.
- **Phím chuyển Việt/Anh**: nhấn và nhả `Ctrl` + `Shift` (không kèm phím khác, như OpenKey/UniKey), hoặc `Alt` + `Z` (chọn trong Cài đặt). Lưu ý: nếu Windows đang dùng `Ctrl+Shift` để đổi bố cục bàn phím, hãy tắt phím đó trong Settings › Time & language › Typing › Advanced keyboard settings › Input language hot keys, hoặc chọn `Alt+Z`.
- **Cài đặt…**: mọi tuỳ chọn của engine (thêm: bỏ dấu tự do, viết hoa đầu câu, phụ âm z/f/w/j, phụ âm đầu/cuối nhanh, sửa lỗi gợi ý trên thanh địa chỉ), phím chuyển, khởi động cùng Windows, và **bảng gõ tắt** (thêm/sửa/xoá; `Ko` → `Không` khi bật tự viết hoa).
- Chạy `VisKey.exe` lần thứ hai: mở cửa sổ Cài đặt của bản đang chạy (chỉ một bản chạy cùng lúc).

## Portable & nơi lưu cài đặt

- `config.json` (UTF-8, đọc được bằng mắt) nằm **cạnh `VisKey.exe` nếu thư mục đó ghi được** — chép cả thư mục sang USB là mang theo cài đặt, như UniKey.
- Nếu thư mục không ghi được (ví dụ `C:\Program Files\VisKey`): `%APPDATA%\VisKey\config.json`.
- File hỏng (không phải JSON hợp lệ) được đổi tên thành `config.json.bad`, VisKey chạy với mặc định.
- "Khởi động cùng Windows" = giá trị `VisKey` trong `HKCU\Software\Microsoft\Windows\CurrentVersion\Run` (đường dẫn exe trong ngoặc kép). Bản portable bị chuyển chỗ: lần chạy sau VisKey tự cập nhật đường dẫn này.

Ví dụ:

```json
{
  "version": 1,
  "inputType": "telex",
  "codeTable": "unicode",
  "vietnamese": true,
  "checkSpelling": true,
  "modernOrthography": true,
  "useMacro": true,
  "hotkey": "ctrl+shift",
  "macros": [{ "abbreviation": "vk", "expansion": "VisKey" }]
}
```

`inputType`: `telex` · `vni` · `simple_telex`; `codeTable`: `unicode` · `unicode_compound` · `tcvn3` · `vni_windows` · `cp1258`; `hotkey`: `ctrl+shift` · `alt+z`.

## SmartScreen, antivirus

- `VisKey.exe` **chưa ký số** → SmartScreen hiện "Windows protected your PC": bấm **More info › Run anyway**. Trình duyệt có thể cảnh báo khi tải file `.zip`/`.exe` ít người tải.
- Antivirus có thể **báo nhầm** vì VisKey dùng keyboard hook toàn cục (`WH_KEYBOARD_LL`) và `SendInput` — kỹ thuật chung của mọi bộ gõ (UniKey, EVKey, OpenKey). VisKey không kết nối mạng, không ghi log phím gõ.

## Giới hạn đã biết

- **Cửa sổ chạy quyền Administrator** (Task Manager, Terminal "Run as administrator", trình cài đặt…): Windows (UIPI) chặn phím gửi từ app quyền thấp hơn. VisKey nhận ra cửa sổ có integrity level cao hơn và **để phím đi qua nguyên vẹn** (gõ không dấu) thay vì nuốt phím. Muốn gõ tiếng Việt ở đó: chạy VisKey bằng quyền Administrator.
- Không chạy ở màn hình đăng nhập/khoá, hộp thoại UAC (secure desktop).
- App UWP/Store (Calculator, ô tìm kiếm Start…) nhận `SendInput` bình thường về lý thuyết, nhưng chưa thử.
- Chưa có chiến lược thay thế theo app như bản macOS (selection/paced): mọi app dùng "backspace + Unicode", kèm tuỳ chọn sửa lỗi gợi ý (U+202F) của OpenKey.
- Không chạy song song với bộ gõ khác (UniKey, EVKey, OpenKey…) — tắt chúng trước.
- Chưa có tạm tắt bằng Ctrl/Alt, smart switch theo app, chuyển mã clipboard (có trong engine, chưa có UI Windows).

## Kiến trúc

```
UI thread (app.cpp, dialogs.cpp)              hook thread (typing_service.cpp)
  tray icon, menu, Settings/About   ──Config──▶  vk_engine (Engine/, Windows VK codes)
  config.json, HKCU Run             ◀─language─   WH_KEYBOARD_LL · WH_MOUSE_LL · EVENT_SYSTEM_FOREGROUND
                                                  OutputPlanner → SendInput (dwExtraInfo = 0x5649534B "VISK")
```

- Engine chạy trên một thread riêng có message loop (callback hook LL chạy trên thread đã cài hook). UI thread không chạm engine; hai bên chỉ trao đổi bằng message (bản sao `Config`, sự kiện đổi ngôn ngữ).
- Trên `_WIN32`, engine dùng [`platforms/win32.h`](../Engine/src/platforms/win32.h): **mã phím của engine chính là virtual-key code của Windows**, không cần bảng dịch.
- Phím chữ/số/dấu câu/Space/Backspace/Enter/Tab/Esc/điều hướng → engine; Shift/CapsLock → `VK_MOD_SHIFT`/`VK_MOD_CAPS_LOCK`. Khi giữ **Ctrl/Alt/Win** (kể cả AltGr) phím là phím tắt: engine nhận `VK_MOD_OTHER` (kết thúc từ, như OpenKey) và phím luôn đi qua — **không bao giờ nuốt tổ hợp Win**. Phím khác (F1–F12, numpad, media…) đi qua và kết thúc từ. Click chuột, đổi cửa sổ foreground → kết thúc từ.
- Sự kiện mang tag `0x5649534B` (do chính VisKey gửi) bị bỏ qua. Sự kiện injected **của app khác** (bàn phím ảo, remote desktop, AutoHotkey) vẫn được xử lý như phím thật để cũng gõ được tiếng Việt.
- `OutputPlanner` (port từ `VisKey/Core/ReplacementPlanner.swift`, giữ `_syncKey` của OpenKey cho VNI Windows / Unicode tổ hợp): Backspace × n rồi chuỗi `KEYEVENTF_UNICODE` (giải mã từng engine word bằng `vk_word_decode`, macro dài > 64 word lấy thêm bằng `vk_last_macro_words`). Với `VK_RESTORE`, `VK_RESTORE_AND_START_NEW_SESSION`, `VK_REPLACE_MACRO`, phím vừa gõ được gửi lại bằng **chính virtual-key của nó** (app tự áp bố cục bàn phím và Shift).
- `SendInput` được gọi ngay trong callback hook: input inject được xếp sau sự kiện đang xử lý nên app nhận phím đúng thứ tự; vài chục event mất vài µs, xa dưới timeout của LL hook.

## Build

Cần Visual Studio 2022 trở lên (MSVC, workload "Desktop development with C++") và CMake ≥ 3.20 (đủ mới để nhận ra bản Visual Studio đang cài). Không cần `-G`: CMake tự chọn generator Visual Studio mới nhất trên máy; muốn chỉ định thì thêm `-G "Visual Studio 17 2022"` hoặc generator tương ứng.

```bat
cmake -S windows -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
build\Release\VisKey.exe
```

- `VisKey.exe` link tĩnh CRT (`/MT`, `CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded`), icon + manifest (Common Controls v6, PerMonitorV2, `asInvoker`) + VERSIONINFO nhúng sẵn. `dumpbin /dependents` chỉ còn DLL hệ thống (KERNEL32, USER32, GDI32, SHELL32, COMCTL32, ADVAPI32, ole32).
- ARM64: `-A ARM64` (CI thử build, không chặn release nếu lỗi).
- Engine được build nguyên trạng; các cờ MSVC nó cần (`NOMINMAX`, `WIN32_LEAN_AND_MEAN`, `/utf-8`, tắt cảnh báo `<codecvt>`) đặt từ [`windows/CMakeLists.txt`](CMakeLists.txt), không sửa `Engine/`.
- Icon: `windows/res/gen_icons.sh` (cần `rsvg-convert` + ImageMagick) sinh lại `VisKey.ico` và `tray-*.ico` từ `design/icons/` và `windows/res/icons-src/`. File `.ico` đã commit, CI không cần chạy script.
- Installer (tuỳ chọn): `iscc /DAppVersion=1.1.0 /DSourceExe=build\Release\VisKey.exe /DRepoRoot=. /Odist windows\installer\VisKey.iss`.

### Test

- `windows/tests/win_keymap_test.cpp` (`ctest`): JSON/config round trip, phân loại phím + modifier + hotkey Ctrl+Shift, kết quả engine → chuỗi phím/`INPUT`, và gõ end-to-end bằng engine thật với VK code Windows (Telex, VNI, Unicode tổ hợp, VNI Windows, macro 200 ký tự, emoji, chế độ tiếng Anh). Không cài hook, chạy được trên CI.
- `crossplatform/tests/test_cpp_engine.cpp` (109 ca, engine header-only của `crossplatform/`) được build bằng `cl` và chạy trong job Windows.
- `Engine/tests` (snapshot + Catch2) **không chạy trong job Windows**: snapshot ghi bằng mã phím macOS, còn trên `_WIN32` engine dùng VK code Windows; target `gen_snapshot_original` lại dùng cờ GCC/Clang (`-include algorithm -w`). CI chỉ thử build chúng ở bước *informational* (`continue-on-error`). Các test này chạy trên Linux/macOS trong `ci.yml`.
