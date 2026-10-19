# VisKey — Giao diện mới (UI v2) — Đặc tả dùng chung

Tài liệu này là đặc tả chung cho giao diện mới của VisKey trên macOS, Windows 11 và Linux (GNOME/libadwaita).
Nguồn thiết kế: 11 ảnh chụp 2x (`01-macos-settings-general.png` … `11-linux-about-update-dark.png`), màu và biểu tượng
lấy từ `design/tokens.json` và `design/icons/*.svg`, kích thước bố cục lấy từ mục `layout` trong `design/tokens.json`
(đã quy về 1x).

> Trạng thái: **kế hoạch**. Tài liệu chỉ mô tả giao diện và mô hình dữ liệu; chưa có mã ứng dụng nào được thay đổi.

## 1. Tổng quan

- **Một ngôn ngữ thiết kế, ba bộ công cụ gốc.** Mỗi nền tảng dùng toolkit gốc của mình (AppKit trên macOS, Win32/Fluent
  trên Windows 11, GTK4 + libadwaita trên Linux), nhưng cùng bố cục, thứ bậc thông tin, chữ tiếng Việt và token màu.
- **Sáng và tối.** Mọi màn hình theo giao diện hệ thống (light/dark). Màu lấy theo cặp `light`/`dark` trong `semantic` và
  `ui` của `tokens.json`: primary `#3044D6` / `#8193FF`, nút chính và toggle bật `primary-fill` `#3044D6` / `#4C60F5`,
  chấm ngọc `#3FE0C0`, cảnh báo `#B45309` / `#F5A65B`, lỗi `#C2302A` / `#FF8A80`.
- **Thẻ bo tròn.** Nhóm cài đặt nằm trong thẻ `group-bg` bo 12 (macOS, libadwaita) hoặc SettingsCard bo 8 (Fluent),
  ngăn bởi đường kẻ `separator`.
- **Tự lưu.** Không có nút "Lưu": mọi thay đổi được áp dụng và ghi xuống ngay ("Đã lưu tự động"; trên Linux là toast
  "Đã áp dụng — không cần khởi động lại IBus" kèm nút "Hoàn tác").
- **Riêng tư.** VisKey không gửi phím gõ đi đâu, không có telemetry. Truy cập mạng duy nhất là kiểm tra cập nhật (tự động
  tối đa 1 lần mỗi 24 giờ khi bật "Tự động kiểm tra cập nhật", hoặc khi người dùng bấm): một yêu cầu GET ẩn danh, chỉ đọc
  metadata công khai của GitHub Releases (mục 3.6).
- **Viết mới hoàn toàn**, giấy phép MIT. Không dùng mã, tài nguyên hay ý tưởng triển khai của bộ gõ khác.

## 2. Danh mục màn hình theo nền tảng

Quy ước cột "Ánh xạ": `EngineConfig.x` là trường của lõi (`core/viskey_engine.hpp` / `.py`); `settings.x` là trường của
mô hình dữ liệu chung ở mục 3; "UI" là trạng thái chỉ hiển thị. Chữ trong ngoặc kép là chữ chính xác trên ảnh thiết kế.

### 2.1 macOS

Cửa sổ Cài đặt (ảnh 01, 03, 05, 06): sidebar trái rộng 224 pt, nội dung cuộn dọc, tiêu đề trang 26 pt đậm.

**Sidebar chung** (mọi trang):

| Thành phần | Chữ | Ánh xạ |
| :--- | :--- | :--- |
| Ô tìm kiếm | "Tìm kiếm" | UI: lọc mục cài đặt theo tên |
| Mục | "Chung" | trang Chung |
| Mục + bộ đếm | "Gõ tắt" · `24` | trang Gõ tắt; số = `len(settings.macros)` |
| Mục | "Theo ứng dụng" | trang Theo ứng dụng |
| Mục | "Phím tắt" (chấm cam khi có xung đột phím) | trang Phím tắt; chấm = có cảnh báo xung đột |
| Mục | "Quyền & Bảo mật" (chấm cam khi thiếu quyền Trợ năng) | trang Quyền & Bảo mật |
| Mục | "Giới thiệu" | trang Giới thiệu |
| Chân sidebar | logo · "VisKey" · "Phiên bản 1.0.0" | phiên bản từ `CFBundleShortVersionString` |

#### Chung (ảnh 01)

| Thành phần | Chữ | Ánh xạ |
| :--- | :--- | :--- |
| Trạng thái lưu (góc phải) | "Đã lưu tự động" (chấm ngọc) | UI |
| Thẻ trạng thái: icon `VI` + chấm ngọc | "Đang gõ Tiếng Việt" / phụ đề "Telex · Dấu kiểu cũ · Tự khôi phục từ tiếng Anh" | `settings.vietnamese_enabled`; phụ đề ghép từ `input_method`, `modern_orthography`, `auto_restore_english` |
| Phím hiển thị | `^` `Space` | `settings.hotkeys.primary` |
| Toggle lớn | — | `settings.vietnamese_enabled` |
| Ô gõ thử | "GÕ THỬ" · "Tiếng Việt chuẩn, không mất chữ trên Chrome" · chip "hoas → hóa" | UI: ô nhập chạy engine cục bộ với cấu hình hiện tại; chip minh hoạ phím thô → kết quả |
| Nhóm "KIỂU GÕ" — thẻ chọn | "Telex" / "aa→â · w→ư · s f r x j"; "VNI" / "a6→â · u7→ư · 1-5" | `EngineConfig.input_method` = `telex` \| `vni` |
| Nhóm "VỊ TRÍ ĐẶT DẤU" — thẻ chọn | "Kiểu cũ" / "hòa · thúy · khỏe"; "Kiểu mới" / "hoà · thuý · khoẻ" | `EngineConfig.modern_orthography` = `false` \| `true` |
| Nhóm "GÕ THÔNG MINH" — hàng toggle | "Tự khôi phục từ tiếng Anh" / "Gõ “window”, “software”, “book” không bị biến thành “ưindơ”, “sòtare”" | `EngineConfig.auto_restore_english` |
| hàng toggle | "Sửa lỗi mất chữ trên trình duyệt & Spotlight" / "Chống “ttiếng” khi thanh địa chỉ tự gợi ý — Safari, Chrome, Arc, Excel" | `EngineConfig.fix_autocomplete` |
| hàng toggle + liên kết "Quản lý" | "Ghi nhớ Anh/Việt theo từng ứng dụng" / "Tự chuyển EN khi mở Terminal, iTerm2, Warp, Alacritty, Kitty" | `settings.per_app.enabled` (hiện là `smartPerAppMode`); "Quản lý" mở trang Theo ứng dụng |
| hàng toggle | "Quick Telex" / "cc=ch · gg=gi · kk=kh · nn=ng · qq=qu · pp=ph · tt=th" | `EngineConfig.quick_telex` |
| Toggle | "Khởi động cùng macOS" | `settings.launch_at_login` (hiện là `launchAtLogin`, LaunchAgent) |
| Toggle | "Hiện HUD khi chuyển VI/EN" | `settings.show_hud` (mới) |

