# VisKey — Bộ Gõ Tiếng Việt Đa Nền Tảng (macOS, Linux & Windows)

[![CI](https://github.com/1vision-work/VisKey/actions/workflows/ci.yml/badge.svg?branch=master)](https://github.com/1vision-work/VisKey/actions/workflows/ci.yml)
[![Windows](https://github.com/1vision-work/VisKey/actions/workflows/windows.yml/badge.svg?branch=master)](https://github.com/1vision-work/VisKey/actions/workflows/windows.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

> **Tải về:** bộ cài dựng sẵn (`.dmg` cho macOS, `.deb` cho Linux, `VisKey64.exe` portable `.zip` / Setup cho Windows) có tại trang [**GitHub Releases**](https://github.com/1vision-work/VisKey/releases). File nhị phân không được lưu trong mã nguồn.

**VisKey** là ứng dụng bộ gõ Tiếng Việt đa nền tảng tốc độ cao dành cho **macOS** (Apple Silicon & Intel), **Linux** (X11 & Wayland qua IBus) và **Windows** (Windows 10 & Windows 11).

## 1. Nguồn gốc Mã nguồn & Bản quyền (Origin & License)

* **100% Viết mới từ đầu (Clean-Room Implementation — Không Fork):**
  Toàn bộ mã nguồn của **VisKey** — bao gồm bộ lõi thuật toán C++17 (`core/viskey_engine.hpp`), bộ lõi Python (`core/viskey_engine.py`), ứng dụng macOS Cocoa/CGEventTap (`macos/VisKeyApp.mm`), ứng dụng Windows Win32 (`windows/VisKeyWin.cpp` & `windows/VisKey_Windows_Portable.pyw`), bộ gõ Linux IBus (`linux/ibus/ibus_viskey.py` & `viskey_settings.py`) và bộ 109 bài kiểm thử tự động — đều được thiết kế và viết mới hoàn toàn từ đầu.
* **Không phụ thuộc mã nguồn GPL của bên thứ ba:**
  VisKey **không fork** và không sao chép bất kỳ dòng code nào từ Unikey, OpenKey, EVKey, IBus-Bamboo hay Fcitx5-Unikey. Các kỹ thuật tương tác hệ thống đều sử dụng trực tiếp API chuẩn công khai của từng hệ điều hành (`Apple CoreGraphics/Cocoa`, `Microsoft Win32 User32`, `Freedesktop IBus`).
* **Giấy phép MIT (`LICENSE`):**
  Vì không bị ràng buộc bởi giấy phép copyleft GPLv2/GPLv3 của các bộ gõ cũ, bạn hoàn toàn sở hữu và toàn quyền sử dụng, chỉnh sửa, phân phối lại hoặc đóng gói thương mại dự án này.

## 2. Các vấn đề thực tế trên thị trường mà VisKey giải quyết

| Vấn đề của các bộ gõ hiện nay | Nguyên nhân kỹ thuật | Giải pháp công nghệ trong VisKey |
| :--- | :--- | :--- |
| **1. Mất chữ đầu / Nhân đôi ký tự trên Spotlight, Chrome, Safari, Arc, Edge, Excel** | Khi gõ ký tự đầu tiên (ví dụ `t`), thanh địa chỉ trình duyệt tự động bôi đen phần gợi ý inline (`t[inhte.vn]`). Phím `Backspace` đầu tiên do bộ gõ gửi chỉ xóa phần gợi ý đang bôi đen thay vì xóa chữ `t`, dẫn tới lỗi `ttiếng` hoặc mất chữ đầu. | **Autocomplete Fix (`U+202F` Injection):** Khi cần xóa ký tự đầu trên các ứng dụng có Autocomplete, VisKey bơm ký tự `U+202F` (Narrow No-Break Space) kèm 1 nhịp xóa để hủy vùng bôi đen gợi ý trước khi đẩy chuỗi Unicode mới. |
| **2. Lỗi gạch chân (Preedit Underline) gây kẹt phím tắt, rớt chữ khi click chuột trên Linux & macOS** | Bộ gõ mặc định giữ từ đang gõ trong bộ đệm gạch chân (*preedit*). Khi nhấn `Ctrl+C`, dùng phím mũi tên trong Vim/Terminal hoặc click chuột chuyển ô nhập liệu, từ đang gõ bị mất hoặc nhảy vị trí. | **Direct Commit (Không gạch chân):** Trên macOS và Windows dùng hook sự kiện bàn phím trực tiếp (`CGEventTap` / `WH_KEYBOARD_LL`); trên Linux hỗ trợ **Chế độ Không gạch chân** (`Surrounding Text` / `Fake Backspace`) kèm cơ chế tự chốt từ tức thời khi mất focus trên Wayland. |
| **3. Biến dạng từ tiếng Anh khi quên tắt bộ gõ (`window` -> `ưindơ`, `software` -> `sòtare`, `book` -> `bôk`)** | Các bộ gõ truyền thống biến đổi mọi phím `w, s, f, r, x, j, oo, ee, aa, dd` mà không kiểm tra cấu trúc âm tiết tiếng Việt. | **Phonotactics Validator & Auto-Restore:** Kiểm tra chặt chẽ cấu trúc âm vị học tiếng Việt (27 phụ âm đầu, 54 vần chuẩn, 8 phụ âm cuối, luật phân bố `c/k/gh/ngh`, luật thanh điệu trên âm tắc `c/ch/p/t` và vần mở), tự động khôi phục nguyên vẹn từ tiếng Anh (`window`, `software`, `refresh`, `book`, `feel`, `view`, `test`, `macos`, `linux`...). |
| **4. Đặt dấu sai vị trí khi gõ tự do hoặc thêm nguyên âm/phụ âm cuối sau dấu** | Người dùng gõ dấu ngay sau nguyên âm đầu rồi mới gõ tiếp nguyên âm sau hoặc phụ âm cuối (`h-o-s-a-n`, `t-h-u-s-o-n-g-w`) khiến dấu nằm sai nguyên âm. | **Dynamic Tone Repositioning:** Tự động tính toán lại vị trí dấu thanh và dấu móc (`ươ`) mỗi khi âm tiết thay đổi (`hoas` -> `hóa`, gõ thêm `n` tự dịch dấu sang `hoán`); hỗ trợ chuyển đổi giữa **Kiểu cũ (`hòa`, `thúy`, `khỏe`)** và **Kiểu mới (`hoà`, `thuý`, `khoẻ`)**. |
| **5. Dính tiếng Việt khi chuyển sang Terminal / CMD / PowerShell để gõ lệnh** | Bộ gõ dùng chung một trạng thái Bật/Tắt cho toàn hệ thống. | **Smart Per-App Mode:** Tự động ghi nhớ chế độ `VI`/`EN` riêng cho từng ứng dụng (`Bundle ID` trên macOS, tên `.exe` trên Windows) và mặc định chuyển sang `EN` khi vào Terminal, iTerm2, Warp, Alacritty, Kitty, CMD, PowerShell, Windows Terminal. |
| **6. Bộ gõ tự ngắt hoặc treo sau khi Sleep/Wake hoặc vào ô nhập mật khẩu** | Hệ điều hành tự vô hiệu hóa hook khi quá thời gian chờ (`kCGEventTapDisabledByTimeout`) hoặc khi bật `SecureEventInput`. | **EventTap Watchdog & Secure Input Guard:** Tự động kích hoạt lại `CGEventTapEnable` ngay khi nhận tín hiệu timeout và tự động nhường quyền an toàn khi người dùng nhập mật khẩu. |
| **7. Gõ lặp lại các cụm từ thông dụng tốn thời gian** | Thiếu cơ chế gõ tắt bảo toàn kiểu chữ hoa/thường. | **Case-Preserving Macro & Quick Telex:** Hỗ trợ bảng Gõ tắt tự nhận diện chữ hoa (`ko` -> `không`, `Ko` -> `Không`, `KO` -> `KHÔNG`, `vn` -> `Việt Nam`) và Quick Telex (`cc=ch, gg=gi, kk=kh, nn=ng, qq=qu, pp=ph, tt=th`). |

## 3. Giao diện Người dùng (GUI) trên cả 3 Hệ điều hành

VisKey cung cấp đầy đủ **2 lớp giao diện đồ họa** trên mọi nền tảng:
1. **Biểu tượng Thanh trạng thái (Menu Bar / System Tray):**
   * Hiển thị trạng thái **`🇻🇳 VI`** (Tiếng Việt) hoặc **`🇬🇧 EN`** (Tiếng Anh) theo thời gian thực.
   * Phím tắt chuyển đổi nhanh: **`Ctrl + Space`** (trên cả macOS, Linux, Windows), **`Cmd + Shift + V`** (trên macOS) hoặc **`Alt + Z`** (trên Windows).
2. **Cửa sổ Bảng Điều Khiển & Quản lý Gõ tắt (Settings & Macro Manager Window):**
   * **Trên macOS:** Cửa sổ Cocoa `NSWindow` gốc mở từ menu *"Bảng điều khiển VisKey..."* (`Cmd + ,`), lưu cấu hình vào `NSUserDefaults` và hỗ trợ tự khởi động qua `~/Library/LaunchAgents/org.viskey.inputmethod.plist`.
   * **Trên Linux:** Ứng dụng đồ họa `viskey-settings` (có icon trong menu ứng dụng của máy), lưu cấu hình vào `~/.config/viskey/config.json` và tự động áp dụng ngay lập tức.
   * **Trên Windows:** Cửa sổ Win32 GUI gốc (`VisKey64.exe` / `VisKey_Windows_Portable.pyw`), lưu cấu hình vào Windows Registry (`HKCU\Software\VisKey`) và hỗ trợ tự khởi động cùng Windows.

## 4. Cấu trúc Thư mục Dự án

```text
VisKey/
├── README.md                          # Tài liệu tổng quan và hướng dẫn sử dụng chi tiết
├── LICENSE                            # Giấy phép mã nguồn mở MIT
├── VISKEY_BUILD_GUIDE.md              # Hướng dẫn chuyên sâu cách Build & Đóng gói bộ cài
├── VISKEY_ARCHITECTURE_FOR_DEVS.md    # Tài liệu Kiến trúc & Thuật toán dành cho Developer
├── CMakeLists.txt                     # Cấu hình build CMake đa nền tảng (macOS, Linux, Windows)
├── Install_macOS.command              # Script 1-click biên dịch & cài đặt trên macOS
├── Build_Windows.bat                  # Script 1-click biên dịch VisKey64.exe trên Windows
├── Run_VisKey_Windows.bat             # Script 1-click chạy ngay trên Windows không cần biên dịch
├── .github/workflows/                 # CI: ci.yml (Linux .deb + macOS .dmg), windows.yml (VisKey64.exe x64/ARM64)
├── core/
│   ├── viskey_engine.hpp              # Bộ lõi xử lý tiếng Việt C++17 Header-only (UTF-8/UTF-32)
│   ├── viskey_engine.py               # Bộ lõi xử lý tiếng Việt Python 3
│   └── viskey_cli.cpp                 # Công cụ giả lập gõ tiếng Việt tương tác trên Terminal
├── macos/
│   ├── VisKeyApp.mm                   # Ứng dụng macOS gốc (Cocoa UI + CGEventTap)
│   └── Info.plist                     # Cấu hình App Bundle macOS (org.viskey.inputmethod)
├── linux/
│   └── ibus/
│       ├── ibus_viskey.py             # Daemon bộ gõ IBus cho Linux (X11 & Wayland)
│       ├── viskey_settings.py         # Ứng dụng Giao diện Bảng điều khiển trên Linux
│       └── viskey.xml                 # Khai báo component cho IBus
├── windows/
│   ├── VisKeyWin.cpp                  # Ứng dụng Windows gốc (Win32 GUI + WH_KEYBOARD_LL + SendInput)
│   ├── VisKey_Windows_Portable.pyw    # Bản chạy ngay trên Windows qua ctypes Win32 + Tkinter
│   ├── res/                           # Icon + thông tin phiên bản cho VisKey64.exe (VisKey.rc, VisKey.ico)
│   └── installer/VisKey.iss           # Bộ cài Inno Setup (tùy chọn)
├── scripts/
│   ├── build_macos_installer.sh       # Script tạo VisKey.app Universal Binary & file .dmg
│   ├── build_linux_deb.sh             # Script đóng gói file .deb trên Linux
│   └── install_linux.sh               # Script cài đặt tự động đa bản phân phối Linux
└── tests/
    ├── test_cpp_engine.cpp            # Bộ 109 bài kiểm thử tự động (Unit Tests) C++17
    └── test_python_engine.py          # Bộ 109 bài kiểm thử tự động (Unit Tests) Python 3
```

## 5. Hướng dẫn Cài đặt & Build trên macOS

### 5.1. Cách Build và Cài đặt nhanh (Khuyên dùng)
Để tránh việc macOS Gatekeeper chặn các file tải từ trình duyệt về (báo lỗi *"Move to Trash"* do cờ `com.apple.quarantine`), bạn hãy mở **Terminal** và chạy đúng 3 dòng lệnh sau tại thư mục vừa giải nén:

```bash
cd ~/Downloads/VisKey
xattr -cr .
bash scripts/build_macos_installer.sh
```

Script sẽ tự động:
1. Chạy 109 bài kiểm thử C++17 để xác minh bộ lõi.
2. Biên dịch ra ứng dụng **`build_macos/VisKey.app`** (Universal Binary chạy trên cả chip Apple Silicon M1/M2/M3/M4 và Intel) và ký số cục bộ (`codesign --force --deep --sign -`).
3. Đóng gói ra file bộ cài **`VisKey-macOS-Installer.dmg`** ngay trong thư mục dự án.
4. Cài đặt `VisKey.app` vào `/Applications` và khởi chạy ứng dụng.

### 5.2. Xử lý khi macOS báo lỗi chưa có chữ ký (Gatekeeper)
Nếu bạn copy file `VisKey.app` sang một máy Mac khác và gặp thông báo bắt xóa vào Thùng rác (*"Move to Trash"*), chỉ cần mở Terminal chạy:

```bash
sudo xattr -rd com.apple.quarantine /Applications/VisKey.app
sudo codesign --force --deep --sign - /Applications/VisKey.app
open /Applications/VisKey.app
```

### 5.3. Cấp quyền Accessibility trên macOS
1. Vào **System Settings (Cài đặt hệ thống) -> Privacy & Security (Quyền riêng tư & Bảo mật) -> Accessibility (Trợ năng)**.
2. Bật công tắc cho **VisKey**.
3. Nếu bạn build lại bản mới đè lên bản cũ mà quyền bị kẹt, hãy chạy lệnh làm mới quyền rồi bật lại:
   ```bash
   tccutil reset Accessibility org.viskey.inputmethod
   open /Applications/VisKey.app
   ```

## 6. Hướng dẫn Cài đặt & Build trên Linux (Ubuntu/Debian, Fedora, Arch)

### 6.1. Cài đặt từ gói `.deb` có sẵn (Ubuntu, Debian, Linux Mint, Pop!_OS)
Tải `ibus-viskey_1.0.0_all.deb` từ [GitHub Releases](https://github.com/1vision-work/VisKey/releases) (hoặc tự đóng gói bằng `bash scripts/build_linux_deb.sh`), rồi chạy:
```bash
sudo apt update
sudo apt install -y ibus python3-gi gir1.2-ibus-1.0 python3-tk
sudo dpkg -i ibus-viskey_1.0.0_all.deb
ibus restart
```

### 6.2. Cài đặt tự động trên mọi bản phân phối (Ubuntu, Fedora, Arch)
```bash
# Trên Fedora: sudo dnf install -y ibus python3-gobject python3-tkinter
# Trên Arch  : sudo pacman -S --needed ibus python-gobject tk
bash scripts/install_linux.sh
```

### 6.3. Kích hoạt và Mở Bảng điều khiển trên Linux
* Thêm bộ gõ trong **Settings -> Keyboard -> Input Sources -> Add (+) -> Vietnamese -> VisKey (Tiếng Việt)**, hoặc chạy lệnh:
  ```bash
  ibus engine viskey
  ```
* Mở cửa sổ giao diện cấu hình & quản lý Gõ tắt bằng lệnh:
  ```bash
  viskey-settings
  ```

## 7. Hướng dẫn Chạy & Build trên Windows (Windows 10 & Windows 11)

* **Cách 1 — Chạy ngay không cần biên dịch (`Run_VisKey_Windows.bat`):**
  Nhấp đúp vào file **`Run_VisKey_Windows.bat`** (hoặc `windows\VisKey_Windows_Portable.pyw` nếu máy đã cài Python 3) để mở thẳng giao diện Bảng điều khiển và kích hoạt bộ gõ.
* **Cách 2 — Biên dịch ra file thực thi độc lập `VisKey64.exe` (`Build_Windows.bat`):**
  Nếu máy có cài **MinGW-w64 (`g++`)** hoặc **Visual Studio (`cl.exe`)**, chỉ cần nhấp đúp vào **`Build_Windows.bat`**. Script sẽ tự động kiểm thử 109 bài test C++17 và biên dịch ra file **`VisKey64.exe`** độc lập (không phụ thuộc `.NET` hay DLL ngoài).

## 8. Chạy Kiểm thử Tự động (Unit Tests) & Giả lập CLI

Bạn có thể kiểm tra trực tiếp 109 kịch bản kiểm thử hoặc gõ thử trên Terminal bất cứ lúc nào:

```bash
# 1. Chạy 109 bài kiểm thử tự động trên nhân Python:
python3 tests/test_python_engine.py

# 2. Biên dịch và chạy 109 bài kiểm thử tự động trên nhân C++17:
g++ -std=c++17 -O2 -Wall -Wextra tests/test_cpp_engine.cpp -o test_cpp_engine && ./test_cpp_engine

# 3. Chạy trình giả lập gõ tiếng Việt tương tác ngay trong Terminal:
g++ -std=c++17 -O2 core/viskey_cli.cpp -o viskey_cli && ./viskey_cli
```
