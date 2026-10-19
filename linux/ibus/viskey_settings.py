#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
VisKey settings for Linux — GTK 4 + libadwaita (application id org.viskey.Settings).

Tabs "Chung" / "Gõ tắt" / "Giới thiệu". Every change is written immediately to
~/.config/viskey/config.json (atomic replace); the IBus engine (ibus_viskey.py) reloads that
file by mtime on the next focus-in, so IBus never needs a restart.

Works on libadwaita 1.1+ (newer widgets are feature-detected) and falls back to the Tk UI in
viskey_settings_tk.py when GTK 4 / libadwaita are not installed.

    viskey_settings.py                      open the settings window
    viskey_settings.py --ui-snapshot DIR    render every tab (light + dark) to PNG and exit;
                                            uses in-memory defaults + demo data only (never
                                            reads/writes the real config, no network)

Importing this module never creates any GUI object (all of that happens under main()).
MIT License, 1VISION.
"""
import os
import platform
import sys
import threading

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
if SCRIPT_DIR not in sys.path:
    sys.path.insert(0, SCRIPT_DIR)

import viskey_settings_model as model  # noqa: E402
from viskey_version import VERSION  # noqa: E402

try:
    import gi
    gi.require_version("Gtk", "4.0")
    gi.require_version("Gdk", "4.0")
    gi.require_version("Adw", "1")
    gi.require_version("Graphene", "1.0")
    from gi.repository import Adw, Gdk, Gio, GLib, GObject, Graphene, Gtk, Pango
    HAVE_GTK = True
except (ImportError, ValueError):
    HAVE_GTK = False

APP_ID = "org.viskey.Settings"
TAGLINE = "Bộ gõ tiếng Việt cho macOS, Linux & Windows · 1VISION"
TOAST_APPLIED = "Đã áp dụng — không cần khởi động lại IBus"
QUICK_TELEX_HINT = "cc=ch gg=gi kk=kh nn=ng qq=qu pp=ph tt=th"
CASE_HINT = "Tự giữ kiểu chữ: ko → không · Ko → Không · KO → KHÔNG"
METHOD_VALUES = ("auto", "backspace")

# Demo data for --ui-snapshot (never written anywhere).
DEMO_MACROS = {
    "bc": "báo cáo", "bt": "bình thường", "ck": "chuyển khoản", "cty": "công ty",
    "dc": "được", "dt": "điện thoại", "hcm": "Hồ Chí Minh", "hn": "Hà Nội",
    "j": "gì", "kh": "khách hàng", "ko": "không", "kq": "kết quả",
    "mn": "mọi người", "ng": "người", "ntn": "như thế nào", "nv": "nhân viên",
    "ql": "quản lý", "sg": "Sài Gòn", "sp": "sản phẩm", "tg": "thời gian",
    "tks": "cảm ơn nhiều", "uh": "ừ", "vd": "ví dụ", "vn": "Việt Nam",
}
DEMO_USAGE = {
    "bt": 212, "dc": 1038, "ko": 2417, "mn": 386, "ntn": 154, "uh": 97, "vn": 640,
    "cty": 75, "tks": 58, "j": 433, "ng": 301, "kh": 64, "sp": 41, "hn": 22,
}

# Brand colours from design/tokens.json (light / dark).
BRAND = {
    False: {"accent": "#3044D6", "accent_fill": "#3044D6", "tint": "#E6E9FF", "jade": "#3FE0C0",
            "jade_text": "#0B7F6D", "jade_graphic": "#14B89A", "warning": "#B45309", "error": "#C2302A"},
    True: {"accent": "#8193FF", "accent_fill": "#4C60F5", "tint": "#262D6B", "jade": "#3FE0C0",
           "jade_text": "#3FE0C0", "jade_graphic": "#3FE0C0", "warning": "#F5A65B", "error": "#FF8A80"},
}

CSS_TEMPLATE = """
@define-color accent_color $accent;
@define-color accent_bg_color $accent_fill;
@define-color accent_fg_color #ffffff;
$root_vars
.viskey-status-card { padding: 16px 18px; }
.viskey-status-title { font-weight: 700; font-size: 1.25em; }
.viskey-tile { background-color: #3044D6; border-radius: 12px; min-width: 48px; min-height: 48px; }
.viskey-tile.off { background-color: alpha(currentColor, 0.25); }
.viskey-tile-label { color: #ffffff; font-weight: 800; font-size: 19px; }
.viskey-tile-dot { background-color: $jade; border-radius: 99px; min-width: 7px; min-height: 7px; margin: 5px; }
.viskey-mono, .viskey-mono-subtitle .subtitle, .viskey-mono-title .title { font-family: monospace; }
.viskey-seg > button { padding-left: 16px; padding-right: 16px; font-weight: 600; }
.viskey-seg > button:checked { background-color: @accent_bg_color; color: @accent_fg_color; }
.viskey-accent-combo box.suffixes label, .viskey-accent-combo label.viskey-combo-value { color: @accent_color; font-weight: 600; }
.viskey-nested > box.header { margin-left: 24px; }
.viskey-link { color: @accent_color; font-weight: 600; }
.viskey-chip { font-family: monospace; font-weight: 600; color: @accent_color; background-color: alpha(@accent_bg_color, 0.14);
               border-radius: 6px; padding: 2px 8px; }
.viskey-pill { border-radius: 99px; padding: 5px 14px; font-weight: 700; }
.viskey-pill-version { background-color: alpha(@accent_bg_color, 0.18); color: @accent_color; }
.viskey-pill-ok { background-color: alpha($jade, 0.16); color: $jade_text; }
.viskey-pill-warn { background-color: alpha($warning, 0.16); color: $warning; }
.viskey-pill-new { background-color: alpha(@accent_bg_color, 0.18); color: @accent_color; }
.viskey-dot { border-radius: 99px; min-width: 8px; min-height: 8px; }
.viskey-dot.ok { background-color: $jade_graphic; }
.viskey-dot.warn { background-color: $warning; }
.viskey-dot.err { background-color: $error; }
.viskey-dot.dim { background-color: alpha(currentColor, 0.3); }
.viskey-warn-text { color: $warning; }
.viskey-err-text { color: $error; }
.viskey-danger-tint { background-color: alpha($error, 0.14); color: $error; font-weight: 700; }
.viskey-danger-tint:hover { background-color: alpha($error, 0.22); }
.viskey-danger-icon:hover { color: $error; }
toast .viskey-toast-check, .viskey-toast-check { color: $jade; }
.viskey-add-card { padding: 12px; }
.viskey-macro-row { padding: 2px 8px 2px 12px; min-height: 40px; }
.viskey-app-name { font-weight: 800; }
"""


def _esc(text):
    return GLib.markup_escape_text(text or "", -1)


def _first_existing(paths):
    for p in paths:
        if p and os.path.exists(p):
            return p
    return None


def symbolic_icon_dir():
    return _first_existing([os.path.join(SCRIPT_DIR, "icons"),
                            os.path.normpath(os.path.join(SCRIPT_DIR, "..", "icons")),
                            "/usr/share/viskey/icons"])


def app_icon_path():
    return _first_existing([
        os.path.normpath(os.path.join(SCRIPT_DIR, "..", "..", "design", "icons", "appicon-1024.svg")),
        os.path.normpath(os.path.join(SCRIPT_DIR, "..", "icons", "appicon-1024.svg")),
        "/usr/share/viskey/icons/appicon-1024.svg",
    ])


if HAVE_GTK:
    # ------------------------------------------------------------------
    # System probes (real + demo)
    # ------------------------------------------------------------------
    class SystemProbe:
        demo = False

        def ibus_running(self):
            return model.ibus_daemon_running()

        def session(self):
            return model.session_type()

        def ibus_version(self):
            return model.ibus_version()

        def hotkey(self):
            candidates = [("org.freedesktop.ibus.general.hotkey", "triggers")]
            if "GNOME" in os.environ.get("XDG_CURRENT_DESKTOP", "").upper():
                candidates.insert(0, ("org.gnome.desktop.wm.keybindings", "switch-input-source"))
            source = Gio.SettingsSchemaSource.get_default()
            for schema_id, key in candidates:
                schema = source.lookup(schema_id, True) if source else None
                if schema is None or not schema.has_key(key):
                    continue
                try:
                    values = Gio.Settings.new(schema_id).get_strv(key)
                except Exception:
                    continue
                for value in values:
                    text = model.format_accelerator(value)
                    if text:
                        return text
            return ""

        def engine_status(self):
            st = model.read_status()
            if st and st.get("engine") == "running" and model.pid_alive(st.get("pid")):
                return st
            return None

    class DemoProbe(SystemProbe):
        demo = True

        def ibus_running(self):
            return True

        def session(self):
            return "Wayland"

        def ibus_version(self):
            return "IBus 1.5.29"

        def hotkey(self):
            return "Ctrl+Space"

        def engine_status(self):
            return {"engine": "running", "pid": 1, "surrounding_text": False}

    # ------------------------------------------------------------------
    # Small compatibility helpers
    # ------------------------------------------------------------------
    def adw_version():
        try:
            return (Adw.get_major_version(), Adw.get_minor_version(), Adw.get_micro_version())
        except AttributeError:
            return (1, 0, 0)

    def load_css(provider, text):
        if hasattr(provider, "load_from_string"):
            provider.load_from_string(text)
        else:
            provider.load_from_data(text.encode("utf-8"), -1)

    class SwitchRowCompat:
        """Adw.SwitchRow (libadwaita 1.4+) or an Adw.ActionRow with a Gtk.Switch suffix.
        With extra_suffix the ActionRow variant is always used so the extra widget sits
        before the switch."""

        def __init__(self, title, subtitle=None, extra_suffix=None):
            if hasattr(Adw, "SwitchRow") and extra_suffix is None:
                self.row = Adw.SwitchRow(title=_esc(title))
                self._switch = self.row
            else:
                self.row = Adw.ActionRow(title=_esc(title))
                if extra_suffix is not None:
                    self.row.add_suffix(extra_suffix)
                self._switch = Gtk.Switch(valign=Gtk.Align.CENTER)
                self.row.add_suffix(self._switch)
                self.row.set_activatable_widget(self._switch)
            if subtitle:
                self.row.set_subtitle(_esc(subtitle))

        def get_active(self):
            return bool(self._switch.get_active())

        def set_active(self, value):
            if self.get_active() != bool(value):
                self._switch.set_active(bool(value))

        def on_changed(self, callback):
            self._switch.connect("notify::active", lambda *_: callback(self.get_active()))

    def make_combo_row(title, items, subtitle=None):
        row = Adw.ComboRow(title=_esc(title))
        row.set_model(Gtk.StringList.new(items))
        if subtitle:
            row.set_subtitle(_esc(subtitle))
        row.add_css_class("viskey-accent-combo")
        # The default factory caps the shown value at 20 chars ("Tự động (Surrounding …");
        # let the inline value use its natural width. No-op if the internals differ.
        row.connect("map", lambda r: GLib.idle_add(_unclamp_combo_value, r))
        row.connect("notify::selected", lambda r, _p: GLib.idle_add(_unclamp_combo_value, r))
        return row

    def _unclamp_combo_value(widget):
        stack = [widget]
        while stack:
            w = stack.pop()
            if isinstance(w, Gtk.Popover):
                continue
            if isinstance(w, Gtk.Label) and w.get_max_width_chars() > 0:
                w.set_max_width_chars(-1)
            child = w.get_first_child()
            while child is not None:
                stack.append(child)
                child = child.get_next_sibling()
        return False

    def icon_button_content(icon_name, label):
        if hasattr(Adw, "ButtonContent"):
            return Adw.ButtonContent(icon_name=icon_name, label=label, use_underline=False)
        box = Gtk.Box(spacing=6)
        box.append(Gtk.Image.new_from_icon_name(icon_name))
        box.append(Gtk.Label(label=label))
        return box

    def make_dot(state="dim"):
        dot = Gtk.Box(valign=Gtk.Align.CENTER, halign=Gtk.Align.CENTER)
        dot.add_css_class("viskey-dot")
        dot.add_css_class(state)
        return dot

    def set_dot_state(dot, state):
        for s in ("ok", "warn", "err", "dim"):
            dot.remove_css_class(s)
        dot.add_css_class(state)

    def copy_to_clipboard(widget, text):
        clipboard = widget.get_clipboard()
        try:
            clipboard.set(text)
            return
        except (AttributeError, TypeError):
            pass
        value = GObject.Value(str, text)
        clipboard.set_content(Gdk.ContentProvider.new_for_value(value))

    def launch_uri(uri, parent=None):
        try:
            Gio.AppInfo.launch_default_for_uri(uri, None)
            return True
        except GLib.Error:
            return False

    # ------------------------------------------------------------------
    # Main window
    # ------------------------------------------------------------------
    class SettingsWindow(Adw.ApplicationWindow):
        def __init__(self, app, store, probe):
            super().__init__(application=app, title="VisKey")
            self.store = store
            self.probe = probe
            self.demo = probe.demo
            self.usage = dict(DEMO_USAGE) if self.demo else model.load_usage()
            self._syncing = False
            self._current_toast = None
            self._chooser = None
            self.sort_mode = "az"
            self.edit_key = None
            self.set_default_size(1020, 780)
            self.set_size_request(360, 480)

            self.stack = Adw.ViewStack(vexpand=True)
            self._add_page(self._build_general(), "general", "Chung", "viskey-sliders-symbolic")
            self._add_page(self._build_macros(), "macros", "Gõ tắt", "viskey-bolt-symbolic")
            self._add_page(self._build_about(), "about", "Giới thiệu", "viskey-info-symbolic")

            self.toast_overlay = Adw.ToastOverlay()
            self.toast_overlay.set_child(self.stack)

            header = Adw.HeaderBar()
            self.switcher = Adw.ViewSwitcher(stack=self.stack, policy=Adw.ViewSwitcherPolicy.WIDE)
            header.set_title_widget(self.switcher)
            menu = Gio.Menu()
            menu.append("Bàn phím tắt", "app.shortcuts")
            menu.append("Giới thiệu VisKey", "app.about")
            menu.append("Thoát", "app.quit")
            menu_btn = Gtk.MenuButton(icon_name="open-menu-symbolic", menu_model=menu,
                                      tooltip_text="Menu chính")
            header.pack_start(menu_btn)

            self.switcher_bar = Adw.ViewSwitcherBar(stack=self.stack)
            if hasattr(Adw, "ToolbarView"):
                view = Adw.ToolbarView()
                view.add_top_bar(header)
                view.set_content(self.toast_overlay)
                view.add_bottom_bar(self.switcher_bar)
                self.set_content(view)
            else:
                box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
                box.append(header)
                box.append(self.toast_overlay)
                box.append(self.switcher_bar)
                self.set_content(box)

            if hasattr(Adw, "Breakpoint") and hasattr(self, "add_breakpoint"):
                bp = Adw.Breakpoint.new(Adw.BreakpointCondition.parse("max-width: 640sp"))
                bp.connect("apply", lambda *_: self._set_narrow(True))
                bp.connect("unapply", lambda *_: self._set_narrow(False))
                self.add_breakpoint(bp)

            self.stack.connect("notify::visible-child-name", self._on_page_changed)
            self.sync_ui()
            self.refresh_status()

        def _add_page(self, child, name, title, icon):
            page = self.stack.add_titled(child, name, title)
            page.set_icon_name(icon)

        def _set_narrow(self, narrow):
            self.switcher.set_visible(not narrow)
            self.switcher_bar.set_reveal(narrow)
            self.about_columns.set_orientation(
                Gtk.Orientation.VERTICAL if narrow else Gtk.Orientation.HORIZONTAL)

        def _scrolled(self, max_width):
            scrolled = Gtk.ScrolledWindow(hscrollbar_policy=Gtk.PolicyType.NEVER, vexpand=True)
            clamp = Adw.Clamp(maximum_size=max_width, tightening_threshold=int(max_width * 0.8))
            box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=24,
                          margin_top=24, margin_bottom=36, margin_start=12, margin_end=12)
            clamp.set_child(box)
            scrolled.set_child(clamp)
            return scrolled, box

        # ------------------------------ Chung ------------------------------
        def _build_general(self):
            scrolled, box = self._scrolled(640)

            card = Gtk.Box(spacing=16)
            card.add_css_class("card")
            card.add_css_class("viskey-status-card")
            tile = Gtk.Overlay(valign=Gtk.Align.CENTER, halign=Gtk.Align.START)
            self.tile_box = Gtk.Box(halign=Gtk.Align.CENTER, valign=Gtk.Align.CENTER)
            self.tile_box.add_css_class("viskey-tile")
            self.tile_label = Gtk.Label(label="VI", halign=Gtk.Align.CENTER, hexpand=False)
            self.tile_label.set_size_request(48, 48)
            self.tile_label.add_css_class("viskey-tile-label")
            self.tile_box.append(self.tile_label)
            tile.set_child(self.tile_box)
            self.tile_dot = Gtk.Box(halign=Gtk.Align.END, valign=Gtk.Align.START)
            self.tile_dot.add_css_class("viskey-tile-dot")
            tile.add_overlay(self.tile_dot)
            card.append(tile)

            texts = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=2,
                            valign=Gtk.Align.CENTER, hexpand=True)
            title = Gtk.Label(label="Tiếng Việt", xalign=0)
            title.add_css_class("viskey-status-title")
            self.status_sub = Gtk.Label(xalign=0, wrap=True)
            self.status_sub.add_css_class("dim-label")
            texts.append(title)
            texts.append(self.status_sub)
            card.append(texts)
            self.master_switch = Gtk.Switch(valign=Gtk.Align.CENTER)
            self.master_switch.connect("notify::active",
                                       lambda *_: self._apply("vietnamese_enabled", self.master_switch.get_active()))
            card.append(self.master_switch)
            box.append(card)

            # Cách gõ
            g = Adw.PreferencesGroup(title="Cách gõ")
            row = Adw.ActionRow(title="Kiểu gõ")
            seg = Gtk.Box(valign=Gtk.Align.CENTER)
            seg.add_css_class("linked")
            seg.add_css_class("viskey-seg")
            self.btn_telex = Gtk.ToggleButton(label="Telex")
            self.btn_vni = Gtk.ToggleButton(label="VNI", group=self.btn_telex)
            for btn, value in ((self.btn_telex, "telex"), (self.btn_vni, "vni")):
                btn.connect("toggled", self._on_method_toggled, value)
                seg.append(btn)
            row.add_suffix(seg)
            g.add(row)

            self.ortho_row = make_combo_row("Vị trí đặt dấu", ["Kiểu cũ", "Kiểu mới"],
                                            "hòa, thúy (kiểu cũ) hoặc hoà, thuý (kiểu mới)")
            self.ortho_row.connect("notify::selected",
                                   lambda r, _p: self._apply("modern_orthography", r.get_selected() == 1))
            g.add(self.ortho_row)

            self.quick_row = SwitchRowCompat("Quick Telex", QUICK_TELEX_HINT)
            self.quick_row.row.add_css_class("viskey-mono-subtitle")
            self.quick_row.on_changed(lambda v: self._apply("quick_telex", v))
            g.add(self.quick_row.row)
            box.append(g)

            # Hiển thị khi gõ
            g = Adw.PreferencesGroup(title="Hiển thị khi gõ",
                                     description="Dành riêng cho IBus trên X11 và Wayland")
            self.underline_row = SwitchRowCompat("Không gạch chân",
                                                 "Gõ thẳng vào ô, không kẹt phím tắt hay rớt chữ khi click chuột")
            self.underline_row.on_changed(lambda v: self._apply("no_underline_mode", v))
            g.add(self.underline_row.row)

            self.method_row = make_combo_row("Phương thức", ["Tự động (Surrounding Text)",
                                                             "Phím xoá lùi giả lập (BackSpace)"])
            self.method_row.add_css_class("viskey-nested")
            self.method_row.connect("notify::selected", lambda r, _p: self._apply(
                "direct_commit_method", METHOD_VALUES[min(r.get_selected(), 1)]))
            g.add(self.method_row)

            self.restore_row = SwitchRowCompat("Tự khôi phục từ tiếng Anh",
                                               "window, linux, refresh không bị biến dạng")
            self.restore_row.on_changed(lambda v: self._apply("auto_restore_english", v))
            g.add(self.restore_row.row)

            link = Gtk.Button(valign=Gtk.Align.CENTER, tooltip_text="Mở danh sách gõ tắt")
            link.add_css_class("flat")
            link.add_css_class("viskey-link")
            link_box = Gtk.Box(spacing=4)
            self.macro_count_label = Gtk.Label()
            link_box.append(self.macro_count_label)
            link_box.append(Gtk.Image.new_from_icon_name("go-next-symbolic"))
            link.set_child(link_box)
            link.connect("clicked", lambda *_: self.stack.set_visible_child_name("macros"))
            self.macros_row = SwitchRowCompat("Bật Gõ tắt", extra_suffix=link)
            self.macros_row.on_changed(lambda v: self._apply("enable_macros", v))
            g.add(self.macros_row.row)
            box.append(g)
            return scrolled

        def _on_method_toggled(self, btn, value):
            if btn.get_active():
                self._apply("input_method", value)

        # ------------------------------ Gõ tắt ------------------------------
        def _build_macros(self):
            scrolled, box = self._scrolled(680)

            g = Adw.PreferencesGroup(title="Gõ tắt", description=CASE_HINT)
            actions = Gtk.Box(spacing=6, valign=Gtk.Align.CENTER)
            btn_import = Gtk.Button(child=icon_button_content("viskey-upload-symbolic", "Nhập"),
                                    tooltip_text="Nhập từ tệp JSON hoặc văn bản “viết tắt<Tab>nội dung”")
            btn_import.connect("clicked", self._on_import)
            btn_export = Gtk.Button(child=icon_button_content("viskey-download-symbolic", "Xuất"),
                                    tooltip_text="Xuất ra tệp JSON")
            btn_export.connect("clicked", self._on_export)
            actions.append(btn_import)
            actions.append(btn_export)
            g.set_header_suffix(actions)
            self.macros_row2 = SwitchRowCompat("Bật Gõ tắt", "Thay cụm viết tắt khi gõ dấu cách hoặc dấu câu")
            self.macros_row2.on_changed(lambda v: self._apply("enable_macros", v))
            g.add(self.macros_row2.row)
            box.append(g)

            # add / edit form
            g = Adw.PreferencesGroup(title="Thêm cụm từ")
            self.add_group = g
            card = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=8)
            card.add_css_class("card")
            card.add_css_class("viskey-add-card")
            line = Gtk.Box(spacing=8)
            self.key_entry = Gtk.Entry(placeholder_text="viết tắt", max_length=model.MACRO_KEY_MAX,
                                       width_chars=9, max_width_chars=12)
            self.key_entry.add_css_class("viskey-mono")
            self.value_entry = Gtk.Entry(placeholder_text="thay bằng", hexpand=True)
            arrow = Gtk.Label(label="→")
            arrow.add_css_class("dim-label")
            self.add_btn = Gtk.Button(child=icon_button_content("list-add-symbolic", "Thêm"))
            self.add_btn.add_css_class("suggested-action")
            self.add_btn.connect("clicked", self._on_add_macro)
            self.cancel_btn = Gtk.Button(label="Huỷ", visible=False)
            self.cancel_btn.connect("clicked", lambda *_: self._end_edit())
            for entry in (self.key_entry, self.value_entry):
                entry.connect("activate", self._on_add_macro)
                entry.connect("changed", lambda *_: self.form_error.set_visible(False))
            for w in (self.key_entry, arrow, self.value_entry, self.cancel_btn, self.add_btn):
                line.append(w)
            card.append(line)
            self.form_error = Gtk.Label(xalign=0, wrap=True, visible=False)
            self.form_error.add_css_class("viskey-err-text")
            card.append(self.form_error)
            g.add(card)
            box.append(g)

            # list
            self.list_group = Adw.PreferencesGroup(title="")
            sort = Gtk.Box(valign=Gtk.Align.CENTER)
            sort.add_css_class("linked")
            self.sort_az = Gtk.ToggleButton(label="A → Z", active=True)
            self.sort_usage = Gtk.ToggleButton(label="Hay dùng", group=self.sort_az)
            self.sort_az.connect("toggled", self._on_sort, "az")
            self.sort_usage.connect("toggled", self._on_sort, "usage")
            sort.append(self.sort_az)
            sort.append(self.sort_usage)
            self.list_group.set_header_suffix(sort)
            self.filter_entry = Gtk.SearchEntry(margin_bottom=12)
            self.filter_entry.connect("search-changed", lambda *_: self.refresh_macro_list())
            self.list_group.add(self.filter_entry)
            self.macro_list = Gtk.ListBox(selection_mode=Gtk.SelectionMode.NONE)
            self.macro_list.add_css_class("boxed-list")
            self.list_group.add(self.macro_list)
            self.list_footer = Gtk.Label(xalign=0, margin_top=10)
            self.list_footer.add_css_class("dim-label")
            self.list_footer.add_css_class("caption")
            self.list_group.add(self.list_footer)
            box.append(self.list_group)
            return scrolled

        def _on_sort(self, btn, mode):
            if btn.get_active():
                self.sort_mode = mode
                self.refresh_macro_list()

        def refresh_macro_list(self):
            macros = self.store.get("macros")
            total = len(macros)
            shown = model.filter_macros(macros, self.filter_entry.get_text())
            items = model.sort_macro_items(shown, self.usage, self.sort_mode)
            child = self.macro_list.get_first_child()
            while child is not None:
                nxt = child.get_next_sibling()
                self.macro_list.remove(child)
                child = nxt
            for key, value in items:
                self.macro_list.append(self._macro_row(key, value))
            if not items:
                text = "Chưa có cụm từ nào" if total == 0 else "Không có cụm từ nào khớp"
                empty = Gtk.Label(label=text, margin_top=18, margin_bottom=18)
                empty.add_css_class("dim-label")
                row = Gtk.ListBoxRow(activatable=False, child=empty)
                self.macro_list.append(row)
            self.list_group.set_title("%d cụm từ" % total)
            self.filter_entry.set_placeholder_text("Lọc %d cụm từ…" % total)
            self.list_footer.set_label("Hiển thị %d / %d cụm từ · Đã lưu tự động" % (len(items), total))

        def _macro_row(self, key, value):
            box = Gtk.Box(spacing=12)
            box.add_css_class("viskey-macro-row")
            chip_holder = Gtk.Box(width_request=96, valign=Gtk.Align.CENTER)
            chip = Gtk.Label(label=key)
            chip.add_css_class("viskey-chip")
            chip_holder.append(chip)
            box.append(chip_holder)
            val = Gtk.Label(label=value, xalign=0, hexpand=True, ellipsize=Pango.EllipsizeMode.END)
            box.append(val)
            count = self.usage.get(key, 0)
            if count:
                used = Gtk.Label(label="Đã dùng %s lần" % model.format_count(count))
                used.add_css_class("dim-label")
                box.append(used)
            edit = Gtk.Button(icon_name="document-edit-symbolic", valign=Gtk.Align.CENTER, tooltip_text="Sửa")
            edit.add_css_class("flat")
            edit.connect("clicked", lambda *_: self._begin_edit(key))
            delete = Gtk.Button(icon_name="user-trash-symbolic", valign=Gtk.Align.CENTER, tooltip_text="Xoá")
            delete.add_css_class("flat")
            delete.add_css_class("viskey-danger-icon")
            delete.connect("clicked", lambda *_: self._delete_macro(key))
            box.append(edit)
            box.append(delete)
            return Gtk.ListBoxRow(activatable=False, child=box)

        def _show_form_error(self, text):
            self.form_error.set_label(text)
            self.form_error.set_visible(True)

        def _begin_edit(self, key):
            macros = self.store.get("macros")
            if key not in macros:
                return
            self.edit_key = key
            self.key_entry.set_text(key)
            self.value_entry.set_text(macros[key])
            self.add_group.set_title("Sửa cụm từ")
            self.add_btn.set_child(icon_button_content("document-save-symbolic", "Lưu"))
            self.cancel_btn.set_visible(True)
            self.value_entry.grab_focus()

        def _end_edit(self):
            self.edit_key = None
            self.key_entry.set_text("")
            self.value_entry.set_text("")
            self.form_error.set_visible(False)
            self.add_group.set_title("Thêm cụm từ")
            self.add_btn.set_child(icon_button_content("list-add-symbolic", "Thêm"))
            self.cancel_btn.set_visible(False)

        def _on_add_macro(self, *_):
            raw_key = self.key_entry.get_text()
            value = self.value_entry.get_text()
            err = model.validate_macro_key(raw_key) or model.validate_macro_value(value)
            if err:
                self._show_form_error(err)
                return
            key = model.normalize_macro_key(raw_key)
            macros = self.store.get("macros")
            if key in macros and key != self.edit_key:
                self._show_form_error("Viết tắt “%s” đã có — bấm Sửa ở dòng đó để thay đổi" % key)
                return
            editing = self.edit_key
            if editing and editing != key:
                macros.pop(editing, None)
            macros[key] = value.strip()
            msg = ("Đã lưu “%s”" if editing else "Đã thêm “%s”") % key
            if self._apply("macros", macros, msg):
                self._end_edit()
                self.key_entry.grab_focus()

        def _delete_macro(self, key):
            macros = self.store.get("macros")
            if macros.pop(key, None) is not None:
                if self.edit_key == key:
                    self._end_edit()
                self._apply("macros", macros, "Đã xoá “%s”" % key)

        # import / export
        def _file_filters(self):
            f = Gtk.FileFilter()
            f.set_name("Danh sách gõ tắt (JSON, TXT)")
            for pattern in ("*.json", "*.txt", "*.tsv"):
                f.add_pattern(pattern)
            return f

        def _choose_file(self, save, callback):
            title = "Xuất danh sách gõ tắt" if save else "Nhập danh sách gõ tắt"
            if hasattr(Gtk, "FileDialog"):
                dialog = Gtk.FileDialog(title=title, modal=True)
                filters = Gio.ListStore.new(Gtk.FileFilter)
                filters.append(self._file_filters())
                dialog.set_filters(filters)

                def done(d, result):
                    try:
                        gfile = d.save_finish(result) if save else d.open_finish(result)
                    except GLib.Error:
                        return  # cancelled
                    if gfile is not None and gfile.get_path():
                        callback(gfile.get_path())

                if save:
                    dialog.set_initial_name("viskey-go-tat.json")
                    dialog.save(self, None, done)
                else:
                    dialog.open(self, None, done)
                return
            action = Gtk.FileChooserAction.SAVE if save else Gtk.FileChooserAction.OPEN
            chooser = Gtk.FileChooserNative.new(title, self, action, "Lưu" if save else "Mở", "Huỷ")
            chooser.add_filter(self._file_filters())
            if save:
                chooser.set_current_name("viskey-go-tat.json")

            def on_response(c, response):
                if response == Gtk.ResponseType.ACCEPT:
                    gfile = c.get_file()
                    if gfile is not None and gfile.get_path():
                        callback(gfile.get_path())
                self._chooser = None

            chooser.connect("response", on_response)
            self._chooser = chooser
            chooser.show()

        def _on_import(self, *_):
            self._choose_file(False, self._import_path)

        def _import_path(self, path):
            try:
                with open(path, "r", encoding="utf-8") as f:
                    text = f.read()
            except (OSError, UnicodeDecodeError):
                self._toast("Không đọc được tệp đã chọn")
                return
            self.import_text(text)

        def import_text(self, text):
            imported, errors = model.parse_macro_import(text)
            if errors:
                more = "" if len(errors) <= 3 else "\n… và %d lỗi khác" % (len(errors) - 3)
                self._show_form_error("\n".join(errors[:3]) + more)
            if not imported:
                self._toast("Không nhập được cụm từ nào")
                return
            macros = self.store.get("macros")
            macros.update(imported)
            msg = "Đã nhập %d cụm từ" % len(imported)
            if errors:
                msg += ", bỏ qua %d mục lỗi" % len(errors)
            self._apply("macros", macros, msg)

        def _on_export(self, *_):
            self._choose_file(True, self._export_path)

        def _export_path(self, path):
            macros = self.store.get("macros")
            try:
                with open(path, "w", encoding="utf-8") as f:
                    f.write(model.export_macros_json(macros))
            except OSError:
                self._toast("Không ghi được tệp đã chọn")
                return
            self._toast("Đã xuất %d cụm từ" % len(macros))

        # ------------------------------ Giới thiệu ------------------------------
        def _build_about(self):
            scrolled, box = self._scrolled(820)
            box.set_spacing(10)

            icon = None
            path = app_icon_path()
            if path:
                try:
                    texture = Gdk.Texture.new_from_filename(path)
                    icon = Gtk.Image.new_from_paintable(texture)
                except (GLib.Error, AttributeError, TypeError):
                    icon = None
            if icon is None:
                icon = Gtk.Image.new_from_icon_name("input-keyboard")
            icon.set_pixel_size(128)
            box.append(icon)
            name = Gtk.Label(label="VisKey")
            name.add_css_class("title-1")
            name.add_css_class("viskey-app-name")
            box.append(name)
            tag = Gtk.Label(label=TAGLINE, wrap=True, justify=Gtk.Justification.CENTER)
            tag.add_css_class("dim-label")
            box.append(tag)

            badges = Gtk.Box(spacing=8, halign=Gtk.Align.CENTER, margin_top=6, margin_bottom=18)
            ver = Gtk.Label(label=VERSION, valign=Gtk.Align.CENTER)
            ver.add_css_class("viskey-pill")
            ver.add_css_class("viskey-pill-version")
            badges.append(ver)
            self.update_btn = Gtk.Button(label="Kiểm tra cập nhật", valign=Gtk.Align.CENTER)
            self.update_btn.add_css_class("pill")
            self.update_btn.add_css_class("viskey-pill")
            self.update_btn.connect("clicked", self._on_check_update)
            badges.append(self.update_btn)
            self.update_spinner = Adw.Spinner() if hasattr(Adw, "Spinner") else Gtk.Spinner(spinning=True)
            self.update_spinner.set_visible(False)
            badges.append(self.update_spinner)
            self.update_pill = Gtk.Button(valign=Gtk.Align.CENTER, visible=False)
            self.update_pill.add_css_class("viskey-pill")
            self.update_pill.connect("clicked", self._on_update_pill)
            badges.append(self.update_pill)
            self.update_retry = Gtk.Button(icon_name="view-refresh-symbolic", valign=Gtk.Align.CENTER,
                                           visible=False, tooltip_text="Kiểm tra lại")
            self.update_retry.add_css_class("flat")
            self.update_retry.add_css_class("circular")
            self.update_retry.connect("clicked", self._on_check_update)
            badges.append(self.update_retry)
            self._update_url = None
            box.append(badges)

            self.about_columns = Gtk.Box(spacing=24, homogeneous=True)
            links = Adw.PreferencesGroup(title="Liên kết", hexpand=True)
            links.add(self._link_row("viskey-code-symbolic", "Mã nguồn", model.REPO_URL))
            links.add(self._link_row("viskey-alert-symbolic", "Báo lỗi", model.ISSUES_URL))
            links.add(self._link_row("viskey-download-symbolic", "Bản phát hành & thay đổi", model.RELEASES_PAGE_URL))
            links.add(self._link_row("viskey-document-symbolic", "Giấy phép", model.LICENSE_URL, suffix_text="MIT"))
            self.about_columns.append(links)

            diag = Adw.PreferencesGroup(title="Chẩn đoán", hexpand=True)
            self.diag_engine = self._diag_row(diag, "Engine IBus")
            self.diag_session = self._diag_row(diag, "Phiên hiển thị")
            self.diag_surrounding = self._diag_row(diag, "Surrounding Text")
            row = Adw.ActionRow(title=_esc(model.CONFIG_PATH_DISPLAY))
            row.add_css_class("viskey-mono-title")
            row.add_prefix(Gtk.Image.new_from_icon_name("folder-symbolic"))
            open_btn = Gtk.Button(label="Mở", valign=Gtk.Align.CENTER, tooltip_text="Mở tệp cấu hình")
            open_btn.add_css_class("flat")
            open_btn.add_css_class("viskey-link")
            open_btn.connect("clicked", self._on_open_config)
            row.add_suffix(open_btn)
            diag.add(row)
            self.about_columns.append(diag)
            box.append(self.about_columns)

            buttons = Gtk.Box(spacing=12, margin_top=14)
            copy_btn = Gtk.Button(child=icon_button_content("edit-copy-symbolic", "Sao chép thông tin hệ thống"))
            copy_btn.connect("clicked", self._on_copy_info)
            buttons.append(copy_btn)
            buttons.append(Gtk.Box(hexpand=True))
            reset_btn = Gtk.Button(label="Khôi phục toàn bộ cài đặt…")
            reset_btn.add_css_class("viskey-danger-tint")
            reset_btn.connect("clicked", self._on_reset)
            buttons.append(reset_btn)
            box.append(buttons)
            return scrolled

        def _link_row(self, icon, title, url, suffix_text=None):
            row = Adw.ActionRow(title=_esc(title), activatable=True, tooltip_text=url)
            row.add_prefix(Gtk.Image.new_from_icon_name(icon))
            if suffix_text:
                lbl = Gtk.Label(label=suffix_text)
                lbl.add_css_class("dim-label")
                row.add_suffix(lbl)
            else:
                row.add_suffix(Gtk.Image.new_from_icon_name("viskey-arrow-symbolic"))
            row.connect("activated", lambda *_: self._open_uri(url))
            return row

        def _diag_row(self, group, title):
            row = Adw.ActionRow(title=_esc(title))
            dot = make_dot()
            row.add_prefix(dot)
            value = Gtk.Label(xalign=1, ellipsize=Pango.EllipsizeMode.START)
            row.add_suffix(value)
            group.add(row)
            return dot, value

        def _set_diag(self, pair, state, text, mono=True):
            dot, label = pair
            set_dot_state(dot, state)
            label.set_label(text)
            for cls in ("viskey-mono", "dim-label", "viskey-warn-text", "viskey-err-text"):
                label.remove_css_class(cls)
            if state == "warn":
                label.add_css_class("viskey-warn-text")
            elif state == "err":
                label.add_css_class("viskey-err-text")
            else:
                label.add_css_class("dim-label")
                if mono:
                    label.add_css_class("viskey-mono")

        def _open_uri(self, uri):
            if self.demo:
                return
            if not launch_uri(uri, self):
                self._toast("Không mở được liên kết")

        def _on_open_config(self, *_):
            if self.demo:
                return
            target = model.CONFIG_PATH
            if not os.path.exists(target):
                try:
                    self.store.save()
                except OSError:
                    target = os.path.dirname(target)
            if not os.path.exists(target):
                self._toast("Chưa có tệp cấu hình")
                return
            self._open_uri(Gio.File.new_for_path(target).get_uri())

        def _on_copy_info(self, *_):
            info = {
                "viskey": VERSION,
                "distro": model.os_pretty_name() or platform.platform(),
                "desktop": os.environ.get("XDG_CURRENT_DESKTOP", ""),
                "session": self.probe.session(),
                "ibus": self.probe.ibus_version(),
                "gtk": "%d.%d.%d" % (Gtk.get_major_version(), Gtk.get_minor_version(), Gtk.get_micro_version()),
                "adwaita": "%d.%d.%d" % adw_version(),
                "python": platform.python_version(),
            }
            copy_to_clipboard(self, model.format_system_info(info))
            self._toast("Đã sao chép thông tin hệ thống")

        def _on_reset(self, *_):
            self.confirm("Khôi phục toàn bộ cài đặt?",
                         "Mọi tuỳ chọn và danh sách gõ tắt sẽ trở về mặc định. "
                         "Bạn có thể hoàn tác ngay sau đó.",
                         "Khôi phục", self._do_reset)

        def _do_reset(self):
            old = self.store.snapshot()
            self.store.replace(model.default_config())
            if not self._save():
                self.store.replace(old)
                return
            self._end_edit()
            self.sync_ui()
            self._toast("Đã khôi phục cài đặt mặc định", undo=lambda: self._restore_all(old))

        def _restore_all(self, cfg):
            self.store.replace(cfg)
            if self._save():
                self.sync_ui()
                self._toast("Đã hoàn tác")

        def confirm(self, heading, body, accept_label, on_accept):
            def handle(response):
                if response == "accept":
                    on_accept()

            if hasattr(Adw, "AlertDialog"):
                d = Adw.AlertDialog(heading=heading, body=body)
            elif hasattr(Adw, "MessageDialog"):
                d = Adw.MessageDialog(transient_for=self, modal=True, heading=heading, body=body)
            else:
                d = Gtk.MessageDialog(transient_for=self, modal=True, message_type=Gtk.MessageType.WARNING,
                                      buttons=Gtk.ButtonsType.NONE, text=heading, secondary_text=body)
                d.add_button("Huỷ", Gtk.ResponseType.CANCEL)
                d.add_button(accept_label, Gtk.ResponseType.ACCEPT).add_css_class("destructive-action")

                def on_gtk_response(dlg, response):
                    dlg.destroy()
                    handle("accept" if response == Gtk.ResponseType.ACCEPT else "cancel")

                d.connect("response", on_gtk_response)
                d.show()
                return
            d.add_response("cancel", "Huỷ")
            d.add_response("accept", accept_label)
            d.set_response_appearance("accept", Adw.ResponseAppearance.DESTRUCTIVE)
            d.set_default_response("cancel")
            d.set_close_response("cancel")
            d.connect("response", lambda _d, r: handle(r))
            if isinstance(d, Adw.MessageDialog):
                d.present()
            else:
                d.present(self)

        # update check (manual only)
        def _on_check_update(self, *_):
            if self.demo:
                return
            self.update_btn.set_visible(False)
            self.update_pill.set_visible(False)
            self.update_retry.set_visible(False)
            self.update_spinner.set_visible(True)
            if isinstance(self.update_spinner, Gtk.Spinner):
                self.update_spinner.start()

            def worker():
                result = model.check_for_update(VERSION)
                GLib.idle_add(self.show_update_result, result)

            threading.Thread(target=worker, daemon=True).start()

        def show_update_result(self, result):
            self.update_spinner.set_visible(False)
            self.update_btn.set_visible(False)
            for cls in ("viskey-pill-ok", "viskey-pill-warn", "viskey-pill-new"):
                self.update_pill.remove_css_class(cls)
            self._update_url = None
            if result.status == "latest":
                content = icon_button_content("viskey-check-symbolic", "Đã là phiên bản mới nhất")
                self.update_pill.add_css_class("viskey-pill-ok")
                self.update_pill.set_tooltip_text(None)
            elif result.status == "newer":
                content = icon_button_content("viskey-download-symbolic", "Có bản mới %s" % result.version)
                self.update_pill.add_css_class("viskey-pill-new")
                self._update_url = result.url or model.RELEASES_PAGE_URL
                self.update_pill.set_tooltip_text(self._update_url)
            else:
                content = icon_button_content("dialog-warning-symbolic", "Không kiểm tra được: %s" % result.message)
                self.update_pill.add_css_class("viskey-pill-warn")
                self.update_pill.set_tooltip_text(None)
            self.update_pill.set_child(content)
            self.update_pill.set_visible(True)
            self.update_retry.set_visible(True)
            return False

        def _on_update_pill(self, *_):
            if self._update_url:
                self._open_uri(self._update_url)

        # ------------------------------ shared ------------------------------
        def _on_page_changed(self, *_):
            name = self.stack.get_visible_child_name()
            if name == "macros" and not self.demo:
                self.usage = model.load_usage()
                self.refresh_macro_list()
            if name in ("general", "about"):
                self.refresh_status()

        def refresh_status(self):
            running = self.probe.ibus_running()
            session = self.probe.session()
            hotkey = self.probe.hotkey()
            parts = [_esc("IBus đang chạy" if running else "IBus chưa chạy")]
            if session:
                parts.append(_esc(session))
            if hotkey:
                parts.append('<span font_family="monospace">%s</span>' % _esc(hotkey))
            self.status_sub.set_markup("  ·  ".join(parts))

            st = self.probe.engine_status()
            if st:
                self._set_diag(self.diag_engine, "ok", "viskey · đang chạy")
            elif running:
                self._set_diag(self.diag_engine, "warn", "IBus đang chạy, chưa chọn VisKey", mono=False)
            else:
                self._set_diag(self.diag_engine, "err", "IBus chưa chạy", mono=False)
            if session:
                self._set_diag(self.diag_session, "ok", session)
            else:
                self._set_diag(self.diag_session, "warn", "Không rõ", mono=False)
            if st is None or "surrounding_text" not in st:
                self._set_diag(self.diag_surrounding, "dim", "Chưa có dữ liệu", mono=False)
            elif st.get("surrounding_text"):
                self._set_diag(self.diag_surrounding, "ok", "Ứng dụng này có hỗ trợ", mono=False)
            else:
                self._set_diag(self.diag_surrounding, "warn", "Ứng dụng này không hỗ trợ", mono=False)

        def sync_ui(self):
            cfg = self.store
            self._syncing = True
            try:
                enabled = bool(cfg.get("vietnamese_enabled"))
                self.master_switch.set_active(enabled)
                self.tile_label.set_label("VI" if enabled else "EN")
                self.tile_dot.set_visible(enabled)
                if enabled:
                    self.tile_box.remove_css_class("off")
                else:
                    self.tile_box.add_css_class("off")
                im = cfg.get("input_method")
                (self.btn_vni if im == "vni" else self.btn_telex).set_active(True)
                self.ortho_row.set_selected(1 if cfg.get("modern_orthography") else 0)
                self.quick_row.set_active(cfg.get("quick_telex"))
                no_ul = bool(cfg.get("no_underline_mode"))
                self.underline_row.set_active(no_ul)
                method = cfg.get("direct_commit_method")
                self.method_row.set_selected(METHOD_VALUES.index(method) if method in METHOD_VALUES else 0)
                self.method_row.set_sensitive(no_ul)
                self.restore_row.set_active(cfg.get("auto_restore_english"))
                self.macros_row.set_active(cfg.get("enable_macros"))
                self.macros_row2.set_active(cfg.get("enable_macros"))
                self.macro_count_label.set_label("%d cụm từ" % len(cfg.get("macros")))
                self.refresh_macro_list()
            finally:
                self._syncing = False

        def _save(self):
            try:
                self.store.save()
                return True
            except OSError as e:
                self._toast("Không lưu được cài đặt: %s" % (e.strerror or e))
                return False

        def _apply(self, key, value, message=TOAST_APPLIED):
            """Store + save one setting immediately and offer "Hoàn tác"."""
            if self._syncing:
                return False
            old = self.store.get(key)
            if old == value:
                return False
            self.store.set(key, value)
            if not self._save():
                self.store.set(key, old)
                self.sync_ui()
                return False
            self.sync_ui()
            self._toast(message, undo=lambda: self._undo(key, old))
            return True

        def _undo(self, key, old):
            self.store.set(key, old)
            if self._save():
                self.sync_ui()
                self._toast("Đã hoàn tác")

        def _toast(self, message, undo=None, timeout=4):
            if self._current_toast is not None:
                try:
                    self._current_toast.dismiss()
                except Exception:
                    pass
            toast = Adw.Toast.new(_esc(message))
            if hasattr(toast, "set_custom_title"):
                row = Gtk.Box(spacing=10)
                check = Gtk.Image.new_from_icon_name("viskey-check-symbolic")
                check.add_css_class("viskey-toast-check")
                row.append(check)
                row.append(Gtk.Label(label=message))
                toast.set_custom_title(row)
            toast.set_timeout(timeout)
            if undo is not None:
                toast.set_button_label("Hoàn tác")
                toast.connect("button-clicked", lambda *_: undo())
            self._current_toast = toast
            self.toast_overlay.add_toast(toast)
            return toast

        def dismiss_toast(self):
            if self._current_toast is not None:
                self._current_toast.dismiss()
                self._current_toast = None

        def focus_filter(self):
            self.stack.set_visible_child_name("macros")
            self.filter_entry.grab_focus()

    # ------------------------------------------------------------------
    # Application
    # ------------------------------------------------------------------
    class VisKeySettingsApp(Adw.Application):
        def __init__(self, snapshot_dir=None):
            flags = getattr(Gio.ApplicationFlags, "DEFAULT_FLAGS", Gio.ApplicationFlags.FLAGS_NONE)
            if snapshot_dir:
                flags |= Gio.ApplicationFlags.NON_UNIQUE
            super().__init__(application_id=APP_ID, flags=flags)
            self.snapshot_dir = snapshot_dir
            self.window = None
            self.exit_code = 0
            self._css = None
            self.connect("startup", self._on_startup)
            self.connect("activate", self._on_activate)

        def _on_startup(self, *_):
            display = Gdk.Display.get_default()
            icons = symbolic_icon_dir()
            if icons:
                Gtk.IconTheme.get_for_display(display).add_search_path(icons)
            Gtk.Window.set_default_icon_name(APP_ID)
            style = Adw.StyleManager.get_default()
            self._css = Gtk.CssProvider()
            Gtk.StyleContext.add_provider_for_display(display, self._css, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)
            style.connect("notify::dark", lambda *_: self._load_brand_css())
            self._load_brand_css()

            def action(name, callback, accels=(), param=None):
                act = Gio.SimpleAction.new(name, param)
                act.connect("activate", callback)
                self.add_action(act)
                if accels:
                    self.set_accels_for_action("app." + name, list(accels))

            action("quit", lambda *_: self.quit(), ["<Control>q"])
            action("shortcuts", lambda *_: self.show_shortcuts(), ["<Control>question"])
            action("about", lambda *_: self.show_about())
            action("find", lambda *_: self.window and self.window.focus_filter(), ["<Control>f"])
            action("page", self._on_page_action, param=GLib.VariantType.new("s"))
            for i, name in enumerate(("general", "macros", "about"), start=1):
                self.set_accels_for_action("app.page::%s" % name, ["<Alt>%d" % i])
            self.set_accels_for_action("window.close", ["<Control>w"])

        def _load_brand_css(self):
            from string import Template
            dark = Adw.StyleManager.get_default().get_dark()
            values = dict(BRAND[dark])
            # libadwaita 1.6+ reads accent colours from CSS variables.
            values["root_vars"] = (":root { --accent-color: %s; --accent-bg-color: %s; }"
                                   % (values["accent"], values["accent_fill"])) if adw_version() >= (1, 6, 0) else ""
            load_css(self._css, Template(CSS_TEMPLATE).substitute(values))

        def _on_page_action(self, _act, param):
            if self.window:
                self.window.stack.set_visible_child_name(param.get_string())

        def _on_activate(self, *_):
            if self.window is not None:
                self.window.present()
                return
            if self.snapshot_dir:
                initial = model.default_config()
                initial["macros"] = dict(DEMO_MACROS)
                store = model.ConfigStore(path=None, initial=initial)
                probe = DemoProbe()
            else:
                store = model.ConfigStore()
                probe = SystemProbe()
            self.window = SettingsWindow(self, store, probe)
            self.window.present()
            if self.snapshot_dir:
                SnapshotRunner(self, self.window, self.snapshot_dir).start()

        def show_about(self):
            win = self.window
            theme = Gtk.IconTheme.get_for_display(Gdk.Display.get_default())
            icon = APP_ID if theme.has_icon(APP_ID) else "input-keyboard"
            props = dict(application_name="VisKey", application_icon=icon, developer_name="1VISION",
                         version=VERSION, website=model.REPO_URL, issue_url=model.ISSUES_URL,
                         license_type=Gtk.License.MIT_X11, comments=TAGLINE, copyright="© 1VISION")
            if hasattr(Adw, "AboutDialog"):
                Adw.AboutDialog(**props).present(win)
            elif hasattr(Adw, "AboutWindow"):
                Adw.AboutWindow(transient_for=win, **props).present()
            elif win is not None:
                win.stack.set_visible_child_name("about")

        def show_shortcuts(self):
            win = Adw.Window(transient_for=self.window, modal=True, title="Bàn phím tắt",
                             default_width=420, resizable=False)
            box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
            header = Adw.HeaderBar()
            header.set_title_widget(Adw.WindowTitle(title="Bàn phím tắt"))
            box.append(header)
            group = Adw.PreferencesGroup(margin_top=12, margin_bottom=18, margin_start=18, margin_end=18)
            for title, keys in (("Thẻ Chung / Gõ tắt / Giới thiệu", "Alt+1 / 2 / 3"),
                                ("Lọc cụm từ gõ tắt", "Ctrl+F"),
                                ("Bàn phím tắt", "Ctrl+?"),
                                ("Đóng cửa sổ", "Ctrl+W"),
                                ("Thoát", "Ctrl+Q")):
                row = Adw.ActionRow(title=_esc(title))
                lbl = Gtk.Label(label=keys)
                lbl.add_css_class("viskey-mono")
                lbl.add_css_class("dim-label")
                row.add_suffix(lbl)
                group.add(row)
            box.append(group)
            win.set_content(box)
            win.present()

    # ------------------------------------------------------------------
    # --ui-snapshot
    # ------------------------------------------------------------------
    def render_widget_png(widget, path, scale=2.0):
        """Render a realized widget offscreen through its GSK renderer and save a PNG."""
        width, height = widget.get_width(), widget.get_height()
        native = widget.get_native()
        if width <= 0 or height <= 0 or native is None:
            return False
        renderer = native.get_renderer()
        if renderer is None:
            return False
        paintable = Gtk.WidgetPaintable.new(widget)
        snapshot = Gtk.Snapshot.new()
        snapshot.scale(scale, scale)
        paintable.snapshot(snapshot, width, height)
        node = snapshot.to_node()
        if node is None:
            return False
        rect = Graphene.Rect().init(0, 0, width * scale, height * scale)
        texture = renderer.render_texture(node, rect)
        return bool(texture.save_to_png(path))

    class SnapshotRunner:
        PAGES = (("general", "linux-general"), ("macros", "linux-macros"), ("about", "linux-about"))

        def __init__(self, app, window, outdir):
            self.app = app
            self.window = window
            self.outdir = outdir
            self.steps = []
            for scheme_name, scheme in (("light", Adw.ColorScheme.FORCE_LIGHT), ("dark", Adw.ColorScheme.FORCE_DARK)):
                for page, base in self.PAGES:
                    self.steps.append((scheme, page, os.path.join(outdir, "%s-%s.png" % (base, scheme_name))))
            self.index = 0
            self.failed = []

        def start(self):
            os.makedirs(self.outdir, exist_ok=True)
            self.app.hold()
            self.window.show_update_result(model.UpdateResult("latest", version=VERSION))
            GLib.timeout_add_seconds(90, self._watchdog)
            GLib.timeout_add(800, self._next)

        def _watchdog(self):
            print("viskey-settings: snapshot timed out", file=sys.stderr)
            self.app.exit_code = 1
            self.app.release()
            self.app.quit()
            return False

        def _next(self):
            if self.index >= len(self.steps):
                self._finish()
                return False
            scheme, page, _path = self.steps[self.index]
            Adw.StyleManager.get_default().set_color_scheme(scheme)
            self.window.stack.set_visible_child_name(page)
            if page == "general":
                self.window._toast(TOAST_APPLIED, undo=lambda: None, timeout=0)
            else:
                self.window.dismiss_toast()
            GLib.timeout_add(700, self._capture)
            return False

        def _capture(self):
            _scheme, _page, path = self.steps[self.index]
            try:
                ok = render_widget_png(self.window, path)
            except Exception as e:  # keep going; report at the end
                print("viskey-settings: render failed for %s: %s" % (path, e), file=sys.stderr)
                ok = False
            if ok:
                print("wrote %s" % path)
            else:
                self.failed.append(path)
            self.index += 1
            GLib.timeout_add(50, self._next)
            return False

        def _finish(self):
            if self.failed:
                # Fallback: keep the window on screen so the caller can take a screenshot.
                print("viskey-settings: offscreen rendering unavailable; window stays open for 20 s "
                      "for an external screenshot", file=sys.stderr)
                GLib.timeout_add_seconds(20, self._quit)
            else:
                self._quit()

        def _quit(self):
            self.app.release()
            self.app.quit()
            return False


def _parse_args(argv):
    snapshot_dir = None
    rest = []
    i = 0
    while i < len(argv):
        arg = argv[i]
        if arg == "--ui-snapshot":
            if i + 1 >= len(argv):
                raise SystemExit("--ui-snapshot cần một thư mục đầu ra")
            snapshot_dir = os.path.abspath(argv[i + 1])
            i += 2
            continue
        if arg.startswith("--ui-snapshot="):
            snapshot_dir = os.path.abspath(arg.split("=", 1)[1])
        else:
            rest.append(arg)
        i += 1
    return snapshot_dir, rest


def main(argv=None):
    argv = list(sys.argv if argv is None else argv)
    snapshot_dir, rest = _parse_args(argv[1:])
    if not HAVE_GTK:
        if snapshot_dir:
            print("viskey-settings: --ui-snapshot cần GTK 4 và libadwaita (gir1.2-gtk-4.0, gir1.2-adw-1)",
                  file=sys.stderr)
            return 2
        try:
            import viskey_settings_tk
        except ImportError:
            print("viskey-settings: cần GTK 4 + libadwaita (gir1.2-adw-1) hoặc python3-tk", file=sys.stderr)
            return 1
        return viskey_settings_tk.main()
    app = VisKeySettingsApp(snapshot_dir=snapshot_dir)
    status = app.run([argv[0]] + rest)
    return app.exit_code or status


if __name__ == "__main__":
    sys.exit(main())
