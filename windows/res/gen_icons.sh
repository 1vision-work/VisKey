#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Regenerates the Windows .ico files of VisKey (by 1VISION; based on OpenKey by Mai Vũ Tuyên).
# The generated .ico files are committed, so CI never runs this script.
#
#   VisKey.ico   app icon, from design/icons/appicon-*.svg following design/brand.md §3:
#                16 px hinted, 20-32 px "small" (no key face, rim or shadow), >= 40 px full icon.
#   tray-*.ico   notification-area icons. Windows does not tint tray icons like macOS template images,
#                so they are coloured tiles (icons-src/*.svg) legible on light and dark taskbars:
#                VI = ink #3044D6 tile with the brand glyph, EN = charcoal tile with a white E,
#                OFF = charcoal tile with a struck-through V. <= 24 px use the hinted (thicker) strokes.
#
# Requires rsvg-convert (librsvg) and ImageMagick (`magick` or `convert`).
# Usage: windows/res/gen_icons.sh
set -euo pipefail

here="$(cd "$(dirname "$0")" && pwd)"
icons="$here/../../design/icons"
src="$here/icons-src"
sizes=(16 20 24 32 40 48 64 256)

if command -v magick >/dev/null 2>&1; then im=(magick); else im=(convert); fi
command -v rsvg-convert >/dev/null || { echo "rsvg-convert not found" >&2; exit 1; }

tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

# render <svg> <size> <out.png>
render() {
  rsvg-convert --width "$2" --height "$2" --keep-aspect-ratio --format png "$1" --output "$3"
}

# pack <out.ico> <png>...: strip metadata (timestamps) so the output is reproducible.
pack() {
  local out="$1"; shift
  "${im[@]}" "$@" -strip -define png:exclude-chunks=date,time "$out"
}

# --- app icon ---------------------------------------------------------------------------------
pngs=()
for s in "${sizes[@]}"; do
  if   (( s <= 16 )); then svg="$icons/appicon-hinted-16.svg"
  elif (( s <= 32 )); then svg="$icons/appicon-small.svg"
  else                     svg="$icons/appicon-1024.svg"
  fi
  render "$svg" "$s" "$tmp/app-$s.png"
  pngs+=("$tmp/app-$s.png")
done
pack "$here/VisKey.ico" "${pngs[@]}"

# --- tray icons -------------------------------------------------------------------------------
for state in vi en off; do
  pngs=()
  for s in "${sizes[@]}"; do
    if (( s <= 24 )); then svg="$src/tray-$state-hinted.svg"; else svg="$src/tray-$state.svg"; fi
    render "$svg" "$s" "$tmp/$state-$s.png"
    pngs+=("$tmp/$state-$s.png")
  done
  pack "$here/tray-$state.ico" "${pngs[@]}"
done

# Preview sheet (not committed): tray icons on a light and a dark taskbar colour.
if [[ "${VISKEY_ICON_PREVIEW:-0}" == 1 ]]; then
  for bg in '#F3F3F3' '#202020'; do
    "${im[@]}" -background "$bg" \
      "$tmp/app-32.png" "$tmp/vi-16.png" "$tmp/en-16.png" "$tmp/off-16.png" \
      "$tmp/vi-24.png" "$tmp/en-24.png" "$tmp/off-24.png" "$tmp/vi-32.png" "$tmp/en-32.png" \
      -gravity center -extent 40x40 +append "$tmp/row-${bg#\#}.png"
  done
  "${im[@]}" "$tmp/row-F3F3F3.png" "$tmp/row-202020.png" -append "$here/preview.png"
  echo "wrote $here/preview.png"
fi

ls -l "$here"/*.ico
