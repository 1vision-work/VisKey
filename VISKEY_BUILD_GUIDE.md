# Hướng Dẫn Chi Tiết Build, Đóng Gói & Cài Đặt Ứng Dụng VisKey (macOS, Linux & Windows)

Tài liệu này hướng dẫn từng bước quy trình kiểm thử, biên dịch mã nguồn, đóng gói bộ cài đặt hoàn chỉnh (`.app`, `.dmg`, `.pkg` trên **macOS** và `.deb`, bộ cài đa bản phân phối trên **Linux**), ký mã ứng dụng và cấu hình tự khởi động cho bộ gõ tiếng Việt **VisKey**.

## 1. Cấu trúc gói mã nguồn `VisKey-macOS-Linux.zip`

Khi giải nén `VisKey-macOS-Linux.zip`, bạn sẽ có đầy đủ các thành phần của một ứng dụng hoàn chỉnh:

| Đường dẫn file | Vai trò trong bản Build |
| :--- | :--- |
| `core/viskey_engine.hpp` | Bộ lõi xử lý tiếng Việt viết bằng C++17 Header-only (UTF-8/UTF-32), dùng chung cho ứng dụng macOS và CLI. |
| `core/viskey_engine.py` | Bộ lõi xử lý tiếng Việt viết bằng Python 3, dùng cho IBus Engine trên Linux. |
| `core/viskey_cli.cpp` | Ứng dụng giả lập gõ tiếng Việt tương tác trên Terminal để kiểm thử nhanh không cần cài vào hệ thống. |
| `macos/VisKeyApp.mm` | Mã nguồn ứng dụng macOS hoàn chỉnh (Objective-C++): Menu Bar `NSStatusItem`, cửa sổ Cài đặt & Quản lý Gõ tắt (`NSWindow`), bộ bắt phím `CGEventTap`, lưu trữ `NSUserDefaults` và tự khởi động `LaunchAgents`. |
| `macos/Info.plist` | Khai báo metadata của gói ứng dụng `VisKey.app` (Bundle ID: `org.viskey.inputmethod`, `LSUIElement`). |
| `windows/VisKeyWin.cpp` | Mã nguồn ứng dụng Windows gốc (Win32 C++17): Khay hệ thống `Shell_NotifyIconW`, cửa sổ cấu hình Win32, hook `WH_KEYBOARD_LL`, `SendInput` nguyên tử và lưu trữ Registry. |
| `windows/VisKey_Windows_Portable.pyw` | Bản chạy ngay không cần biên dịch cho Windows (dùng Win32 `ctypes` + giao diện `Tkinter`). |
| `Build_Windows.bat` & `Run_VisKey_Windows.bat` | Script 1-click trên Windows để biên dịch ra `VisKey.exe` hoặc chạy trực tiếp bản Portable. |
| `linux/ibus/ibus_viskey.py` | Daemon bộ gõ IBus cho Linux (hỗ trợ X11 & Wayland, chế độ Không gạch chân và tự động đọc cấu hình thời gian thực). |
| `linux/ibus/viskey_settings.py` | Ứng dụng Giao diện Bảng điều khiển & Quản lý Gõ tắt (GUI) trên Linux (`viskey-settings`), lưu cấu hình vào `~/.config/viskey/config.json`. |
| `linux/ibus/viskey.xml` | File đăng ký thành phần bộ gõ với hệ thống IBus (`/usr/share/ibus/component/viskey.xml`). |
| `tests/test_cpp_engine.cpp` | Bộ 109 bài kiểm thử tự động (Unit Tests) cho nhân C++17. |
| `tests/test_python_engine.py` | Bộ 109 bài kiểm thử tự động (Unit Tests) cho nhân Python. |
| `CMakeLists.txt` | File cấu hình hệ thống build CMake đa nền tảng (macOS & Linux). |
| `Install_macOS.command` | File chạy tự động bằng cú nhấp đúp chuột trên macOS để test, build `VisKey.app` và tạo bộ cài `.dmg`. |
| `scripts/build_macos_installer.sh` | Script build chuyên sâu cho macOS: biên dịch Universal Binary (`arm64` + `x86_64`), ký `codesign`, đóng gói `.dmg`. |
| `scripts/build_linux_deb.sh` | Script đóng gói bộ cài chuẩn Debian/Ubuntu (`ibus-viskey_<phiên bản>_all.deb`). |
| `scripts/install_linux.sh` | Script cài đặt tự động trên mọi bản phân phối Linux (Ubuntu/Debian/Mint, Fedora, Arch). |
| `ibus-viskey_<phiên bản>_all.deb` | File cài đặt `.deb` đã được đóng gói sẵn cho họ Debian/Ubuntu. |

