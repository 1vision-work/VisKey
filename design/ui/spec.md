# UI spec cho SwiftUI

Chép từ trang 1 của `design/source/VisKey — Brand & UI.pdf` (ảnh: `design/pages/page-01.png`). Đơn vị: pt (1 pt = 1 CSS px trong mockup, vẽ ở @2x). Mọi frame dựng từ các giá trị dưới đây. Nếu SwiftUI có control chuẩn cho một mục, dùng control chuẩn; chỉ tuỳ biến màu nhấn và icon.

> Trang 1 của PDF bị cắt ở đáy (hai thẻ cuối không có trong PDF). Hai thẻ đó được bổ sung ở cuối file này từ nguồn canvas (`design/source/canvas/UISpec.dc.html`).

## Lưới & khoảng cách

Lưới nền 4 pt; khoảng cách dùng bậc 4 · 8 · 12 · 16 · 20 · 24 · 32. Căn mọi cạnh theo bội của 4, ngoại trừ chiều cao control do AppKit quyết định (22, 24, 28).

| Bậc | Dùng cho |
|---|---|
| 4 pt | gap trong cụm icon + chữ |
| 8 pt | gap giữa control cùng hàng |
| 12 pt | padding ngang hàng Form, gap thẻ |
| 16 pt | khoảng cách tối thiểu nhãn ↔ control |
| 20 pt | padding cửa sổ, gap giữa nhóm |
| 24 pt | padding thẻ lớn, lề cửa sổ cập nhật |
| 32 pt | khoảng cách khối trong onboarding |

## Cửa sổ

| Mục | Giá trị |
|---|---|
| Cài đặt | 720 × 520, tối thiểu 640 × 480, co giãn được. Sidebar 200. Tab Gõ tiếng Việt và Ứng dụng cuộn khi thấp hơn nội dung. |
| Thanh tiêu đề | Cao 52, chữ 15 semibold, đường kẻ 0,5 dưới. |
| Nội dung | Padding 20 mọi cạnh · khoảng cách giữa nhóm 20 (B2: 16) · nhãn nhóm 11 semibold cách nhóm 6, thụt vào 12. |
| Nhóm (Form section) | Bo 10, viền 0,5, nền nhóm; hàng có đường kẻ 0,5 giữa các hàng. |
| Cửa sổ khác | Onboarding 560 × 440 cố định · Công cụ chuyển mã 600 × 400 · Cập nhật 620 × 440 · Sheet chẩn đoán rộng 440. |
| Bo góc | Cửa sổ 14 · nhóm 10 · control 6 · nút chính onboarding 8 · pill 999. |

## Kích thước control

| Control | Giá trị |
|---|---|
| Hàng Form | Cao tối thiểu 36, padding 6 / 12, giữa nhãn và control cách tối thiểu 16. Dòng phụ 11 secondary cách nhãn 2. |
| Toggle | 38 × 22, núm 18 trắng, lề núm 2. Bật: nền primary fill. Tắt: nền xám "off". |
| Popup / Picker | Cao 24, rộng tối thiểu 168 (trong bảng: không viền, 12 pt). |
| Nút | Thường cao 24, padding 12 · onboarding 28 · nút chính onboarding 32, padding 20. |
| Segmented | Nền thụt 2, mỗi đoạn padding 3 / 12, chữ 12, đoạn chọn có nổi bóng nhẹ. |
| Ô ghi phím | Cao 24, rộng tối thiểu 104; đang ghi: viền primary 2; có nút ⊗ để xoá. |
| Bảng | Header 24 · hàng 28 (Gõ tắt) / 30 (Ứng dụng) · sọc xen kẽ bằng nền surface · hàng chọn nền primary fill, chữ trắng. |

## Màu trong control

Primary và accent đúng token thương hiệu (`design/tokens.json`). Riêng chế độ tối: nền tô có chữ trắng (nút chính, toggle bật, hàng được chọn) dùng **primary 500 `#4C60F5`**, vì `#8193FF` với chữ trắng chỉ đạt 2,9:1; `#8193FF` dùng cho chữ, liên kết, viền chọn, viền focus.

