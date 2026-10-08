# Chuỗi chữ trên mockup

Nguồn: text layer của `design/source/VisKey — Brand & UI.pdf` (pdftotext -raw), giữ nguyên chính tả và dấu. Bản light và dark có chữ giống hệt nhau (đã đối chiếu từng frame) nên mỗi frame chỉ liệt kê một lần. Mỗi dòng là một đoạn chữ theo thứ tự đọc của PDF; cột/ô trong bảng được PDF gộp thành một dòng. Mọi chữ trong mockup đều có text layer, nên **không có chuỗi [ocr]**; chỉ biểu tượng/glyph (icon ứng dụng, ô biểu tượng) là ảnh và không được chép ở đây.

File này là nguồn cho `Localizable.xcstrings` ở M2. Ghi chú kỹ thuật (AppKit/SwiftUI) của từng trang nằm ở `notes.md`.


## A1-menubar (trang 2)

Tiêu đề trang: Menu bar — ba trạng thái

- Tiếng Việt · khối đặc, chữ V có dấu mũ
- Safari Tệp Chỉnh sửa Hiển thị Lịch sử Dấu trang T5 14:32
- Tiếng Anh · khung rỗng, chữ E
- Safari Tệp Chỉnh sửa Hiển thị Lịch sử Dấu trang T5 14:32
- Tạm dừng / thiếu quyền · khung rỗng, V bị gạch chéo
- Safari Tệp Chỉnh sửa Hiển thị Lịch sử Dấu trang T5 14:32

## A2-menu-vi (trang 3)

Tiêu đề trang: Menu dropdown — Tiếng Việt, đang dùng Safari

- Safari Tệp Chỉnh sửa Hiển thị Lịch sử T5 14:32
- Tiếng Việt
- ⌃⇧ để chuyển
- Kiểu gõ Telex ›
- Bảng mã Unicode ›
- ✓ Kiểm tra chính tả
- ✓ Gõ tắt
- ✓ Tự chuyển theo ứng dụng
- Tắt cho Safari
- Công cụ chuyển mã… ⌃⇧F6
- Cài đặt… ⌘,
- Kiểm tra cập nhật…
- Thoát VisKey ⌘Q
- ✓ Telex
- VNI
- Simple Telex
- Khi rê chuột vào “Bảng mã”
- ✓ Unicode
- TCVN3 (ABC)
- VNI Windows
- Unicode tổ hợp
- CP1258

## A3-menu-no-permission (trang 4)

Tiêu đề trang: Menu dropdown — thiếu quyền Trợ năng

- Safari Tệp Chỉnh sửa Hiển thị Lịch sử T5 14:32
- VisKey chưa có quyền Trợ năng
- Cần quyền này để nhận phím bạn gõ. Cấp
- quyền xong, VisKey tự chạy lại.
- Mở Cài đặt hệ thống…
- Kiểu gõ Telex ›
- Bảng mã Unicode ›
- Kiểm tra chính tả
- Gõ tắt
- Tự chuyển theo ứng dụng
- Vì sao cần quyền này?
- Cài đặt… ⌘,
- Thoát VisKey ⌘Q

## B1-settings-general (trang 5)

Tiêu đề trang: Cài đặt — Chung

- Chung
- Gõ tiếng Việt
- Gõ tắt
- Ứng dụng
- Chuyển mã
- Nâng cao
- Giới thiệu
- VisKey 1.0.0
- Chung
- Gõ tiếng Việt
- Kiểu gõ Telex
- Bảng mã Unicode dựng sẵn
- Phím tắt
- Phím chuyển Việt / Anh
- Nhấn tổ hợp phím để ghi. Có thể chỉ dùng phím bổ trợ.
- ⌃⇧ ⊗
- Phím phụ
- Tuỳ chọn. Tránh ⌘Space vì trùng Spotlight.
- Nhấn để ghi
- Khởi động & hiển thị
- Mở cùng macOS
- Hiện biểu tượng trên Dock
- Biểu tượng menu bar VI / EN Chỉ biểu tượng

## B2-settings-typing (trang 6)

Tiêu đề trang: Cài đặt — Gõ tiếng Việt