## 2. Bước kiểm thử trước khi Build (Chạy trên cả macOS & Linux)

Trước khi đóng gói bộ cài, bạn có thể xác minh 100% logic xử lý tiếng Việt và tự động khôi phục từ tiếng Anh ngay trong Terminal:

```bash
unzip VisKey-macOS-Linux.zip
cd VisKey

# 1. Chạy bộ 109 kịch bản kiểm thử tự động trên nhân Python:
python3 tests/test_python_engine.py

# 2. Biên dịch và chạy bộ 109 kịch bản kiểm thử tự động trên nhân C++17:
g++ -std=c++17 -O2 -Wall -Wextra tests/test_cpp_engine.cpp -o test_cpp_engine
./test_cpp_engine

# 3. Chạy thử bộ gõ tương tác ngay trong cửa sổ Terminal:
g++ -std=c++17 -O2 core/viskey_cli.cpp -o viskey_cli
./viskey_cli
```

## 3. Hướng dẫn Build và Đóng gói Hoàn chỉnh trên macOS

### 3.1. Chuẩn bị công cụ trên macOS
Ứng dụng macOS sử dụng các framework gốc của Apple (`Cocoa`, `ApplicationServices`, `Carbon`) nên không cần cài thêm bất kỳ thư viện bên thứ ba nào ngoài bộ biên dịch chuẩn **Xcode Command Line Tools**:

```bash
xcode-select --install
```

### 3.2. Cách 1: Build tự động ra `VisKey.app` và `VisKey-<phiên bản>-macOS.dmg` (Khuyên dùng)
Bạn có thể nhấp đúp chuột trực tiếp vào file **`Install_macOS.command`** trong Finder, hoặc mở Terminal tại thư mục `VisKey` và chạy:

```bash
bash scripts/build_macos_installer.sh
```

Script này thực hiện tự động 4 công đoạn:
1. Biên dịch và chạy toàn bộ 109 bài Unit Test C++17 để đảm bảo không có lỗi.
2. Biên dịch `macos/VisKeyApp.mm` cùng `core/viskey_engine.hpp` thành gói ứng dụng hoàn chỉnh tại `build_macos/VisKey.app` dưới dạng **Universal Binary** (chạy gốc trên cả chip Apple Silicon M1/M2/M3/M4 và chip Intel `x86_64`).
3. Thực hiện ký số cục bộ (`codesign --force --deep --sign - build_macos/VisKey.app`) để macOS Gatekeeper và hệ thống quyền Accessibility nhận diện chữ ký ổn định.
4. Tạo bộ cài ảnh đĩa **`VisKey-<phiên bản>-macOS.dmg`** (có sẵn shortcut kéo-thả vào thư mục `/Applications`) và hỏi bạn có muốn cài đặt tự động vào `/Applications/VisKey.app` ngay lập tức hay không.

### 3.3. Cách 2: Build thủ công từng bước hoặc dùng CMake / tạo bộ cài `.pkg`
Nếu bạn muốn tự kiểm soát từng lệnh biên dịch hoặc tích hợp vào CI/CD (GitHub Actions):

```bash
# Cách A: Biên dịch trực tiếp bằng clang++ tạo Universal App Bundle
mkdir -p VisKey.app/Contents/MacOS VisKey.app/Contents/Resources
cp macos/Info.plist VisKey.app/Contents/Info.plist

clang++ -std=c++17 -O2 -fobjc-arc -arch arm64 -arch x86_64 \
    macos/VisKeyApp.mm \
    -framework Cocoa -framework ApplicationServices -framework Carbon \
    -o VisKey.app/Contents/MacOS/VisKey

# Ký ad-hoc cho ứng dụng
codesign --force --deep --sign - VisKey.app

# Cách B: Biên dịch bằng CMake
mkdir -p build && cd build
cmake ..
make -j4

# Tùy chọn: Đóng gói thành file cài đặt .pkg chuẩn của macOS
pkgbuild --install-location /Applications \
         --component VisKey.app \
         VisKey-1.0.0-macOS.pkg
```

### 3.4. Cấp quyền Accessibility và Sử dụng trên macOS
Vì VisKey sử dụng cơ chế `CGEventTap` cấp thấp để triệt tiêu lỗi gạch chân (Preedit Underline) và sửa lỗi mất chữ trên Spotlight/Chrome, macOS yêu cầu bạn cấp quyền **Accessibility (Trợ năng)** ở lần chạy đầu tiên:

