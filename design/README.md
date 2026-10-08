# design/

Nguồn sự thật cho giao diện và brand của VisKey. Hành vi: `CLAUDE.md` §6 (khi hai bên khác nhau về hành vi, theo CLAUDE.md; về hình thức, theo `design/ui/`).

| Đường dẫn | Có gì |
|---|---|
| `source/` | PDF gốc: `VisKey — Brand & UI.pdf` (22 trang: 1 UI spec + 21 trang frame) và `VisKey — Brand v1 (5 trang, commit 42d8a1b).pdf` (5 trang brand — phần brand không còn trong bản mới). |
| `pages/` | Mọi trang PDF render PNG 200 dpi: `page-01…22.png` (UI) và `brand-v1-01…05.png` (brand). |
| `ui/` | 21 frame × light/dark (`<id>-<tên>-light|dark.png`), `INDEX.md`, `spec.md` (UI spec), `strings.md` (mọi chuỗi mockup → `Localizable.xcstrings` ở M2), `notes.md` (ghi chú SwiftUI/AppKit từng frame). |
| `brand.md` | Brand đầy đủ: 3 hướng logo, "Mũ phím", thông số dựng hình, menu bar icon, màu, typography, guideline, giọng văn, chính tả. |
| `tokens.json` | Thang màu + token semantic light/dark. `Tools/gen-colors.py` sinh colorset cho `Assets.xcassets`. |
| `icons/` | SVG dựng lại từ bản raster: `symbol*.svg`, `menubar-{vi,en,off}.svg`, `appicon-1024.svg`; `compare.png` so với bản gốc. Sinh bằng `Tools/gen-icons.py`. |

Tài liệu thiết kế gốc dạng văn bản: `docs/design-doc.md`.

## Còn thiếu / cần soát

1. **UI spec (trang 1) bị cắt đáy**: sau bảng "Màu trong control" còn hai thẻ chỉ lộ phần đầu, không có chữ. Số đo badge "Tối ưu sẵn" nằm ở đó (hoặc không có trong PDF). Cần xuất lại trang.
2. **Brand không còn trong PDF "Brand & UI" mới**: brand lấy từ PDF cũ (commit `42d8a1b`), giữ ở `source/`.
3. **Tên frame lệch**: PDF gọi `D-converter-tool` và `E3-update`; ở đây dùng `D1-convert-window` và `E3-update-window` theo danh sách yêu cầu. `B4b` trong PDF không có "s" cuối.
4. **Tỉ lệ ảnh frame** ≈ 3 px/pt, không phải @2x chính xác (xem `ui/INDEX.md`).
5. **Lockup, wordmark, appicon rim/shadow**: lockup ngang/dọc và wordmark chưa dựng (wordmark theo `brand.md`: dựng bằng font, không path). `appicon-1024.svg` chưa có viền sáng phía trên và bóng đổ như bản raster; squircle là hình chữ nhật bo góc 185 pt, chưa phải siêu elip (có thể thay khi xuất icns).
6. **Số tương phản lệch** giữa PDF brand cũ và UI spec mới ở text-secondary dark và primary: xem ghi chú trong `brand.md` §4.
7. **Icon app trong bảng Ứng dụng** (B4) là ô thay thế trong mockup; icon thật lấy lúc chạy bằng `NSWorkspace.icon(forFile:)`.
8. Không có chuỗi nào phải đọc bằng OCR: toàn bộ chữ trong mockup có text layer.