- Chung
- Gõ tiếng Việt
- Gõ tắt
- Ứng dụng
- Chuyển mã
- Nâng cao
- Giới thiệu
- VisKey 1.0.0
- Gõ tiếng Việt
- Đặt dấu
- Đặt dấu kiểu mới
- hoà, thuỷ, khoẻ. Tắt để dùng kiểu cũ: hòa, thủy, khỏe.
- Bỏ dấu tự do
- Gõ dấu ở bất kỳ vị trí nào trong từ, VisKey tự đặt đúng chỗ.
- Chính tả
- Kiểm tra chính tả
- Khôi phục từ sai khi nhấn Space
- Gõ “user” sẽ giữ nguyên, không biến thành “úẻ”.
- Nhấn Esc để khôi phục từ vừa gõ
- Giữ ⌃ để tạm tắt kiểm tra chính tả
- Gõ nhanh
- Quick Telex
- cc → ch, gg → gi, kk → kh, nn → ng, qq → qu
- Phụ âm đầu gõ tắt
- f → ph, j → gi, w → qu
- Phụ âm cuối gõ tắt
- g → ng, h → nh, k → ch
- Cho phép f, z, w, j ở đầu từ
- Khác
- Viết hoa chữ cái đầu câu
- Giữ ⌘ hoặc ⌥ để tạm tắt VisKey

## B3-settings-macro (trang 7)

Tiêu đề trang: Cài đặt — Gõ tắt

- Chung
- Gõ tiếng Việt
- Gõ tắt
- Ứng dụng
- Chuyển mã
- Nâng cao
- Giới thiệu
- VisKey 1.0.0
- Gõ tắt Tìm gõ tắt
- Bật gõ tắt
- Dùng cả khi ở tiếng Anh
- Từ tắt Nội dung
- vn Việt Nam
- ko không
- dc được
- tphcm Thành phố Hồ Chí Minh
- hn Hà Nội
- sdt số điện thoại
- cty công ty
- tks cảm ơn bạn nhiều
- + − Sửa
- Hiển thị 8
- / 128
- Nhập từ Unikey / EVKey / OpenKey… Xuất…

## B4-settings-apps (trang 8)

Tiêu đề trang: Cài đặt — Ứng dụng

- Chung
- Gõ tiếng Việt
- Gõ tắt
- Ứng dụng
- Chuyển mã
- Nâng cao
- Giới thiệu
- VisKey 1.0.0
- Ứng dụng
- VisKey tự chọn cách gửi phím phù hợp cho từng ứng dụng. Chỉ chỉnh khi gặp lỗi.
- Ứng dụng Tiếng Việt Cách gửi Bảng mã
- ⌕ Spotlight Tối ưu sẵn Tự nhớ Tự động Mặc định
- >_ Terminal Tối ưu sẵn Tự nhớ Tự động Mặc định
- ◍ Google Chrome Tối ưu sẵn Tự nhớ Tự động Mặc định
- </> Visual Studio Code Tự nhớ Tự động Mặc định
- ▦ Microsoft Excel Tối ưu sẵn Tự nhớ Tự động Mặc định
- # Slack Tự nhớ Tự động Mặc định
- “ Zalo Tự nhớ Backspace Mặc định
- ◧ Adobe Photoshop Tự nhớ Tự động Unicode tổ hợp
- ◈ Tựa game X Tắt Tự động Mặc định
- Giá trị đã chỉnh hiện màu primary. Bấm một dòng để xem chi tiết.
- >_
- Terminal
- com.apple.Terminal
- Tối ưu sẵn
- Cách gửi
- Tự
- động
- Backspace
- Chọn rồi
- thay
- Gửi
- chậm
- Đang dùng Gửi chậm: gửi từng ký tự, cách nhau 3 ms,
- để Terminal và các ứng dụng dòng lệnh nhận đủ dấu.
- Nâng cao
- Độ trễ giữa các phím 3 ms
- 0 – 30
- ms
- Số ký tự mỗi lần gửi − 1 +
- Loại trừ hoàn toàn ứng dụng này
- Khôi phục mặc định Báo lỗi ứng dụng này

## B4b-settings-apps-diagnostics (trang 9)

Tiêu đề trang: Sheet — Đã sao chép thông tin chẩn đoán