#### Gõ tắt (ảnh 03, dark)

| Thành phần | Chữ | Ánh xạ |
| :--- | :--- | :--- |
| Tiêu đề + phụ đề | "Gõ tắt" / "Tự giữ kiểu chữ: ko → không · Ko → Không · KO → KHÔNG" | UI (engine đã xử lý hoa/thường) |
| Nút | "Nhập" | nhập file gõ tắt (mục 4) |
| Nút | "Xuất" | xuất file gõ tắt (mục 4) |
| Toggle chính | — | `EngineConfig.enable_macros` |
| Ô nhập | "VIẾT TẮT" (vd. `cty`) → "THAY BẰNG" (vd. `công ty`) | thêm cặp vào `EngineConfig.macros` |
| Nút chính | "+ Thêm" | thêm / ghi đè cặp |
| Ô lọc | "Lọc 24 cụm từ…" | UI: lọc theo viết tắt hoặc nội dung |
| Segmented | "A → Z" / "Hay dùng" | `settings.macro_sort` = `alpha` \| `usage` |
| Bảng — cột | "VIẾT TẮT" · "NỘI DUNG" · "ĐÃ DÙNG" | `macros` + `settings.macro_usage` |
| Ô "Đã dùng" | "212 lần", "1.038 lần" (dấu chấm phân cách nghìn) | `settings.macro_usage[key]` |
| Nút khi rê chuột | biểu tượng sửa · biểu tượng xoá (đỏ) | sửa / xoá cặp |
| Chân bảng | "Hiển thị 7 / 24 cụm từ" · "Đã lưu tự động" | UI |

#### Theo ứng dụng (ảnh 05)

| Thành phần | Chữ | Ánh xạ |
| :--- | :--- | :--- |
| Tiêu đề + phụ đề | "Theo ứng dụng" / "VisKey nhớ lần cuối bạn dùng VI hay EN trong từng ứng dụng." | — |
| Toggle chính | — | `settings.per_app.enabled` |
| Thẻ | "Khi mở ứng dụng mới" / "Áp dụng cho ứng dụng chưa có trong danh sách" | — |
| Segmented | "Nhớ lần cuối" / "Luôn VI" / "Luôn EN" | `settings.per_app.new_app_policy` = `remember` \| `vi` \| `en` |
| Ô lọc | "Lọc 12 ứng dụng…" | UI |
| Nút chính | "+ Thêm ứng dụng" | chọn file `.app` → thêm luật `source=user` |
| Bảng — cột | "ỨNG DỤNG" · "NGUỒN" · "CHẾ ĐỘ" | `settings.per_app.rules[]` |
| Hàng | icon · tên (vd. "Terminal") · bundle id đơn cách (`com.apple.Terminal`) · huy hiệu "Đang mở" khi là ứng dụng trước mặt | `id`, `display_name` |
| Nguồn | "Mặc định" / "Bạn đặt" (màu primary) / "Tự ghi nhớ" | `source` = `default` \| `user` \| `learned` |
| Segmented chế độ | "VI" / "EN" / "Nhớ" (khi chọn Nhớ hiển thị "Nhớ · VI" hoặc "Nhớ · EN") | `mode` = `vi` \| `en` \| `remember` (+ `last` cho giá trị đã nhớ) |
| Chân bảng | "Hiển thị 6 / 12 ứng dụng" · liên kết "Khôi phục danh sách mặc định" | xoá luật `source=user`/`learned` trùng với danh sách mặc định và khôi phục các luật mặc định |

Đặt chế độ bằng tay luôn chuyển `source` thành `user`.

#### Phím tắt (ảnh 06)

| Thành phần | Chữ | Ánh xạ |
| :--- | :--- | :--- |
| Bàn phím minh hoạ | phím `fn` `^control` `⌥` `⌘` `space` `⌘` `⌥`, phím đang dùng tô primary | `settings.hotkeys.primary` |
| Chú thích | "Nhấn ^ Control + Space ở bất kỳ đâu để đổi giữa VI và EN" | sinh từ `hotkeys.primary` |
| Nhóm "CHUYỂN NGÔN NGỮ" — hàng | "Bật / tắt Tiếng Việt" / "Phím tắt chính" · ô ghi phím `^` `Space` · nút xoá × | `settings.hotkeys.primary` |
| Băng cảnh báo xung đột | "Trùng với “Chọn nguồn nhập trước đó” của macOS. Tắt phím đó trong Cài đặt bàn phím để VisKey nhận phím ổn định." · liên kết "Mở Cài đặt bàn phím" | UI: kiểm tra xung đột cục bộ |
| Hàng | "Phím tắt phụ" / "Dùng khi phím chính bị ứng dụng khác chiếm" · ô ghi phím đang ghi "Nhấn tổ hợp phím…" (chấm đỏ) · nút × | `settings.hotkeys.secondary` |
| Hàng | "Đổi Telex ↔ VNI" · ô trống viền đứt "Ghi phím tắt" | `settings.hotkeys.toggle_method` |
| Nhóm "KHÁC" — hàng | "Mở Bảng điều khiển" · `⌘` `,` | cố định (menu chuẩn macOS) |
| Hàng | "Thêm gõ tắt từ chữ đang chọn" / "Bôi đen một cụm từ rồi nhấn để lưu nhanh" · `^` `⌥` `M` | **backlog** (mục 8) — hiển thị mờ, vô hiệu, nhãn "Sắp có" |

#### Quyền & Bảo mật

