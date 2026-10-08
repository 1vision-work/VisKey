#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Generate VisKey vector icons into design/icons/ from the geometry in design/brand.md.

Usage: python3 Tools/gen-icons.py
"""
import math
from pathlib import Path

OUT = Path(__file__).resolve().parent.parent / "design" / "icons"

INK_500, INK_600, INK_700, INK_400 = "#4C60F5", "#3044D6", "#2534B8", "#8193FF"
JADE_400, JADE_500 = "#3FE0C0", "#14B89A"

# --- Symbol on a 64 u grid (measured from design/pages/brand-v1-02.png) -----------------
# V: stroke 8.5 u, 68 deg between arms (half angle 34 deg). Caret: stroke 6 u, 20 u wide
# (centreline end to end). Round caps and joins.
HALF = math.radians(34)


def symbol_paths(v_w=8.5, c_w=6.0, zoom=1.0):
    cx, cy = 32.0, 32.0
    def z(x, y):
        return (cx + (x - cx) * zoom, cy + (y - cy) * zoom)
    apex = z(32, 52.0)
    ve_l, ve_r = z(32 - 17.1, 26.6), z(32 + 17.1, 26.6)
    cap = z(32, 10.5)
    ce_l, ce_r = z(32 - 10, 19.5), z(32 + 10, 19.5)
    v = f"M{ve_l[0]:.2f} {ve_l[1]:.2f} L{apex[0]:.2f} {apex[1]:.2f} L{ve_r[0]:.2f} {ve_r[1]:.2f}"
    c = f"M{ce_l[0]:.2f} {ce_l[1]:.2f} L{cap[0]:.2f} {cap[1]:.2f} L{ce_r[0]:.2f} {ce_r[1]:.2f}"
    return v, c, v_w, c_w


def symbol_svg(v_color, c_color, v_w=8.5, c_w=6.0, zoom=1.0, size=64):
    v, c, vw, cw = symbol_paths(v_w, c_w, zoom)
    return (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64" width="{size}" height="{size}">\n'
            f'  <g fill="none" stroke-linecap="round" stroke-linejoin="round">\n'
            f'    <path d="{v}" stroke="{v_color}" stroke-width="{vw}"/>\n'
            f'    <path d="{c}" stroke="{c_color}" stroke-width="{cw}"/>\n  </g>\n</svg>\n')


# --- App icon: 1024 canvas, 824 pt squircle, radius 185 pt -----------------------------
def appicon_svg():
    s = 9.7  # pt per grid unit (V stroke 8.5 u -> 82 pt)
    dy = 6   # optical offset measured on the reference
    v, c, _, _ = symbol_paths()
    t = f'translate({512 - 32 * s:.2f} {512 - 32 * s + dy:.2f}) scale({s})'
    return f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1024 1024" width="1024" height="1024">
  <defs>
    <linearGradient id="body" x1="0" y1="0" x2="0" y2="1">
      <stop offset="0" stop-color="{INK_500}"/>
      <stop offset="1" stop-color="{INK_700}"/>
    </linearGradient>
    <linearGradient id="rim" x1="0" y1="0" x2="0" y2="1">
      <stop offset="0" stop-color="#FFFFFF" stop-opacity="0.35"/>
      <stop offset="0.18" stop-color="#FFFFFF" stop-opacity="0"/>
    </linearGradient>
  </defs>
  <rect x="100" y="100" width="824" height="824" rx="185" ry="185" fill="url(#body)"/>
  <rect x="100.75" y="100.75" width="822.5" height="822.5" rx="184.25" ry="184.25" fill="none" stroke="url(#rim)" stroke-width="1.5"/>
  <!-- key face -->
  <rect x="215" y="215" width="594" height="594" rx="128" ry="128" fill="#FFFFFF" fill-opacity="0.08"/>
  <g transform="{t}" fill="none" stroke-linecap="round" stroke-linejoin="round">
    <path d="{v}" stroke="#FFFFFF" stroke-width="8.5"/>
    <path d="{c}" stroke="{JADE_400}" stroke-width="6"/>
  </g>
</svg>
'''


# --- Menu bar templates: 18 x 18 pt, black + alpha only --------------------------------
# Measured from design/pages/brand-v1-03.png (17.9 px/pt). Shapes are outlined with shapely
# so the files are plain filled paths: the "cut-outs" are real holes (alpha 0), not masks.
from shapely.geometry import LineString, box
from shapely.ops import unary_union

Q = 24  # segments per quarter circle


def line(pts, w):
    return LineString(pts).buffer(w / 2, quad_segs=Q, cap_style="round", join_style="round")


def rrect(x, y, w, h, r):
    return box(x + r, y + r, x + w - r, y + h - r).buffer(r, quad_segs=Q)


def outline_rect():  # stroke 1.5 pt ring, outer 15 x 14 pt
    return rrect(1.5, 2, 15, 14, 4.2).difference(rrect(3, 3.5, 12, 11, 2.7))


def glyph_shapes():
    v = line([(5.5, 8.5), (9, 13.7), (12.5, 8.5)], 1.7)
    c = line([(6.65, 6.85), (9, 4.5), (11.35, 6.85)], 1.4)
    return unary_union([v, c])


def to_path(geom):
    polys = [geom] if geom.geom_type == "Polygon" else list(geom.geoms)
    d = []
    for p in polys:
        for ring in [p.exterior, *p.interiors]:
            pts = list(ring.coords)[:-1]
            d.append("M" + " L".join(f"{x:.3f} {y:.3f}" for x, y in pts) + " Z")
    return " ".join(d)


def mb_svg(geom):
    return (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 18 18" width="18" height="18">\n'
            f'  <path fill="#000" fill-rule="evenodd" d="{to_path(geom)}"/>\n</svg>\n')


def menubar_vi():
    return mb_svg(rrect(1.5, 2, 15, 14, 4.2).difference(glyph_shapes()))


def menubar_en():
    e = unary_union([line([(11.25, 5.87), (7.1, 5.87), (7.1, 12.07), (11.25, 12.07)], 1.7),
                     line([(7.1, 8.94), (9.8, 8.94)], 1.7)])
    return mb_svg(unary_union([outline_rect(), e]))


def menubar_off():
    slash = [(2.1, 16.3), (16.2, 2.0)]
    body = unary_union([outline_rect(), glyph_shapes()]).difference(line(slash, 4.4))
    return mb_svg(unary_union([body, line(slash, 1.5)]))


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    files = {
        "symbol.svg": symbol_svg(INK_600, JADE_500),
        "symbol-dark.svg": symbol_svg(INK_400, JADE_400),
        "symbol-mono.svg": symbol_svg("#000000", "#000000"),
        "symbol-hinted.svg": symbol_svg(INK_600, JADE_500, v_w=11, c_w=8.5, zoom=1.12),
        "menubar-vi.svg": menubar_vi(),
        "menubar-en.svg": menubar_en(),
        "menubar-off.svg": menubar_off(),
        "appicon-1024.svg": appicon_svg(),
    }
    for name, body in files.items():
        (OUT / name).write_text(body, encoding="utf-8")
        print("wrote", OUT / name)


if __name__ == "__main__":
    main()
