# VisKey

Bộ gõ tiếng Việt native cho macOS — gõ đúng ở mọi nơi.

> Trạng thái: đang phát triển (M0). Chưa có bản phát hành.

- License: GPL-3.0-or-later
- Based on OpenKey by Mai Vũ Tuyên — https://github.com/tuyenvm/OpenKey
- By 1VISION

Spec dự án: [CLAUDE.md](CLAUDE.md). Quyết định kỹ thuật: [docs/decisions.md](docs/decisions.md).

## Engine (C++17, chạy được trên Linux và macOS)

```sh
cmake -S Engine -B build/engine && cmake --build build/engine -j
ctest --test-dir build/engine --output-on-failure
```

Xem [Engine/tests/README.md](Engine/tests/README.md) về snapshot so với engine OpenKey gốc.

## App macOS (Swift, cần Xcode 15+ và XcodeGen)

```sh
brew install xcodegen
xcodegen generate                  # sinh VisKey.xcodeproj từ project.yml (không commit)
xcodebuild -project VisKey.xcodeproj -scheme VisKey -configuration Debug -derivedDataPath build/DerivedData build
open build/DerivedData/Build/Products/Debug/VisKey.app
xcodebuild -project VisKey.xcodeproj -scheme VisKey -derivedDataPath build/DerivedData test   # XCTest
```

Lần chạy đầu VisKey mở Cài đặt hệ thống › Quyền riêng tư & Bảo mật › Trợ năng; bật VisKey là gõ được, không cần mở lại app.
Bản dev mặc định ký ad-hoc nên mỗi lần build lại macOS coi là app mới: tắt rồi bật lại VisKey trong danh sách Trợ năng (hoặc xoá bằng nút − và cấp lại).
Có chứng chỉ "Apple Development" thì tạo `Config/Signing.local.xcconfig` (không commit, xem `Config/Signing.xcconfig`) rồi `xcodegen generate` lại; quyền Trợ năng sẽ giữ qua các lần build.

Tài sản giao diện (`VisKey/Resources/Assets.xcassets`) sinh từ `design/` bằng `Tools/gen-assets.sh`.