1. Mở ứng dụng từ `/Applications/VisKey.app` (hoặc qua Spotlight).
2. Khi hộp thoại xin quyền xuất hiện, vào **System Settings (Cài đặt hệ thống) -> Privacy & Security (Quyền riêng tư & Bảo mật) -> Accessibility (Trợ năng)** và gạt bật công tắc cho **VisKey**.
3. **Lưu ý khi bạn tự biên dịch lại nhiều lần:** Nếu bạn sửa code và build đè `VisKey.app` mới vào `/Applications`, macOS có thể lưu chữ ký (hash) của bản build cũ trong cơ sở dữ liệu TCC khiến bộ gõ không bắt được phím dù công tắc vẫn đang bật. Khi đó chỉ cần chạy lệnh sau trong Terminal để làm mới quyền rồi bật lại:
   ```bash
   tccutil reset Accessibility org.viskey.inputmethod
   open /Applications/VisKey.app
   ```
4. Nhấp vào biểu tượng **`🇻🇳 VI`** trên thanh Menu Bar góc phải màn hình:
   * Nhấn **`Ctrl + Space`** hoặc **`Cmd + Shift + V`** để bật/tắt nhanh chế độ gõ Tiếng Việt (`VI` / `EN`).
   * Chọn **"Bảng điều khiển VisKey..."** (hoặc nhấn `Cmd + ,`) để mở cửa sổ giao diện cài đặt đầy đủ (chọn kiểu gõ Telex/VNI, bật/tắt sửa lỗi Spotlight, thêm/xóa từ Gõ tắt Macro và bật **Tự khởi động VisKey cùng hệ điều hành macOS**).

## 4. Hướng dẫn Build và Đóng gói Hoàn chỉnh trên Linux

Trên Linux, ứng dụng hoàn chỉnh của VisKey bao gồm 2 thành phần chạy song song:
1. **Daemon bộ gõ IBus (`ibus-engine-viskey`)**: Tích hợp trực tiếp vào hệ thống nhập liệu của GNOME, KDE Plasma, XFCE, Cinnamon trên cả **X11** và **Wayland**.
2. **Ứng dụng Giao diện Bảng điều khiển (`viskey-settings`)**: Có biểu tượng riêng trong menu ứng dụng của hệ điều hành (`VisKey - Cấu hình Bộ gõ Tiếng Việt`), cho phép chỉnh mọi tùy chọn và quản lý danh sách Gõ tắt (Macro) bằng giao diện đồ họa.

### 4.1. Cách 1: Cài đặt ngay từ gói `ibus-viskey_<phiên bản>_all.deb` (Ubuntu, Debian, Linux Mint, Pop!_OS, Kali)
Trong thư mục giải nén đã có sẵn gói `.deb` hoàn chỉnh. Bạn chỉ cần chạy:

```bash
sudo apt update
sudo apt install -y ibus python3-gi gir1.2-ibus-1.0 python3-tk
sudo dpkg -i ibus-viskey_*_all.deb
ibus restart
```

Nếu bạn sửa mã nguồn Python và muốn **tự đóng gói lại file `.deb` mới**, chỉ cần chạy:

```bash
bash scripts/build_linux_deb.sh
```
File `ibus-viskey_<phiên bản>_all.deb` mới sẽ được xuất ra ngay tại thư mục gốc của dự án.

### 4.2. Cách 2: Cài đặt trên Fedora / RHEL, Arch Linux / Manjaro hoặc openSUSE
Với các bản phân phối không dùng `.deb`, trước tiên hãy cài các gói thư viện hệ thống (`ibus`, `pygobject`, `tkinter`), sau đó chạy script cài đặt tự động `scripts/install_linux.sh`:

```bash
# Trên Fedora / RHEL / CentOS:
sudo dnf install -y ibus python3-gobject python3-tkinter
bash scripts/install_linux.sh

# Trên Arch Linux / Manjaro / EndeavourOS:
sudo pacman -S --needed ibus python-gobject tk
bash scripts/install_linux.sh
```

### 4.3. Kích hoạt bộ gõ và Cấu hình biến môi trường trên Linux
1. **Thêm VisKey vào danh sách bộ gõ:**
   * Trên **GNOME (Ubuntu/Fedora)**: Mở **Settings -> Keyboard -> Input Sources -> Add (+)** -> chọn **Vietnamese (Tiếng Việt)** -> chọn **VisKey (Tiếng Việt)**.
   * Trên **XFCE / Cinnamon / Mate / KDE**: Chạy lệnh `ibus-setup` trong Terminal -> chuyển sang tab **Input Method -> Add -> Vietnamese -> VisKey (Tiếng Việt)**.
   * Hoặc kích hoạt tức thì bằng dòng lệnh:
     ```bash
     ibus engine viskey
     ```
