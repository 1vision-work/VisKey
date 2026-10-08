#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Generate light/dark colorsets for Assets.xcassets from design/tokens.json.

Usage: python3 Tools/gen-colors.py [--out VisKey/Resources/Assets.xcassets]

Each name in tokens.json "colorsets" becomes Colors/<Name>.colorset with a universal
(light) entry and a dark appearance entry. Names are PascalCase: text-secondary -> TextSecondary.
"""
import argparse
import json
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def comps(hex_color):
    h = hex_color.lstrip("#")
    r, g, b = (int(h[i:i + 2], 16) / 255 for i in (0, 2, 4))
    return {"alpha": "1.000", "blue": f"{b:.3f}", "green": f"{g:.3f}", "red": f"{r:.3f}"}


def entry(hex_color, dark=False):
    e = {"color": {"color-space": "srgb", "components": comps(hex_color)}, "idiom": "universal"}
    if dark:
        e["appearances"] = [{"appearance": "luminosity", "value": "dark"}]
    return e


def pascal(name):
    return "".join(p.capitalize() for p in name.split("-"))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--tokens", default=str(ROOT / "design" / "tokens.json"))
    ap.add_argument("--out", default=str(ROOT / "VisKey" / "Resources" / "Assets.xcassets"))
    args = ap.parse_args()

    tokens = json.loads(Path(args.tokens).read_text(encoding="utf-8"))
    pool = {**tokens["semantic"], **tokens["ui"]}
    out = Path(args.out) / "Colors"
    if out.exists():
        shutil.rmtree(out)
    out.mkdir(parents=True)
    info = {"info": {"author": "xcode", "version": 1}}
    (out / "Contents.json").write_text(json.dumps({**info, "properties": {"provides-namespace": False}}, indent=2) + "\n")
    for name in tokens["colorsets"]:
        t = pool[name]
        d = out / f"{pascal(name)}.colorset"
        d.mkdir()
        body = {"colors": [entry(t["light"]), entry(t["dark"], dark=True)], **info}
        (d / "Contents.json").write_text(json.dumps(body, indent=2) + "\n")
        print(f"{pascal(name):18} light {t['light']}  dark {t['dark']}")


if __name__ == "__main__":
    main()
