# VisKey — Bộ Gõ Tiếng Việt Đa Nền Tảng (macOS, Linux & Windows)

[![CI](https://github.com/1vision-work/VisKey/actions/workflows/ci.yml/badge.svg?branch=master)](https://github.com/1vision-work/VisKey/actions/workflows/ci.yml)
[![Windows](https://github.com/1vision-work/VisKey/actions/workflows/windows.yml/badge.svg?branch=master)](https://github.com/1vision-work/VisKey/actions/workflows/windows.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

> **Tải về:** bộ cài dựng sẵn có tại trang [**GitHub Releases**](https://github.com/1vision-work/VisKey/releases) (xem [mục 5](#5-cài-đặt-từ-github-releases)). File nhị phân không được lưu trong mã nguồn.

**VisKey** là bộ gõ Tiếng Việt Telex/VNI cho **macOS** 11 trở lên (Apple Silicon & Intel), **Linux** (qua IBus) và **Windows** 10/11 (x64; bản ARM64 là thử nghiệm).

## 1. Giấy phép & Mã nguồn

* **Giấy phép MIT** ([`LICENSE`](LICENSE), Copyright (c) 2026 VisKey Project): được tự do sử dụng, sửa đổi, phân phối lại và dùng cho mục đích thương mại, với điều kiện giữ nguyên thông báo bản quyền và nội dung giấy phép trong các bản sao.
* **Không nhúng mã hay thư viện bên thứ ba:** kho mã không chứa thư viện, font chữ hay bộ biểu tượng của bên thứ ba (vì vậy không có thông báo ghi công bên thứ ba bắt buộc nào). Các biểu tượng trong `design/icons/` và `linux/ibus/icons/` là SVG của dự án. Ứng dụng chỉ dùng API và tài nguyên có sẵn của hệ điều hành: Cocoa/CoreGraphics/Carbon trên macOS (kể cả SF Symbols), Win32/Direct2D/DirectWrite trên Windows (kể cả font biểu tượng Segoe Fluent Icons / Segoe MDL2 Assets nếu máy có), IBus + GTK4/libadwaita (hoặc Tkinter) trên Linux.
* VisKey không phải bản fork và không liên kết với mã nguồn của Unikey, OpenKey, EVKey, IBus-Bamboo hay Fcitx5-Unikey.

## 2. Tính năng (đối chiếu với mã nguồn)

Bộ lõi có hai bản tương đương: C++17 header-only (`core/viskey_engine.hpp`, dùng cho macOS, Windows và CLI) và Python 3 (`core/viskey_engine.py`, dùng cho Linux).

| Tính năng | Cách hoạt động | Nền tảng |
| :--- | :--- | :--- |
| **Telex & VNI** | Chuyển kiểu gõ trong Bảng điều khiển, bảng nhanh, hoặc bằng phím tắt (macOS/Windows; mặc định chưa gán). | Cả 3 |
| **Đặt dấu tự do & tự dịch dấu** | Dấu thanh được lưu riêng và đặt lại mỗi khi âm tiết thay đổi (`hoas` → `hóa`, gõ thêm `n` → `hoán`; `tuasna` → `tuấn`). Tùy chọn **Kiểu cũ** (`hòa`, `thúy`, `khỏe` — mặc định) hoặc **Kiểu mới** (`hoà`, `thuý`, `khoẻ`), áp dụng cho vần `oa`, `oe`, `uy` không có phụ âm cuối. | Cả 3 |
| **Kiểm tra âm vị & tự khôi phục từ tiếng Anh** | Kiểm tra 27 phụ âm đầu, 8 phụ âm cuối, bảng tổ hợp nguyên âm hợp lệ, luật `c/k/gh/ngh`, luật chỉ cho dấu sắc/nặng với âm cuối `c/ch/p/t`, kèm danh sách khoảng 300 từ tiếng Anh dễ nhầm. Từ không hợp lệ được trả về nguyên phím đã gõ (`window`, `software`, `refresh`, `book`, `feel`, `view`, `test`, `macos`, `linux`…). Bật mặc định. | Cả 3 |
| **Sửa lỗi gợi ý (autocomplete) của trình duyệt** | Trước khi viết lại chữ, VisKey gửi ký tự `U+202F` (Narrow No-Break Space) kèm một phím xóa để bỏ vùng gợi ý đang bôi đen. Dùng khi bung gõ tắt, khôi phục từ tiếng Anh, sửa sau phím xóa, thay từ 1 ký tự, và cho mọi lần sửa 1 ký tự trong các ứng dụng có sẵn trong danh sách — macOS: Spotlight, Safari, Chrome, Arc, Brave, Edge, Firefox, Excel; Windows: `chrome`, `msedge`, `brave`, `firefox`, `arc`, `searchhost`, `excel`. Bật mặc định. | macOS, Windows |
| **Gõ không gạch chân** | macOS bắt phím bằng `CGEventTap`, Windows bằng `WH_KEYBOARD_LL`, rồi gửi thẳng chuỗi Unicode. Linux có tùy chọn **Không gạch chân** (bật mặc định): xóa chữ cũ bằng `delete_surrounding_text` nếu ứng dụng hỗ trợ, nếu không thì giả lập phím BackSpace. Khi tắt tùy chọn này (dùng preedit), từ đang gõ được chốt lại khi ô nhập mất focus. | Cả 3 |
| **Ghi nhớ VI/EN theo ứng dụng** | Lưu chế độ riêng cho từng ứng dụng (Bundle ID trên macOS, tên `.exe` trên Windows). Mặc định dùng EN cho: Terminal, iTerm2, Warp, Alacritty, kitty (macOS); `cmd`, `powershell`, `pwsh`, Windows Terminal, `mintty`, `conhost` (Windows). | macOS, Windows |
| **Tự bật lại event tap & Secure Input** | Tự bật lại `CGEventTap` khi macOS tắt nó (do quá thời gian hoặc do người dùng). Khi bật Secure Input (ô mật khẩu), VisKey để phím đi thẳng, không xử lý. | macOS |
| **Gõ tắt giữ kiểu chữ hoa** | Có sẵn: `vn` → `Việt Nam`, `ko` → `không`, `dc` → `được`, `uh` → `ừ`, `ntn` → `như thế nào`, `bt` → `bình thường`, `mn` → `mọi người`. Giữ kiểu chữ (`ko` → `không`, `Ko` → `Không`, `KO` → `KHÔNG`). Nhập/xuất danh sách dạng JSON hoặc dạng `viết tắt<TAB>nội dung`, có đếm số lần dùng. | Cả 3 |
| **Quick Telex** | `cc=ch`, `gg=gi`, `kk=kh`, `nn=ng`, `qq=qu`, `pp=ph`, `tt=th` ở đầu từ, chỉ trong Telex. Tắt mặc định. | Cả 3 |
| **Kiểm tra cập nhật** | Bấm nút kiểm tra, hoặc tự kiểm tra tối đa 1 lần mỗi 24 giờ (bật mặc định, tắt được). Chỉ đọc danh sách bản phát hành công khai trên GitHub và báo khi có bản mới; không tự tải hay tự cài. | Cả 3 |

## 3. Giao diện & Phím tắt

### 3.1. Biểu tượng trạng thái
* **macOS:** biểu tượng VisKey kèm chữ `VI` / `EN` trên thanh menu. Bấm chuột trái mở bảng nhanh (VI/EN, Telex/VNI, quy tắc cho ứng dụng đang mở). Bấm chuột phải (hoặc Ctrl+click) mở menu rút gọn: *Bảng điều khiển…* (`Cmd + ,`), *Thoát VisKey*. Khi đổi VI/EN, một HUD nhỏ hiện lên (tắt được).
* **Windows:** biểu tượng ở khay hệ thống (màu khi đang gõ VI, xám khi đang gõ EN). Bấm chuột trái mở bảng nhanh, bấm đúp mở Bảng điều khiển, bấm chuột phải mở menu (Mở Bảng điều khiển / Bật-Tắt tiếng Việt / Thoát VisKey).
* **Linux:** VisKey không có biểu tượng khay riêng. Nó dùng biểu tượng của IBus (ký hiệu `VI`), và menu của IBus có mục chọn Telex/VNI.

### 3.2. Phím tắt mặc định

| Thao tác | macOS | Windows | Linux |
| :--- | :--- | :--- | :--- |
| Bật/tắt tiếng Việt | `Ctrl + Space` (đổi được; có phím phụ, mặc định chưa gán) | `Ctrl + Space` hoặc `Alt + Z` (đổi được) | Phím chuyển bộ gõ của IBus / GNOME (VisKey không đặt phím riêng) |
| Đổi Telex ↔ VNI | Chưa gán (đặt trong *Phím tắt*) | Chưa gán (đặt trong *Phím tắt*) | Menu IBus |
| Mở Bảng điều khiển | `Cmd + ,` | Bấm đúp biểu tượng khay | Lệnh `viskey-settings` |

### 3.3. Bảng điều khiển (tự lưu, không có nút "Lưu")
* **macOS:** các trang *Chung, Gõ tắt, Theo ứng dụng, Phím tắt, Quyền & Bảo mật, Giới thiệu* (kiểm tra cập nhật nằm trong *Giới thiệu*). Lần đầu mở, hoặc khi chưa có quyền Trợ năng, có cửa sổ hướng dẫn 3 bước: chọn kiểu gõ, cấp quyền, gõ thử. Cấu hình lưu trong `NSUserDefaults`. Tùy chọn khởi động cùng máy (tắt mặc định) tạo file `~/Library/LaunchAgents/org.viskey.inputmethod.plist`.
* **Windows (`VisKey64.exe`):** các trang *Chung, Gõ tắt, Theo ứng dụng, Phím tắt, Cập nhật, Giới thiệu*, giao diện sáng/tối theo Windows. Cấu hình lưu trong Registry `HKCU\Software\VisKey` (khóa con `Macros`, `MacroUsage`, `AppRules`). Tùy chọn khởi động cùng Windows ghi giá trị `VisKey` vào `HKCU\Software\Microsoft\Windows\CurrentVersion\Run`.
* **Linux (`viskey-settings`):** giao diện GTK4/libadwaita (nếu thiếu thì dùng bản Tkinter) với các tab *Chung, Gõ tắt, Giới thiệu*. Cấu hình lưu ở `~/.config/viskey/config.json`; bộ gõ đọc lại file này khi ô nhập nhận focus, nên không cần khởi động lại IBus.
* Cả 3 nền tảng đều có tham số `--ui-snapshot <thư-mục>` để chụp ảnh giao diện bằng dữ liệu mẫu (không bắt phím, không đọc cấu hình thật):
  `VisKey.app/Contents/MacOS/VisKey --ui-snapshot ./shots` · `VisKey64.exe --ui-snapshot shots` · `python3 linux/ibus/viskey_settings.py --ui-snapshot shots`.

## 4. Cấu trúc Thư mục Dự án

```text
VisKey/
├── README.md                          # Tài liệu tổng quan (file này)
├── LICENSE                            # Giấy phép MIT
├── VISKEY_BUILD_GUIDE.md              # Hướng dẫn build & đóng gói chi tiết
├── VISKEY_ARCHITECTURE_FOR_DEVS.md    # Kiến trúc & thuật toán cho developer
├── CMakeLists.txt                     # CMake: CLI, các bài test (ctest), VisKey.app (macOS), VisKey64.exe (Windows)
├── Install_macOS.command              # Nhấp đúp trên macOS để chạy scripts/build_macos_installer.sh
├── Build_Windows.bat                  # Script build Windows cũ (xem mục 6.3)
├── Run_VisKey_Windows.bat             # Chạy VisKey64.exe nếu có, nếu không thì chạy bản Python portable
├── .github/workflows/
│   ├── ci.yml                         # Linux (.deb) + macOS (.app/.dmg), tạo pre-release khi push tag v*
│   └── windows.yml                    # VisKey64.exe x64/ARM64, zip portable, bộ cài Inno Setup
├── core/
│   ├── viskey_engine.hpp              # Bộ lõi C++17 header-only
│   ├── viskey_engine.py               # Bộ lõi Python 3 (dùng cho Linux)
│   └── viskey_cli.cpp                 # Công cụ gõ thử trên Terminal
├── macos/
│   ├── VisKeyApp.mm                   # Ứng dụng macOS (thanh menu, CGEventTap)
│   ├── Info.plist                     # Bundle ID org.viskey.inputmethod, LSUIElement, macOS 11.0+
│   └── ui/                            # Bảng điều khiển, bảng nhanh, HUD, hướng dẫn lần đầu, kiểm tra cập nhật
├── linux/ibus/
│   ├── ibus_viskey.py                 # Bộ gõ IBus
│   ├── viskey_settings.py             # Bảng điều khiển GTK4/libadwaita (viskey-settings)
│   ├── viskey_settings_tk.py          # Bảng điều khiển dự phòng bằng Tkinter
│   ├── viskey_settings_model.py       # Logic cấu hình dùng chung (đọc/ghi config, gõ tắt, cập nhật)
│   ├── viskey_version.py              # Số phiên bản Linux (dùng khi đóng gói .deb)
│   ├── viskey.xml                     # Khai báo component IBus
│   └── icons/                         # Biểu tượng symbolic cho giao diện Linux
├── windows/
│   ├── VisKeyWin.cpp                  # Ứng dụng Windows (khay hệ thống, WH_KEYBOARD_LL, SendInput)
│   ├── ui/                            # Bảng điều khiển & bảng nhanh (Direct2D), lưu Registry, chính sách cập nhật
│   ├── res/                           # Icon + thông tin phiên bản (VisKey.rc, VisKey.ico)
│   ├── installer/VisKey.iss           # Bộ cài Inno Setup (x64, cài cho người dùng hiện tại)
│   ├── tests/test_update_policy.cpp   # Test logic kiểm tra cập nhật trên Windows
│   ├── README-Windows.txt             # Hướng dẫn kèm trong file zip portable
│   └── VisKey_Windows_Portable.pyw    # Bản Python (ctypes + Tkinter) rút gọn, không cần biên dịch
├── design/                            # tokens.json (màu, bố cục) và biểu tượng SVG
├── docs/UI_V2.md                      # Đặc tả giao diện chung cho 3 nền tảng
├── scripts/
│   ├── build_macos_installer.sh       # Build VisKey.app (Universal nếu được) + VisKey-macOS-Installer.dmg
│   ├── build_linux_deb.sh             # Đóng gói ibus-viskey_<phiên bản>_all.deb
│   └── install_linux.sh               # Cài đặt trên Debian/Ubuntu (qua .deb) hoặc chép file trực tiếp
└── tests/
    ├── test_cpp_engine.cpp            # 109 bài test bộ lõi C++
    ├── test_python_engine.py          # 109 bài test bộ lõi Python
    ├── data/typing_regression.tsv     # 2.750 kịch bản gõ Telex/VNI (sinh tự động)
    ├── gen_typing_regression.py       # Sinh file .tsv (--check để kiểm tra file có cũ không)
    ├── test_typing_regression.cpp     # Chạy các kịch bản .tsv trên bộ lõi C++
    ├── test_typing_regression.py      # Chạy các kịch bản .tsv trên bộ lõi Python
    ├── test_update_policy.cpp         # Test logic kiểm tra cập nhật trên macOS
    ├── test_linux_settings.py         # 42 bài test logic cấu hình Linux
    ├── test_linux_update_check.py     # 20 bài test kiểm tra cập nhật Linux
    └── test_design_tokens.py          # Kiểm tra design/tokens.json
```

## 5. Cài đặt từ GitHub Releases

Khi push tag `v*`, CI tạo một **pre-release** trên trang [Releases](https://github.com/1vision-work/VisKey/releases). File Windows được workflow riêng đính kèm nên có thể xuất hiện chậm vài phút.

| Nền tảng | File |
| :--- | :--- |
| macOS (Apple Silicon + Intel) | `VisKey-macOS-Installer.dmg` |
| Linux (Debian/Ubuntu) | `ibus-viskey_<phiên bản>_all.deb` |
| Windows x64 | `VisKey-<phiên bản>-Windows-x64-portable.zip`, `VisKey-<phiên bản>-Windows-Setup.exe` |
| Windows ARM64 (thử nghiệm) | `VisKey-<phiên bản>-Windows-arm64-portable.zip` |

### 5.1. macOS
1. Mở `VisKey-macOS-Installer.dmg` rồi kéo **VisKey.app** vào **Applications**.
2. Ứng dụng chỉ được ký ad-hoc. Lần đầu, bấm chuột phải vào VisKey.app → **Open**, hoặc chạy:
   ```bash
   xattr -cr /Applications/VisKey.app
   open /Applications/VisKey.app
   ```
3. Cấp quyền **System Settings → Privacy & Security → Accessibility → VisKey** (cửa sổ hướng dẫn sẽ tự nhận ra khi quyền đã được cấp). Nếu cài bản mới đè lên bản cũ mà bộ gõ không nhận phím dù công tắc vẫn bật, hãy làm mới quyền:
   ```bash
   tccutil reset Accessibility org.viskey.inputmethod
   open /Applications/VisKey.app
   ```

### 5.2. Linux (Debian, Ubuntu, Mint, Pop!_OS…)
```bash
sudo apt install ./ibus-viskey_*_all.deb
ibus restart
```
Gói cần `python3`, `ibus`, `python3-gi`, `gir1.2-ibus-1.0` và `gir1.2-adw-1` (để có giao diện GTK4/libadwaita) hoặc `python3-tk` (giao diện dự phòng). Sau đó thêm bộ gõ ở **Settings → Keyboard → Input Sources → Add (+) → Vietnamese → VisKey (Tiếng Việt)** (hoặc dùng `ibus-setup`), hoặc chạy `ibus engine viskey`. Mở Bảng điều khiển bằng `viskey-settings` hoặc mục **VisKey** trong menu ứng dụng.

Nếu dùng môi trường tối giản (i3, Sway, Hyprland…), hãy đặt các biến sau trong `~/.profile`:
```bash
export GTK_IM_MODULE=ibus
export QT_IM_MODULE=ibus
export XMODIFIERS=@im=ibus
```

### 5.3. Windows
* **Portable:** giải nén `VisKey-*-Windows-*-portable.zip` vào một thư mục bất kỳ rồi chạy `VisKey64.exe`. Chỉ có một file exe, không cần cài đặt.
* **Bộ cài:** chạy `VisKey-*-Windows-Setup.exe`. Chỉ có bản x64, cài cho người dùng hiện tại, không cần quyền admin.
* File chưa được ký số. Nếu SmartScreen cảnh báo, bấm **More info → Run anyway**.
* Để cập nhật, thoát VisKey rồi thay `VisKey64.exe` bằng bản mới; cài đặt được giữ nguyên. Cách gỡ bỏ có trong `README-Windows.txt` (đi kèm file zip).

## 6. Build từ mã nguồn

Chi tiết xem thêm [`VISKEY_BUILD_GUIDE.md`](VISKEY_BUILD_GUIDE.md). Số phiên bản nằm ở `project(VisKey VERSION …)` trong `CMakeLists.txt`, `macos/Info.plist` và `linux/ibus/viskey_version.py`.

### 6.1. macOS (cần Xcode Command Line Tools: `xcode-select --install`)
```bash
xattr -cr .                              # nếu mã nguồn được tải bằng trình duyệt
bash scripts/build_macos_installer.sh    # hoặc nhấp đúp Install_macOS.command
```
Script sẽ:
1. Kiểm tra `clang++`.
2. Chạy `tests/test_cpp_engine.cpp` (109 bài test) và `tests/test_update_policy.cpp`.
3. Build `build_macos/VisKey.app` dạng Universal (arm64 + x86_64); nếu không được thì build cho kiến trúc của máy. Sau đó ký ad-hoc (`codesign --force --deep --sign -`).
4. Tạo `VisKey-macOS-Installer.dmg` ở thư mục gốc, rồi hỏi có cài vào `/Applications` và mở luôn không (mặc định là có).

Nếu dùng CMake: `cmake -S . -B build && cmake --build build -j && ctest --test-dir build` (cho ra `build/VisKey.app`).

### 6.2. Linux
```bash
bash scripts/build_linux_deb.sh          # tạo ibus-viskey_<phiên bản>_all.deb ở thư mục gốc
bash scripts/install_linux.sh            # cài đặt (xem bên dưới)
```
`install_linux.sh` chạy tự kiểm tra của `core/viskey_engine.py`, rồi:
* **Debian/Ubuntu:** build file `.deb` nếu chưa có, cài các gói phụ thuộc rồi chạy `dpkg -i`.
* **Bản phân phối khác:** chép file vào `/usr/share/viskey`, `/usr/libexec/ibus-engine-viskey` và `/usr/share/ibus/component`. Cần cài trước các gói phụ thuộc, ví dụ:
  ```bash
  # Fedora: sudo dnf install -y ibus python3-gobject python3-tkinter
  # Arch  : sudo pacman -S --needed ibus python-gobject tk
  ```
  Cách này không tạo lệnh `viskey-settings` hay mục trong menu ứng dụng. Hãy mở Bảng điều khiển bằng `python3 /usr/share/viskey/ibus/viskey_settings.py`.

Script kết thúc bằng `ibus restart`.

### 6.3. Windows (Visual Studio + CMake)
```bat
cmake -S . -B build -A x64
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```
Kết quả là `build\Release\VisKey64.exe`: một file exe, không cần DLL runtime đi kèm. Dùng `-A ARM64` để build cho ARM64. Có thể tạo bộ cài (tùy chọn) bằng Inno Setup 6:
```bat
iscc /DAppVersion=1.0.0 /DSourceExe=build\Release\VisKey64.exe /DRepoRoot=. /Odist windows\installer\VisKey.iss
```

> **Lưu ý:** `Build_Windows.bat` hiện chỉ biên dịch `windows\VisKeyWin.cpp`, thiếu các file `windows\ui\*.cpp` và `VisKey.rc`, nên không build được bản hiện tại. Hãy dùng CMake như trên. `Run_VisKey_Windows.bat` chạy `VisKey64.exe` ở thư mục gốc nếu có; nếu không có, nó chạy `windows\VisKey_Windows_Portable.pyw` bằng Python, và nếu cũng không có Python thì gọi `Build_Windows.bat`.
>
> `VisKey_Windows_Portable.pyw` là bản rút gọn, không cần biên dịch (cửa sổ Tkinter, `Ctrl + Space` / `Alt + Z` cố định). Bản này không có biểu tượng khay, không ghi nhớ theo ứng dụng, không kiểm tra cập nhật, và **không lưu cấu hình** khi thoát.

## 7. Kiểm thử & Gõ thử trên Terminal

```bash
# Bộ lõi (109 bài test mỗi bản):
python3 tests/test_python_engine.py
g++ -std=c++17 -O2 -Wall -Wextra tests/test_cpp_engine.cpp -o test_cpp_engine && ./test_cpp_engine

# 2.750 kịch bản gõ Telex/VNI (tests/data/typing_regression.tsv):
python3 tests/gen_typing_regression.py --check
python3 tests/test_typing_regression.py

# Logic cấu hình & kiểm tra cập nhật trên Linux, design tokens:
python3 tests/test_linux_settings.py
python3 tests/test_linux_update_check.py
python3 tests/test_design_tokens.py

# Toàn bộ test C++ qua CMake (bộ lõi, kịch bản gõ, chính sách cập nhật macOS & Windows):
cmake -S . -B build && cmake --build build -j && ctest --test-dir build --output-on-failure

# Gõ thử tương tác (":vni" / ":telex" để đổi kiểu gõ, "q" để thoát):
g++ -std=c++17 -O2 core/viskey_cli.cpp -o viskey_cli && ./viskey_cli
```

CI (`.github/workflows/`) chạy các bài test này trên Linux, macOS và Windows. CI cũng build và kiểm tra file `.deb`, `.dmg` và `VisKey64.exe`, và chụp ảnh giao diện bằng `--ui-snapshot`.

## 8. Credits / Ghi công

1. **Ý tưởng & thiết kế sản phẩm:** Ngô Tiến Đạt (datntpro)
2. **Phát triển:** AI Gemini 4
3. **Thiết kế UI:** Claude Design
4. **Build:** Grok Bot
5. **Sản phẩm của** 1VISION / Ngô Tiến Đạt
