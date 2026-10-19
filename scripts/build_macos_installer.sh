#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build_macos"
APP_BUNDLE="$BUILD_DIR/VisKey.app"
DMG_PATH="$ROOT_DIR/VisKey-macOS-Installer.dmg"

echo "============================================================"
echo "  VisKey - Trinh tao bo cai dat macOS (.app & .dmg)       "
echo "============================================================"

if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "[!] Script nay danh cho macOS (Darwin). Tren Linux hay dung scripts/install_linux.sh."
    exit 1
fi

echo "[1/4] Kiem tra trinh bien dich clang++ (Xcode Command Line Tools)..."
if ! command -v clang++ >/dev/null 2>&1; then
    echo "Chua co Xcode Command Line Tools. Dang kich hoat hop thoai cai dat..."
    xcode-select --install
    exit 1
fi

echo "[2/4] Chay bo kiem thu Unit Test C++17..."
mkdir -p "$BUILD_DIR"
clang++ -std=c++17 -O2 "$ROOT_DIR/tests/test_cpp_engine.cpp" -o "$BUILD_DIR/test_cpp_engine"
"$BUILD_DIR/test_cpp_engine"

echo "[3/4] Bien dich VisKey.app (Universal Binary arm64 + x86_64)..."
rm -rf "$APP_BUNDLE"
mkdir -p "$APP_BUNDLE/Contents/MacOS"
mkdir -p "$APP_BUNDLE/Contents/Resources"

cp "$ROOT_DIR/macos/Info.plist" "$APP_BUNDLE/Contents/Info.plist"

# Sources: keep in sync with VISKEY_MACOS_SOURCES in CMakeLists.txt.
MACOS_SOURCES=(
    "$ROOT_DIR/macos/VisKeyApp.mm"
    "$ROOT_DIR/macos/ui/VKUIKit.mm"
    "$ROOT_DIR/macos/ui/VKModel.mm"
    "$ROOT_DIR/macos/ui/VKPageBase.mm"
    "$ROOT_DIR/macos/ui/VKPageGeneral.mm"
    "$ROOT_DIR/macos/ui/VKPageMacros.mm"
    "$ROOT_DIR/macos/ui/VKPageApps.mm"
    "$ROOT_DIR/macos/ui/VKPageShortcuts.mm"
    "$ROOT_DIR/macos/ui/VKPageInfo.mm"
    "$ROOT_DIR/macos/ui/VKSettingsWindow.mm"
    "$ROOT_DIR/macos/ui/VKPopover.mm"
    "$ROOT_DIR/macos/ui/VKHUD.mm"
    "$ROOT_DIR/macos/ui/VKOnboarding.mm"
)

# Try Universal build (arm64 + x86_64), fallback to host architecture
if clang++ -std=c++17 -O2 -fobjc-arc -mmacosx-version-min=11.0 -arch arm64 -arch x86_64 \
    "${MACOS_SOURCES[@]}" \
    -framework Cocoa -framework ApplicationServices -framework Carbon \
    -o "$APP_BUNDLE/Contents/MacOS/VisKey" 2>/dev/null; then
    echo " -> Da bien dich thanh cong Universal Binary (Apple Silicon + Intel)!"
else
    clang++ -std=c++17 -O2 -fobjc-arc -mmacosx-version-min=11.0 \
        "${MACOS_SOURCES[@]}" \
        -framework Cocoa -framework ApplicationServices -framework Carbon \
        -o "$APP_BUNDLE/Contents/MacOS/VisKey"
    echo " -> Da bien dich thanh cong cho kien truc may hien tai ($(uname -m))!"
fi

# Ad-hoc codesign so macOS Gatekeeper & TCC Accessibility recognize the bundle cleanly
codesign --force --deep --sign - "$APP_BUNDLE"

echo "[4/4] Dong goi bo cai dat VisKey-macOS-Installer.dmg..."
DMG_STAGE="$BUILD_DIR/dmg_stage"
rm -rf "$DMG_STAGE" "$DMG_PATH"
mkdir -p "$DMG_STAGE"
cp -R "$APP_BUNDLE" "$DMG_STAGE/"
ln -s /Applications "$DMG_STAGE/Applications"

hdiutil create -volname "VisKey Installer" \
    -srcfolder "$DMG_STAGE" \
    -ov -format UDZO "$DMG_PATH" >/dev/null

echo ""
echo "============================================================"
echo " HOAN TAT! Da tao xong:"
echo "  1. App Bundle : $APP_BUNDLE"
echo "  2. File .DMG  : $DMG_PATH"
echo "============================================================"

read -p "Ban co muon cai dat luon VisKey.app vao /Applications va khoi dong ngay khong? [Y/n]: " ans
ans=${ans:-Y}
if [[ "$ans" =~ ^[Yy]$ ]]; then
    killall VisKey 2>/dev/null || true
    cp -R "$APP_BUNDLE" /Applications/
    xattr -cr /Applications/VisKey.app 2>/dev/null || true
    open /Applications/VisKey.app
    echo "Da cai dat vao /Applications/VisKey.app va khoi dong!"
    echo "Luu y: Hay cap quyen cho VisKey trong System Settings -> Privacy & Security -> Accessibility."
fi
