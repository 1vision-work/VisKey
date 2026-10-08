# Chỉ mục frame UI

21 frame × 2 (light/dark) = 42 file, cắt từ ảnh gốc 288 ppi nhúng trong PDF (không qua render lại) rồi cộng 16 px lề quanh viền cửa sổ/khung. Khung 1 trang PDF được cắt đúng viền, nên kích thước pixel khác nhau theo frame. **Tỉ lệ**: ≈ 3 px cho mỗi pt trong spec (cửa sổ Cài đặt 720 pt → ≈ 2160 px), tức lớn hơn "@2x" 1,5 lần; PDF vẽ mockup nhỏ hơn kích thước pt thật (cửa sổ Cài đặt chiếm 540 css px trên trang), nên **kích thước pt chuẩn nằm trong `spec.md`/`notes.md`, không đo từ ảnh**. Muốn @2x đúng 1440 px thì thu nhỏ 2/3 khi cần.

Trang PDF: `design/pages/page-NN.png` (200 dpi). Chuỗi chữ: `strings.md`. Ghi chú kỹ thuật SwiftUI/AppKit: `notes.md`.

| Frame | Tên | Light | Dark | Trang PDF | CLAUDE.md liên quan | Ghi chú |
|---|---|---|---|---|---|---|
| A1 | Menu bar — ba trạng thái | [A1-menubar-light.png](A1-menubar-light.png) | [A1-menubar-dark.png](A1-menubar-dark.png) | 2 | §6 Menu bar; brand §3 | Trang PDF đặt mỗi bản trong một khung nền "màn hình" (gradient) chứa cả 3 trạng thái; light/dark là hai khung. |
| A2 | Menu dropdown — Tiếng Việt | [A2-menu-vi-light.png](A2-menu-vi-light.png) | [A2-menu-vi-dark.png](A2-menu-vi-dark.png) | 3 | §6 Menu bar; §5 tạm tắt/smart switch | Khung gồm cả submenu Kiểu gõ / Bảng mã (mở sang trái). |
| A3 | Menu dropdown — thiếu quyền Trợ năng | [A3-menu-no-permission-light.png](A3-menu-no-permission-light.png) | [A3-menu-no-permission-dark.png](A3-menu-no-permission-dark.png) | 4 | §6 Mất quyền; §4.1 |  |
| B1 | Cài đặt — Chung | [B1-settings-general-light.png](B1-settings-general-light.png) | [B1-settings-general-dark.png](B1-settings-general-dark.png) | 5 | §6 Settings; hotkey §4.1; login item |  |
| B2 | Cài đặt — Gõ tiếng Việt | [B2-settings-typing-light.png](B2-settings-typing-light.png) | [B2-settings-typing-dark.png](B2-settings-typing-dark.png) | 6 | §5 tính năng Telex/VNI… | Cao 800 pt để thấy đủ 4 nhóm. |
| B3 | Cài đặt — Gõ tắt | [B3-settings-macro-light.png](B3-settings-macro-light.png) | [B3-settings-macro-dark.png](B3-settings-macro-dark.png) | 7 | §5 macro; §7 lưu trữ | Hàng `tphcm` đang chọn. |
| B4 | Cài đặt — Ứng dụng | [B4-settings-apps-light.png](B4-settings-apps-light.png) | [B4-settings-apps-dark.png](B4-settings-apps-dark.png) | 8 | §6 tab Ứng dụng; §4.2–4.5 | Panel chi tiết hiện Terminal đang chọn; mockup dùng ô thay thế cho icon app. "Chọn rồi thay" = selection, "Gửi chậm" = paced. |
| B4b | Sheet — Đã sao chép thông tin chẩn đoán | [B4b-settings-apps-diagnostics-light.png](B4b-settings-apps-diagnostics-light.png) | [B4b-settings-apps-diagnostics-dark.png](B4b-settings-apps-diagnostics-dark.png) | 9 | §6 nút "Báo lỗi app này" | PDF đặt tên khung là `B4b-settings-apps-diagnostic` (không có s). |
| B5 | Cài đặt — Chuyển mã | [B5-settings-convert-light.png](B5-settings-convert-light.png) | [B5-settings-convert-dark.png](B5-settings-convert-dark.png) | 10 | §5 chuyển mã, clipboard |  |
| B6 | Cài đặt — Nâng cao | [B6-settings-advanced-light.png](B6-settings-advanced-light.png) | [B6-settings-advanced-dark.png](B6-settings-advanced-dark.png) | 11 | §5; §4.3 delay/chunk |  |
| B7 | Cài đặt — Giới thiệu | [B7-settings-about-light.png](B7-settings-about-light.png) | [B7-settings-about-dark.png](B7-settings-about-dark.png) | 12 | §0 ghi công; §8 Sparkle |  |
| C1 | Onboarding — Chào mừng | [C1-onboarding-welcome-light.png](C1-onboarding-welcome-light.png) | [C1-onboarding-welcome-dark.png](C1-onboarding-welcome-dark.png) | 13 | §6 Onboarding bước 1 |  |
| C2 | Onboarding — Quyền Trợ năng, đang chờ | [C2-onboarding-permission-waiting-light.png](C2-onboarding-permission-waiting-light.png) | [C2-onboarding-permission-waiting-dark.png](C2-onboarding-permission-waiting-dark.png) | 14 | §6 Onboarding (poll AXIsProcessTrusted) |  |
| C3 | Onboarding — Quyền Trợ năng, đã cấp | [C3-onboarding-permission-granted-light.png](C3-onboarding-permission-granted-light.png) | [C3-onboarding-permission-granted-dark.png](C3-onboarding-permission-granted-dark.png) | 15 | §6 Onboarding |  |
| C4 | Onboarding — Chọn kiểu gõ | [C4-onboarding-input-method-light.png](C4-onboarding-input-method-light.png) | [C4-onboarding-input-method-dark.png](C4-onboarding-input-method-dark.png) | 16 | §6 Onboarding |  |
| C5 | Onboarding — Cảnh báo xung đột | [C5-onboarding-conflict-light.png](C5-onboarding-conflict-light.png) | [C5-onboarding-conflict-dark.png](C5-onboarding-conflict-dark.png) | 17 | §6 Onboarding (OpenKey/EVKey/Gõ Nhanh); §7 migration | Bước 4 / 5, chỉ hiện khi phát hiện bộ gõ khác. |
| C6 | Onboarding — Hoàn tất | [C6-onboarding-done-light.png](C6-onboarding-done-light.png) | [C6-onboarding-done-dark.png](C6-onboarding-done-dark.png) | 18 | §6 Onboarding ô thử gõ |  |
| D1 | Công cụ chuyển mã | [D1-convert-window-light.png](D1-convert-window-light.png) | [D1-convert-window-dark.png](D1-convert-window-dark.png) | 19 | §5 chuyển mã văn bản | PDF đặt tên khung là `D-converter-tool`; frame này lưu theo mã D1 của yêu cầu. |
| E1 | Thông báo — Đã chuyển mã clipboard | [E1-notify-converted-light.png](E1-notify-converted-light.png) | [E1-notify-converted-dark.png](E1-notify-converted-dark.png) | 20 | §5 hotkey chuyển mã clipboard | Khung gồm cả menu bar giả lập và banner thông báo. |
| E2 | Thông báo — Mất quyền Trợ năng khi đang chạy | [E2-notify-permission-lost-light.png](E2-notify-permission-lost-light.png) | [E2-notify-permission-lost-dark.png](E2-notify-permission-lost-dark.png) | 21 | §6 Mất quyền | Banner có hai nút: Mở Cài đặt hệ thống / Đóng. |
| E3 | Cửa sổ cập nhật | [E3-update-window-light.png](E3-update-window-light.png) | [E3-update-window-dark.png](E3-update-window-dark.png) | 22 | §8 Sparkle | PDF đặt tên khung là `E3-update`; frame lưu theo `E3-update-window`. |
| UI spec | UI spec cho SwiftUI | — | — | 1 | §6 | Chép ở [spec.md](spec.md). **Trang bị cắt đáy, thiếu 2 thẻ cuối** (xem spec.md). |