Không có ảnh riêng. Nội dung theo cùng phong cách thẻ: trạng thái quyền Trợ năng (đã cấp / chưa cấp, chấm cam ở sidebar khi
chưa cấp), nút "Mở Cài đặt hệ thống", đoạn riêng tư giống onboarding ("Mọi xử lý diễn ra trên máy bạn. VisKey không ghi lại
hay gửi phím gõ ra ngoài, tự nhường quyền khi bạn nhập mật khẩu. Mã nguồn mở, giấy phép MIT."). Không ánh xạ trường cấu
hình; đọc `AXIsProcessTrusted()`.

#### Giới thiệu

Không có ảnh macOS riêng; dùng cùng nội dung với Giới thiệu của Linux (ảnh 11): logo, "VisKey", "Bộ gõ tiếng Việt cho
macOS, Linux & Windows · 1VISION", huy hiệu phiên bản, trạng thái cập nhật, "Liên kết" (Mã nguồn, Báo lỗi, Bản phát hành
& thay đổi, Giấy phép · MIT), "Chẩn đoán" (đặc thù macOS: quyền Trợ năng, event tap, đường dẫn cấu hình), "Sao chép
thông tin hệ thống", "Khôi phục toàn bộ cài đặt…".

#### Popover thanh menu (ảnh 02)

Rộng 352 pt, bo 16.

| Thành phần | Chữ | Ánh xạ |
| :--- | :--- | :--- |
| Biểu tượng thanh menu | logo + "VI" / "EN" | `design/icons/menubar-vi.svg`, `menubar-en.svg`, `menubar-off.svg` |
| Đầu | logo · "VisKey" · nút bánh răng | mở Cài đặt |
| Segmented lớn | "VI" (chấm ngọc) / "Tiếng Việt" · "EN" / "English" | `settings.vietnamese_enabled` |
| Thẻ ứng dụng hiện tại | "Safari" / "Đã ghi nhớ: Tiếng Việt cho ứng dụng này" · liên kết "Luôn EN" | luật của ứng dụng trước mặt; "Luôn EN" đặt `mode=en`, `source=user` |
| Hàng | "Kiểu gõ" · segmented "Telex" / "VNI" | `EngineConfig.input_method` |
| Ô (bật = icon primary, chữ trạng thái màu ngọc) | "Khôi phục tiếng Anh" / "Bật" | `EngineConfig.auto_restore_english` |
| Ô | "Gõ tắt" / "24 cụm từ" | `EngineConfig.enable_macros` |
| Ô (tắt = icon xám) | "Dấu kiểu mới" / "Tắt · hòa" | `EngineConfig.modern_orthography` |
| Ô | "Theo ứng dụng" / "Terminal → EN" | `settings.per_app.enabled` |
| Menu | "Bảng điều khiển…" `⌘ ,` | mở Cài đặt |
| Menu | "Thoát VisKey" `⌘ Q` | thoát |

#### HUD (ảnh 02, phía dưới)

Thẻ tối bo 18, 224 × 72 pt, hiện ngắn (≈1 s) giữa dưới màn hình khi đổi VI/EN, không nhận chuột/focus.
Nội dung: icon "VI" (chấm ngọc) · "Tiếng Việt" · "Telex · ^ Space để đổi" (với EN: "English"). Ánh xạ:
`settings.show_hud`, `vietnamese_enabled`, `input_method`, `hotkeys.primary`.

#### Onboarding 3 bước (ảnh 04 — bước 2)

Cửa sổ 800 × 640 pt, panel trái màu primary rộng 300 pt.

- Panel trái: "Gõ tiếng Việt đúng ngay lần đầu." / "Không gạch chân, không mất chữ, không biến dạng từ tiếng Anh."
  Danh sách bước: "Chọn kiểu gõ" (✓), "Cấp quyền Trợ năng" (đang ở), "Gõ thử & hoàn tất".
- Bước 1 "Chọn kiểu gõ": hai thẻ Telex / VNI như trang Chung → `EngineConfig.input_method`.
- Bước 2 (ảnh): nhãn "BƯỚC 2 / 3"; tiêu đề "Cho phép VisKey nhận phím gõ"; mô tả "macOS yêu cầu quyền Trợ năng để bộ gõ
  chuyển “aa” thành “â” trong mọi ứng dụng."; danh sách: "1 Bấm **Mở Cài đặt hệ thống** bên dưới", "2 Tìm **VisKey**
  trong danh sách Trợ năng", "3 Bật công tắc" (hình toggle); băng chờ (màu cảnh báo) "Đang chờ quyền… VisKey sẽ tự tiếp tục
  khi bạn bật xong."; ghi chú riêng tư (khiên ngọc) "Mọi xử lý diễn ra trên máy bạn. VisKey không ghi lại hay gửi phím gõ
  ra ngoài, tự nhường quyền khi bạn nhập mật khẩu. Mã nguồn mở, giấy phép MIT."; nút "Để sau" (chữ), "Quay lại" (viền),
  "Mở Cài đặt hệ thống ↗" (chính). Tự chuyển sang bước 3 khi `AXIsProcessTrusted()` trả về true (thăm dò định kỳ).
- Bước 3 "Gõ thử & hoàn tất": ô gõ thử như trang Chung, nút hoàn tất. Ghi `settings.onboarding_done = true`.

### 2.2 Windows 11 (Fluent)

Cửa sổ (ảnh 07, 09): thanh tiêu đề tuỳ biến cao 48 epx (logo · "VisKey" · nút thu nhỏ/phóng to/đóng), NavigationView mở
rộng 300 epx, nền Mica, SettingsCard bo 8 cách nhau 4 epx, tiêu đề trang 28 epx.

**Khung điều hướng chung:** thẻ ứng dụng (icon · "VisKey" · chấm ngọc "Đang gõ Tiếng Việt" / khi tắt: "Đang gõ tiếng Anh"
— chấm xám) · ô "Tìm cài đặt" · mục "Chung", "Gõ tắt", "Theo ứng dụng", "Phím tắt", "Cập nhật", "Giới thiệu"
(mục chọn có vạch dọc 3 epx bên trái).

#### Chung (ảnh 07)

