#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
PKG_NAME="ibus-viskey"
PKG_VER="1.0.0"
DEB_STAGE="/tmp/${PKG_NAME}_${PKG_VER}_all"
OUT_DEB="$ROOT_DIR/${PKG_NAME}_${PKG_VER}_all.deb"

rm -rf "$DEB_STAGE"
mkdir -p "$DEB_STAGE/DEBIAN"
mkdir -p "$DEB_STAGE/usr/share/viskey/core"
mkdir -p "$DEB_STAGE/usr/share/viskey/ibus"
mkdir -p "$DEB_STAGE/usr/libexec"
mkdir -p "$DEB_STAGE/usr/bin"
mkdir -p "$DEB_STAGE/usr/share/ibus/component"
mkdir -p "$DEB_STAGE/usr/share/applications"

cat > "$DEB_STAGE/DEBIAN/control" <<EOF
Package: ibus-viskey
Version: 1.0.0
Section: utils
Priority: optional
Architecture: all
Depends: python3, ibus, python3-gi, gir1.2-ibus-1.0, python3-tk
Maintainer: VisKey Project <support@viskey.org>
Description: Bo go Tieng Viet VisKey cho Linux (IBus + GUI Settings)
 Bo go Tieng Viet Telex/VNI thong minh cho Linux:
  - Ho tro che do Khong gach chan (No Preedit Underline) va Wayland
  - Tu dong khoi phuc tu tieng Anh khi go song ngu (window, software, book...)
  - Dat dau tu do va tu dong dich chuyen dau thanh (Kieu cu & Kieu moi)
  - Giao dien Bang dieu khien (viskey-settings) quan ly Macro va Quick Telex
EOF

cat > "$DEB_STAGE/DEBIAN/postinst" <<'EOF'
#!/bin/sh
set -e
chmod +x /usr/libexec/ibus-engine-viskey
chmod +x /usr/bin/viskey-settings
chmod +x /usr/share/viskey/ibus/ibus_viskey.py
chmod +x /usr/share/viskey/ibus/viskey_settings.py
if command -v ibus >/dev/null 2>&1; then
    ibus write-cache --system 2>/dev/null || true
fi
exit 0
EOF
chmod 0755 "$DEB_STAGE/DEBIAN/postinst"

cp "$ROOT_DIR/core/viskey_engine.py" "$DEB_STAGE/usr/share/viskey/core/"
cp "$ROOT_DIR/linux/ibus/ibus_viskey.py" "$DEB_STAGE/usr/share/viskey/ibus/"
cp "$ROOT_DIR/linux/ibus/viskey_settings.py" "$DEB_STAGE/usr/share/viskey/ibus/"
cp "$ROOT_DIR/linux/ibus/viskey.xml" "$DEB_STAGE/usr/share/ibus/component/"

cat > "$DEB_STAGE/usr/libexec/ibus-engine-viskey" <<'EOF'
#!/usr/bin/env bash
exec /usr/bin/python3 /usr/share/viskey/ibus/ibus_viskey.py "$@"
EOF

cat > "$DEB_STAGE/usr/bin/viskey-settings" <<'EOF'
#!/usr/bin/env bash
exec /usr/bin/python3 /usr/share/viskey/ibus/viskey_settings.py "$@"
EOF

cat > "$DEB_STAGE/usr/share/applications/viskey-settings.desktop" <<'EOF'
[Desktop Entry]
Name=VisKey - Cấu hình Bộ gõ Tiếng Việt
Comment=Bảng điều khiển bộ gõ Tiếng Việt VisKey (Telex/VNI/Macro)
Exec=/usr/bin/viskey-settings
Icon=input-keyboard
Terminal=false
Type=Application
Categories=Settings;Utility;
EOF

chmod -R 0755 "$DEB_STAGE"
chmod 0644 "$DEB_STAGE/DEBIAN/control" \
           "$DEB_STAGE/usr/share/ibus/component/viskey.xml" \
           "$DEB_STAGE/usr/share/viskey/core/viskey_engine.py" \
           "$DEB_STAGE/usr/share/applications/viskey-settings.desktop"

dpkg-deb --build --root-owner-group "$DEB_STAGE" "/tmp/${PKG_NAME}_${PKG_VER}_all.deb"
cp "/tmp/${PKG_NAME}_${PKG_VER}_all.deb" "$OUT_DEB"
rm -rf "$DEB_STAGE" "/tmp/${PKG_NAME}_${PKG_VER}_all.deb"
echo "Built Debian package: $OUT_DEB"
