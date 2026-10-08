#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Generate VisKey vector icons into design/icons/.

Every number below is copied from the Claude Design canvas (Logo, Icons, Assets boards);
see design/brand.md. Nothing is measured from raster images.

Usage: python3 Tools/gen-icons.py        (needs: pip install shapely)
"""
from pathlib import Path

from shapely.geometry import LineString, box
from shapely.ops import unary_union

OUT = Path(__file__).resolve().parent.parent / "design" / "icons"

INK_400, INK_600 = "#8193FF", "#3044D6"
JADE_400, JADE_500 = "#3FE0C0", "#14B89A"

# --- Symbol, 64 u grid ------------------------------------------------------------------
# V: stroke 8.5 u, 68 deg between arms. Caret: stroke 6 u, 20 u wide. Round caps and joins.
V = "M15 28L32 53L49 28"
CARET = "M22 21L32 12L42 21"
# Hinted (<= 32 px): V 11 u, caret 8.5 u, drawn larger so it fills the cell.
V_HINT = "M14 30L32 54L50 30"
CARET_HINT = "M21 22L32 11L43 22"


def symbol(v_color, c_color, v=V, c=CARET, vw=8.5, cw=6, size=64):
    return (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64" width="{size}" height="{size}">\n'
            f'  <g fill="none" stroke-linecap="round" stroke-linejoin="round">\n'
            f'    <path d="{c}" stroke="{c_color}" stroke-width="{cw}"/>\n'
            f'    <path d="{v}" stroke="{v_color}" stroke-width="{vw}"/>\n  </g>\n</svg>\n')


# --- App icon: 100 u canvas = 1024 px; squircle 80.4 u (= 824 pt), radius 18 u (= 185 pt) ---
GLYPH = 'transform="translate(19.6 19.1) scale(0.95)" fill="none" stroke-linecap="round" stroke-linejoin="round"'


def appicon():
    return f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100" width="1024" height="1024">
  <defs>
    <linearGradient id="body" x1="0" y1="0" x2="0" y2="1">
      <stop offset="0" stop-color="#5266F7"/>
      <stop offset="1" stop-color="#2433B5"/>
    </linearGradient>
    <linearGradient id="rim" x1="0" y1="0" x2="0" y2="1">
      <stop offset="0" stop-color="#FFFFFF" stop-opacity="0.35"/>
      <stop offset="0.5" stop-color="#FFFFFF" stop-opacity="0"/>
    </linearGradient>
    <filter id="shadow" x="-20%" y="-20%" width="140%" height="140%">
      <feDropShadow dx="0" dy="1.6" stdDeviation="1.8" flood-color="#0E0F12" flood-opacity="0.3"/>
    </filter>
  </defs>
  <rect x="9.8" y="9.8" width="80.4" height="80.4" rx="18" fill="url(#body)" filter="url(#shadow)"/>
  <rect x="19" y="18" width="62" height="62" rx="13" fill="#FFFFFF" fill-opacity="0.07"/>
  <rect x="10.3" y="10.3" width="79.4" height="79.4" rx="17.6" fill="none" stroke="url(#rim)" stroke-width="0.8"/>
  <g {GLYPH}>
    <path d="{CARET}" stroke="{JADE_400}" stroke-width="6"/>
    <path d="{V}" stroke="#FFFFFF" stroke-width="8.5"/>
  </g>
</svg>
'''


def appicon_small():
    """32 px and below: no key face, no rim, no shadow; strokes 6.5 / 9."""
    return f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100" width="32" height="32">
  <defs>
    <linearGradient id="body" x1="0" y1="0" x2="0" y2="1">
      <stop offset="0" stop-color="#5266F7"/>
      <stop offset="1" stop-color="#2433B5"/>
    </linearGradient>
  </defs>
  <rect x="9.8" y="9.8" width="80.4" height="80.4" rx="18" fill="url(#body)"/>
  <g {GLYPH}>
    <path d="{CARET}" stroke="{JADE_400}" stroke-width="6.5"/>
    <path d="{V}" stroke="#FFFFFF" stroke-width="9"/>
  </g>
</svg>
'''


def appicon_hinted():
    """16 px: flat ink-600 tile, larger glyph, V 11 u / caret 8.5 u."""
    return f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100" width="16" height="16">
  <rect x="6" y="6" width="88" height="88" rx="20" fill="{INK_600}"/>
  <g transform="translate(16.4 16) scale(1.05)" fill="none" stroke-linecap="round" stroke-linejoin="round">
    <path d="{CARET_HINT}" stroke="{JADE_400}" stroke-width="8.5"/>
    <path d="{V_HINT}" stroke="#FFFFFF" stroke-width="11"/>
  </g>
</svg>
'''


def favicon():
    """Favicon: no squircle padding, tile fills the cell (Assets board)."""
    return f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100" width="32" height="32">
  <rect x="0" y="0" width="100" height="100" rx="22" fill="{INK_600}"/>
  <g transform="translate(12 12) scale(1.18)" fill="none" stroke-linecap="round" stroke-linejoin="round">
    <path d="{CARET_HINT}" stroke="{JADE_400}" stroke-width="8.5"/>
    <path d="{V_HINT}" stroke="#FFFFFF" stroke-width="11"/>
  </g>
</svg>
'''


# --- Menu bar templates: 18 x 18 pt, black + alpha only ---------------------------------
# Outlined with shapely so the files are plain filled paths: cut-outs are real holes (alpha 0).
Q = 24


def line(pts, w):
    return LineString(pts).buffer(w / 2, quad_segs=Q, cap_style="round", join_style="round")


def rrect(x, y, w, h, r):
    return box(x + r, y + r, x + w - r, y + h - r).buffer(r, quad_segs=Q)


def frame():  # outline 13.5 x 12.5, stroke 1.5, radius 3 on the centre line
    return rrect(2.25 - .75, 2.75 - .75, 13.5 + 1.5, 12.5 + 1.5, 3.75).difference(
        rrect(2.25 + .75, 2.75 + .75, 13.5 - 1.5, 12.5 - 1.5, 2.25))


def to_path(geom):
    polys = [geom] if geom.geom_type == "Polygon" else list(geom.geoms)
    d = []
    for p in polys:
        for ring in [p.exterior, *p.interiors]:
            pts = list(ring.coords)[:-1]
            d.append("M" + " L".join(f"{x:.3f} {y:.3f}" for x, y in pts) + " Z")
    return " ".join(d)


def mb(geom):
    return ('<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 18 18" width="18" height="18">\n'
            f'  <path fill="#000" fill-rule="evenodd" d="{to_path(geom)}"/>\n</svg>\n')


def menubar_vi():
    solid = rrect(1.5, 2, 15, 14, 3.5)
    cut = unary_union([line([(6.6, 6.3), (9, 4.4), (11.4, 6.3)], 1.5),
                       line([(5.6, 8.2), (9, 13.6), (12.4, 8.2)], 2)])
    return mb(solid.difference(cut))


def menubar_en():
    e = unary_union([line([(11.2, 5.9), (7, 5.9), (7, 12.1), (11.2, 12.1)], 1.6),
                     line([(7, 9), (10.5, 9)], 1.6)])
    return mb(unary_union([frame(), e]))


def menubar_off():
    slash = [(2, 16), (16, 2)]
    glyph = unary_union([line([(7.2, 6.6), (9, 5.2), (10.8, 6.6)], 1.4),
                         line([(6.4, 8.2), (9, 12.4), (11.6, 8.2)], 1.4)])
    # design: a 3.4 pt gap (line from (1.6,16.4) to (16.4,1.6)) in the background colour, then the 1.4 pt slash
    gap = line([(1.6, 16.4), (16.4, 1.6)], 3.4)
    return mb(unary_union([unary_union([frame(), glyph]).difference(gap), line(slash, 1.4)]))


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    files = {
        "symbol.svg": symbol(INK_600, JADE_500),
        "symbol-dark.svg": symbol(INK_400, JADE_400),
        "symbol-mono.svg": symbol("#000000", "#000000"),
        "symbol-hinted.svg": symbol(INK_600, JADE_500, V_HINT, CARET_HINT, 11, 8.5, 16),
        "menubar-vi.svg": menubar_vi(),
        "menubar-en.svg": menubar_en(),
        "menubar-off.svg": menubar_off(),
        "appicon-1024.svg": appicon(),
        "appicon-small.svg": appicon_small(),
        "appicon-hinted-16.svg": appicon_hinted(),
        "favicon.svg": favicon(),
    }
    for name, body in files.items():
        (OUT / name).write_text(body, encoding="utf-8")
        print("wrote", OUT / name)


if __name__ == "__main__":
    main()
