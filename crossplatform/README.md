> Thư mục này là bản triển khai đa nền tảng (macOS app + IBus Linux) kèm script đóng gói `.dmg`/`.pkg`/`.deb`, tách biệt với app Swift ở thư mục gốc. Giấy phép: GPL-3.0-or-later (xem `../LICENSE`, `../NOTICE`; dựa trên OpenKey của Mai Vũ Tuyên).

# VisKey — Bộ Gõ Tiếng Việt Đa Nền Tảng (macOS & Linux)

**VisKey** là bộ gõ Tiếng Việt mã nguồn mở được thiết kế chuyên biệt cho **macOS** (Apple Silicon & Intel) và **Linux** (X11 & Wayland qua IBus), tập trung giải quyết triệt để **7 vấn đề cốt lõi** mà người dùng các bộ gõ hiện nay trên thị trường thường xuyên gặp phải.

## 1. Tổng hợp các vấn đề của bộ gõ hiện tại và giải pháp trong VisKey

| Vấn đề trên thị trường | Nguyên nhân kỹ thuật | Giải pháp của VisKey |
| :--- | :--- | :--- |
| **1. Mất chữ đầu / Nhân đôi chữ trên Spotlight, Chrome, Safari, Arc, VSCode (macOS)** | Khi gõ chữ đầu tiên (ví dụ `t`), thanh địa chỉ tự bôi đen phần gợi ý (`t[inhte.vn]`). Phím `Backspace` do bộ gõ gửi chỉ xóa phần gợi ý thay vì xóa chữ `t`, gây ra lỗi `ttiếng` hoặc mất chữ đầu. | **Autocomplete Fix (`U+202F` Injection)**: Trước khi gửi chuỗi `Backspace` trên các ứng dụng có Autocomplete, VisKey bơm ký tự `U+202F` kèm 1 nhịp xóa để hủy vùng bôi đen gợi ý rồi đẩy toàn bộ chuỗi Unicode qua `CGEventKeyboardSetUnicodeString`. |
| **2. Lỗi gạch chân (Preedit Underline) gây nhảy chữ, kẹt phím tắt trên Linux & macOS** | Bộ gõ mặc định giữ từ đang gõ trong bộ đệm gạch chân (*preedit*). Khi nhấn `Ctrl+C`, dùng phím mũi tên trong Vim/Terminal hoặc click chuột sang ô khác, từ đang gõ bị mất hoặc nhảy vị trí. | **Direct Commit / Không gạch chân**: Trên macOS dùng `CGEventTap` trực tiếp; trên Linux hỗ trợ **Chế độ Không gạch chân** (Surrounding Text / Fake Backspace) kèm cơ chế tự động commit tức thời khi chuyển cửa sổ trên Wayland. |
| **3. Biến dạng từ tiếng Anh khi quên tắt bộ gõ (`window` -> `ưindơ`, `software` -> `sòtare`, `book` -> `bôk`)** | Các bộ gõ truyền thống không kiểm tra cấu trúc âm vị học tiếng Việt mà biến đổi mọi phím `w, s, f, r, x, j, oo, ee, aa, dd`. | **Phonotactics Validator & Auto-Restore**: Kiểm tra cấu trúc âm tiết tiếng Việt (phụ âm đầu, 54 vần chuẩn, phụ âm cuối, luật thanh điệu `c/ch/p/t` và vần mở) + từ điển xung đột, tự động khôi phục nguyên vẹn từ tiếng Anh (`window`, `software`, `refresh`, `book`, `feel`, `view`, `test`, `macos`, `linux`...). |
| **4. Đặt dấu sai vị trí khi gõ tự do hoặc thêm nguyên âm/phụ âm sau dấu** | Người dùng gõ dấu trước khi kết thúc vần (`h-o-s-a-n` hoặc `t-h-u-s-o-n-g-w`) khiến dấu nằm sai nguyên âm. | **Dynamic Tone Repositioning**: Tự động tính toán lại vị trí đặt dấu thanh và dấu móc (`ươ`) mỗi khi âm tiết thay đổi; hỗ trợ cả **Kiểu cũ (`hòa`, `thúy`, `khỏe`)** và **Kiểu mới (`hoà`, `thuý`, `khoẻ`)**. |
| **5. Dính tiếng Việt khi chuyển sang Terminal / IDE gõ lệnh** | Bộ gõ giữ chung một trạng thái Bật/Tắt cho toàn hệ thống. | **Smart Per-App Mode**: Tự động ghi nhớ chế độ Anh/Việt riêng cho từng ứng dụng (`Bundle ID` trên macOS) và mặc định chuyển sang `EN` khi vào Terminal, iTerm2, Warp, Alacritty, Kitty. |
| **6. Bộ gõ tự ngắt hoặc treo sau khi Sleep/Wake hoặc cập nhật macOS** | macOS tự động vô hiệu hóa `CGEventTap` khi quá thời gian chờ (`kCGEventTapDisabledByTimeout`) hoặc khi vào ô nhập mật khẩu (`SecureEventInput`). | **EventTap Watchdog & Secure Input Guard**: Tự động kích hoạt lại `CGEventTapEnable` ngay khi nhận tín hiệu timeout và tự động nhường quyền khi `IsSecureEventInputEnabled()` bật. |
| **7. Tốc độ gõ các cụm từ lặp lại chậm** | Thiếu gõ tắt thông minh giữ nguyên chữ hoa/thường. | **Case-Preserving Macro & Quick Telex**: Hỗ trợ gõ tắt tự nhận diện chữ hoa (`ko` -> `không`, `Ko` -> `Không`, `KO` -> `KHÔNG`, `vn` -> `Việt Nam`) và Quick Telex (`cc=ch, gg=gi, kk=kh, nn=ng, qq=qu, pp=ph, tt=th`). |

