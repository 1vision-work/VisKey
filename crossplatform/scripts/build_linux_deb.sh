#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
PKG_NAME="ibus-viskey"
PKG_VER="1.0.0"
OUT_DIR="${OUT_DIR:-$ROOT_DIR}"
mkdir -p "$OUT_DIR"
OUT_DIR="$(cd "$OUT_DIR" && pwd)"
OUT_DEB="$OUT_DIR/${PKG_NAME}_${PKG_VER}_all.deb"

BUILD_ROOT="$(mktemp -d)"
trap 'rm -rf "$BUILD_ROOT"' EXIT
DEB_STAGE="$BUILD_ROOT/${PKG_NAME}_${PKG_VER}_all"

mkdir -p "$DEB_STAGE/DEBIAN" \
         "$DEB_STAGE/usr/bin" \
         "$DEB_STAGE/usr/libexec" \
         "$DEB_STAGE/usr/share/viskey/core" \
         "$DEB_STAGE/usr/share/viskey/ibus" \
         "$DEB_STAGE/usr/share/ibus/component" \
         "$DEB_STAGE/usr/share/applications"

# ---- Payload ----
install -m 0644 "$ROOT_DIR/core/viskey_engine.py"         "$DEB_STAGE/usr/share/viskey/core/"
install -m 0755 "$ROOT_DIR/linux/ibus/ibus_viskey.py"     "$DEB_STAGE/usr/share/viskey/ibus/"
install -m 0755 "$ROOT_DIR/linux/ibus/viskey_settings.py" "$DEB_STAGE/usr/share/viskey/ibus/"
install -m 0644 "$ROOT_DIR/linux/ibus/viskey.xml"         "$DEB_STAGE/usr/share/ibus/component/"
install -m 0644 "$ROOT_DIR/linux/viskey-settings.desktop" "$DEB_STAGE/usr/share/applications/"

cat > "$DEB_STAGE/usr/libexec/ibus-engine-viskey" <<'EOF'
#!/bin/sh
exec /usr/bin/python3 /usr/share/viskey/ibus/ibus_viskey.py "$@"
EOF
cat > "$DEB_STAGE/usr/bin/viskey-settings" <<'EOF'
#!/bin/sh
exec /usr/bin/python3 /usr/share/viskey/ibus/viskey_settings.py "$@"
EOF
chmod 0755 "$DEB_STAGE/usr/libexec/ibus-engine-viskey" "$DEB_STAGE/usr/bin/viskey-settings"

# ---- Maintainer scripts ----
cat > "$DEB_STAGE/DEBIAN/postinst" <<'EOF'
#!/bin/sh
set -e
if command -v ibus >/dev/null 2>&1; then
    ibus write-cache --system 2>/dev/null || true
fi
exit 0
EOF
cat > "$DEB_STAGE/DEBIAN/postrm" <<'EOF'
#!/bin/sh
set -e
if command -v ibus >/dev/null 2>&1; then
    ibus write-cache --system 2>/dev/null || true
fi
exit 0
EOF
chmod 0755 "$DEB_STAGE/DEBIAN/postinst" "$DEB_STAGE/DEBIAN/postrm"

# Normalize permissions: dirs 0755; files keep 0755 only if executable above, else 0644
find "$DEB_STAGE" -type d -exec chmod 0755 {} +
find "$DEB_STAGE" -type f ! -perm -u+x -exec chmod 0644 {} +
find "$DEB_STAGE" -type f -perm -u+x -exec chmod 0755 {} +

# ---- Metadata ----
(cd "$DEB_STAGE" && find usr -type f -print0 | sort -z | xargs -0 md5sum) > "$DEB_STAGE/DEBIAN/md5sums"
INSTALLED_SIZE="$(du -sk --apparent-size "$DEB_STAGE/usr" | cut -f1)"

cat > "$DEB_STAGE/DEBIAN/control" <<EOF
Package: ${PKG_NAME}
Version: ${PKG_VER}
Section: utils
Priority: optional
Architecture: all
Installed-Size: ${INSTALLED_SIZE}
Depends: python3, ibus, python3-gi, gir1.2-ibus-1.0, python3-tk
Maintainer: VisKey Project <support@viskey.org>
Description: Bo go Tieng Viet VisKey cho Linux (IBus)
 Bo go Tieng Viet Telex/VNI thong minh cho Linux:
  - Ho tro che do Khong gach chan (No Preedit Underline) va Wayland
  - Tu dong khoi phuc tu tieng Anh khi go song ngu (window, software, book...)
  - Dat dau tu do va tu dong dich chuyen dau thanh (Kieu cu & Kieu moi)
  - Ho tro Go tat (Macro) va Quick Telex
  - Cong cu cau hinh do hoa: viskey-settings
EOF
chmod 0644 "$DEB_STAGE/DEBIAN/control" "$DEB_STAGE/DEBIAN/md5sums"

dpkg-deb --build --root-owner-group "$DEB_STAGE" "$OUT_DEB"
echo "Built Debian package: $OUT_DEB"