| Thành phần | Chữ | Ánh xạ |
| :--- | :--- | :--- |
| Thẻ trạng thái | icon VI · "Tiếng Việt đang bật" / "Nhấn `Ctrl` + `Space` hoặc `Alt` + `Z` để đổi" · "Bật" + toggle | `settings.vietnamese_enabled`; phím từ `hotkeys.primary`/`secondary` |
| Tiêu đề nhóm | "Cách gõ" | — |
| Thẻ + combo | "Kiểu gõ" / "Telex: aa → â, w → ư · VNI: a6 → â" · "Telex" | `EngineConfig.input_method` |
| Thẻ + combo | "Vị trí đặt dấu" / "Kiểu cũ: hòa, thúy · Kiểu mới: hoà, thuý" · "Kiểu cũ (hòa)" | `EngineConfig.modern_orthography` |
| Thẻ + toggle | "Tự khôi phục từ tiếng Anh" / "window, software, book giữ nguyên khi quên tắt bộ gõ" · "Bật" | `EngineConfig.auto_restore_english` |
| Thẻ + toggle | "Sửa lỗi mất / nhân đôi chữ" / "Chrome, Edge, Windows Search, Excel" · "Bật" | `EngineConfig.fix_autocomplete` |
| Thẻ mở rộng + toggle + nút ^ | "Ghi nhớ Anh/Việt theo ứng dụng" / "Tự chuyển EN khi vào cửa sổ dòng lệnh" · "Bật" | `settings.per_app.enabled` |
| Nội dung mở rộng | chip `cmd.exe EN`, `powershell.exe EN`, `WindowsTerminal.exe EN`, `zalo.exe VI` (chip VI màu ngọc) · nút viền đứt "+ Thêm ứng dụng" | `settings.per_app.rules[]` (`id` = tên `.exe`) |
| Thẻ + toggle | "Khởi động cùng Windows" · "Tắt" | `settings.launch_at_login` (hiện `LaunchAtStartup` + khoá `Run`) |

Chữ "Bật" / "Tắt" bên trái toggle là trạng thái hiện tại (chuẩn Fluent ToggleSwitch).

#### Gõ tắt (ảnh 09, dark)

| Thành phần | Chữ | Ánh xạ |
| :--- | :--- | :--- |
| Nút đầu trang | "Nhập" · "Xuất" | mục 4 |
| Thẻ + toggle | "Bật gõ tắt" / "Tự giữ hoa/thường: ko → không, Ko → Không, KO → KHÔNG" | `EngineConfig.enable_macros` |
| Thẻ + toggle | "Quick Telex" / "cc=ch gg=gi kk=kh nn=ng qq=qu pp=ph tt=th" · "Tắt" | `EngineConfig.quick_telex` (**tách khỏi** `enable_macros`; bản 1.0 đang gộp hai cờ) |
| Nhóm | "Thêm cụm từ" · ô viết tắt (vd. `tks`) → ô nội dung (vd. `cảm ơn nhiều`) · nút "Thêm" | thêm vào `EngineConfig.macros` |
| Tiêu đề danh sách + ô lọc | "24 cụm từ" · "Lọc" | UI |
| Hàng | chip viết tắt đơn cách · nội dung · nút "⋯"; khi rê chuột: "Sửa" · "Xoá" (đỏ) | sửa / xoá cặp |

Windows chưa hiện cột "Đã dùng" trong ảnh, nhưng dùng chung `settings.macro_usage` để có thể thêm sắp xếp "Hay dùng".

#### Theo ứng dụng, Phím tắt

Không có ảnh Windows riêng. Dùng cùng nội dung và mô hình dữ liệu với trang macOS tương ứng, trình bày bằng SettingsCard:
"Khi mở ứng dụng mới" (Nhớ lần cuối / Luôn VI / Luôn EN), danh sách luật (`id` = tên `.exe` viết thường, tên hiển thị
lấy từ thông tin phiên bản của file), "Bật / tắt Tiếng Việt" (mặc định `Ctrl + Space`), "Phím tắt phụ" (mặc định
`Alt + Z`), "Đổi Telex ↔ VNI". Cảnh báo xung đột: trùng phím đổi bộ gõ của Windows (`Win + Space`, `Alt + Shift`,
`Ctrl + Shift`) hoặc không đăng ký được.

#### Cập nhật

Không có ảnh Windows riêng. Nội dung: phiên bản hiện tại, toggle "Tự động kiểm tra cập nhật" (mặc định bật,
`settings.update.auto_check`), nút "Kiểm tra cập nhật" (thủ công), "Lần kiểm tra cuối", kết quả "Đã là phiên bản mới
nhất" hoặc "Có phiên bản mới x.y.z" + nút mở trang Releases (không tự tải/cài). Mục "Cập nhật" trong khung điều hướng có
chấm khi có bản mới. Xem mục 3.6.

#### Giới thiệu

Như Giới thiệu Linux (ảnh 11), phần "Chẩn đoán" đặc thù Windows: hook bàn phím đang chạy, kiến trúc (x64/ARM64), đường
dẫn khoá Registry `HKCU\Software\VisKey`.

#### Flyout khay hệ thống (ảnh 08, dark)

Rộng 360 epx, cao ≈ 388 epx, bo 8, neo trên biểu tượng khay ("• VI").

Nền trong suốt thật: Windows 11 22H2+ dùng `DwmSetWindowAttribute(DWMWA_SYSTEMBACKDROP_TYPE, DWMSBT_TRANSIENTWINDOW)`
(acrylic transient); bản cũ hơn dùng acrylic qua `SetWindowCompositionAttribute` (nạp động từ `user32.dll`). Dùng nền
đặc bo góc khi tắt hiệu ứng trong suốt, bật tương phản cao, đang tiết kiệm pin, hoặc API lỗi. Vẫn một file exe portable,
không thêm phụ thuộc.

| Thành phần | Chữ | Ánh xạ |
| :--- | :--- | :--- |
| Đầu | logo · "VisKey" · "Ctrl + Space" (chữ phụ) | `hotkeys.primary` |
| Hai nút lớn | "VI" "Tiếng Việt" / "Đang bật" · "EN" "English" / "Alt + Z" | `settings.vietnamese_enabled`; dòng phụ của nút không chọn hiện phím tắt |
| Hàng | "Kiểu gõ" · segmented "Telex" / "VNI" | `EngineConfig.input_method` |
| Ô bật/tắt (bật = nền primary) | "Khôi phục EN" | `EngineConfig.auto_restore_english` |
| Ô | "Gõ tắt" | `EngineConfig.enable_macros` |
| Ô | "Theo ứng dụng" | `settings.per_app.enabled` |
| Ô | "Sửa trình duyệt" | `EngineConfig.fix_autocomplete` |
| Ô | "Dấu kiểu mới" | `EngineConfig.modern_orthography` |
| Ô | "Quick Telex" | `EngineConfig.quick_telex` |
| Chân | liên kết "Mở Bảng điều khiển" · nút nguồn | mở Cài đặt · thoát VisKey |