## 2. Cấu trúc thư mục dự án

* `core/viskey_engine.hpp`: Bộ lõi xử lý tiếng Việt viết bằng **C++17 Header-only** (không phụ thuộc thư viện ngoài, hỗ trợ UTF-8/UTF-32).
* `core/viskey_engine.py`: Bộ lõi xử lý tiếng Việt viết bằng **Python 3** (dùng cho IBus Engine trên Linux).
* `core/viskey_cli.cpp`: Công cụ giả lập gõ tiếng Việt trực tiếp trên Terminal để kiểm thử nhanh.
* `macos/VisKeyApp.mm` & `macos/Info.plist`: Ứng dụng macOS gốc (Objective-C++ / Cocoa Menu Bar + `CGEventTap`).
* `linux/ibus/ibus_viskey.py` & `linux/ibus/viskey.xml`: Bộ gõ IBus Engine cho Linux (X11 & Wayland).
* `linux/ibus/viskey_settings.py` & `linux/viskey-settings.desktop`: Ứng dụng cấu hình **`viskey-settings`** (Tkinter) cho Linux.
* `tests/test_cpp_engine.cpp` & `tests/test_python_engine.py`: Bộ 109 kịch bản kiểm thử tự động (Unit Tests) cho cả C++17 và Python.
* `scripts/build_macos_installer.sh` & `Install_macOS.command`: Script tự động biên dịch `VisKey.app` (Universal Binary) và đóng gói file cài đặt **`VisKey-macOS-Installer.dmg`**.
* `scripts/build_linux_deb.sh` & `scripts/install_linux.sh`: Script đóng gói bộ cài **`ibus-viskey_1.0.0_all.deb`** và cài đặt tự động cho Linux.
* `.github/workflows/build.yml`: CI tự động chạy kiểm thử, build `.deb`, `.dmg`, `.pkg` và đính kèm vào GitHub Release khi đẩy tag `v*`.

## 3. Hướng dẫn Build và Cài đặt trên macOS (Tạo bộ cài `.app` & `.dmg`)

### Yêu cầu hệ thống
* macOS 11.0 trở lên (Hỗ trợ cả chip **Apple Silicon M1/M2/M3/M4** và **Intel**).
* Bộ công cụ dòng lệnh **Xcode Command Line Tools** (nếu máy chưa có, script sẽ tự bật hộp thoại cài đặt).

### Cách 1: Nhấp đúp chuột (Nhanh nhất)
1. Giải nén file `VisKey-macOS-Linux.zip`.
2. Nhấp đúp vào file **`Install_macOS.command`** (hoặc chuột phải chọn **Open**).
3. Script sẽ tự động:
   * Chạy 109 bài kiểm thử C++17 để xác nhận bộ lõi hoạt động chính xác.
   * Biên dịch ứng dụng **`build_macos/VisKey.app`** (Universal Binary chạy trên cả chip M và Intel) và ký mã cục bộ (`codesign`).
   * Đóng gói thành file bộ cài **`VisKey-macOS-Installer.dmg`** và **`VisKey-1.0.0-macOS.pkg`** ngay trong thư mục dự án.
   * Hỏi bạn có muốn chép thẳng `VisKey.app` vào `/Applications` và khởi động ngay không. *(Bỏ qua khi chạy trong CI — biến `CI` được đặt — hoặc không có terminal.)*

### Cách 2: Chạy qua Terminal trên macOS
```bash
unzip VisKey-macOS-Linux.zip
cd VisKey
bash scripts/build_macos_installer.sh
```

