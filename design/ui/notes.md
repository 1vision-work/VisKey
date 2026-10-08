# Ghi chú kỹ thuật từng frame
Chép nguyên văn dòng caption + khối ghi chú đầu mỗi trang PDF (đây là spec hành vi/kích thước đi kèm mockup).

## A1-menubar (trang 2)

`A1-menubar · thanh menu bar cao 24 pt · hiển thị phóng 2×`

**Menu bar — ba trạng thái**

AppKit: NSStatusItem(length: .squareLength), button.image là template image 18 × 18 pt (@2x 36 × 36), isTemplate = true nên macOS tự đổi màu theo nền và khi highlight · khoảng cách giữa các mục do hệ thống quyết định · khi mở menu, mục được tô pill bo 5 pt · trạng thái đổi bằng cách thay button.image, không đổi chiều rộng nên các mục khác không nhảy.

## A2-menu-vi (trang 3)

`A2-menu-vi · menu rộng 268 pt, hàng 24 pt, bo 12 pt`

**Menu dropdown — Tiếng Việt, đang dùng Safari**

AppKit: NSMenu gồm một NSMenuItem.view tuỳ biến ở đầu (cao 58 pt: ô biểu tượng 40 × 40, “Tiếng Việt” 20 pt bold, dòng phụ 11 pt) và các mục chuẩn · dấu ✓ là state = .on · “Kiểu gõ” và “Bảng mã” là submenu, giá trị hiện tại hiện bên phải · mục “Tắt cho …” lấy tên từ NSWorkspace.shared.frontmostApplication, đổi thành “Bật lại cho …” khi đã tắt · phím tắt ⌃⇧F6 cho công cụ chuyển mã, ⌘, cho Cài đặt, ⌘Q thoát. Submenu mở sang trái vì menu sát mép phải màn hình.

## A3-menu-no-permission (trang 4)

`A3-menu-no-permission · menu rộng 268 pt`

**Menu dropdown — thiếu quyền Trợ năng**

