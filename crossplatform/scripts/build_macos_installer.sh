#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
APP_VERSION="1.0.0"
BUNDLE_ID="org.viskey.inputmethod"
BUILD_DIR="$ROOT_DIR/build_macos"
APP_BUNDLE="$BUILD_DIR/VisKey.app"
DMG_PATH="$ROOT_DIR/VisKey-macOS-Installer.dmg"
PKG_PATH="$ROOT_DIR/VisKey-${APP_VERSION}-macOS.pkg"

# Non-interactive when running in CI or without a terminal
IS_CI=0
if [[ -n "${CI:-}" && "${CI}" != "0" && "${CI}" != "false" ]]; then
    IS_CI=1
fi

echo "============================================================"
echo "  VisKey - Trinh tao bo cai dat macOS (.app, .dmg & .pkg)  "
echo "============================================================"

if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "[!] Script nay danh cho macOS (Darwin). Tren Linux hay dung scripts/install_linux.sh."
    exit 1
fi

echo "[1/5] Kiem tra trinh bien dich clang++ (Xcode Command Line Tools)..."
if ! command -v clang++ >/dev/null 2>&1; then
    if [[ "$IS_CI" == 1 ]]; then
        echo "[!] clang++ not found in CI." >&2
        exit 1
    fi
    echo "Chua co Xcode Command Line Tools. Dang kich hoat hop thoai cai dat..."
    xcode-select --install || true
    exit 1
fi

echo "[2/5] Chay bo kiem thu Unit Test C++17..."
mkdir -p "$BUILD_DIR"
clang++ -std=c++17 -O2 "$ROOT_DIR/tests/test_cpp_engine.cpp" -o "$BUILD_DIR/test_cpp_engine"
"$BUILD_DIR/test_cpp_engine"

echo "[3/5] Bien dich VisKey.app (Universal Binary arm64 + x86_64)..."
rm -rf "$APP_BUNDLE"
mkdir -p "$APP_BUNDLE/Contents/MacOS"
mkdir -p "$APP_BUNDLE/Contents/Resources"

cp "$ROOT_DIR/macos/Info.plist" "$APP_BUNDLE/Contents/Info.plist"
plutil -lint "$APP_BUNDLE/Contents/Info.plist"

APP_BIN="$APP_BUNDLE/Contents/MacOS/VisKey"
CXX_FLAGS=(-std=c++17 -O2 -fobjc-arc -mmacosx-version-min=11.0)
FRAMEWORKS=(-framework Cocoa -framework ApplicationServices -framework Carbon)
BUILD_LOG="$BUILD_DIR/universal_build.log"

# Universal build (arm64 + x86_64). In CI a failure is fatal; locally fall back to host arch.
if clang++ "${CXX_FLAGS[@]}" -arch arm64 -arch x86_64 \
    "$ROOT_DIR/macos/VisKeyApp.mm" "${FRAMEWORKS[@]}" \
    -o "$APP_BIN" 2>"$BUILD_LOG"; then
    echo " -> Da bien dich thanh cong Universal Binary (Apple Silicon + Intel)!"
else
    cat "$BUILD_LOG" >&2
    if [[ "$IS_CI" == 1 ]]; then
        echo "[!] Universal build failed in CI (no fallback allowed)." >&2
        exit 1
    fi
    echo "[!] Universal build that bai, bien dich cho kien truc may hien tai ($(uname -m))..." >&2
    clang++ "${CXX_FLAGS[@]}" \
        "$ROOT_DIR/macos/VisKeyApp.mm" "${FRAMEWORKS[@]}" \
        -o "$APP_BIN"
    echo " -> Da bien dich thanh cong cho kien truc may hien tai ($(uname -m))!"
fi
echo " -> Kien truc: $(lipo -archs "$APP_BIN")"

# Ad-hoc codesign so macOS Gatekeeper & TCC Accessibility recognize the bundle cleanly
codesign --force --deep --sign - "$APP_BUNDLE"
codesign -v "$APP_BUNDLE"

echo "[4/5] Dong goi bo cai dat VisKey-macOS-Installer.dmg..."
DMG_STAGE="$BUILD_DIR/dmg_stage"
rm -rf "$DMG_STAGE" "$DMG_PATH"
mkdir -p "$DMG_STAGE"
cp -R "$APP_BUNDLE" "$DMG_STAGE/"
ln -s /Applications "$DMG_STAGE/Applications"

hdiutil create -volname "VisKey Installer" \
    -srcfolder "$DMG_STAGE" \
    -ov -format UDZO "$DMG_PATH" >/dev/null

echo "[5/5] Dong goi bo cai dat VisKey-${APP_VERSION}-macOS.pkg..."
PKG_ROOT="$BUILD_DIR/pkg_root"
PKG_COMPONENTS="$BUILD_DIR/pkg_components.plist"
rm -rf "$PKG_ROOT" "$PKG_PATH"
mkdir -p "$PKG_ROOT"
cp -R "$APP_BUNDLE" "$PKG_ROOT/"

# Always install to /Applications (don't let Installer relocate onto another copy of VisKey.app)
pkgbuild --analyze --root "$PKG_ROOT" "$PKG_COMPONENTS" >/dev/null
plutil -replace 0.BundleIsRelocatable -bool NO "$PKG_COMPONENTS"

pkgbuild --root "$PKG_ROOT" \
    --component-plist "$PKG_COMPONENTS" \
    --identifier "$BUNDLE_ID" \
    --version "$APP_VERSION" \
    --install-location /Applications \
    "$PKG_PATH"

echo ""
echo "============================================================"
echo " HOAN TAT! Da tao xong:"
echo "  1. App Bundle : $APP_BUNDLE"
echo "  2. File .DMG  : $DMG_PATH"
echo "  3. File .PKG  : $PKG_PATH"
echo "============================================================"

if [[ "$IS_CI" == 1 || ! -t 0 ]]; then
    echo "(Che do khong tuong tac: bo qua buoc cai dat vao /Applications)"
    exit 0
fi

read -r -p "Ban co muon cai dat luon VisKey.app vao /Applications va khoi dong ngay khong? [Y/n]: " ans
ans=${ans:-Y}
if [[ "$ans" =~ ^[Yy]$ ]]; then
    killall VisKey 2>/dev/null || true
    rm -rf /Applications/VisKey.app
    cp -R "$APP_BUNDLE" /Applications/
    xattr -cr /Applications/VisKey.app 2>/dev/null || true
    open /Applications/VisKey.app
    echo "Da cai dat vao /Applications/VisKey.app va khoi dong!"
    echo "Luu y: Hay cap quyen cho VisKey trong System Settings -> Privacy & Security -> Accessibility."
fi
