# VisKey — Brand

Chép từ `design/source/VisKey — Brand v1 (5 trang, commit 42d8a1b).pdf` (5 trang brand; bản "Brand & UI" mới chỉ còn phần UI). Ảnh tham chiếu: `design/pages/brand-v1-01.png` … `-05.png`. Vector dựng lại: `design/icons/`. Token màu dạng máy đọc: `design/tokens.json`.

## 1. Ba hướng logo (trang 1)

Mục tiêu: một dấu hiệu nhỏ, chính xác, mang chất Việt qua chính hệ thống dấu thanh — không cờ, không nón lá, không hoa sen.

| Hướng | Ý tưởng | Ưu / nhược |
|---|---|---|
| **A — Mũ phím** *(đề xuất, đã chọn)* | Chữ V lật ngược chính là dấu mũ của â, ê, ô. Ghép V với dấu mũ thành một ký tự "có dấu" — đúng việc VisKey làm: đặt dấu đúng chỗ. | + Hình học tối giản, rõ ở 16 px; tự sinh cặp trạng thái có dấu / không dấu cho menu bar. |
| B — Móc ơ | Chữ v mang chiếc móc của ơ, ư — nét chữ chỉ tiếng Việt mới có. Rất Việt mà không cần hình ảnh minh hoạ nào. | − Móc nhỏ dễ mất ở 16 px; ở menu bar dễ đọc nhầm thành chữ "r" hoặc "y". |
| C — Con trỏ có dấu | Con trỏ nhập văn bản đội dấu sắc — "gõ đúng ở mọi ô nhập". Nói thẳng vào điểm khác biệt của sản phẩm. | − Con trỏ I-beam là hình chung của mọi trình soạn thảo, khó sở hữu và khó nhận ra là VisKey. |

## 2. Logo cuối — "Mũ phím" (hướng A, trang 2)

Dựng hình trên **lưới 64 u**:

- **V**: nét **8,5 u**, góc **68°** giữa hai cánh (nửa góc 34° so với phương thẳng đứng), đầu nét và khớp bo tròn.
- **Dấu mũ**: nét **6 u**, rộng **20 u** (đo theo đường tâm, từ tâm đầu nét này sang tâm đầu nét kia), đầu nét bo tròn.
- **Khe** = **6 u**: dấu mũ luôn mảnh hơn V (≈ 0,7×) và tách rời bằng một khe bằng đúng độ dày của nó — như dấu thanh nằm trên chữ cái. Đầu nét bo tròn để dịu đi chất kỹ thuật.

Toạ độ đường tâm đo từ trang 2 (xem `Tools/gen-icons.py`, IoU với bản gốc ≈ 0,96): đỉnh V (32; 52,0), hai đầu V (32 ± 17,1; 26,6); đỉnh dấu mũ (32; 10,5), hai đầu (32 ± 10; 19,5).

Biến thể:

| Biến thể | Mô tả | File |
|---|---|---|
| Symbol, nền sáng | V `#3044D6` (lam mực 600), mũ `#14B89A` (ngọc 500) | `icons/symbol.svg` |
| Symbol, nền tối | V `#8193FF` (lam mực 400), mũ `#3FE0C0` (ngọc 400) | `icons/symbol-dark.svg` |
| Đơn sắc | Bản 1 màu đen / trắng cho in ấn, khắc, sticker | `icons/symbol-mono.svg` |
| Hinted (≤ 32 px) | V 11 u, mũ 8,5 u, phủ gần kín ô | `icons/symbol-hinted.svg` |
| Lockup ngang · nền sáng | Mặc định cho README, website, social | (chưa dựng; xem Wordmark) |
| Lockup ngang · nền tối | V sáng lên `#8193FF`, dấu mũ `#3FE0C0` | |
| Lockup dọc + thương hiệu mẹ | Symbol trên, wordmark giữa, "BY 1VISION" dưới | |
| Đơn sắc · nền màu chính | Trắng trên nền `#3044D6` | |

### Wordmark

**VisKey** — Be Vietnam Pro SemiBold, tracking −3,5 %, "K" hoa để tách Vis / Key. Không vẽ bằng path: dựng bằng font. Tagline: "Gõ đúng ở mọi nơi." (kiểm tra dấu xếp chồng: Ở Ẩ Ỡ Ữ Ặ ộ).

## 3. App icon & menu bar icon (trang 3)

### App icon