AppKit: khi AXIsProcessTrusted() == false, đầu menu đổi thành view cảnh báo: chấm cảnh báo 10 pt (#B45309 / #F5A65B), tiêu đề 13 pt semibold, mô tả 11 pt secondary và nút “Mở Cài đặt hệ thống…” cao 28 pt, nền primary · các mục điều khiển gõ bị isEnabled = false (mờ 40%), chỉ còn “Vì sao cần quyền này?”, Cài đặt và Thoát dùng được · biểu tượng menu bar ở trạng thái “tạm dừng”.

## B1-settings-general (trang 5)

`B1-settings-general · 720 × 520 pt`

**Cài đặt — Chung**

SwiftUI: Settings { TabView/NavigationSplitView } · sidebar 200 pt · nội dung Form{}.formStyle(.grouped) · padding 20 pt · hàng 36 pt · ô ghi phím là KeyboardShortcuts.Recorder tuỳ biến (viền primary 2 pt khi đang ghi).

## B2-settings-typing (trang 6)

`B2-settings-typing · 720 × 800 pt (kéo giãn chiều cao để thấy đủ 4 nhóm; ở 520 pt nội dung cuộn)`

**Cài đặt — Gõ tiếng Việt**

SwiftUI: Form{ Section("Đặt dấu"){…} … }.formStyle(.grouped) · mỗi tuỳ chọn là Toggle(…).toggleStyle(.switch) với Text phụ 11 pt secondary bên dưới · mặc định bật: Đặt dấu kiểu mới, Bỏ dấu tự do, Kiểm tra chính tả, Khôi phục từ sai, Esc, Giữ ⌃, Viết hoa đầu câu, Giữ ⌘/⌥ · mặc định tắt: toàn bộ nhóm “Gõ nhanh”.

## B3-settings-macro (trang 7)

`B3-settings-macro · 720 × 520 pt`

**Cài đặt — Gõ tắt**

SwiftUI: Table(macros, selection:) hai cột, hàng 28 pt, sọc xen kẽ · ô tìm kiếm là .searchable trên thanh tiêu đề (rộng 200 pt) · thanh dưới 36 pt: nút + − (24 × 24), “Sửa”, menu nhập, “Xuất…” · nhấp đúp hàng để sửa tại chỗ · cột “Từ tắt” dùng font monospace 12 pt.

## B4-settings-apps (trang 8)

`B4-settings-apps · 720 × 780 pt (tab quan trọng nhất; chiều cao tối thiểu 520 pt, khi thấp panel chi tiết cuộn cùng danh sách)`

**Cài đặt — Ứng dụng**

SwiftUI: Table(apps, selection:) 4 cột 190 / 68 / 100 / 98 pt, hàng 30 pt, ô menu là Picker(.menu).buttonStyle(.borderless) cỡ chữ 12 pt · giá trị người dùng đã chỉnh in primary + semibold, mặc định in màu chữ thường · hàng ứng dụng bị tắt tiếng Việt: các cột còn lại mờ 40% · panel chi tiết = GroupBox bo 10 pt, padding 12 pt, chỉ hiện khi có dòng được chọn · “Nâng cao” là DisclosureGroup, mặc định đóng · biểu tượng ứng dụng lấy lúc chạy bằng NSWorkspace.icon(forFile:) (mockup dùng ô thay thế).

## B4b-settings-apps-diagnostics (trang 9)

`B4b-settings-apps-diagnostic · sheet 440 pt trên cửa sổ Cài đặt`

**Sheet — Đã sao chép thông tin chẩn đoán**

SwiftUI: .sheet(isPresented:) rộng 440 pt, padding 20 pt, bo 14 pt · khối mã là Text(.monospaced) 11 pt trong GroupBox chọn được (.textSelection(.enabled)) · phím mặc định Return = “Mở GitHub Issues”, Esc = “Đóng” · nội dung không chứa chữ người dùng đã gõ, chỉ metadata ứng dụng.

## B5-settings-convert (trang 10)

`B5-settings-convert · 720 × 520 pt`

**Cài đặt — Chuyển mã**

SwiftUI: hai Picker(.menu) bảng mã · “Viết HOA” và “Viết thường” loại trừ nhau (bật cái này tự tắt cái kia) · phím tắt mặc định ⌃⇧F6 · thông báo gửi qua UNUserNotificationCenter (xem E1).

## B6-settings-advanced (trang 11)

`B6-settings-advanced · 720 × 520 pt`

**Cài đặt — Nâng cao**

SwiftUI: nút “Khôi phục toàn bộ cài đặt…” là Button(role: .destructive) (chữ lỗi #C2302A / #FF8A80, không tô nền) và luôn mở .confirmationDialog trước khi xoá · “Xuất / Nhập” dùng NSSavePanel / NSOpenPanel, tệp .viskeyconfig (JSON, gồm cài đặt, gõ tắt, quy tắc ứng dụng).

## B7-settings-about (trang 12)

`B7-settings-about · 720 × 520 pt`

**Cài đặt — Giới thiệu**

SwiftUI: VStack(spacing: 8) căn giữa · app icon 128 × 128 pt (NSApp.applicationIconImage) · tên 26 pt bold · phiên bản 11 pt secondary lấy từ CFBundleShortVersionString (CFBundleVersion) · liên kết là Link màu primary, mở trình duyệt mặc định · “Ủng hộ” trỏ tới GitHub Sponsors.

## C1-onboarding-welcome (trang 13)

`C1-onboarding-welcome · 560 × 440 pt · bước 1 / 4`

**Onboarding — Chào mừng**

SwiftUI: cửa sổ không đổi kích thước, thanh tiêu đề trong suốt (.windowStyle(.hiddenTitleBar)) · tiêu đề dùng Be Vietnam Pro 24 pt (nơi duy nhất trong app dùng font thương hiệu) · số chấm = số bước thực tế (4, hoặc 5 khi có bước cảnh báo xung đột) · nút chính cao 32 pt, bo 8 pt, nền primary 600 (dark: 500 để chữ trắng đạt AA).

## C2-onboarding-permission-waiting (trang 14)

`C2-onboarding-permission-waiting · 560 × 440 pt · bước 2 / 4`

**Onboarding — Quyền Trợ năng, đang chờ**

Logic: nút “Mở Cài đặt hệ thống” mở x-apple.systempreferences:com.apple.preference.security?Privacy_Accessibility · thăm dò AXIsProcessTrusted() mỗi 1 giây, khi thành true chuyển sang C3 và khởi động event tap, không cần khởi động lại · nút “Tiếp tục” bị tắt cho tới lúc đó · hình minh hoạ vẽ lại, không chụp màn hình hệ thống.

## C3-onboarding-permission-granted (trang 15)

`C3-onboarding-permission-granted · 560 × 440 pt · bước 2 / 4`

**Onboarding — Quyền Trợ năng, đã cấp**

Chuyển cảnh: thay nội dung bước 2 bằng .transition(.opacity) 0,2 s · dấu ✓ dùng accent ngọc (#14B89A nền sáng, #3FE0C0 nền tối) trên vòng tròn alpha 16% · hết thời gian trễ 0,6 s, nút “Tiếp tục” sáng lên (không tự chuyển bước).

## C4-onboarding-input-method (trang 16)

`C4-onboarding-input-method · 560 × 440 pt · bước 3 / 4`

**Onboarding — Chọn kiểu gõ**

SwiftUI: ba thẻ là Picker(.radioGroup) tuỳ biến, mỗi thẻ cao 112 pt, thẻ được chọn viền primary 2 pt + nền primary 100 · mặc định chọn Telex · VIQR và các bảng mã khác nằm trong Cài đặt, không đưa vào onboarding · ô ghi phím nhận ⌃⇧ làm mặc định, nhấn để ghi lại.

## C5-onboarding-conflict (trang 17)

`C5-onboarding-conflict · 560 × 440 pt · bước 4 / 5 (chỉ hiện khi phát hiện bộ gõ khác)`

**Onboarding — Cảnh báo xung đột**

Logic: phát hiện qua NSRunningApplication.runningApplications(withBundleIdentifier:) cho OpenKey, EVKey, Gõ Nhanh · “Thoát OpenKey” gọi terminate() rồi tự sang bước kế · mỗi bộ gõ phát hiện thêm một dòng trong danh sách · cảnh báo dùng màu cảnh báo (#B45309 / #F5A65B), không dùng màu lỗi · biểu tượng ứng dụng khác là ô trung tính, không mô phỏng logo của họ.

## C6-onboarding-done (trang 18)

`C6-onboarding-done · 560 × 440 pt · bước 4 / 4`

**Onboarding — Hoàn tất**

SwiftUI: ô thử gõ là TextEditor tự focus khi vào bước (viền primary 2 pt, chữ 20 pt) · placeholder “Gõ thử: Tiếng Việt, khoẻ, thuỷ…” là Text secondary phủ lên, biến mất khi có ký tự · “Xong” đóng cửa sổ và để VisKey chạy nền trên menu bar; không hiện lại onboarding ở lần mở sau.

## D1-convert-window (trang 19)

`D-converter-tool · 600 × 400 pt, co giãn, tối thiểu 520 × 320`

**Công cụ chuyển mã**

SwiftUI: HSplitView hai TextEditor (ô Kết quả chỉ đọc) · mỗi ô có Picker(.menu) bảng mã ngay phía trên · nút ⇄ (24 × 24) giữa hai popup đổi nguồn/đích và đổi luôn nội dung · “Chuyển” là nút mặc định (Return = ⌘↩) · “Sao chép kết quả” hiện ✓ 1,5 giây sau khi bấm · văn bản mẫu: TCVN3 sang Unicode dựng sẵn.

## E1-notify-converted (trang 20)

`E1-notify-converted · banner hệ thống rộng 344 pt`

**Thông báo — Đã chuyển mã clipboard**

UserNotifications: UNMutableNotificationContent · title “VisKey”, body mô tả kết quả, không âm thanh, tự ẩn sau 4 giây (hệ thống quyết định) · chỉ gửi khi bật “Thông báo khi chuyển xong” ở Cài đặt › Chuyển mã · banner do macOS vẽ, mockup chỉ để thống nhất nội dung chữ.

## E2-notify-permission-lost (trang 21)

`E2-notify-permission-lost · banner có nút hành động`

**Thông báo — Mất quyền Trợ năng khi đang chạy**

Logic: khi event tap bị vô hiệu vì AXIsProcessTrusted() trả về false: đổi biểu tượng menu bar sang “tạm dừng”, gửi đúng một thông báo (không lặp lại cho tới khi quyền được cấp rồi mất lần nữa) · nút “Mở Cài đặt hệ thống” là UNNotificationAction mở pane Trợ năng · khi quyền được cấp lại VisKey tự chạy tiếp và xoá thông báo.

## E3-update-window (trang 22)

`E3-update · cửa sổ Sparkle chuẩn (SUUpdateAlert) 620 × 440 pt`

**Cửa sổ cập nhật**

Sparkle 2: giữ bố cục chuẩn của SPUStandardUserDriver để người dùng nhận ra ngay; chỉ tuỳ biến biểu tượng, chữ tiếng Việt (Localizable.strings) và CSS ghi chú phát hành · ghi chú là HTML từ appcast, chữ 13 pt, tiêu đề nhóm “Mới / Sửa” màu accent ngọc · nút mặc định “Cài đặt và khởi động lại” (Return) · nội dung ghi chú bên dưới là mẫu minh hoạ, không phải bản phát hành thật.