2. **Mở giao diện Bảng điều khiển & Quản lý Gõ tắt (GUI):**
   Tìm ứng dụng **"VisKey - Cấu hình Bộ gõ Tiếng Việt"** trong menu ứng dụng của máy, hoặc gõ lệnh:
   ```bash
   viskey-settings
   ```
   Mọi thay đổi khi nhấn **"Lưu cấu hình & Áp dụng"** sẽ được lưu vào `~/.config/viskey/config.json` và được bộ gõ tự động nạp ngay lập tức khi bạn chuyển về cửa sổ soạn thảo mà không cần khởi động lại máy.
3. **Đảm bảo biến môi trường IBus hoạt động trong mọi ứng dụng (GTK, Qt, Electron, X11, Wayland):**
   Nếu bạn dùng môi trường cửa sổ tối giản (như i3wm, Sway, Hyprland), hãy thêm 3 dòng sau vào `~/.profile` hoặc `~/.bashrc`:
   ```bash
   export GTK_IM_MODULE=ibus
   export QT_IM_MODULE=ibus
   export XMODIFIERS=@im=ibus
   ```


## 5. Hướng dẫn Chạy và Build Hoàn chỉnh trên Windows (Windows 10 & Windows 11)

Phiên bản **VisKey cho Windows** cung cấp **2 lựa chọn linh hoạt**: chạy ngay lập tức không cần cài trình biên dịch C++ (Portable Edition) hoặc biên dịch ra file thực thi gốc **`VisKey.exe`** độc lập (Native Win32 C++17, dung lượng dưới 500 KB, không cần cài `.NET` hay thư viện ngoài).

### 5.1. Cách 1: Chạy trực tiếp bản Portable không cần Build (`Run_VisKey_Windows.bat`)
Nếu máy Windows của bạn đã có sẵn Python 3:
1. Giải nén file `VisKey-macOS-Linux.zip` (hoặc `VisKey-macOS-Linux-Windows.zip`).
2. Nhấp đúp vào **`Run_VisKey_Windows.bat`** (hoặc `windows\VisKey_Windows_Portable.pyw`).
3. Cửa sổ **Bảng Điều Khiển VisKey (Windows 10/11)** sẽ mở lên ngay lập tức và kích hoạt bộ bắt phím `WH_KEYBOARD_LL` toàn hệ thống.

### 5.2. Cách 2: Build ra file thực thi độc lập `VisKey.exe` (`Build_Windows.bat`)
Nếu máy Windows của bạn có **MinGW-w64 (`g++`)** (qua MSYS2 / WinLibs) hoặc **Visual Studio (MSVC `cl.exe`)**:
1. Nhấp đúp vào file **`Build_Windows.bat`** (hoặc chạy trong *Developer Command Prompt for VS* / *PowerShell*).
2. Script sẽ tự động chạy 109 bài Unit Test C++17 và biên dịch `windows\VisKeyWin.cpp` thành file **`VisKey.exe`** ngay tại thư mục gốc:
   ```bat
   :: Biên dịch bằng MinGW-w64 (GCC trên Windows):
   g++ -std=c++17 -O2 -mwindows -municode windows\VisKeyWin.cpp -luser32 -lgdi32 -lshell32 -ladvapi32 -lcomctl32 -o VisKey.exe

   :: Hoặc biên dịch bằng MSVC (Visual Studio):
   cl /std:c++17 /O2 /EHsc /utf-8 /DUNICODE /D_UNICODE windows\VisKeyWin.cpp user32.lib gdi32.lib shell32.lib advapi32.lib comctl32.lib /link /SUBSYSTEM:WINDOWS /OUT:VisKey.exe
   ```
3. Khi chạy `VisKey.exe`, biểu tượng **VisKey** xuất hiện dưới khay hệ thống (System Tray góc dưới bên phải màn hình):
   * Nhấn **`Ctrl + Space`** hoặc **`Alt + Z`** (hoặc click chuột trái vào icon dưới khay hệ thống) để đổi nhanh giữa Tiếng Việt (`VI`) và Tiếng Anh (`EN`).
   * Click đúp (hoặc chuột phải vào icon chọn *"Mở Bảng điều khiển VisKey..."*) để mở cửa sổ cấu hình, thêm/xóa từ Gõ tắt và bật **"Tự khởi động VisKey cùng hệ điều hành Windows"**.
   * Mọi cấu hình được lưu bền vững trong Windows Registry tại `HKEY_CURRENT_USER\Software\VisKey`.
