#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
VisKey Linux IBus Engine
Supports two seamless commit strategies to solve the classic Linux "Preedit Underline" bug:
1. Direct / Surrounding-Text & Fake-Backspace Mode (No Underline / Không gạch chân)
2. Wayland-Safe Preedit Mode (Auto-commits cleanly on focus out / mouse click / shortcut)
"""
import json
import os
import sys

# Ensure core module is importable
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
CORE_DIR = os.path.abspath(os.path.join(SCRIPT_DIR, "..", "..", "core"))
if os.path.isdir(CORE_DIR):
    sys.path.insert(0, CORE_DIR)
elif os.path.isdir("/usr/share/viskey/core"):
    sys.path.insert(0, "/usr/share/viskey/core")

from viskey_engine import EngineConfig, VisKeyEngine

try:
    import viskey_settings_model as settings_model
except ImportError:  # the engine must keep working even without the settings helpers
    settings_model = None

try:
    from viskey_version import VERSION
except ImportError:
    VERSION = "1.0.0"

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
            self.config = EngineConfig(
                input_method="telex",
                modern_orthography=False,
                auto_restore_english=True,
                quick_telex=False,
                enable_macros=True,
                fix_autocomplete=True,
            )
            self.engine = VisKeyEngine(self.config)
            self.vietnamese_enabled = True
            # Default to No-Underline (Surrounding Text / Fake Backspace) on X11;
            # on strict Wayland without Surrounding Text, seamlessly fall back to Preedit if needed.
            self.no_underline_mode = True
            # "auto": Surrounding Text when the client supports it, else fake BackSpace;
            # "backspace": always fake BackSpace (set from the settings app, "Phương thức").
            self.direct_commit_method = "auto"
            self.client_caps = 0
            self._last_status = None
            self._usage = settings_model.MacroUsageRecorder() if settings_model else None
            self._usage_timer = 0
            self.is_wayland = "WAYLAND_DISPLAY" in os.environ
            self.config_path = os.path.expanduser("~/.config/viskey/config.json")
            self.config_mtime = 0.0
            self._reload_json_config()
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
                label=IBus.Text.new_from_string("Đổi kiểu gõ: Telex / VNI (Đang: Telex)"),
                tooltip=IBus.Text.new_from_string("Nhấn để chuyển giữa Telex và VNI"),
                sensitive=True,
                visible=True,
                state=IBus.PropState.CHECKED,
            )
            props.append(self.prop_telex)

            self.prop_underline = IBus.Property(
                key="NoUnderline",
                prop_type=IBus.PropType.TOGGLE,
                label=IBus.Text.new_from_string("Chế độ Không gạch chân (No Preedit)"),
                tooltip=IBus.Text.new_from_string("Tắt gạch chân khi gõ (khuyên dùng)"),
                sensitive=True,
                visible=True,
                state=IBus.PropState.CHECKED,
            )
            props.append(self.prop_underline)

            self.prop_restore = IBus.Property(
                key="AutoRestore",
                prop_type=IBus.PropType.TOGGLE,
                label=IBus.Text.new_from_string("Tự khôi phục từ tiếng Anh"),
                tooltip=IBus.Text.new_from_string("Tự trả về từ tiếng Anh khi gõ window, software, book..."),
                sensitive=True,
                visible=True,
                state=IBus.PropState.CHECKED,
            )
            props.append(self.prop_restore)

            self.prop_ortho = IBus.Property(
                key="ModernOrtho",
                prop_type=IBus.PropType.TOGGLE,
                label=IBus.Text.new_from_string("Đặt dấu kiểu mới (hoà, thuý)"),
                tooltip=IBus.Text.new_from_string("Bật: hoà, thuý / Tắt: hòa, thúy"),
                sensitive=True,
                visible=True,
                state=IBus.PropState.UNCHECKED,
            )
            props.append(self.prop_ortho)

            return props

        def _reload_json_config(self):
            if not os.path.exists(self.config_path):
                return
            try:
                mtime = os.path.getmtime(self.config_path)
                if mtime <= self.config_mtime:
                    return
                with open(self.config_path, "r", encoding="utf-8") as f:
                    data = json.load(f)
                self.config_mtime = mtime
                self.vietnamese_enabled = bool(data.get("vietnamese_enabled", self.vietnamese_enabled))
                self.config.input_method = str(data.get("input_method", self.config.input_method))
                self.no_underline_mode = bool(data.get("no_underline_mode", self.no_underline_mode))
                method = data.get("direct_commit_method", self.direct_commit_method)
                if method in ("auto", "backspace"):
                    self.direct_commit_method = method
                self.config.auto_restore_english = bool(data.get("auto_restore_english", self.config.auto_restore_english))
                self.config.modern_orthography = bool(data.get("modern_orthography", self.config.modern_orthography))
                self.config.quick_telex = bool(data.get("quick_telex", self.config.quick_telex))
                self.config.enable_macros = bool(data.get("enable_macros", self.config.enable_macros))
                if isinstance(data.get("macros"), dict):
                    self.config.macros = {str(k): str(v) for k, v in data["macros"].items()}
            except Exception:
                pass

        def do_focus_in(self):
            self._reload_json_config()
            self.register_properties(self.prop_list)
            self.request_surrounding_text()
            self._write_status()

        def do_focus_out(self):
            self._commit_preedit_if_any()
            self.engine.reset()
            self._flush_usage(force=True)

        def do_set_capabilities(self, caps):
            self.client_caps = caps
            self._write_status()

        def _supports_surrounding_text(self) -> bool:
            caps = self.client_caps
            if not caps:
                try:
                    caps = self.client_capabilities
                except AttributeError:
                    caps = 0
            return bool(caps & IBus.Capabilite.SURROUNDING_TEXT)

        def _write_status(self):
            """~/.cache/viskey/status.json for the settings app's "Chẩn đoán" section.
            Only engine state is written: no app names, no typed text."""
            if settings_model is None:
                return
            surrounding = self._supports_surrounding_text()
            now = GLib.get_monotonic_time() / 1e6
            if self._last_status and self._last_status[0] == surrounding and now - self._last_status[1] < 60:
                return
            try:
                settings_model.write_status(surrounding)
                self._last_status = (surrounding, now)
            except OSError:
                pass

        def _process_printable(self, ch: str):
            """Run the engine on one character and count macro expansions. Detection happens
            here in the adapter (the engine is untouched); only the macro key is recorded."""
            raw_before = "".join(self.engine.raw_keys)
            res = self.engine.process_key(ch)
            if self._usage is not None:
                key = settings_model.detect_macro_expansion(
                    raw_before, ch, res.handled, res.commit_text,
                    self.config.macros, self.config.enable_macros)
                if key:
                    self._usage.record(key)
                    self._flush_usage()
            return res

        def _flush_usage(self, force: bool = False):
            if self._usage is None or not self._usage.dirty:
                return
            if self._usage.flush(force=force):
                return
            if not self._usage_timer:
                self._usage_timer = GLib.timeout_add_seconds(int(self._usage.min_interval) + 1, self._on_usage_timer)

        def _on_usage_timer(self):
            self._usage_timer = 0
            self._flush_usage(force=True)
            return False

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
            elif prop_name == "MethodTelex":
                self.config.input_method = "telex" if checked else "vni"
                label = f"Đổi kiểu gõ: Telex / VNI (Đang: {self.config.input_method.upper()})"
                self.prop_telex.set_label(IBus.Text.new_from_string(label))
                self.update_property(self.prop_telex)
            elif prop_name == "NoUnderline":
                self.no_underline_mode = checked
            elif prop_name == "AutoRestore":
                self.config.auto_restore_english = checked
            elif prop_name == "ModernOrtho":
                self.config.modern_orthography = checked

        def _send_direct_replacement(self, backspaces: int, text: str):
            """
            Sends replacement without preedit underline using Surrounding Text or Fake Backspaces.
            """
            if backspaces > 0:
                # If client supports Surrounding Text deletion, use it first (unless the user
                # chose "Phím xoá lùi giả lập (BackSpace)" in the settings app)
                if self.direct_commit_method != "backspace" and self._supports_surrounding_text():
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
                    res = self.engine.process_backspace()
                    cur = self.engine.get_displayed_word()
                    if cur:
                        self.update_preedit_text(IBus.Text.new_from_string(cur), len(cur), True)
                        return True
                    else:
                        self.hide_preedit_text()
                        # The last preedit char was erased: swallow the key so the app
                        # does not also delete the committed char before it.
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
                    res = self._process_printable(ch)
                    if not res.handled:
                        return False
                    self._send_direct_replacement(res.backspaces, res.commit_text)
                    return True
                else:
                    # Clean Preedit Mode (Wayland fallback)
                    word_before = self.engine.get_displayed_word()
                    res = self._process_printable(ch)
                    cur = self.engine.get_displayed_word()
                    if cur:
                        self.update_preedit_text(IBus.Text.new_from_string(cur), len(cur), True)
                        return True
                    else:
                        # Word boundary reached: commit the completed word + boundary char
                        if res.handled and res.commit_text:
                            self.hide_preedit_text()
                            self.commit_text(IBus.Text.new_from_string(res.commit_text))
                            return True
                        self.hide_preedit_text()
                        # Commit the composed word, then let the boundary key through
                        if word_before:
                            self.commit_text(IBus.Text.new_from_string(word_before))
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
    factory = IBus.Factory.new(bus.get_connection())
    factory.add_engine("viskey", GObject.type_from_name("VisKeyIBusEngine"))

    if "--ibus" in sys.argv:
        bus.request_name("org.freedesktop.IBus.VisKey", 0)
    else:
        component = IBus.Component.new(
            "org.freedesktop.IBus.VisKey",
            "VisKey Vietnamese Input Method",
            VERSION,
            "MIT",
            "VisKey Project",
            "https://github.com/1vision-work/VisKey",
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

    loop = GLib.MainLoop()
    loop.run()


if __name__ == "__main__":
    main()
