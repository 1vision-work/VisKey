# design/

Nguồn sự thật cho giao diện và brand của VisKey. Hành vi: `CLAUDE.md` §6 (khi hai bên khác nhau về hành vi, theo CLAUDE.md; về hình thức, theo `design/ui/`).

| Đường dẫn | Có gì |
|---|---|
| `source/` | PDF gốc: `VisKey — Brand & UI.pdf` (22 trang: 1 UI spec + 21 trang frame) và `VisKey — Brand v1 (5 trang, commit 42d8a1b).pdf` (5 trang brand — phần brand không còn trong bản mới). |
| `pages/` | Mọi trang PDF render PNG 200 dpi: `page-01…22.png` (UI) và `brand-v1-01…05.png` (brand). |
| `ui/` | 21 frame × light/dark @2x đúng (`<id>-<tên>-light|dark.png`), `INDEX.md`, `spec.md` (UI spec), `strings.md` (mọi chuỗi mockup → `Localizable.xcstrings` ở M2), `notes.md` (ghi chú SwiftUI/AppKit từng frame). |
| `brand.md` | Brand đầy đủ: 3 hướng logo, "Mũ phím", thông số dựng hình, menu bar icon, màu, typography, guideline, giọng văn, chính tả. |
| `tokens.json` | Thang màu + token semantic light/dark. `Tools/gen-colors.py` sinh colorset cho `Assets.xcassets`. |
| `icons/` | SVG theo đúng toạ độ trong canvas: `symbol*.svg`, `menubar-{vi,en,off}.svg`, `appicon-1024.svg` (+ `appicon-small.svg`, `appicon-hinted-16.svg`), `favicon.svg`; `compare.png` so với bản gốc. Sinh bằng `Tools/gen-icons.py`. |
| `source/canvas/` | Nguồn HTML của canvas Claude Design (một phần, xem bên dưới). |

Tài liệu thiết kế gốc dạng văn bản: `docs/design-doc.md`.

## Nguồn canvas

`source/canvas/` chứa file nguồn HTML của Claude Design (canvas https://claude.ai/artifact/FyrWsuZFsp8y2VDJF3dqwD) mà số liệu trong `ui/spec.md`, `brand.md`, `icons/` và kích thước frame được đối chiếu: `UISpec`, `Logo`, `Icons`, `Palette`, `Guideline`, `Assets`, `A1`, `A2`, `E1`, `B4b` và `canvas.json`. Các frame còn lại (A3, B1–B7, C1–C6, D, E2, E3) chưa chép; cần đọc từ canvas khi nào đối chiếu tiếp. File HTML cần runtime của canvas (`support.js`) mới render được; dùng như tài liệu tra số liệu.

## Còn thiếu / cần soát

1. **Kích thước A3 (760 × 412) và E2 (520 × 250)** đo từ ảnh PDF, chưa đối chiếu file nguồn. Các frame khác dùng cỡ pt từ spec/canvas.
2. **Brand không còn trong PDF "Brand & UI" mới**: brand lấy từ PDF cũ (commit `42d8a1b`), giữ ở `source/`; số liệu đã đối chiếu với canvas.
3. **Tên frame lệch**: PDF/canvas gọi `D-converter-tool` và `E3-update`; ở đây dùng `D1-convert-window` và `E3-update-window` theo danh sách yêu cầu. `B4b` trong PDF không có "s" cuối.
4. **Lockup và wordmark** chưa xuất thành file; theo `brand.md` dựng bằng font Be Vietnam Pro (không path). `appicon-1024.svg` dùng hình chữ nhật bo góc 185 pt, chưa phải siêu elip của macOS (đúng như canvas); có thể thay khi xuất `.icns`.
5. **Số tương phản lệch** giữa PDF brand cũ và UI spec mới ở text-secondary dark và primary: xem `brand.md` §4. `tokens.json` theo UI spec.
6. **Icon app trong bảng Ứng dụng** (B4) là ô thay thế trong mockup; icon thật lấy lúc chạy bằng `NSWorkspace.icon(forFile:)`.
7. Không có chuỗi nào phải đọc bằng OCR: toàn bộ chữ trong mockup có text layer.