### 2.3 Linux (GNOME / libadwaita)

AdwHeaderBar với AdwViewSwitcher ba tab "Chung", "Gõ tắt", "Giới thiệu", nút menu ☰ bên trái, nút đóng bên phải. Nội dung
trong AdwClamp 640 px (trang Giới thiệu 780 px).

#### Chung (ảnh 10)

| Thành phần | Chữ | Ánh xạ |
| :--- | :--- | :--- |
| Thẻ trạng thái | icon VI · "Tiếng Việt" / "IBus đang chạy • Wayland • `Ctrl+Space`" · switch | `settings.vietnamese_enabled`; phiên hiển thị từ `XDG_SESSION_TYPE` |
| Nhóm | "Cách gõ" | — |
| Hàng + nút liên kết | "Kiểu gõ" · "Telex" / "VNI" | `EngineConfig.input_method` |
| AdwComboRow | "Vị trí đặt dấu" / "hòa, thúy (kiểu cũ) hoặc hoà, thuý (kiểu mới)" · "Kiểu cũ" | `EngineConfig.modern_orthography` |
| Hàng switch | "Quick Telex" / "cc=ch gg=gi nn=ng tt=th" | `EngineConfig.quick_telex` |
| Nhóm + mô tả | "Hiển thị khi gõ" / "Dành riêng cho IBus trên X11 và Wayland" | — |
| Hàng switch | "Không gạch chân" / "Gõ thẳng vào ô, không kẹt phím tắt hay rớt chữ khi click chuột" | `settings.linux.no_underline_mode` (hiện `no_underline_mode`) |
| AdwComboRow con (thụt lề) | "Phương thức" · "Tự động (Surrounding Text)" | `settings.linux.commit_method` (mới; giá trị khác cần quyết định) |
| Hàng switch | "Tự khôi phục từ tiếng Anh" / "window, linux, refresh không bị biến dạng" | `EngineConfig.auto_restore_english` |
| Hàng switch + liên kết | "Bật Gõ tắt" · "24 cụm từ ›" | `EngineConfig.enable_macros`; liên kết mở tab Gõ tắt |
| AdwToast | "✓ Đã áp dụng — không cần khởi động lại IBus" · nút "Hoàn tác" | UI: hoàn tác thay đổi gần nhất |

#### Gõ tắt

Không có ảnh Linux riêng. Dùng cùng nội dung với trang Gõ tắt macOS/Windows: "Nhập" / "Xuất", ô "Viết tắt" → "Thay bằng"
+ "Thêm", ô lọc, danh sách AdwActionRow (viết tắt đơn cách · nội dung · "Đã dùng N lần"), sắp xếp "A → Z" / "Hay dùng".

#### Giới thiệu (ảnh 11, dark)

| Thành phần | Chữ | Ánh xạ |
| :--- | :--- | :--- |
| Logo lớn, tên | "VisKey" / "Bộ gõ tiếng Việt cho macOS, Linux & Windows · 1VISION" | — |
| Huy hiệu | "1.0.0" · "✓ Đã là phiên bản mới nhất" (màu ngọc) | phiên bản gói; trạng thái từ lần kiểm tra cập nhật gần nhất (tự động hoặc thủ công) |
| Nhóm "Liên kết" | "Mã nguồn ↗" · "Báo lỗi ↗" · "Bản phát hành & thay đổi ↗" · "Giấy phép" "MIT" | `https://github.com/1vision-work/VisKey`, `/issues`, `/releases`, file `LICENSE` |
| Nhóm "Chẩn đoán" | chấm ngọc "Engine IBus" · `viskey · đang chạy` | trạng thái engine IBus |
| | chấm ngọc "Phiên hiển thị" · `Wayland` | `XDG_SESSION_TYPE` |
| | chấm cam "Surrounding Text" · "Ứng dụng này không hỗ trợ" | khả năng surrounding text của ô nhập gần nhất |
| | thư mục `~/.config/viskey/config.json` · liên kết "Mở" | mở thư mục cấu hình |
| Nút | "Sao chép thông tin hệ thống" | sao chép văn bản chẩn đoán (phiên bản, OS, phiên hiển thị, trạng thái) vào clipboard — không gửi đi đâu |
| Nút phá huỷ | "Khôi phục toàn bộ cài đặt…" | hỏi xác nhận, rồi ghi lại cấu hình mặc định |

Nút "Kiểm tra cập nhật" (thủ công) nằm cạnh huy hiệu trạng thái hoặc trong menu ☰; hàng switch "Tự động kiểm tra cập
nhật" và dòng "Lần kiểm tra cuối" nằm dưới huy hiệu. Khi có bản mới, tab "Giới thiệu" có chấm. Xem mục 3.6.

## 3. Mô hình dữ liệu cài đặt dùng chung

Mọi nền tảng dùng cùng một mô hình logic. Biểu diễn chuẩn là JSON (Linux ghi đúng như vậy vào
`~/.config/viskey/config.json`); macOS ghi vào `NSUserDefaults`, Windows ghi vào `HKCU\Software\VisKey`, với tên khoá
ánh xạ 1-1. Khoá đã có ở bản 1.0 được giữ nguyên để không mất cấu hình khi nâng cấp; khoá mới thêm có giá trị mặc định nên
đọc cấu hình cũ vẫn chạy.

```json
{
  "schema": 2,
  "vietnamese_enabled": true,
  "input_method": "telex",
  "modern_orthography": false,
  "auto_restore_english": true,
  "quick_telex": false,
  "fix_autocomplete": true,
  "enable_macros": true,
  "macros": { "ko": "không", "vn": "Việt Nam" },
  "macro_usage": { "ko": 2417, "vn": 640 },
  "macro_sort": "alpha",
  "launch_at_login": false,
  "show_hud": true,
  "onboarding_done": false,
  "per_app": {
    "enabled": true,
    "new_app_policy": "remember",
    "rules": [
      { "id": "com.apple.Terminal", "display_name": "Terminal", "mode": "en", "source": "default" },
      { "id": "com.apple.Safari", "display_name": "Safari", "mode": "remember", "last": "vi", "source": "learned" }
    ]
  },
  "hotkeys": {
    "primary":       { "key": "space", "modifiers": ["ctrl"] },
    "secondary":     null,
    "toggle_method": null
  },
  "update": { "auto_check": true, "last_checked": null, "latest_seen": null, "last_notified_version": null },
  "linux": { "no_underline_mode": true, "commit_method": "auto" }
}
```

