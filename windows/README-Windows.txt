VisKey cho Windows / VisKey for Windows
=======================================
Bộ gõ tiếng Việt Telex/VNI - by 1VISION - MIT License
https://github.com/1vision-work/VisKey


TIẾNG VIỆT
----------
Chạy:
  Giải nén file zip vào một thư mục bất kỳ rồi chạy VisKey64.exe.
  Không cần cài đặt. Biểu tượng VisKey xuất hiện ở khay hệ thống
  (góc phải thanh Taskbar).

Bật / tắt tiếng Việt:
  - Ctrl+Space hoặc Alt+Z
  - Hoặc bấm chuột trái vào biểu tượng VisKey ở khay hệ thống.
  Bấm chuột phải vào biểu tượng để chọn Telex/VNI và các tùy chọn khác;
  bấm đúp để mở Bảng điều khiển (gõ tắt/macro, ...).

Cấu hình:
  Lưu trong Registry: HKCU\Software\VisKey

Khởi động cùng Windows:
  Mở Bảng điều khiển, đánh dấu "Tự khởi động VisKey cùng hệ điều hành
  Windows" rồi bấm Lưu. VisKey ghi giá trị "VisKey" vào
  HKCU\Software\Microsoft\Windows\CurrentVersion\Run.

Cảnh báo SmartScreen:
  VisKey64.exe chưa được ký số, nên Windows có thể hiện
  "Windows protected your PC". Bấm "More info" -> "Run anyway".

Gỡ bỏ:
  1. Bỏ chọn "Tự khởi động..." (hoặc xóa giá trị "VisKey" trong khóa Run ở trên).
  2. Thoát VisKey (chuột phải biểu tượng -> Thoát VisKey).
  3. Xóa thư mục chứa VisKey64.exe.
  4. Xóa khóa Registry HKCU\Software\VisKey, ví dụ:
       reg delete HKCU\Software\VisKey /f

Tải bản mới:
  https://github.com/1vision-work/VisKey/releases


ENGLISH
-------
Run:
  Unzip anywhere and run VisKey64.exe. No installation needed. The VisKey
  icon appears in the system tray (notification area).

Toggle Vietnamese on/off:
  - Ctrl+Space or Alt+Z
  - Or left-click the VisKey tray icon.
  Right-click the tray icon for Telex/VNI and other options; double-click
  it to open the control panel (macros, ...).

Settings:
  Stored in the registry: HKCU\Software\VisKey

Start with Windows:
  In the control panel tick "Tự khởi động VisKey cùng hệ điều hành Windows"
  and click Save. VisKey writes the "VisKey" value under
  HKCU\Software\Microsoft\Windows\CurrentVersion\Run.

SmartScreen:
  VisKey64.exe is not code-signed, so Windows may show
  "Windows protected your PC". Click "More info" -> "Run anyway".

Uninstall:
  1. Untick start-with-Windows (or delete the "VisKey" value in the Run key above).
  2. Quit VisKey (right-click the tray icon -> Thoát VisKey).
  3. Delete the folder containing VisKey64.exe.
  4. Delete the registry key HKCU\Software\VisKey, e.g.:
       reg delete HKCU\Software\VisKey /f

Downloads / updates:
  https://github.com/1vision-work/VisKey/releases


License: MIT (see LICENSE.txt). Copyright (c) 2026 VisKey Project.
