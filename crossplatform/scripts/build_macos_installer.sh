#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
APP_VERSION="1.0.0"
BUNDLE_ID="org.viskey.inputmethod"
BUILD_DIR="$ROOT_DIR/build_macos"
BRANDING_DIR="$ROOT_DIR/branding/macos"
DMG_BRANDING_DIR="$ROOT_DIR/branding/dmg"
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

# App icon: prefer building from the iconset with iconutil, fall back to the committed .icns
if command -v iconutil >/dev/null 2>&1 && \
    iconutil -c icns "$BRANDING_DIR/VisKey.iconset" -o "$APP_BUNDLE/Contents/Resources/VisKey.icns" 2>/dev/null; then
    echo " -> VisKey.icns tao tu iconset (iconutil)"
else
    cp "$BRANDING_DIR/VisKey.icns" "$APP_BUNDLE/Contents/Resources/VisKey.icns"
    echo " -> VisKey.icns lay tu ban da commit"
fi
# Menu bar template images (VI / EN / off)
for state in vi en off; do
    cp "$BRANDING_DIR/menubar-$state.png" "$BRANDING_DIR/menubar-$state@2x.png" "$APP_BUNDLE/Contents/Resources/"
done

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

# Branding: background image + volume icon (layout applied below, best-effort)
mkdir -p "$DMG_STAGE/.background"
cp "$DMG_BRANDING_DIR/background.png" "$DMG_BRANDING_DIR/background@2x.png" "$DMG_STAGE/.background/"
cp "$APP_BUNDLE/Contents/Resources/VisKey.icns" "$DMG_STAGE/.VolumeIcon.icns"

DMG_VOLNAME="VisKey Installer"
DMG_RW="$BUILD_DIR/VisKey-rw.dmg"
rm -f "$DMG_RW"
hdiutil create -volname "$DMG_VOLNAME" \
    -srcfolder "$DMG_STAGE" \
    -fs HFS+ -ov -format UDRW "$DMG_RW" >/dev/null

# Run a command with a time limit (macOS has no coreutils `timeout` by default)
run_with_timeout() {
    local secs="$1"; shift
    "$@" &
    local pid=$!
    ( sleep "$secs"; kill "$pid" 2>/dev/null ) >/dev/null 2>&1 &
    local watcher=$!
    local rc=0
    wait "$pid" || rc=$?
    kill "$watcher" 2>/dev/null || true
    wait "$watcher" 2>/dev/null || true
    return "$rc"
}

# Finder window layout needs a GUI session + Finder automation permission, so only try it outside CI
DO_LAYOUT=0
if [[ "$IS_CI" != 1 ]] && command -v osascript >/dev/null 2>&1; then
    DO_LAYOUT=1
fi
ATTACH_FLAGS=(-readwrite -noverify -noautoopen)
[[ "$DO_LAYOUT" == 1 ]] || ATTACH_FLAGS+=(-nobrowse)

MOUNT_DIR="$(mktemp -d /tmp/viskey-dmg.XXXXXX)"
if hdiutil attach "$DMG_RW" "${ATTACH_FLAGS[@]}" -mountpoint "$MOUNT_DIR" >/dev/null; then
    # Custom volume icon flag (SetFile ships with Xcode; skip quietly if missing)
    if command -v SetFile >/dev/null 2>&1; then
        SetFile -a C "$MOUNT_DIR" 2>/dev/null || true
    fi
    # Icons centred at (179,204) / (469,204) to match the 660x400 background
    if [[ "$DO_LAYOUT" == 1 ]]; then
        LAYOUT_SCRIPT="$BUILD_DIR/dmg_layout.applescript"
        cat > "$LAYOUT_SCRIPT" <<OSA
tell application "Finder"
    set dmgFolder to (POSIX file "$MOUNT_DIR" as alias)
    open dmgFolder
    set theWindow to container window of dmgFolder
    set current view of theWindow to icon view
    set toolbar visible of theWindow to false
    set statusbar visible of theWindow to false
    set bounds of theWindow to {200, 120, 860, 520}
    set viewOptions to icon view options of theWindow
    set arrangement of viewOptions to not arranged
    set icon size of viewOptions to 128
    set background picture of viewOptions to file ".background:background.png" of dmgFolder
    set position of item "VisKey.app" of dmgFolder to {179, 204}
    set position of item "Applications" of dmgFolder to {469, 204}
    update dmgFolder without registering applications
    delay 1
    close theWindow
end tell
OSA
        run_with_timeout 30 osascript "$LAYOUT_SCRIPT" >/dev/null 2>&1 || echo " -> (Bo qua bo cuc Finder cho DMG)"
    fi
    sync
    hdiutil detach "$MOUNT_DIR" -quiet || hdiutil detach "$MOUNT_DIR" -force -quiet || true
fi
rmdir "$MOUNT_DIR" 2>/dev/null || true

hdiutil convert "$DMG_RW" -format UDZO -imagekey zlib-level=9 -ov -o "$DMG_PATH" >/dev/null
rm -f "$DMG_RW"

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
