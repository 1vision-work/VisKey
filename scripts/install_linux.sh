#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "============================================================"
echo "  VisKey - Trinh cai dat bo go Tieng Viet cho Linux       "
echo "============================================================"

echo "[1/3] Chay kiem thu Unit Test (Python Core Engine)..."
python3 "$ROOT_DIR/core/viskey_engine.py"

if command -v apt-get >/dev/null 2>&1 && command -v dpkg >/dev/null 2>&1; then
    echo "[2/3] Phat hien he thong Debian/Ubuntu/Mint. Dang cai dat goi .deb..."
    DEB_FILE="$ROOT_DIR/ibus-viskey_1.0.0_all.deb"
    if [[ ! -f "$DEB_FILE" ]]; then
        bash "$ROOT_DIR/scripts/build_linux_deb.sh"
    fi
    sudo apt-get update -y || true
    sudo apt-get install -y ibus python3-gi gir1.2-ibus-1.0 || true
    sudo dpkg -i "$DEB_FILE"
else
    echo "[2/3] Dang cai dat truc tiep vao /usr/share/viskey va /usr/share/ibus/component..."
    sudo mkdir -p /usr/share/viskey/core /usr/share/viskey/ibus /usr/libexec /usr/share/ibus/component
    sudo cp "$ROOT_DIR/core/viskey_engine.py" /usr/share/viskey/core/
    sudo cp "$ROOT_DIR/linux/ibus/ibus_viskey.py" /usr/share/viskey/ibus/
    sudo cp "$ROOT_DIR/linux/ibus/viskey.xml" /usr/share/ibus/component/
    sudo tee /usr/libexec/ibus-engine-viskey >/dev/null <<'EOF'
#!/usr/bin/env bash
exec /usr/bin/python3 /usr/share/viskey/ibus/ibus_viskey.py "$@"
EOF
    sudo chmod 0755 /usr/libexec/ibus-engine-viskey /usr/share/viskey/ibus/ibus_viskey.py
fi

echo "[3/3] Khoi dong lai IBus va kich hoat VisKey..."
ibus write-cache 2>/dev/null || true
ibus restart 2>/dev/null || true

echo ""
echo "============================================================"
echo " CAI DAT HOAN TAT!"
echo " -> Vao Settings -> Keyboard -> Input Sources -> Add -> Vietnamese -> VisKey (Tiếng Việt)"
echo " -> Hoac chay lenh: ibus engine viskey"
echo "============================================================"