- Chung
- Gõ tiếng Việt
- Gõ tắt
- Ứng dụng
- Chuyển mã
- Nâng cao
- Giới thiệu
- VisKey 1.0.0
- Đã sao chép thông tin chẩn đoán
- Dán vào GitHub Issues để nhóm phát triển tái hiện lỗi.
- VisKey 1.0.0 (100)
- macOS 26.0 (25A354)
- Ứng dụng Terminal 2.15 (455)
- Bundle ID com.apple.Terminal
- AX role AXTextArea
- AX subrole (không có)
- Cách gửi paced · tự động · 3 ms · 1 ký tự
- Kiểu gõ Telex · Unicode dựng sẵn
- Không chứa nội dung bạn đã gõ. Thông tin nằm trong clipboard.
- Sao chép lại Đóng Mở GitHub Issues

## B5-settings-convert (trang 10)

Tiêu đề trang: Cài đặt — Chuyển mã

- Chung
- Gõ tiếng Việt
- Gõ tắt
- Ứng dụng
- Chuyển mã
- Nâng cao
- Giới thiệu
- VisKey 1.0.0
- Chuyển mã
- Bảng mã
- Từ bảng mã TCVN3 (ABC)
- Sang bảng mã Unicode dựng sẵn
- Tuỳ chọn
- Viết HOA tất cả
- Viết thường tất cả
- Bỏ dấu
- “Thuỷ” thành “Thuy”. Hữu ích cho tên tệp và đường dẫn.
- Chuyển mã clipboard
- Phím tắt
- Sao chép văn bản, nhấn phím tắt, rồi dán.
- ⌃⇧F6 ⊗
- Thông báo khi chuyển xong
- Mở công cụ chuyển mã…

## B6-settings-advanced (trang 11)

Tiêu đề trang: Cài đặt — Nâng cao

- Chung
- Gõ tiếng Việt
- Gõ tắt
- Ứng dụng
- Chuyển mã
- Nâng cao
- Giới thiệu
- VisKey 1.0.0
- Nâng cao
- Tương thích
- Gửi từng phím
- Dành cho ứng dụng không tương thích. Gõ chậm hơn đôi chút.
- Tự tắt khi dùng bàn phím Nhật / Hàn / Trung
- VisKey tự bật lại khi bạn quay về bàn phím Latin.
- Sao lưu
- Cài đặt, gõ tắt và quy tắc ứng dụng
- Lưu thành một tệp để chuyển sang máy
- khác.
- Nhập cài
- đặt…
- Xuất cài
- đặt…
- Đặt lại
- Khôi phục toàn bộ cài đặt
- Xoá mọi tuỳ chỉnh về mặc định. Gõ tắt được giữ
- lại.
- Khôi phục toàn bộ cài
- đặt…

## B7-settings-about (trang 12)

Tiêu đề trang: Cài đặt — Giới thiệu

- Chung
- Gõ tiếng Việt
- Gõ tắt
- Ứng dụng
- Chuyển mã
- Nâng cao
- Giới thiệu
- VisKey 1.0.0
- Giới thiệu
- VisKey
- Phiên bản 1.0.0 (100)
- Gõ đúng ở mọi nơi.
- Kiểm tra cập nhật…
- GitHub Báo lỗi Ủng hộ
- Mã nguồn mở theo giấy phép GPL-3.0
- Dựa trên OpenKey của Mai Vũ Tuyên
- by 1VISION

## C1-onboarding-welcome (trang 13)

Tiêu đề trang: Onboarding — Chào mừng

- Chào mừng đến VisKey
- Bộ gõ tiếng Việt gõ đúng ở mọi nơi trên Mac.
- Miễn phí
- Không bản trả phí, không quảng cáo.
- Mã nguồn mở
- Giấy phép GPL-3.0. Ai cũng đọc và kiểm chứng được.
- Không thu thập dữ liệu
- Chạy offline, không gửi gì ra ngoài.
- Bắt đầu

## C2-onboarding-permission-waiting (trang 14)

Tiêu đề trang: Onboarding — Quyền Trợ năng, đang chờ

- VisKey cần quyền Trợ năng
- Cần quyền Trợ năng để nhận phím bạn gõ. VisKey không lưu hay gửi nội
- dung này đi đâu.
- Cài đặt hệ thống › Quyền riêng tư & Bảo mật › Trợ năng
- Ứng dụng khác
- VisKey Bật công tắc này
- Đang chờ cấp quyền…
- Quay lại Mở Cài đặt hệ thống

