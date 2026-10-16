# VisKey

Bộ gõ tiếng Việt cho macOS (và Linux qua IBus) — mục tiêu: gõ đúng ở mọi nơi, kể cả Spotlight, Terminal, thanh địa chỉ trình duyệt.

- By 1VISION · License GPL-3.0-or-later
- Based on OpenKey by Mai Vũ Tuyên — https://github.com/tuyenvm/OpenKey

## Hai nhánh phát triển trong repo

| | (a) App macOS native (Swift) | (b) `crossplatform/` (macOS + Linux) |
| :--- | :--- | :--- |
| Thư mục | `Engine/`, `VisKey/`, `VisKeyTests/`, `Tools/`, `design/`, `Config/`, `project.yml` | [`crossplatform/`](crossplatform/README.md) |
| Engine | C++17 của OpenKey, refactor sang C API (`Engine/include/viskey_engine.h`) | Engine viết riêng, độc lập với engine OpenKey: C++17 header-only (`core/viskey_engine.hpp`) + bản Python 3 tương đương (`core/viskey_engine.py`) |
| Nền tảng | macOS 13+ (Swift/SwiftUI + `CGEventTap`) | macOS 11+ (Objective-C++ `CGEventTap`, menu bar) · Linux (IBus engine + công cụ cấu hình `viskey-settings`) |
| Trạng thái | **Đang phát triển, milestone M0.** Chưa có bản phát hành. Spec: [CLAUDE.md](CLAUDE.md) | Có bản **pre-release** cài đặt được (xem [Tải về](#tải-về)) |

## Tính năng (nhánh `crossplatform/`)

Chỉ liệt kê những gì đã có trong mã nguồn `crossplatform/`:

- Kiểu gõ **Telex** và **VNI**; xuất **Unicode dựng sẵn** (chưa có bảng mã khác).
- Đặt dấu **kiểu cũ** (`hòa`, `khỏe`, `thúy`) hoặc **kiểu mới** (`hoà`, `khoẻ`, `thuý`); dấu tự dịch chuyển khi gõ thêm nguyên âm/phụ âm cuối (gõ dấu tự do, ví dụ `h-o-s-a-n` → `hoán`).
- **Tự khôi phục từ tiếng Anh**: kiểm tra cấu trúc âm tiết tiếng Việt (phụ âm đầu/cuối, vần, luật thanh với `c/ch/p/t`) + danh sách từ trùng (`soon`, `test`, `code`…), ví dụ `window`, `software`, `book` không bị biến thành chữ Việt.
- **Gõ tắt (macro)** giữ kiểu hoa/thường (`ko` → `không`, `Ko` → `Không`, `KO` → `KHÔNG`); có sẵn vài macro mặc định (`vn`, `ko`, `dc`, `uh`, `ntn`, `bt`, `mn`).
- **Quick Telex** (`cc=ch, gg=gi, kk=kh, nn=ng, qq=qu, pp=ph, tt=th`), mặc định tắt.
- Thay thế tối thiểu: chỉ xoá phần khác nhau giữa từ cũ và từ mới (tiền tố chung dài nhất).
- **macOS** (`crossplatform/macos/VisKeyApp.mm`):
  - Menu bar `🇻🇳 VI` / `🇬🇧 EN`; menu chọn Telex/VNI, kiểu đặt dấu, tự khôi phục tiếng Anh, sửa lỗi autocomplete, nhớ Anh/Việt theo app, bật/tắt gõ tắt + Quick Telex.
  - Phím tắt bật/tắt: `Ctrl+Space` hoặc `Cmd+Shift+V` (lưu ý `Ctrl+Space` trùng phím tắt đổi nguồn nhập mặc định của macOS).
  - Sửa lỗi mất/đúp chữ ở ô có gợi ý inline (Spotlight, Safari, Chrome, Arc, Brave, Edge, Firefox, Excel) bằng cách chèn `U+202F` trước khi xoá.
  - Nhớ chế độ Anh/Việt theo từng app; Terminal, iTerm2, Warp, Alacritty, kitty mặc định là EN.
  - Tự bật lại event tap khi bị macOS tắt (timeout); nhường phím khi đang ở ô mật khẩu (Secure Event Input); reset từ đang gõ khi click chuột hoặc đổi app.
  - Hạn chế hiện tại: chưa có cửa sổ cài đặt, chưa có UI quản lý gõ tắt, các tuỳ chọn và chế độ theo app **chưa được lưu** sau khi thoát app, chưa có tự khởi động cùng hệ thống.
- **Linux** (`crossplatform/linux/`):
  - IBus engine `viskey` (hiển thị "VisKey (Tiếng Việt)"), chế độ **Không gạch chân** (mặc định bật: dùng surrounding text, nếu app không hỗ trợ thì gửi Backspace) hoặc chế độ preedit tự chốt chữ khi mất focus/reset.
  - Menu IBus: bật/tắt tiếng Việt, Telex/VNI, không gạch chân, tự khôi phục tiếng Anh, đặt dấu kiểu mới.
  - `viskey-settings`: giao diện Tkinter + dòng lệnh (`--print-config`, `--set KEY=VALUE`) để chỉnh tuỳ chọn và quản lý gõ tắt; lưu ở `~/.config/viskey/config.json` (theo `$XDG_CONFIG_HOME`), IBus engine tự nạp lại khi file đổi.

## Tải về

Pre-release [**v1.0.0-installers**](https://github.com/1vision-work/VisKey/releases/tag/v1.0.0-installers) (build bởi CI từ `crossplatform/`, workflow [`.github/workflows/installers.yml`](.github/workflows/installers.yml)):

| File | Dành cho |
| :--- | :--- |
| [VisKey-macOS-Installer.dmg](https://github.com/1vision-work/VisKey/releases/download/v1.0.0-installers/VisKey-macOS-Installer.dmg) | macOS 11+ (Universal: Apple Silicon + Intel), kéo-thả vào Applications |
| [VisKey-1.0.0-macOS.pkg](https://github.com/1vision-work/VisKey/releases/download/v1.0.0-installers/VisKey-1.0.0-macOS.pkg) | macOS 11+, trình cài đặt cài vào `/Applications` |
| [ibus-viskey_1.0.0_all.deb](https://github.com/1vision-work/VisKey/releases/download/v1.0.0-installers/ibus-viskey_1.0.0_all.deb) | Debian / Ubuntu / Linux Mint / Pop!_OS (IBus) |

## Cài đặt trên macOS

> ⚠️ **Lưu ý trung thực:** `VisKey.app` chỉ được **ký ad-hoc**, **chưa notarize**, và **chưa được thử trên máy Mac thật** — CI chỉ kiểm tra build Universal, chữ ký ad-hoc, `Info.plist` và nội dung `.pkg`/`.dmg`. Hãy coi đây là bản thử nghiệm.

1. Tải `.dmg` (mở ra, kéo `VisKey.app` vào `Applications`) hoặc `.pkg` (mở để cài vào `/Applications`).
2. Vượt Gatekeeper (vì chưa notarize): trong Finder **chuột phải › Open** (với `.pkg` và với `VisKey.app`), hoặc chạy:
   ```sh
   xattr -cr /Applications/VisKey.app
   ```
3. Mở VisKey, rồi vào **System Settings › Privacy & Security** cấp quyền:
   - **Accessibility (Trợ năng)** cho VisKey — bắt buộc, app tự hiện hộp thoại hỏi quyền ở lần chạy đầu;
   - **Input Monitoring (Theo dõi đầu vào)** cho VisKey nếu macOS yêu cầu hoặc nếu vẫn không gõ được.
   
   Sau khi cấp quyền, thoát VisKey (menu › Thoát VisKey) rồi mở lại — bản này chỉ tạo event tap lúc khởi động.
4. Biểu tượng `🇻🇳 VI` xuất hiện trên menu bar. Nếu cài đè bản mới mà không gõ được dù công tắc vẫn bật: `tccutil reset Accessibility org.viskey.inputmethod` rồi cấp lại quyền.

## Cài đặt trên Linux (Debian/Ubuntu)

```sh
sudo apt install ./ibus-viskey_1.0.0_all.deb   # tự kéo ibus, python3-gi, gir1.2-ibus-1.0, python3-tk
ibus restart
```

1. Thêm nguồn nhập: **Settings › Keyboard › Input Sources › Add (+) › Vietnamese › VisKey (Tiếng Việt)** (hoặc dùng `ibus-setup`). Có thể kích hoạt ngay bằng `ibus engine viskey`.
2. Cấu hình: chạy `viskey-settings` hoặc mở **VisKey - Cấu hình Bộ gõ Tiếng Việt** trong menu ứng dụng.

Fedora/Arch và cài từ mã nguồn: xem [crossplatform/README.md](crossplatform/README.md) (`crossplatform/scripts/install_linux.sh`).

## Kiến trúc (nhánh `crossplatform/`)

Ba tầng, tách hẳn thuật toán ngôn ngữ khỏi lớp hệ điều hành:

1. **Core engine** (độc lập nền tảng) — `core/viskey_engine.hpp` (C++17) và `core/viskey_engine.py` (Python): nhận từng phím, giữ bộ đệm âm tiết UTF-32, tách âm tiết, đặt dấu, kiểm tra chính tả, khôi phục tiếng Anh, mở rộng macro, trả về số Backspace + chuỗi thay thế.
2. **OS adapter** — `macos/VisKeyApp.mm` (`CGEventTap`, gửi Backspace + chuỗi Unicode, đánh dấu event tự sinh để không xử lý lại) và `linux/ibus/ibus_viskey.py` (`IBus.Engine`, `delete_surrounding_text` / `forward_key_event` + `commit_text`).
3. **UI & lưu trữ** — menu bar `NSStatusItem` trên macOS; `viskey_settings.py` + `config.json` trên Linux.

Chi tiết (cấu trúc dữ liệu, thuật toán đặt dấu, kiểm tra âm vị học, sửa lỗi autocomplete): [docs/crossplatform/ARCHITECTURE.md](docs/crossplatform/ARCHITECTURE.md).

Nhánh Swift có luồng riêng `KeyTap → ContextResolver → EngineBridge → ReplacementPlanner → EventSender`, mô tả trong [CLAUDE.md](CLAUDE.md) §4.

## Cấu trúc repo

```
VisKey/
├─ Engine/             # (a) engine C++ từ OpenKey + C API + test snapshot (CMake)
├─ VisKey/             # (a) app Swift: App/ Bridge/ Core/ Features/ UI/ Resources/
├─ VisKeyTests/        # (a) XCTest
├─ Tools/              # (a) gen-assets.sh, gen-colors.py, gen-icons.py
├─ design/             # (a) thiết kế UI + brand (nguồn sự thật cho giao diện)
├─ Config/             # (a) Signing.xcconfig
├─ project.yml         # (a) XcodeGen
├─ crossplatform/      # (b) dự án macOS + Linux
│  ├─ core/            #     viskey_engine.hpp, viskey_engine.py, viskey_cli.cpp
│  ├─ macos/           #     VisKeyApp.mm, Info.plist
│  ├─ linux/           #     ibus/ (ibus_viskey.py, viskey_settings.py, viskey.xml), viskey-settings.desktop
│  ├─ tests/           #     test_cpp_engine.cpp, test_python_engine.py
│  ├─ scripts/         #     build_macos_installer.sh, build_linux_deb.sh, install_linux.sh
│  ├─ CMakeLists.txt  Install_macOS.command
├─ docs/               # decisions.md (ADR), design-doc.md, crossplatform/ (ARCHITECTURE.md, BUILD.md)
└─ .github/workflows/  # ci.yml (nhánh a), installers.yml (nhánh b)
```

## Build từ mã nguồn & chạy test

### (a) Engine + app Swift

Engine (C++17, chạy được trên Linux và macOS):

```sh
cmake -S Engine -B build/engine && cmake --build build/engine -j
ctest --test-dir build/engine --output-on-failure
```

Xem [Engine/tests/README.md](Engine/tests/README.md) về snapshot so với engine OpenKey gốc.

App macOS (Swift, cần Xcode 15+ và XcodeGen):

```sh
brew install xcodegen
xcodegen generate                  # sinh VisKey.xcodeproj từ project.yml (không commit)
xcodebuild -project VisKey.xcodeproj -scheme VisKey -configuration Debug -derivedDataPath build/DerivedData build
open build/DerivedData/Build/Products/Debug/VisKey.app
xcodebuild -project VisKey.xcodeproj -scheme VisKey -derivedDataPath build/DerivedData test   # XCTest
```

Lần chạy đầu VisKey mở Cài đặt hệ thống › Quyền riêng tư & Bảo mật › Trợ năng; bật VisKey là gõ được, không cần mở lại app.
Bản dev mặc định ký ad-hoc nên mỗi lần build lại macOS coi là app mới: tắt rồi bật lại VisKey trong danh sách Trợ năng (hoặc xoá bằng nút − và cấp lại).
Có chứng chỉ "Apple Development" thì tạo `Config/Signing.local.xcconfig` (không commit, xem [Config/Signing.xcconfig](Config/Signing.xcconfig)) rồi `xcodegen generate` lại; quyền Trợ năng sẽ giữ qua các lần build.

Tài sản giao diện (`VisKey/Resources/Assets.xcassets`) sinh từ `design/` bằng `Tools/gen-assets.sh`.

### (b) `crossplatform/`

Chạy trong thư mục `crossplatform/`. Bộ test C++ và bộ test Python mỗi bộ có **109 ca**:

```sh
cd crossplatform
python3 tests/test_python_engine.py                                   # 109 ca, engine Python
g++ -std=c++17 -O2 -Wall tests/test_cpp_engine.cpp -o build_test_cpp && ./build_test_cpp   # 109 ca, engine C++
g++ -std=c++17 -O2 -Icore core/viskey_cli.cpp -o viskey_cli && ./viskey_cli               # giả lập gõ trong terminal

# hoặc bằng CMake (CLI + test C++; trên macOS thêm target VisKey.app)
cmake -S . -B build && cmake --build build -j && ctest --test-dir build --output-on-failure
```

Đóng gói:

```sh
bash scripts/build_macos_installer.sh   # macOS: build_macos/VisKey.app, VisKey-macOS-Installer.dmg, VisKey-1.0.0-macOS.pkg
bash scripts/build_linux_deb.sh         # Linux: ibus-viskey_1.0.0_all.deb (đổi thư mục ra bằng OUT_DIR=...)
```

Hướng dẫn chi tiết: [docs/crossplatform/BUILD.md](docs/crossplatform/BUILD.md) và [crossplatform/README.md](crossplatform/README.md).

## License & ghi công

- **GPL-3.0-or-later** — xem [LICENSE](LICENSE) và [NOTICE](NOTICE).
- Based on OpenKey by Mai Vũ Tuyên — https://github.com/tuyenvm/OpenKey
- By 1VISION.
- Spec dự án: [CLAUDE.md](CLAUDE.md). Quyết định kỹ thuật (ADR): [docs/decisions.md](docs/decisions.md).
