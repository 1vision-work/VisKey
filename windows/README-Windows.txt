VisKey cho Windows — bộ gõ tiếng Việt
by 1VISION · https://github.com/1vision-work/VisKey
Dựa trên OpenKey của Mai Vũ Tuyên · GPL-3.0-or-later (xem LICENSE, NOTICE)
SPDX-License-Identifier: GPL-3.0-or-later

BẢN THỬ NGHIỆM (PREVIEW): chưa được nhóm phát triển kiểm tra trên máy Windows thật.

CÁCH DÙNG
- Giải nén, chạy VisKey.exe. Không cần cài đặt, không cần DLL nào khác (Windows 10/11, 64-bit).
- Biểu tượng VisKey nằm ở khay hệ thống (góc phải thanh tác vụ; có thể nằm trong mũi tên ^):
    ô xanh có chữ V  = đang gõ tiếng Việt
    ô xám có chữ E   = đang gõ tiếng Anh
- Nhấp chuột trái vào biểu tượng: chuyển Việt/Anh.
- Nhấp chuột phải: kiểu gõ (Telex, VNI, Simple Telex), bảng mã, chính tả, gõ tắt, khởi động cùng
  Windows, Cài đặt…, Giới thiệu…, Thoát.
- Phím tắt chuyển Việt/Anh: nhấn rồi nhả Ctrl + Shift (mặc định), hoặc Alt + Z (đổi trong Cài đặt).
- Chạy VisKey.exe lần nữa khi nó đang chạy sẽ mở cửa sổ Cài đặt.

NƠI LƯU CÀI ĐẶT
- config.json nằm cạnh VisKey.exe nếu thư mục đó ghi được (chế độ portable, như UniKey);
- nếu không (ví dụ đặt trong Program Files): %APPDATA%\VisKey\config.json
- "Khởi động cùng Windows" ghi vào HKCU\Software\Microsoft\Windows\CurrentVersion\Run (giá trị "VisKey").
  Nếu di chuyển VisKey.exe, mở lại VisKey một lần để cập nhật đường dẫn.

CẢNH BÁO CỦA WINDOWS
- SmartScreen: VisKey.exe chưa được ký số, Windows có thể hiện "Windows protected your PC".
  Bấm "More info" (Thông tin thêm) > "Run anyway" (Vẫn chạy).
- Phần mềm diệt virus có thể báo nhầm vì VisKey dùng keyboard hook (giống mọi bộ gõ).
  VisKey không kết nối mạng, không ghi lại phím gõ. Mã nguồn công khai ở địa chỉ trên.

GIỚI HẠN
- Không gõ được tiếng Việt vào cửa sổ chạy quyền Administrator (Task Manager, cmd "Run as
  administrator"...) trừ khi chính VisKey cũng chạy bằng quyền Administrator. Ở các cửa sổ đó VisKey
  để phím đi qua nguyên vẹn.
- Không hoạt động ở màn hình đăng nhập, màn hình khoá, hộp thoại UAC.
- Tắt bộ gõ khác (UniKey, EVKey, OpenKey...) trước khi dùng VisKey.

English: VisKey is a Vietnamese input method for Windows. Run VisKey.exe; it lives in the
notification area. Left-click toggles Vietnamese/English, right-click opens the menu, Ctrl+Shift
(or Alt+Z) toggles. Settings are stored in config.json next to the exe when that folder is writable,
otherwise in %APPDATA%\VisKey. Unsigned preview build: on SmartScreen choose More info > Run anyway.