- 1024 × 1024, lưới macOS **824 pt**, bo **185 pt**.
- Squircle lam mực, chuyển sắc dọc rất nhẹ (lam **500 → 700**: `#4C60F5` → `#2534B8`); một "mặt phím" mờ bên trong tạo chiều sâu (khoảng 594 × 594 pt, bo ≈ 128 pt, trắng ~8 %); V trắng, **dấu mũ ngọc `#3FE0C0` là điểm nhấn duy nhất**.
- Từ **32 px trở xuống**: bỏ "mặt phím" và chuyển sắc, tăng nét V lên **11 u**, dấu mũ **8,5 u**, phủ gần kín ô để vẫn đọc rõ trong Finder và Dock nhỏ.
- Kiểm tra trên nền tối: viền squircle đủ tách nền nhờ độ sáng lam 500 → 700, không cần thêm stroke.
- Các cỡ trình bày: 512 (50 %), 128 (50 %), 32, 16 · hinted; Dock (dark), 32, 16.

### Menu bar icon

Template image **18 × 18 pt** (hiển thị phóng 2×):

| Trạng thái | Hình | File |
|---|---|---|
| Tiếng Việt | Khối đặc, V có dấu mũ (khoét alpha 0) | `icons/menubar-vi.svg` |
| Tiếng Anh | Khung rỗng, chữ E | `icons/menubar-en.svg` |
| Tạm tắt / thiếu quyền | Khung rỗng, V có gạch chéo | `icons/menubar-off.svg` |

Quy tắc xuất file:

- Xuất PDF vector hoặc PNG 18 px / 36 px, **chỉ màu đen + alpha**; đặt `isTemplate = true` để macOS tự đảo màu.
- Phần "khoét" trong khối VI là **alpha 0**, không phải màu trắng.
- Ba trạng thái khác nhau ở cả hình khối (đặc / rỗng / gạch) lẫn ký tự — nhận ra được kể cả khi mờ mắt hay bị tint màu.
- Thiếu quyền: kèm chấm cảnh báo trong menu dropdown, **không dùng màu** trên menu bar.
- Nét tối thiểu **1,4 pt** (= 2,8 px @2x) để không vỡ ở @1x.

## 4. Màu (trang 4)

Dữ liệu máy đọc: `design/tokens.json`. ★ = màu gốc, ◐ = bản dùng trong dark mode.

**Lam mực** — màu chính: nút, link, trạng thái bật, thương hiệu.

| 50 | 100 | 200 | 400 ◐ | 500 | 600 ★ | 700 | 900 |
|---|---|---|---|---|---|---|---|
| #F2F4FF | #E6E9FF | #C9D0FF | #8193FF | #4C60F5 | #3044D6 | #2534B8 | #141C66 |

**Ngọc** — màu nhấn: dấu mũ, trạng thái "Tối ưu sẵn", thành công. Dùng ít.

| 100 | 300 | 400 ★◐ | 500 | 700 | 900 |
|---|---|---|---|---|---|
| #D5F8F0 | #7BEDD5 | #3FE0C0 | #14B89A | #0B7F6D | #064A40 |

**Cảnh báo** `#B45309` · **Lỗi** `#C2302A` (dark: `#F5A65B` / `#FF8A80`, theo `ui/spec.md`).

**Xám than** — neutral ánh lạnh nhẹ, ăn ý với chrome của macOS.

| 0 | 50 | 100 | 200 | 400 | 500 | 600 | 700 | 850 | 900 |
|---|---|---|---|---|---|---|---|---|---|
| #FFFFFF | #F6F7F9 | #ECEEF2 | #DADDE4 | #A3A9B5 | #666D7A | #4B515C | #343841 | #1F2226 | #16181C |

### Token ngữ nghĩa — light / dark / tương phản WCAG

| Token | Light | Dark | Tỉ lệ (L / D) |
|---|---|---|---|
| bg | #FFFFFF | #16181C | — |
| surface | #F6F7F9 | #1F2226 | — |
| text | #16181C | #ECEEF2 | 17,8 / 15,3 · AAA |
| text-secondary | #666D7A | #A3A9B5 | 5,2 / 7,5 · AA (UI spec ghi 5,2 / 6,8) |
| primary (text/link) | #3044D6 | #8193FF | 7,2 / 6,4 · AA (UI spec ghi 6,6 / 5,7) |
| on-primary | #FFFFFF | #FFFFFF | 7,2 trên #3044D6 |
| accent (text) | #0B7F6D | #3FE0C0 | 4,9 / 10,7 · AA |
| accent (đồ hoạ) | #14B89A | #3FE0C0 | 2,5 — chỉ cho hình, không cho chữ |