## C3-onboarding-permission-granted (trang 15)

Tiêu đề trang: Onboarding — Quyền Trợ năng, đã cấp

- Đã có quyền
- VisKey đã sẵn sàng nhận phím. Không cần khởi động
- lại.
- Trợ năng ● Đã cấp
- Quay lại Tiếp tục

## C4-onboarding-input-method (trang 16)

Tiêu đề trang: Onboarding — Chọn kiểu gõ

- Bạn quen gõ kiểu nào?
- Đổi lại bất cứ lúc nào trong Cài đặt.
- Telex
- tieengs
- ↓
- tiếng
- VNI
- tie6ng1
- ↓
- tiếng
- Simple Telex
- tieengs
- ↓
- tiếng
- w giữ nguyên
- Phím chuyển Việt / Anh
- Nhấn rồi thả cả hai phím.
- ⌃⇧ ⊗
- Quay lại Tiếp tục

## C5-onboarding-conflict (trang 17)

Tiêu đề trang: Onboarding — Cảnh báo xung đột

- Phát hiện OpenKey đang chạy
- Hai bộ gõ chạy cùng lúc sẽ gây lặp chữ, ví dụ “tiếng” thành “tiếếng”. Nên
- thoát OpenKey trước khi dùng VisKey.
- OpenKey
- ● Đang chạy
- Nhập gõ tắt và cài đặt từ OpenKey
- 42 gõ tắt, kiểu gõ Telex. OpenKey được giữ nguyên.
- Để sau Thoát OpenKey

## C6-onboarding-done (trang 18)

Tiêu đề trang: Onboarding — Hoàn tất

- Mọi thứ đã sẵn sàng
- Gõ thử vào ô dưới đây. Chữ ra đúng dấu nghĩa là bạn đã xong.
- Gõ thử: Tiếng Việt, khoẻ, thuỷ…
- Nhấn ⌃⇧ để chuyển Việt / Anh. Biểu tượng trên menu bar cho biết bạn đang
- ở chế độ nào.
- Quay lại Xong

## D1-convert-window (trang 19)

Tiêu đề trang: Công cụ chuyển mã

- Công cụ chuyển mã
- Nguồn
- TCVN3 (ABC)
- Hîp ®ång mua b¸n hµng ho¸
- Sè: 12/2026/H§MB
- Bªn A: C«ng ty Cæ phÇn Vis
- ⇄
- Kết quả
- Unicode dựng sẵn
- Hợp đồng mua bán hàng hoá
- Số: 12/2026/HĐMB
- Bên A: Công ty Cổ phần Vis
- TCVN3 (ABC) → Unicode dựng sẵn · 3 dòng Sao chép kết quả Chuyển

## E1-notify-converted (trang 20)

Tiêu đề trang: Thông báo — Đã chuyển mã clipboard

- Finder Tệp Chỉnh sửa T5 14:32
- VisKey bây giờ
- Đã chuyển mã clipboard sang Unicode
- TCVN3 (ABC) → Unicode dựng sẵn · 128 ký tự

## E2-notify-permission-lost (trang 21)

Tiêu đề trang: Thông báo — Mất quyền Trợ năng khi đang chạy

- Safari Tệp Chỉnh sửa T5 14:32
- VisKey tạm dừng bây giờ
- Quyền Trợ năng đã bị tắt.
- Bật lại để tiếp tục gõ tiếng Việt.
- Mở Cài đặt hệ thống Đóng

## E3-update-window (trang 22)

Tiêu đề trang: Cửa sổ cập nhật

- Có phiên bản VisKey mới
- VisKey 1.1.0 đã có sẵn. Bạn đang dùng 1.0.0. Bạn muốn cài đặt ngay bây giờ không?
- Ghi chú phát hành
- VisKey 1.1.0
- Mới
- Nhận diện ô nhập trong Ghostty và Warp.
- Nhập gõ tắt từ tệp .txt của EVKey.
- Sửa
- Mất dấu khi gõ nhanh trên thanh địa chỉ trình duyệt.
- Khôi phục từ sai không còn xoá chữ đứng trước.
- Tự tải bản cập nhật Bỏ qua phiên bản này Để sau Cài đặt và khởi động lại