### 3.1 Trường của lõi (EngineConfig)

| Trường | Kiểu | Mặc định | macOS (`NSUserDefaults`) | Windows (Registry) | Linux (JSON) |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `input_method` | `"telex"` \| `"vni"` | `telex` | `inputMethod` | `InputMethodVNI` (DWORD 0/1) | `input_method` |
| `modern_orthography` | bool | `false` | `modernOrthography` | `ModernOrthography` | `modern_orthography` |
| `auto_restore_english` | bool | `true` | `autoRestoreEnglish` | `AutoRestore` | `auto_restore_english` |
| `quick_telex` | bool | `false` | `quickTelex` | `QuickTelex` (mới; bản 1.0 lấy theo `EnableMacros`) | `quick_telex` |
| `fix_autocomplete` | bool | `true` | `fixAutocomplete` | `FixAutocomplete` | `fix_autocomplete` (mới; bản 1.0 cố định `true`) |
| `enable_macros` | bool | `true` | `enableMacros` | `EnableMacros` | `enable_macros` |
| `macros` | map chuỗi → chuỗi | 7 cặp mặc định của lõi | `customMacros` | khoá con `Macros` (mới; bản 1.0 chỉ giữ trong bộ nhớ) | `macros` |

Thay đổi các trường này không đổi hành vi gõ của lõi; adapter chỉ nạp giá trị vào `EngineConfig`.

### 3.2 Trường của ứng dụng

| Trường | Kiểu / giá trị | Mặc định | Ghi chú |
| :--- | :--- | :--- | :--- |
| `vietnamese_enabled` | bool | `true` | macOS `vietnameseEnabled`, Windows `VietnameseEnabled` |
| `launch_at_login` | bool | `false` | macOS `launchAtLogin` (LaunchAgent), Windows `LaunchAtStartup` (+ khoá `Run`), Linux: file autostart nếu cần |
| `show_hud` | bool | `true` | chỉ macOS |
| `onboarding_done` | bool | `false` | chỉ macOS |
| `macro_sort` | `"alpha"` \| `"usage"` | `alpha` | trạng thái segmented "A → Z" / "Hay dùng" |
| `linux.no_underline_mode` | bool | `true` | đã có (`no_underline_mode` ở gốc JSON bản 1.0; đọc cả hai vị trí) |
| `linux.commit_method` | `"auto"` | `auto` | "Tự động (Surrounding Text)"; các giá trị khác cần quyết định |

### 3.3 Luật theo ứng dụng (`per_app`)

| Trường | Kiểu / giá trị | Ý nghĩa |
| :--- | :--- | :--- |
| `enabled` | bool | công tắc "Ghi nhớ Anh/Việt theo từng ứng dụng" (bản 1.0: `smartPerAppMode` / `SmartPerApp`) |
| `new_app_policy` | `"remember"` \| `"vi"` \| `"en"` | "Nhớ lần cuối" / "Luôn VI" / "Luôn EN" cho ứng dụng chưa có luật |
| `rules[].id` | chuỗi | macOS: bundle id (`com.apple.Terminal`); Windows: tên `.exe` viết thường (`cmd.exe`); Linux: chưa dùng |
| `rules[].display_name` | chuỗi | tên hiển thị ("Terminal", "Ghi chú") |
| `rules[].mode` | `"vi"` \| `"en"` \| `"remember"` | "VI" / "EN" / "Nhớ" |
| `rules[].last` | `"vi"` \| `"en"` (tuỳ chọn) | giá trị đã nhớ khi `mode = remember`, hiển thị "Nhớ · VI" |
| `rules[].source` | `"default"` \| `"user"` \| `"learned"` | "Mặc định" (danh sách có sẵn) / "Bạn đặt" (người dùng chọn chế độ) / "Tự ghi nhớ" (sinh ra khi gặp ứng dụng mới với chính sách `remember`) |

Quy tắc:
1. Khi ứng dụng trước mặt đổi: có luật → `vi`/`en` áp dụng ngay; `remember` → khôi phục `last` (nếu có).
   Không có luật → theo `new_app_policy`; với `remember` tạo luật `{mode: remember, source: learned}`.
2. Khi người dùng bấm phím tắt VI/EN trong ứng dụng có `mode = remember`: cập nhật `last`.
3. Người dùng đổi chế độ trên giao diện → `source = user`.
4. "Khôi phục danh sách mặc định": khôi phục toàn bộ luật `default` về giá trị gốc; giữ nguyên luật `user`/`learned` của
   ứng dụng khác.
5. Danh sách mặc định (nguồn `default`, `mode = en`) giữ như bản 1.0 — macOS: `com.apple.Terminal`,
   `com.googlecode.iterm2`, `dev.warp.Warp-Stable`, `io.alacritty`, `net.kovidgoyal.kitty`; Windows: `cmd.exe`,
   `powershell.exe`, `pwsh.exe`, `windowsterminal.exe`, `mintty.exe`, `conhost.exe`.
6. Lưu xuống đĩa có gom nhịp (debounce, vd. 1 s) để không ghi liên tục khi chuyển cửa sổ.

### 3.4 Phím tắt (`hotkeys`)

| Trường | Mặc định macOS | Mặc định Windows | Ý nghĩa |
| :--- | :--- | :--- | :--- |
| `primary` | `^ Space` (`ctrl` + `space`) | `Ctrl + Space` | "Bật / tắt Tiếng Việt" — "Phím tắt chính" |
| `secondary` | không đặt (`null`; bản 1.0 có `⌘ ⇧ V`) | `Alt + Z` (bản 1.0 có sẵn) | "Phím tắt phụ" |
| `toggle_method` | không đặt (`null`) | không đặt (`null`) | "Đổi Telex ↔ VNI" |

Mỗi phím tắt: `{ "key": <tên phím>, "modifiers": [ "ctrl" | "alt" | "shift" | "cmd" | "win" ... ] }` hoặc `null` (chưa
đặt — giao diện hiển thị "Ghi phím tắt"). Tên phím: chữ/số viết thường (`"z"`), hoặc `"space"`, `"tab"`, `"f1"`…
Phải có ít nhất một phím bổ trợ, trừ phím chức năng. Khi ghi phím, kiểm tra xung đột với phím tắt khác của VisKey và với
phím hệ thống đã biết (macOS: "Chọn nguồn nhập trước đó" `^ Space`; Windows: `Win + Space`), hiển thị băng cảnh báo
nhưng vẫn cho phép lưu. Linux: phím tắt do IBus quản lý, không có trang Phím tắt.