| Vai trò | Light | Dark | Tương phản L / D | Dùng ở |
|---|---|---|---|---|
| Nền cửa sổ Cài đặt | #F6F7F9 | #16181C | — | Settings, Công cụ chuyển mã, Cập nhật |
| Nền nhóm / thẻ | #FFFFFF | #1F2226 | — | Form section, bảng, khối mã; onboarding đảo lại: cửa sổ #FFFFFF / #16181C, thẻ #F6F7F9 / #1F2226 |
| Chữ | #16181C | #ECEEF2 | 17,8 / 15,3 | Nhãn, giá trị, nội dung bảng |
| Chữ phụ | #666D7A | #A3A9B5 | 5,2 / 6,8 | Dòng phụ 11, tiêu đề cột, gợi ý |
| Đường kẻ | #E3E5EA | #2E3238 | — | Giữa hàng, dưới thanh tiêu đề (0,5 pt) |
| Nền control | #FFFFFF | #343841 | — | Popup, nút, ô ghi phím (viền #C9CCD4 / #4B515C) |
| Toggle tắt | #D3D6DD | #4B515C | — | Nền toggle khi tắt |
| Primary — chữ, viền | #3044D6 | #8193FF | 6,6 / 5,7 | Liên kết, viền chọn và focus, giá trị đã chỉnh, chữ mục sidebar đang chọn |
| Primary — nền tô | #3044D6 | #4C60F5 | 7,2 / 4,9 với chữ trắng | Nút chính, toggle bật, hàng bảng đang chọn |
| Primary — nền nhạt | #E6E9FF | #262D6B | — | Thẻ chọn, mục sidebar đang chọn (#DDE2FF / #262D6B) |
| Accent ngọc — chữ | #0B7F6D | #3FE0C0 | 4,9 / 10,7 | Chữ badge "Tối ưu sẵn", "Đã cấp" |
| Accent ngọc — đồ hoạ | #14B89A | #3FE0C0 | chỉ cho hình | Dấu ✓ thành công, dấu mũ trong logo |
| Cảnh báo | #B45309 | #F5A65B | 5,0 / 8,9 | Chấm "chưa có quyền", xung đột bộ gõ; chế độ tối sáng hơn một bậc để đạt AA |
| Lỗi / huỷ | #C2302A | #FF8A80 | 5,6 / 7,8 | Nút "Khôi phục toàn bộ cài đặt…" |

## Quy tắc badge "Tối ưu sẵn"

(Thẻ bị cắt khỏi PDF; chép từ `design/source/canvas/UISpec.dc.html`.)

| Mục | Quy tắc |
|---|---|
| Khi nào hiện | Chỉ khi ứng dụng khớp một quy tắc có sẵn trong AppPolicyStore (built-in). Người dùng tự chỉnh không làm hiện badge. |
| Hình dạng | Pill, chữ **10 semibold**, cao **14**, padding ngang **6**, viền trong **0,5**. Đặt ngay sau tên ứng dụng, cách **8**. |
| Màu | Chữ accent text; nền accent 12–14 %. Hàng đang chọn: nền trắng 22 %, chữ trắng. Không có màu khác. |
| Giới hạn | Tối đa một badge mỗi hàng. Không dùng ngọc cho nút, toggle hay tiêu đề. Ngọc chỉ có ở: badge này, trạng thái thành công (✓ Đã cấp), dấu mũ trong logo. |
| Trợ năng | `accessibilityLabel` "Đã tối ưu sẵn cho ứng dụng này". |

Giá trị cụ thể trong mockup: light — nền `#E6FAF5`, chữ `#0B7F6D`, viền trong `rgba(11,127,109,.28)`; dark — nền `rgba(63,224,192,.14)`, chữ `#3FE0C0`, viền trong `rgba(63,224,192,.3)`; hàng chọn — nền `rgba(255,255,255,.22)`, chữ `#FFFFFF`, không viền.

## Chữ & trạng thái

| Mục | Quy tắc |
|---|---|
| Font | SF Pro (`.system`). Onboarding: tiêu đề Be Vietnam Pro 24 semibold, còn lại SF Pro. |
| Thang chữ | Header menu 20 bold · Title 1 22 · Title 3 15 semibold · Body 13 · bảng 12 · Subheadline / dòng phụ 11 · mã 11–12 monospace. |
| Focus | Viền 2 pt primary (#3044D6 / #8193FF) bo theo control, cách mép 0. |
| Disabled | Toàn bộ control mờ 40 %, giữ nguyên bố cục. |
| Giá trị đã chỉnh | Trong bảng Ứng dụng: chữ primary + semibold; mặc định in màu chữ thường. |
| Văn bản UI | Ngắn, rõ, trung tính, không chấm than. Mục mở hộp thoại kết thúc bằng "…". Phím tắt viết bằng ký hiệu ⌃ ⇧ ⌥ ⌘. Dấu kiểu mới: hoà, khoẻ, thuỷ, tuỳ. |
| Thuật ngữ macOS | Cài đặt hệ thống · Trợ năng · Theo dõi đầu vào · Quyền riêng tư & Bảo mật. |
| Menu bar | Template image 18 × 18; ba trạng thái VI (khối đặc) / EN (khung rỗng) / tạm dừng (khung rỗng, V gạch chéo). |

## Kích thước mockup (đo từ nguồn canvas)

Mockup vẽ 1 pt = 1 CSS px. Cảnh menu/thông báo: A1 1304 × 246 · A2 760 × 420 (menu 268 rộng, bo 12, hàng 24, đầu menu 58) · A3 760 × 412* · E1 520 × 230 · E2 520 × 250* (banner 344 rộng). *A3, E2 đo từ ảnh PDF vì chưa đọc file nguồn của hai frame này. Cửa sổ: xem bảng "Cửa sổ" ở trên và `INDEX.md`.
