#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Regenerate the committed raster branding assets for the crossplatform apps
# from the vector sources in design/icons/ (see design/brand.md §3).
#
#   16 px       -> appicon-hinted-16.svg
#   17..32 px   -> appicon-small.svg
#   > 32 px     -> appicon-1024.svg
#
# Requires: rsvg-convert, png2icns (libicns). Output is written next to this script.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
ICONS="$REPO_DIR/design/icons"
OUT="$SCRIPT_DIR"

for tool in rsvg-convert png2icns; do
    command -v "$tool" >/dev/null 2>&1 || { echo "error: $tool not found" >&2; exit 1; }
done

# Pick the brand.md source SVG for a given pixel size.
appicon_src() {
    local px="$1"
    if (( px <= 16 )); then
        echo "$ICONS/appicon-hinted-16.svg"
    elif (( px <= 32 )); then
        echo "$ICONS/appicon-small.svg"
    else
        echo "$ICONS/appicon-1024.svg"
    fi
}

render() { # render <svg> <px> <out.png>
    rsvg-convert -w "$2" -h "$2" "$1" -o "$3"
}

# ---- macOS: iconset + icns ----
ICONSET="$OUT/macos/VisKey.iconset"
rm -rf "$ICONSET"
mkdir -p "$ICONSET"
for base in 16 32 128 256 512; do
    render "$(appicon_src "$base")" "$base" "$ICONSET/icon_${base}x${base}.png"
    render "$(appicon_src $((base * 2)))" $((base * 2)) "$ICONSET/icon_${base}x${base}@2x.png"
done

# png2icns picks the icon type from each PNG's dimensions; feed one file per size.
ICNS_TMP="$(mktemp -d)"
trap 'rm -rf "$ICNS_TMP"' EXIT
for px in 16 32 128 256 512 1024; do
    render "$(appicon_src "$px")" "$px" "$ICNS_TMP/$px.png"
done
png2icns "$OUT/macos/VisKey.icns" \
    "$ICNS_TMP/16.png" "$ICNS_TMP/32.png" "$ICNS_TMP/128.png" \
    "$ICNS_TMP/256.png" "$ICNS_TMP/512.png" "$ICNS_TMP/1024.png" >/dev/null

# ---- macOS: menu bar template images (black + alpha only) ----
for state in vi en off; do
    render "$ICONS/menubar-$state.svg" 18 "$OUT/macos/menubar-$state.png"
    render "$ICONS/menubar-$state.svg" 36 "$OUT/macos/menubar-$state@2x.png"
done

# ---- Linux: hicolor theme + IBus symbol ----
rm -rf "$OUT/linux/hicolor"
for px in 16 22 24 32 48 64 128 256 512; do
    dir="$OUT/linux/hicolor/${px}x${px}/apps"
    mkdir -p "$dir"
    render "$(appicon_src "$px")" "$px" "$dir/viskey.png"
done
mkdir -p "$OUT/linux/hicolor/scalable/apps"
cp "$ICONS/appicon-1024.svg" "$OUT/linux/hicolor/scalable/apps/viskey.svg"
cp "$ICONS/symbol.svg" "$OUT/linux/viskey-symbol.svg"
cp "$OUT/linux/hicolor/256x256/apps/viskey.png" "$OUT/linux/viskey-256.png"

# ---- DMG window background (brand.md §3 "Nền cửa sổ DMG") ----
# 660x400 pt, bg charcoal 50 #F6F7F9. Finder places the 128 px icons with their
# top-left at (115,140) and (405,140), i.e. centres at (179,204) and (469,204);
# the arrow (charcoal 400 #A3A9B5) sits in the gap between them. No text, so the
# output does not depend on locally installed fonts.
mkdir -p "$OUT/dmg"
DMG_SVG="$ICNS_TMP/dmg-background.svg"
cat > "$DMG_SVG" <<'SVG'
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 660 400" width="660" height="400">
  <rect width="660" height="400" fill="#F6F7F9"/>
  <g fill="none" stroke="#A3A9B5" stroke-width="4" stroke-linecap="round" stroke-linejoin="round">
    <path d="M272 204 H384"/>
    <path d="M370 190 L386 204 L370 218"/>
  </g>
</svg>
SVG
rsvg-convert -w 660 -h 400 "$DMG_SVG" -o "$OUT/dmg/background.png"
rsvg-convert -w 1320 -h 800 "$DMG_SVG" -o "$OUT/dmg/background@2x.png"

# Shrink PNGs losslessly when optipng is around (optional).
if command -v optipng >/dev/null 2>&1; then
    find "$OUT" -name '*.png' -exec optipng -quiet -o2 {} +
fi

echo "Branding assets written to $OUT"
