#!/bin/zsh
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Regenerate VisKey/Resources/Assets.xcassets from design/:
#   - Colors/*.colorset          from design/tokens.json (Tools/gen-colors.py)
#   - MenuBar{VI,EN,Off}.imageset from design/icons/menubar-*.svg (vector, template)
#   - AppIcon.appiconset         from design/icons/appicon-*.svg (PNG, rendered by Quick Look)
#
# macOS only (qlmanage + sips). Run from anywhere: Tools/gen-assets.sh
set -euo pipefail

ROOT=${0:A:h:h}
ICONS=$ROOT/design/icons
XCASSETS=$ROOT/VisKey/Resources/Assets.xcassets
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

mkdir -p "$XCASSETS"
cat > "$XCASSETS/Contents.json" <<'EOF'
{
  "info" : {
    "author" : "xcode",
    "version" : 1
  }
}
EOF

python3 "$ROOT/Tools/gen-colors.py" --out "$XCASSETS"

# Menu bar: black + alpha SVG kept as vector, rendered as template (brand.md §3).
for state name in vi MenuBarVI en MenuBarEN off MenuBarOff; do
  set=$XCASSETS/$name.imageset
  rm -rf "$set" && mkdir -p "$set"
  cp "$ICONS/menubar-$state.svg" "$set/menubar-$state.svg"
  cat > "$set/Contents.json" <<EOF
{
  "images" : [
    {
      "filename" : "menubar-$state.svg",
      "idiom" : "universal"
    }
  ],
  "info" : {
    "author" : "xcode",
    "version" : 1
  },
  "properties" : {
    "preserves-vector-representation" : true,
    "template-rendering-intent" : "template"
  }
}
EOF
done

# App icon. brand.md §3: 16 px uses the hinted variant, up to 32 px the small variant, larger the full icon.
# Quick Look renders an SVG at its intrinsic size, so each source is rendered at 1024 and scaled down.
render() { # <svg> <out.png>
  sed -E 's/width="[0-9.]+" height="[0-9.]+"/width="1024" height="1024"/' "$1" > "$TMP/src.svg"
  qlmanage -t -s 1024 -o "$TMP" "$TMP/src.svg" >/dev/null 2>&1
  mv "$TMP/src.svg.png" "$2"
}
render "$ICONS/appicon-1024.svg" "$TMP/full.png"
render "$ICONS/appicon-small.svg" "$TMP/small.png"
render "$ICONS/appicon-hinted-16.svg" "$TMP/hinted.png"

set=$XCASSETS/AppIcon.appiconset
rm -rf "$set" && mkdir -p "$set"
images=()
for spec in 16:1:hinted 16:2:small 32:1:small 32:2:full 128:1:full 128:2:full 256:1:full 256:2:full 512:1:full 512:2:full; do
  pt=${spec%%:*}; rest=${spec#*:}; scale=${rest%%:*}; src=${rest#*:}
  px=$((pt * scale))
  file=icon_${pt}x${pt}@${scale}x.png
  sips -z $px $px "$TMP/$src.png" --out "$set/$file" >/dev/null
  images+=("    { \"filename\" : \"$file\", \"idiom\" : \"mac\", \"scale\" : \"${scale}x\", \"size\" : \"${pt}x${pt}\" }")
done
{
  print '{'
  print '  "images" : ['
  print -l -- "${(j:,\n:)images}"
  print '  ],'
  print '  "info" : {'
  print '    "author" : "xcode",'
  print '    "version" : 1'
  print '  }'
  print '}'
} > "$set/Contents.json"

echo "Assets written to ${XCASSETS#$ROOT/}"