macOS: chỉ `^ Space` là phím mặc định. Phím tắt chỉ kích hoạt khi tập phím bổ trợ đang nhấn khớp **đúng** với cấu hình
(vd. `^ Space` không kích hoạt khi nhấn `^ ⇧ Space` hay `^ ⌥ Space`).

### 3.5 Bộ đếm gõ tắt (`macro_usage`)

- `macro_usage`: map viết tắt (đã chuẩn hoá chữ thường, giống khoá trong `macros`) → số lần mở rộng (số nguyên ≥ 0).
- Adapter tăng bộ đếm mỗi khi lõi mở rộng một gõ tắt (mọi kiểu hoa/thường của cùng một khoá cộng chung).
- Chỉ lưu con số, **không** lưu ngữ cảnh, thời điểm hay nội dung gõ xung quanh. Ghi xuống đĩa có gom nhịp.
- Xoá gõ tắt thì xoá bộ đếm của nó; đổi nội dung thì giữ bộ đếm. Bộ đếm không được xuất ra file (mục 4).
- Hiển thị: "N lần", nhóm nghìn bằng dấu chấm (`1.038 lần`). Sắp xếp "Hay dùng": giảm dần theo số lần, bằng nhau thì A → Z.

### 3.6 Kiểm tra cập nhật — tự động và thủ công

- Cài đặt "Tự động kiểm tra cập nhật" (`update.auto_check`, mặc định `true`, lưu bền) có trên cả ba nền tảng.
- **Tự động:** tối đa 1 lần mỗi 24 giờ, tính từ lần thử gần nhất (thành công hay lỗi, tự động hay thủ công). macOS và
  Windows kiểm tra khi khởi động và trong lúc chạy nền (khi đã đủ 24 giờ). Linux: chỉ trong ứng dụng cài đặt (khi mở),
  **không** chạy trong engine IBus.
- **Thủ công:** vẫn giữ nút "Kiểm tra cập nhật"; chạy ngay, không phụ thuộc `auto_check` hay mốc 24 giờ.
- Cả hai dùng cùng một yêu cầu. Nguồn duy nhất: GitHub Releases của `1vision-work/VisKey`
  (`https://api.github.com/repos/1vision-work/VisKey/releases/latest`, hoặc danh sách `/releases` nếu cần tính cả bản
  pre-release). Yêu cầu HTTPS GET ẩn danh: không kèm thông tin người dùng, không cookie, không định danh máy, không
  telemetry. Timeout 8 s.
- So sánh `tag_name` (bỏ tiền tố `v`) với phiên bản hiện tại theo semver.
- Kết quả: "Đã là phiên bản mới nhất" hoặc "Có phiên bản mới x.y.z" + nút mở trang release trong trình duyệt. Không tự
  tải, không tự cài, không hộp thoại.
- Khi lần kiểm tra tự động thấy bản mới: chỉ báo nhẹ — chấm/badge ở mục "Giới thiệu" (macOS, Linux) hoặc "Cập nhật"
  (Windows), và một banner/dòng "Có phiên bản mới x.y.z" trong popover thanh menu (macOS) / flyout khay (Windows). Tối đa
  1 thông báo cho mỗi phiên bản: sau khi báo, ghi `update.last_notified_version = x.y.z` và không báo lại phiên bản đó.
- Lỗi: lần kiểm tra tự động lỗi thì im lặng, chỉ cập nhật "Lần kiểm tra cuối"; lần bấm thủ công lỗi thì thông báo ngắn,
  không thử lại tự động.
- `update.last_checked` (ISO 8601, thời điểm lần thử gần nhất — dùng cho mốc 24 giờ và dòng "Lần kiểm tra cuối"),
  `update.latest_seen` (phiên bản mới nhất đã thấy) và `update.last_notified_version` (phiên bản đã báo, hoặc `null`).

## 4. Định dạng file nhập / xuất gõ tắt

**Xuất** luôn ghi JSON, UTF-8, không BOM, tên gợi ý `viskey-macros.json`:

```json
{
  "version": 1,
  "macros": {
    "bt": "bình thường",
    "ko": "không"
  }
}
```

- `version`: số nguyên, hiện là `1`. `macros`: object, khoá là viết tắt, giá trị là nội dung. Khoá xuất theo A → Z.
- Không xuất bộ đếm, không xuất cài đặt khác.

**Nhập** chấp nhận hai dạng (nhận diện theo nội dung, không theo đuôi file):

1. JSON như trên. `version` phải là `1` (thiếu thì coi là `1`; lớn hơn thì báo không hỗ trợ).
2. Văn bản thuần, mỗi dòng `viết_tắt<TAB>nội dung`. Dòng trống và dòng bắt đầu bằng `#` bị bỏ qua; dòng không có TAB bị
   bỏ qua và được đếm là lỗi. Chỉ tách ở TAB đầu tiên (nội dung có thể chứa TAB). Chấp nhận kết thúc dòng `\n` và `\r\n`,
   bỏ BOM UTF-8 nếu có.

Quy tắc chung khi nhập: cắt khoảng trắng hai đầu viết tắt; viết tắt không rỗng, không chứa khoảng trắng; nội dung không
rỗng; viết tắt trùng thì **ghi đè** giá trị cũ (giữ bộ đếm). Sau khi nhập hiển thị tóm tắt, ví dụ "Đã nhập 12 cụm từ
(3 ghi đè, 1 dòng bị bỏ qua)". File lớn hơn 1 MB bị từ chối.

## 5. Quy ước chụp màn hình `--ui-snapshot <outdir>`

Mỗi ứng dụng cài đặt hỗ trợ cờ `--ui-snapshot <outdir>` để reviewer và CI tạo ảnh thật của giao diện, không cần tương tác:

- Dựng từng màn hình chính bằng chính toolkit của nền tảng, render offscreen ra PNG trong `<outdir>` rồi thoát mã `0`
  (lỗi thì mã khác `0` và in lý do ra stderr).
