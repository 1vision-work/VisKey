#!/usr/bin/env python3
# Checks design/tokens.json: valid JSON, colors are #RRGGBB, and the UI v2 "layout" section is complete.
# Standard library only. Run: python3 tests/test_design_tokens.py  (exit code 1 on failure)
# MIT License, 1VISION.

import json
import os
import re
import sys

TOKENS_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "design", "tokens.json")
HEX_COLOR = re.compile(r"^#[0-9A-Fa-f]{6}$")

# Non-color metadata keys allowed inside color groups.
COLOR_META_KEYS = {"label", "notes", "use", "contrast"}

REQUIRED_LAYOUT = {
    "radius": ["window", "card", "option-card", "control", "chip", "keycap", "popover", "hud", "tray-flyout"],
    "spacing": ["xs", "sm", "md", "lg", "xl", "xxl", "xxxl", "page-padding", "card-padding", "section-gap"],
    "control-height": ["button", "text-field", "search-field", "segmented", "keycap", "sidebar-item",
                       "row", "row-with-subtitle", "table-row", "toggle-width", "toggle-height"],
    "typography": ["page-title", "section-label", "row-title", "row-subtitle", "body", "caption", "mono", "button"],
    "window": ["settings", "onboarding", "menubar-popover", "hud", "tray-flyout"],
    "platform": ["macos", "windows", "linux"],
}
REQUIRED_PLATFORM = {
    "macos": ["sidebar-width"],
    "windows": ["nav-width"],
    "linux": ["clamp-width"],
}

failures = []


def check(cond, msg):
    if not cond:
        failures.append(msg)


def is_number(v):
    return isinstance(v, (int, float)) and not isinstance(v, bool)


def check_color_group(path, group):
    """Every value in a color group that is not metadata must be #RRGGBB."""
    for key, value in group.items():
        if key in COLOR_META_KEYS:
            continue
        check(isinstance(value, str) and HEX_COLOR.match(value) is not None,
              "%s.%s: not a #RRGGBB color: %r" % (path, key, value))


def walk_hash_strings(path, node):
    """Any string anywhere that starts with '#' must be a #RRGGBB color."""
    if isinstance(node, dict):
        for k, v in node.items():
            walk_hash_strings("%s.%s" % (path, k), v)
    elif isinstance(node, list):
        for i, v in enumerate(node):
            walk_hash_strings("%s[%d]" % (path, i), v)
    elif isinstance(node, str) and node.startswith("#"):
        check(HEX_COLOR.match(node) is not None, "%s: malformed color %r" % (path, node))


def main():
    try:
        with open(TOKENS_PATH, encoding="utf-8") as f:
            tokens = json.load(f)
    except (OSError, ValueError) as e:
        print("FAIL: cannot load %s: %s" % (TOKENS_PATH, e))
        return 1

    check(isinstance(tokens, dict), "top level must be an object")
    check(isinstance(tokens.get("_note"), str) and tokens["_note"], "_note must be a non-empty string")
    check(is_number(tokens.get("version")) and tokens["version"] >= 2, "version must be a number >= 2")

    # Colors
    for name, scale in tokens.get("scales", {}).items():
        check(isinstance(scale, dict), "scales.%s must be an object" % name)
        if isinstance(scale, dict):
            check_color_group("scales." + name, scale)
    for section in ("semantic", "ui"):
        roles = tokens.get(section)
        check(isinstance(roles, dict) and roles, "%s section missing" % section)
        for name, role in (roles or {}).items():
            check("light" in role and "dark" in role, "%s.%s needs light and dark" % (section, name))
            check_color_group("%s.%s" % (section, name), role)
    walk_hash_strings("tokens", tokens)

    # Every colorset name refers to a defined semantic/ui role.
    roles = set(tokens.get("semantic", {})) | set(tokens.get("ui", {}))
    for name in tokens.get("colorsets", []):
        check(name in roles, "colorsets: %r is not defined in semantic or ui" % name)

    # Layout
    layout = tokens.get("layout")
    check(isinstance(layout, dict), "layout section missing")
    layout = layout if isinstance(layout, dict) else {}
    for section, keys in REQUIRED_LAYOUT.items():
        group = layout.get(section)
        check(isinstance(group, dict), "layout.%s missing" % section)
        for key in keys:
            check(isinstance(group, dict) and key in group, "layout.%s.%s missing" % (section, key))
    for section in ("radius", "spacing", "control-height"):
        for key, value in (layout.get(section) or {}).items():
            check(is_number(value) and value >= 0, "layout.%s.%s must be a non-negative number" % (section, key))
    for key, value in (layout.get("typography") or {}).items():
        if key == "font-family":
            continue
        check(isinstance(value, dict) and is_number(value.get("size")) and is_number(value.get("weight")),
              "layout.typography.%s needs numeric size and weight" % key)
        if isinstance(value, dict) and is_number(value.get("weight")):
            check(100 <= value["weight"] <= 900, "layout.typography.%s weight out of range" % key)
    settings = (layout.get("window") or {}).get("settings") or {}
    check(is_number(settings.get("width")) and is_number(settings.get("height")),
          "layout.window.settings needs width and height")
    for plat, keys in REQUIRED_PLATFORM.items():
        p = (layout.get("platform") or {}).get(plat) or {}
        for key in keys:
            check(is_number(p.get(key)), "layout.platform.%s.%s must be a number" % (plat, key))

    if failures:
        for msg in failures:
            print("FAIL:", msg)
        print("%d failure(s)" % len(failures))
        return 1
    print("design/tokens.json OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