> Lệch giữa hai bản PDF: UI spec (mới hơn) ghi tỉ lệ khác cho text-secondary dark và primary, và quy định riêng **nền tô có chữ trắng ở dark mode dùng `#4C60F5`** (vì `#8193FF` + chữ trắng chỉ đạt 2,9:1). `tokens.json` theo UI spec. Cần chốt lại các con số tương phản khi M2.

## 5. Typography (trang 4)

**Font thương hiệu — Be Vietnam Pro**: logo, website, social, banner. Trong app chỉ dùng cho tiêu đề onboarding (24 pt), xem `ui/notes.md` C1.

- Display 64 / 700 / −3 % — "Gõ đúng ở mọi nơi."
- H2 32 / 600 — "Thuỷ, khoẻ, hoà — dấu đặt đúng chỗ"
- Body 18 / 400 / 1,6 — "Bộ gõ tiếng Việt mã nguồn mở cho macOS. Miễn phí, không quảng cáo, không thu thập dữ liệu và chạy hoàn toàn offline."
- Dấu xếp chồng trên chữ hoa được thiết kế riêng, không đè lên nhau (Ẩ Ẫ Ặ Ể Ỗ Ở Ữ Ỹ) — lý do chọn Be Vietnam Pro hơn Inter (chữ hoa có dấu hơi chật) và Lexend (rộng, kém gọn ở UI dày).

**Trong app — SF Pro (hệ thống) theo HIG macOS**: Large Title 26 (trạng thái menu) · Title 1 22 ("Chào mừng đến VisKey") · Title 3 15 ("Chiến lược gửi phím") · Body 13 ("Khôi phục từ sai khi nhấn Esc") · Subheadline 11 ("Áp dụng cho ô tìm kiếm có tự hoàn thành."). Code: JetBrains Mono (`brew install --cask viskey`).

## 6. Guideline (trang 5)

- **Khoảng trống tối thiểu**: x = chiều cao của biểu tượng ÷ 2. Không đặt chữ, ảnh hay mép khung vào vùng này.
- **Kích thước nhỏ nhất**: symbol 16 px; lockup 80 px; in ấn 6 mm. Dưới 32 px dùng bản "hinted" nét dày. Dưới 80 px chiều ngang, bỏ wordmark, chỉ dùng symbol.
- **Quan hệ với 1VISION**: VisKey đứng trước, 1VISION là chữ ký phụ ("by 1VISION", chữ hoa giãn 12 %, xám 500, cao ≤ 30 % wordmark). Không ghép logo 1VISION vào symbol. Dòng ghi công "Based on OpenKey by Mai Vũ Tuyên · GPL-3.0" luôn có ở README, About và footer website.
- **Không làm**: ✕ đổi màu tuỳ ý · ✕ kéo giãn, bóp méo · ✕ sửa dấu mũ · ✕ xoay, nghiêng · ✕ nền rối, thiếu tương phản · ✕ thêm bóng, hiệu ứng. Cũng không thêm cờ, nón lá, hoa sen hay bản đồ vào logo; không đặt logo VisKey cạnh logo Apple, Unikey, OpenKey, EVKey, Gõ Nhanh như thể là đối tác.

### Giọng văn trong UI

Ngắn, rõ, trung tính — không "bạn ơi", không từ lóng, không chấm than. Nói việc người dùng làm được, rồi mới nói kỹ thuật.

| Nên | Tránh |
|---|---|
| Cần quyền Trợ năng để nhận phím bạn gõ. VisKey không lưu hay gửi nội dung này đi đâu. | Ê, cấp quyền Accessibility cho app đi nha!!! Không thì không chạy được đâu =)) |
| Tắt cho Terminal | Disable Vietnamese input for current |

### Chính tả & quy ước

- Tên sản phẩm luôn viết **VisKey** (V, K hoa, liền). Không: Viskey, VISKEY, Vis Key.
- Dấu đặt kiểu mới, nhất quán: hoà, khoẻ, thuỷ, tuỳ — trùng với tuỳ chọn mặc định trong app.
- Thuật ngữ macOS giữ theo bản Việt của Apple: Cài đặt hệ thống, Trợ năng, Theo dõi đầu vào; tên tiếng Anh để trong ngoặc khi cần.
- Phím tắt hiển thị bằng ký hiệu: ⌃ ⇧ ⌥ ⌘ — không viết "Ctrl+Shift".
- Ghi công: *Based on OpenKey by Mai Vũ Tuyên* — giữ nguyên tiếng Anh ở README; tiếng Việt "Dựa trên OpenKey của Mai Vũ Tuyên" trong app.