- Sáng và tối nếu toolkit cho phép. Tên file: `<platform>-<screen>-<light|dark>.png`, `platform` là `macos` / `windows` /
  `linux`, `screen` viết thường nối gạch, ví dụ `macos-general-light.png`, `macos-macros-dark.png`,
  `macos-per-app-light.png`, `macos-shortcuts-light.png`, `macos-privacy-light.png`, `macos-about-light.png`,
  `macos-popover-light.png`, `macos-hud-dark.png`, `macos-onboarding-2-light.png`, `windows-general-light.png`,
  `windows-tray-dark.png`, `linux-general-light.png`, `linux-about-dark.png`.
- Ở chế độ này **không** cài event tap / hook bàn phím / engine IBus, **không** đọc hay ghi cài đặt thật của người dùng
  (chỉ dùng mặc định + dữ liệu mẫu trong bộ nhớ, ví dụ 24 gõ tắt, 12 ứng dụng), **không** hiện hộp thoại xin quyền,
  **không** truy cập mạng.
- Kích thước cửa sổ theo `layout.window` trong `design/tokens.json`, tỉ lệ 2x nếu có thể.

## 6. Token bố cục

Các kích thước đo từ ảnh (2x, quy về 1x, sai số ±2) nằm ở `design/tokens.json` → `layout`: `radius`, `spacing`,
`control-height`, `icon-size`, `typography` (cỡ chữ/độ đậm), `window` (kích thước cửa sổ) và `platform` (sidebar macOS
224 pt, NavigationView Windows 300 epx, AdwClamp 640 px / 780 px). `tests/test_design_tokens.py` kiểm tra file này trong CI.

## 7. Bảng trạng thái tính năng

| Tính năng | macOS | Windows | Linux |
| :--- | :--- | :--- | :--- |
| Trang Chung theo thiết kế mới | kế hoạch | kế hoạch | kế hoạch |
| Ô "Gõ thử" | kế hoạch | — | — |
| Trang Gõ tắt: thêm / sửa / xoá / lọc | kế hoạch | kế hoạch | kế hoạch |
| Bộ đếm gõ tắt + sắp xếp "Hay dùng" | kế hoạch | kế hoạch | kế hoạch |
| Nhập / xuất gõ tắt (JSON + TAB) | kế hoạch | kế hoạch | kế hoạch |
| Lưu gõ tắt xuống đĩa | kế hoạch (đã có `customMacros`) | kế hoạch | kế hoạch (đã có) |
| Quick Telex tách khỏi gõ tắt | — | kế hoạch | — |
| Luật theo ứng dụng được lưu (VI / EN / Nhớ, nguồn) | kế hoạch | kế hoạch | — (bỏ) |
| Chính sách ứng dụng mới | kế hoạch | kế hoạch | — (bỏ) |
| Phím tắt tuỳ chỉnh (chính / phụ / Telex↔VNI) + cảnh báo xung đột | kế hoạch | kế hoạch | — (bỏ; IBus quản lý) |
| Phím tắt khớp đúng phím bổ trợ, chỉ `^ Space` mặc định | kế hoạch | — | — |
| Popover thanh menu / flyout khay | kế hoạch | kế hoạch | — |
| Nền acrylic thật cho flyout khay (dự phòng nền đặc) | — | kế hoạch | — |
| HUD khi đổi VI/EN | kế hoạch | — | — |
| Onboarding 3 bước (quyền Trợ năng) | kế hoạch | — | — |
| Trang Quyền & Bảo mật | kế hoạch | — | — |
| Kiểm tra cập nhật thủ công | kế hoạch | kế hoạch | kế hoạch |
| Tự động kiểm tra cập nhật (24 giờ, mặc định bật, báo nhẹ) | kế hoạch | kế hoạch | kế hoạch (chỉ ứng dụng cài đặt) |
| Chẩn đoán + sao chép thông tin hệ thống | kế hoạch | kế hoạch | kế hoạch |
| Khôi phục toàn bộ cài đặt | kế hoạch | kế hoạch | kế hoạch |
| Toast "Hoàn tác" | — | — | kế hoạch |
| "Thêm gõ tắt từ chữ đang chọn" | backlog (mờ "Sắp có") | backlog (mờ "Sắp có") | backlog (mờ "Sắp có") |
| `--ui-snapshot <outdir>` | kế hoạch | kế hoạch | kế hoạch |

## 8. Quyết định CEO 19/10/2026

1. **"Thêm gõ tắt từ chữ đang chọn"** (`^ ⌥ M` trên ảnh 06): làm sau — **backlog**. Mọi nền tảng hiển thị mờ, vô hiệu,
   nhãn "Sắp có"; không triển khai (cần đọc vùng chọn từ ứng dụng khác, ảnh hưởng quyền riêng tư và clipboard).
2. **Tự động kiểm tra cập nhật**: có, trên cả ba nền tảng. "Tự động kiểm tra cập nhật" mặc định bật; tối đa 1 lần mỗi
   24 giờ; chỉ báo nhẹ, không hộp thoại, không tự tải/cài; lỗi tự động thì im lặng (mục 3.6).
3. **Phím tắt macOS**: chỉ `^ Space` là phím mặc định (bỏ mặc định `⌘ ⇧ V` của bản 1.0); phím tắt phải khớp đúng phím bổ
   trợ (mục 3.4) — đã chốt.
4. **Quick Telex** là công tắc riêng (`quick_telex`, tách khỏi `enable_macros`) — đã chốt.
5. **Khoá gõ tắt có chữ số**: giữ nguyên như hiện tại, không thêm giới hạn (mục 4) — đã chốt.
6. **Trang Theo ứng dụng / Phím tắt trên Linux**: bỏ, chưa làm (IBus quản lý phím tắt, Wayland không cho biết ứng dụng
   trước mặt một cách ổn định).
7. **Flyout khay Windows**: nền Mica/acrylic thật, dự phòng nền đặc bo góc; vẫn một exe portable, không thêm phụ thuộc
   (mục 2.2).
8. **Telemetry**: không có — theo nguyên tắc không telemetry, không định danh của quyết định 2; bộ đếm gõ tắt chỉ lưu cục bộ.

Còn mở:

- **Linux "Phương thức"**: ngoài "Tự động (Surrounding Text)" có cho chọn phương thức khác (vd. preedit / forward
  key) hay không, và tên hiển thị.
- **Bộ đếm gõ tắt có nên tắt được** (một tuỳ chọn "Không đếm số lần dùng") cho người dùng nhạy cảm về quyền riêng tư.
- **Ô "Gõ thử" trên Windows/Linux**: ảnh chỉ có trên macOS.
