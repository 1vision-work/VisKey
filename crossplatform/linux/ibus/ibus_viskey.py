#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
VisKey Linux IBus Engine
Supports two seamless commit strategies to solve the classic Linux "Preedit Underline" bug:
1. Direct / Surrounding-Text & Fake-Backspace Mode (No Underline / Không gạch chân)
2. Wayland-Safe Preedit Mode (Auto-commits cleanly on focus out / mouse click / shortcut)
"""
import os
import sys
import time

# Ensure core module is importable
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
CORE_DIR = os.path.abspath(os.path.join(SCRIPT_DIR, "..", "..", "core"))
if os.path.isdir(CORE_DIR):
    sys.path.insert(0, CORE_DIR)
elif os.path.isdir("/usr/share/viskey/core"):
    sys.path.insert(0, "/usr/share/viskey/core")

if SCRIPT_DIR not in sys.path:
    sys.path.insert(0, SCRIPT_DIR)

from viskey_engine import EngineConfig, VisKeyEngine
from viskey_settings import config_path, load_config, save_config

# Minimum interval (seconds) between config.json mtime checks on key events
CONFIG_CHECK_INTERVAL = 1.0


def config_mtime(path):
    """Cheap change token for config.json (None if missing). Includes inode/size since
    viskey-settings replaces the file atomically and mtime granularity may be coarse."""
    try:
        st = os.stat(path)
    except OSError:
        return None
    return (st.st_mtime_ns, st.st_ino, st.st_size)


def apply_config(config: EngineConfig, data: dict) -> None:
    """Copy normalized settings (see viskey_settings.load_config) onto an EngineConfig in place."""
    config.input_method = data["input_method"]
    config.modern_orthography = data["modern_orthography"]
    config.auto_restore_english = data["auto_restore_english"]
    config.quick_telex = data["quick_telex"]
    config.enable_macros = data["enable_macros"]
    config.fix_autocomplete = data["fix_autocomplete"]
    config.macros = dict(data["macros"])

try:
    import gi
    gi.require_version("IBus", "1.0")
    from gi.repository import GLib, GObject, IBus
except ImportError:
    IBus = None


if IBus is not None:
    class VisKeyIBusEngine(IBus.Engine):
        __gtype_name__ = "VisKeyIBusEngine"

        def __init__(self):
            super().__init__()
            self.config = EngineConfig()
            self.engine = VisKeyEngine(self.config)
            self.vietnamese_enabled = True
            # Default to No-Underline (Surrounding Text / Fake Backspace) on X11;
            # on strict Wayland without Surrounding Text, seamlessly fall back to Preedit if needed.
            self.no_underline_mode = True
            self.is_wayland = "WAYLAND_DISPLAY" in os.environ
            self.capabilities = 0
            self.config_path = config_path()
            self.config_mtime = None
            self.config_checked_at = 0.0
            self._load_config()
            self.prop_list = self._build_properties()

        def _build_properties(self) -> IBus.PropList:
            props = IBus.PropList()

            self.prop_mode = IBus.Property(
                key="InputMode",
                prop_type=IBus.PropType.TOGGLE,
                label=IBus.Text.new_from_string("Tiếng Việt (VI)"),
                icon="input-keyboard",
                tooltip=IBus.Text.new_from_string("Bật/Tắt gõ Tiếng Việt"),
                sensitive=True,
                visible=True,
                state=IBus.PropState.CHECKED,
            )
            props.append(self.prop_mode)

            self.prop_telex = IBus.Property(
                key="MethodTelex",
                prop_type=IBus.PropType.TOGGLE,
                label=IBus.Text.new_from_string(self._method_label()),
                tooltip=IBus.Text.new_from_string("Nhấn để chuyển giữa Telex và VNI"),
                sensitive=True,
                visible=True,
                state=self._prop_state(self.config.input_method == "telex"),
            )
            props.append(self.prop_telex)

            self.prop_underline = IBus.Property(
                key="NoUnderline",
                prop_type=IBus.PropType.TOGGLE,
                label=IBus.Text.new_from_string("Chế độ Không gạch chân (No Preedit)"),
                tooltip=IBus.Text.new_from_string("Tắt gạch chân khi gõ (khuyên dùng)"),
                sensitive=True,
                visible=True,
                state=self._prop_state(self.no_underline_mode),
            )
            props.append(self.prop_underline)

            self.prop_restore = IBus.Property(
                key="AutoRestore",
                prop_type=IBus.PropType.TOGGLE,
                label=IBus.Text.new_from_string("Tự khôi phục từ tiếng Anh"),
                tooltip=IBus.Text.new_from_string("Tự trả về từ tiếng Anh khi gõ window, software, book..."),
                sensitive=True,
                visible=True,
                state=self._prop_state(self.config.auto_restore_english),
            )
            props.append(self.prop_restore)

            self.prop_ortho = IBus.Property(
                key="ModernOrtho",
                prop_type=IBus.PropType.TOGGLE,
                label=IBus.Text.new_from_string("Đặt dấu kiểu mới (hoà, thuý)"),
                tooltip=IBus.Text.new_from_string("Bật: hoà, thuý / Tắt: hòa, thúy"),
                sensitive=True,
                visible=True,
                state=self._prop_state(self.config.modern_orthography),
            )
            props.append(self.prop_ortho)

            return props

        @staticmethod
        def _prop_state(checked: bool) -> int:
            return IBus.PropState.CHECKED if checked else IBus.PropState.UNCHECKED

        def _method_label(self) -> str:
            return f"Đổi kiểu gõ: Telex / VNI (Đang: {self.config.input_method.upper()})"

        def _sync_properties(self):
            self.prop_telex.set_label(IBus.Text.new_from_string(self._method_label()))
            for prop, checked in (
                (self.prop_telex, self.config.input_method == "telex"),
                (self.prop_underline, self.no_underline_mode),
                (self.prop_restore, self.config.auto_restore_english),
                (self.prop_ortho, self.config.modern_orthography),
            ):
                prop.set_state(self._prop_state(checked))
                self.update_property(prop)

        # ---- config.json (shared with viskey-settings) ----
        def _load_config(self):
            self.config_mtime = config_mtime(self.config_path)
            data = load_config(self.config_path)
            apply_config(self.config, data)
            self.no_underline_mode = bool(data["no_underline_mode"])

        def _maybe_reload_config(self, force: bool = False):
            now = time.monotonic()
            if not force and now - self.config_checked_at < CONFIG_CHECK_INTERVAL:
                return
            self.config_checked_at = now
            if config_mtime(self.config_path) == self.config_mtime:
                return
            self._commit_preedit_if_any()
            self.engine.reset()
            self._load_config()
            self._sync_properties()

        def _save_config(self):
            data = load_config(self.config_path)
            data.update(
                input_method=self.config.input_method,
                auto_restore_english=self.config.auto_restore_english,
                modern_orthography=self.config.modern_orthography,
                no_underline_mode=self.no_underline_mode,
            )
            try:
                save_config(data, self.config_path)
            except OSError as exc:
                print(f"VisKey: cannot save {self.config_path}: {exc}", file=sys.stderr)
            self.config_mtime = config_mtime(self.config_path)

        def do_set_capabilities(self, caps: int):
            self.capabilities = caps

        def do_focus_in(self):
            self._maybe_reload_config(force=True)
            self.register_properties(self.prop_list)
            self.request_surrounding_text()

        def do_focus_out(self):
            self._commit_preedit_if_any()
            self.engine.reset()

        def do_reset(self):
            self._commit_preedit_if_any()
            self.engine.reset()

        def _commit_preedit_if_any(self):
            if not self.no_underline_mode:
                word = self.engine.get_displayed_word()
                if word:
                    self.commit_text(IBus.Text.new_from_string(word))
                    self.hide_preedit_text()

        def do_property_activate(self, prop_name: str, state: int):
            checked = (state == IBus.PropState.CHECKED)
            self._commit_preedit_if_any()
            self.engine.reset()

            if prop_name == "InputMode":
                self.vietnamese_enabled = checked
                self.prop_mode.set_state(state)
                self.update_property(self.prop_mode)
                return
            elif prop_name == "MethodTelex":
                self.config.input_method = "telex" if checked else "vni"
            elif prop_name == "NoUnderline":
                self.no_underline_mode = checked
            elif prop_name == "AutoRestore":
                self.config.auto_restore_english = checked
            elif prop_name == "ModernOrtho":
                self.config.modern_orthography = checked
            else:
                return
            self._sync_properties()
            self._save_config()

        def _send_direct_replacement(self, backspaces: int, text: str):
            """
            Sends replacement without preedit underline using Surrounding Text or Fake Backspaces.
            """
            if backspaces > 0:
                # If client supports Surrounding Text deletion, use it first
                if self.capabilities & IBus.Capabilite.SURROUNDING_TEXT:
                    self.delete_surrounding_text(-backspaces, backspaces)
                else:
                    for _ in range(backspaces):
                        self.forward_key_event(IBus.KEY_BackSpace, 14, 0)
                        self.forward_key_event(IBus.KEY_BackSpace, 14, IBus.ModifierType.RELEASE_MASK)

            if text:
                self.commit_text(IBus.Text.new_from_string(text))

        def do_process_key_event(self, keyval: int, keycode: int, state: int) -> bool:
            # Ignore key release events
            if state & IBus.ModifierType.RELEASE_MASK:
                return False

            self._maybe_reload_config()

            # Ignore modifier shortcuts (Ctrl, Alt, Super)
            mod_mask = (
                IBus.ModifierType.CONTROL_MASK
                | IBus.ModifierType.MOD1_MASK
                | IBus.ModifierType.SUPER_MASK
            )
            if state & mod_mask:
                self._commit_preedit_if_any()
                self.engine.reset()
                return False

            if not self.vietnamese_enabled:
                return False

            # Handle Backspace
            if keyval == IBus.KEY_BackSpace:
                if self.no_underline_mode:
                    res = self.engine.process_backspace()
                    if res.handled:
                        self._send_direct_replacement(res.backspaces, res.commit_text)
                        return True
                    return False
                else:
                    had_preedit = bool(self.engine.get_displayed_word())
                    self.engine.process_backspace()
                    cur = self.engine.get_displayed_word()
                    if cur:
                        self.update_preedit_text(IBus.Text.new_from_string(cur), len(cur), True)
                        return True
                    self.hide_preedit_text()
                    # Swallow the key if it only erased the last preedit char; otherwise
                    # let the application delete already-committed text.
                    return had_preedit

            # Navigation / Enter / Escape / Tab keys
            if keyval in (
                IBus.KEY_Return, IBus.KEY_KP_Enter, IBus.KEY_Escape, IBus.KEY_Tab,
                IBus.KEY_Left, IBus.KEY_Right, IBus.KEY_Up, IBus.KEY_Down,
                IBus.KEY_Home, IBus.KEY_End, IBus.KEY_Page_Up, IBus.KEY_Page_Down,
            ):
                self._commit_preedit_if_any()
                self.engine.reset()
                return False

            # Printable ASCII characters (32..126)
            if 32 <= keyval <= 126:
                ch = chr(keyval)
                if self.no_underline_mode:
                    res = self.engine.process_key(ch)
                    if not res.handled:
                        return False
                    self._send_direct_replacement(res.backspaces, res.commit_text)
                    return True
                else:
                    # Clean Preedit Mode (Wayland fallback)
                    old_display = self.engine.get_displayed_word()
                    res = self.engine.process_key(ch)
                    cur = self.engine.get_displayed_word()
                    if cur:
                        self.update_preedit_text(IBus.Text.new_from_string(cur), len(cur), True)
                        return True
                    # Word boundary reached: commit the completed word + boundary char.
                    # Macro / auto-restore results already contain the whole replacement;
                    # otherwise the preedit word itself must be committed or it is lost.
                    self.hide_preedit_text()
                    text = res.commit_text if res.handled else old_display + ch
                    if old_display or res.handled:
                        self.commit_text(IBus.Text.new_from_string(text))
                        return True
                    return False

            self._commit_preedit_if_any()
            self.engine.reset()
            return False


def main():
    if IBus is None:
        print("Error: python3-gi and gir1.2-ibus-1.0 are required to run the IBus daemon.", file=sys.stderr)
        sys.exit(1)

    IBus.init()
    bus = IBus.Bus()
    if not bus.is_connected():
        print("Error: cannot connect to ibus-daemon.", file=sys.stderr)
        sys.exit(1)
    loop = GLib.MainLoop()
    bus.connect("disconnected", lambda *_: loop.quit())
    factory = IBus.Factory.new(bus.get_connection())
    factory.add_engine("viskey", GObject.type_from_name("VisKeyIBusEngine"))

    if "--ibus" in sys.argv:
        bus.request_name("org.freedesktop.IBus.VisKey", 0)
    else:
        component = IBus.Component.new(
            "org.freedesktop.IBus.VisKey",
            "VisKey Vietnamese Input Method",
            "1.0.0",
            "MIT",
            "VisKey Project",
            "https://github.com/viskey/viskey",
            "/usr/libexec/ibus-engine-viskey --ibus",
            "viskey",
        )
        engine_desc = IBus.EngineDesc.new(
            "viskey",
            "VisKey (Tiếng Việt)",
            "Bộ gõ Tiếng Việt thông minh không gạch chân cho Linux",
            "vi",
            "MIT",
            "VisKey Project",
            "input-keyboard",
            "us",
        )
        component.add_engine(engine_desc)
        bus.register_component(component)

    loop.run()


if __name__ == "__main__":
    main()