### Cấp quyền Accessibility trên macOS (Bắt buộc cho mọi bộ gõ bên thứ 3)
1. Khi `VisKey.app` chạy lần đầu, macOS sẽ hỏi quyền **Accessibility (Trợ năng)**.
2. Mở **System Settings (Cài đặt hệ thống) -> Privacy & Security (Quyền riêng tư & Bảo mật) -> Accessibility (Trợ năng)**.
3. Bật công tắc cho **VisKey**. (Nếu bạn build lại bản mới mà macOS báo quyền cũ bị kẹt, hãy chạy lệnh `tccutil reset Accessibility org.viskey.inputmethod` trong Terminal rồi bật lại).
4. Trên thanh Menu Bar góc phải màn hình sẽ xuất hiện biểu tượng **`🇻🇳 VI`**. Bạn có thể nhấn **`Ctrl + Space`** hoặc **`Cmd + Shift + V`** để chuyển nhanh giữa Tiếng Việt (`VI`) và Tiếng Anh (`EN`).

## 4. Hướng dẫn Cài đặt và Build trên Linux (Ubuntu/Debian, Fedora, Arch)

### Cách 1: Cài đặt từ gói `.deb` (Ubuntu, Debian, Linux Mint, Pop!_OS)
Tải gói **`ibus-viskey_1.0.0_all.deb`** từ trang **GitHub Releases** (gói được CI build tự động, không còn lưu sẵn trong mã nguồn), rồi chạy:
```bash
sudo apt update
sudo apt install -y ./ibus-viskey_1.0.0_all.deb   # tự cài ibus, python3-gi, gir1.2-ibus-1.0, python3-tk
ibus restart
```

### Cách 2: Chạy script tự động cài đặt cho mọi bản phân phối Linux (Ubuntu, Fedora, Arch)
```bash
unzip VisKey-macOS-Linux.zip
cd VisKey
bash scripts/install_linux.sh
```
*(Nếu bạn muốn tự tạo file `.deb` từ mã nguồn trên Linux, chạy lệnh: `bash scripts/build_linux_deb.sh` — có thể đổi thư mục đầu ra bằng biến `OUT_DIR=/duong/dan`).*

### Kích hoạt bộ gõ VisKey trên Linux
1. Mở **Settings -> Keyboard -> Input Sources** (hoặc chạy `ibus-setup` trong Terminal).
2. Nhấn **Add (+)** -> chọn **Vietnamese (Tiếng Việt)** -> chọn **VisKey (Tiếng Việt)**.
3. Hoặc kích hoạt ngay bằng lệnh:
   ```bash
   ibus engine viskey
   ```
4. Nhấp vào biểu tượng bàn phím trên thanh System Tray để bật/tắt nhanh:
   * **Đổi kiểu gõ**: Telex / VNI
   * **Chế độ Không gạch chân (No Preedit)**: Bật mặc định để triệt tiêu lỗi gạch chân.
   * **Tự khôi phục từ tiếng Anh**: Tự trả về từ tiếng Anh khi gõ song ngữ.
   * **Đặt dấu kiểu mới (`hoà`, `thuý`)**: Bật/tắt tùy sở thích.

### Cấu hình bộ gõ bằng `viskey-settings`
Mở **VisKey - Cấu hình Bộ gõ Tiếng Việt** trong menu ứng dụng (hoặc chạy `viskey-settings`) để chọn Telex/VNI, bật/tắt các tùy chọn (đặt dấu kiểu mới, tự khôi phục tiếng Anh, Quick Telex, gõ tắt, sửa lỗi autocomplete, không gạch chân) và **quản lý danh sách gõ tắt** (thêm/sửa/xóa). Nhấn **Lưu cấu hình & Áp dụng**: cấu hình được ghi vào `~/.config/viskey/config.json` (theo `$XDG_CONFIG_HOME`) và bộ gõ IBus tự nạp lại ngay, không cần `ibus restart`.

Dùng không cần giao diện (qua SSH / script):
```bash
viskey-settings --print-config                       # in cấu hình hiện tại (JSON)
viskey-settings --set input_method=vni --set quick_telex=true
viskey-settings --set macros.hs="học sinh"           # thêm/sửa gõ tắt
viskey-settings --set macros.hs=                     # xóa gõ tắt
```

## 5. Hướng dẫn chạy bộ kiểm thử (Unit Tests) & Giả lập trên Terminal

Bạn có thể kiểm tra trực tiếp logic của bộ gõ trên bất kỳ máy tính nào mà chưa cần cài vào hệ thống:

```bash
# 1. Chạy 109 bài kiểm thử trên bộ lõi Python:
python3 tests/test_python_engine.py

# 2. Biên dịch và chạy 109 bài kiểm thử trên bộ lõi C++17:
g++ -std=c++17 -O2 tests/test_cpp_engine.cpp -o build_test_cpp && ./build_test_cpp

# 3. Chạy trình giả lập gõ tiếng Việt tương tác ngay trong Terminal:
g++ -std=c++17 -O2 core/viskey_cli.cpp -o viskey_cli && ./viskey_cli
```
