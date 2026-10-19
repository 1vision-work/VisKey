VisKey cho Windows / VisKey for Windows
=======================================
Bộ gõ tiếng Việt Telex/VNI - by 1VISION - MIT License
https://github.com/1vision-work/VisKey


TIẾNG VIỆT
----------
Chạy:
  Giải nén file zip vào một thư mục bất kỳ rồi chạy VisKey64.exe.
  Không cần cài đặt, chỉ một tệp exe. Biểu tượng VisKey xuất hiện ở khay
  hệ thống (góc phải thanh Taskbar); biểu tượng xám nghĩa là đang gõ
  tiếng Anh.

Bật / tắt tiếng Việt:
  - Ctrl+Space hoặc Alt+Z (đổi được trong Bảng điều khiển > Phím tắt).
  - Bấm chuột trái vào biểu tượng VisKey: mở bảng nhanh với nút VI / EN,
    Telex / VNI, các công tắc (Khôi phục EN, Gõ tắt, Theo ứng dụng,
    Sửa trình duyệt, Dấu kiểu mới, Quick Telex), "Mở Bảng điều khiển"
    và nút nguồn để thoát. Esc hoặc bấm ra ngoài để đóng.
  - Bấm chuột phải: menu nhỏ (Mở Bảng điều khiển, Bật/Tắt tiếng Việt,
    Thoát VisKey). Bấm đúp: mở Bảng điều khiển.

Bảng điều khiển:
  Chung (kiểu gõ, đặt dấu, khôi phục từ tiếng Anh, sửa lỗi trình duyệt,
  ghi nhớ theo ứng dụng, khởi động cùng Windows), Gõ tắt (thêm/sửa/xoá,
  lọc, số lần dùng, Nhập/Xuất JSON hoặc tệp "viết tắt<Tab>nội dung"),
  Theo ứng dụng (VI / EN / Nhớ cho từng .exe), Phím tắt, Cập nhật (nút
  "Kiểm tra cập nhật" và công tắc "Tự động kiểm tra cập nhật", mặc định
  bật, tối đa mỗi ngày một lần), Giới thiệu (sao chép thông tin hệ thống,
  khôi phục toàn bộ cài đặt). Giao diện sáng/tối theo Windows.

Cấu hình (tự lưu):
  Registry HKCU\Software\VisKey
    ...\Macros       cụm từ gõ tắt
    ...\MacroUsage   số lần đã dùng từng cụm từ
    ...\AppRules     quy tắc theo ứng dụng
  VisKey không ghi tệp nào khác và không gửi phím gõ đi đâu.

Khởi động cùng Windows:
  Bật "Khởi động cùng Windows" trong Bảng điều khiển > Chung. VisKey ghi
  giá trị "VisKey" vào HKCU\Software\Microsoft\Windows\CurrentVersion\Run.

Cảnh báo SmartScreen:
  VisKey64.exe chưa được ký số, nên Windows có thể hiện
  "Windows protected your PC". Bấm "More info" -> "Run anyway".

Cập nhật:
  Khi bật "Tự động kiểm tra cập nhật", VisKey đọc danh sách bản phát hành
  công khai trên GitHub tối đa mỗi ngày một lần (yêu cầu ẩn danh, không gửi
  phím gõ hay thông tin máy). Có bản mới thì chỉ hiện chấm ở mục Cập nhật,
  một dòng trong bảng nhanh và một thông báo nhỏ ở khay (một lần cho mỗi
  phiên bản); VisKey không tự tải hay tự cài. Tắt công tắc này để chỉ kiểm
  tra khi bạn bấm nút.
  Tải zip mới ở https://github.com/1vision-work/VisKey/releases, thoát
  VisKey rồi thay VisKey64.exe. Cài đặt được giữ nguyên.

Gỡ bỏ:
  1. Tắt "Khởi động cùng Windows" (hoặc xóa giá trị "VisKey" trong khóa Run ở trên).
  2. Thoát VisKey (chuột phải biểu tượng -> Thoát VisKey).
  3. Xóa thư mục chứa VisKey64.exe.
  4. Xóa khóa Registry HKCU\Software\VisKey, ví dụ:
       reg delete HKCU\Software\VisKey /f


ENGLISH
-------
Run:
  Unzip anywhere and run VisKey64.exe - a single portable exe, no
  installation. The VisKey icon appears in the system tray; a grey icon
  means English mode.

Toggle Vietnamese on/off:
  - Ctrl+Space or Alt+Z (configurable in Settings > Phím tắt).
  - Left-click the tray icon: quick flyout with VI / EN, Telex / VNI,
    option tiles, "Mở Bảng điều khiển" (open settings) and a power button
    to quit. Esc or clicking elsewhere closes it.
  - Right-click: small menu (open settings, toggle Vietnamese, quit).
    Double-click: open settings.

Settings are stored automatically in the registry:
  HKCU\Software\VisKey (plus subkeys Macros, MacroUsage, AppRules).
  Nothing else is written and keystrokes never leave your computer. The
  update check reads the public GitHub release list (anonymous GET, no
  IDs, no machine info): when you click "Kiểm tra cập nhật" and, unless
  you turn off "Tự động kiểm tra cập nhật" (on by default), automatically
  at most once per 24 hours. A newer version only shows a dot on
  "Cập nhật", a line in the tray flyout and one quiet tray notification
  per version; nothing is downloaded or installed automatically.

Start with Windows:
  Enable "Khởi động cùng Windows" in Settings > Chung. VisKey writes the
  "VisKey" value under HKCU\Software\Microsoft\Windows\CurrentVersion\Run.

SmartScreen:
  VisKey64.exe is not code-signed, so Windows may show
  "Windows protected your PC". Click "More info" -> "Run anyway".

Update:
  Download the new zip from https://github.com/1vision-work/VisKey/releases,
  quit VisKey and replace VisKey64.exe. Settings are kept.

Uninstall:
  1. Turn off start-with-Windows (or delete the "VisKey" value in the Run key above).
  2. Quit VisKey (right-click the tray icon -> Thoát VisKey).
  3. Delete the folder containing VisKey64.exe.
  4. Delete the registry key HKCU\Software\VisKey, e.g.:
       reg delete HKCU\Software\VisKey /f

Screenshots for review: VisKey64.exe --ui-snapshot <folder> writes PNGs of
every screen (no hooks, no tray icon, no registry access) and exits.


License: MIT (see LICENSE.txt). Copyright (c) 2026 VisKey Project.
