#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "============================================================"
echo "  VisKey - Trinh cai dat bo go Tieng Viet cho Linux       "
echo "============================================================"

echo "[1/3] Chay kiem thu Unit Test (Python Core Engine)..."
python3 "$ROOT_DIR/core/viskey_engine.py"

if command -v apt-get >/dev/null 2>&1 && command -v dpkg >/dev/null 2>&1; then
    echo "[2/3] Phat hien he thong Debian/Ubuntu/Mint. Dang dong goi va cai dat goi .deb..."
    DEB_OUT="$(mktemp -d)"
    trap 'rm -rf "$DEB_OUT"' EXIT
    OUT_DIR="$DEB_OUT" bash "$ROOT_DIR/scripts/build_linux_deb.sh"
    sudo apt-get update -y || true
    sudo apt-get install -y ibus python3-gi gir1.2-ibus-1.0 python3-tk || true
    sudo dpkg -i "$DEB_OUT/ibus-viskey_1.0.0_all.deb"
else
    echo "[2/3] Dang cai dat truc tiep vao /usr/share/viskey va /usr/share/ibus/component..."
    echo "      (Can cai them: ibus, python-gobject va Tkinter - vd. 'python3-tkinter' tren Fedora, 'tk' tren Arch)"
    sudo install -d -m 0755 /usr/share/viskey/core /usr/share/viskey/ibus /usr/libexec /usr/bin \
        /usr/share/ibus/component /usr/share/applications
    sudo install -m 0644 "$ROOT_DIR/core/viskey_engine.py" /usr/share/viskey/core/
    sudo install -m 0755 "$ROOT_DIR/linux/ibus/ibus_viskey.py" /usr/share/viskey/ibus/
    sudo install -m 0755 "$ROOT_DIR/linux/ibus/viskey_settings.py" /usr/share/viskey/ibus/
    sudo install -m 0644 "$ROOT_DIR/linux/ibus/viskey.xml" /usr/share/ibus/component/
    sudo install -m 0644 "$ROOT_DIR/linux/viskey-settings.desktop" /usr/share/applications/
    (cd "$ROOT_DIR/branding/linux/hicolor" && find . -type f) | while read -r icon; do
        sudo install -D -m 0644 "$ROOT_DIR/branding/linux/hicolor/$icon" "/usr/share/icons/hicolor/${icon#./}"
    done
    sudo gtk-update-icon-cache -q /usr/share/icons/hicolor 2>/dev/null || true
    sudo tee /usr/libexec/ibus-engine-viskey >/dev/null <<'EOF'
#!/bin/sh
exec /usr/bin/python3 /usr/share/viskey/ibus/ibus_viskey.py "$@"
EOF
    sudo tee /usr/bin/viskey-settings >/dev/null <<'EOF'
#!/bin/sh
exec /usr/bin/python3 /usr/share/viskey/ibus/viskey_settings.py "$@"
EOF
    sudo chmod 0755 /usr/libexec/ibus-engine-viskey /usr/bin/viskey-settings
    sudo ibus write-cache --system 2>/dev/null || true
fi

echo "[3/3] Khoi dong lai IBus va kich hoat VisKey..."
ibus write-cache 2>/dev/null || true
ibus restart 2>/dev/null || true

echo ""
echo "============================================================"
echo " CAI DAT HOAN TAT!"
echo " -> Vao Settings -> Keyboard -> Input Sources -> Add -> Vietnamese -> VisKey (Tiếng Việt)"
echo " -> Hoac chay lenh: ibus engine viskey"
echo " -> Cau hinh bo go: chay 'viskey-settings' hoac mo 'VisKey - Cấu hình Bộ gõ Tiếng Việt'"
echo "============================================================"
