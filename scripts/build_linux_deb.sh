#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
PKG_NAME="ibus-viskey"
# Single source of the version: linux/ibus/viskey_version.py (VERSION = "x.y.z").
PKG_VER="$(sed -n 's/^VERSION = "\([^"]*\)".*/\1/p' "$ROOT_DIR/linux/ibus/viskey_version.py")"
if [[ -z "$PKG_VER" ]]; then
    echo "Cannot read VERSION from linux/ibus/viskey_version.py" >&2
    exit 1
fi
DEB_STAGE="/tmp/${PKG_NAME}_${PKG_VER}_all"
OUT_DEB="$ROOT_DIR/${PKG_NAME}_${PKG_VER}_all.deb"
ICON_SRC="$ROOT_DIR/linux/ibus/icons/hicolor/scalable/actions"

rm -rf "$DEB_STAGE"
mkdir -p "$DEB_STAGE/DEBIAN"
mkdir -p "$DEB_STAGE/usr/share/viskey/core"
mkdir -p "$DEB_STAGE/usr/share/viskey/ibus"
mkdir -p "$DEB_STAGE/usr/share/viskey/icons/hicolor/scalable/actions"
mkdir -p "$DEB_STAGE/usr/share/icons/hicolor/scalable/apps"
mkdir -p "$DEB_STAGE/usr/libexec"
mkdir -p "$DEB_STAGE/usr/bin"
mkdir -p "$DEB_STAGE/usr/share/ibus/component"
mkdir -p "$DEB_STAGE/usr/share/applications"

# Settings UI: "gir1.2-adw-1 | python3-tk". apt satisfies an alternative with its first entry,
# so every distro that ships libadwaita (Ubuntu 22.04+, Debian 12+; gir1.2-adw-1 itself pulls
# gir1.2-gtk-4.0) gets the GTK4/libadwaita UI, while older ones (e.g. Debian 11) can still
# install the package and get the Tk fallback instead of an unsatisfiable dependency.
cat > "$DEB_STAGE/DEBIAN/control" <<EOF
Package: ibus-viskey
Version: ${PKG_VER}
Section: utils
Priority: optional
Architecture: all
Depends: python3, ibus, python3-gi, gir1.2-ibus-1.0, gir1.2-adw-1 | python3-tk
Maintainer: VisKey Project <support@viskey.org>
Description: Bo go Tieng Viet VisKey cho Linux (IBus + GUI Settings)
 Bo go Tieng Viet Telex/VNI thong minh cho Linux:
  - Ho tro che do Khong gach chan (No Preedit Underline) va Wayland
  - Tu dong khoi phuc tu tieng Anh khi go song ngu (window, software, book...)
  - Dat dau tu do va tu dong dich chuyen dau thanh (Kieu cu & Kieu moi)
  - Giao dien Bang dieu khien (viskey-settings, GTK4/libadwaita) quan ly Go tat va Quick Telex
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
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache -q -t /usr/share/icons/hicolor 2>/dev/null || true
fi
exit 0
EOF
chmod 0755 "$DEB_STAGE/DEBIAN/postinst"

cp "$ROOT_DIR/core/viskey_engine.py" "$DEB_STAGE/usr/share/viskey/core/"
cp "$ROOT_DIR/linux/ibus/ibus_viskey.py" "$DEB_STAGE/usr/share/viskey/ibus/"
cp "$ROOT_DIR/linux/ibus/viskey_settings.py" "$DEB_STAGE/usr/share/viskey/ibus/"
cp "$ROOT_DIR/linux/ibus/viskey_settings_tk.py" "$DEB_STAGE/usr/share/viskey/ibus/"
cp "$ROOT_DIR/linux/ibus/viskey_settings_model.py" "$DEB_STAGE/usr/share/viskey/ibus/"
cp "$ROOT_DIR/linux/ibus/viskey_version.py" "$DEB_STAGE/usr/share/viskey/ibus/"
cp "$ROOT_DIR/linux/ibus/viskey.xml" "$DEB_STAGE/usr/share/ibus/component/"
cp "$ROOT_DIR/design/icons/appicon-1024.svg" "$DEB_STAGE/usr/share/viskey/icons/"
cp "$ROOT_DIR/design/icons/appicon-1024.svg" "$DEB_STAGE/usr/share/icons/hicolor/scalable/apps/org.viskey.Settings.svg"
cp "$ICON_SRC"/viskey-*-symbolic.svg "$DEB_STAGE/usr/share/viskey/icons/hicolor/scalable/actions/"

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
Name=VisKey
GenericName=Bộ gõ tiếng Việt
Comment=Cài đặt bộ gõ tiếng Việt VisKey: kiểu gõ Telex/VNI, gõ tắt, chẩn đoán IBus
Exec=/usr/bin/viskey-settings
Icon=org.viskey.Settings
Terminal=false
Type=Application
Categories=Settings;Utility;
Keywords=Vietnamese;Telex;VNI;IBus;tiếng Việt;bộ gõ;
StartupWMClass=org.viskey.Settings
EOF

chmod -R 0755 "$DEB_STAGE"
chmod 0644 "$DEB_STAGE/DEBIAN/control" \
           "$DEB_STAGE/usr/share/ibus/component/viskey.xml" \
           "$DEB_STAGE/usr/share/viskey/core/viskey_engine.py" \
           "$DEB_STAGE/usr/share/viskey/ibus/viskey_settings_tk.py" \
           "$DEB_STAGE/usr/share/viskey/ibus/viskey_settings_model.py" \
           "$DEB_STAGE/usr/share/viskey/ibus/viskey_version.py" \
           "$DEB_STAGE/usr/share/viskey/icons/appicon-1024.svg" \
           "$DEB_STAGE/usr/share/icons/hicolor/scalable/apps/org.viskey.Settings.svg" \
           "$DEB_STAGE/usr/share/applications/viskey-settings.desktop"
find "$DEB_STAGE/usr/share/viskey/icons/hicolor" -type f -exec chmod 0644 {} +

dpkg-deb --build --root-owner-group "$DEB_STAGE" "/tmp/${PKG_NAME}_${PKG_VER}_all.deb"
cp "/tmp/${PKG_NAME}_${PKG_VER}_all.deb" "$OUT_DEB"
rm -rf "$DEB_STAGE" "/tmp/${PKG_NAME}_${PKG_VER}_all.deb"
echo "Built Debian package: $OUT_DEB"
