#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Tests for the GUI-free logic of the Linux settings app (stdlib unittest, no GTK needed).

Run: python3 tests/test_linux_settings.py
"""
import json
import os
import re
import socket
import sys
import tempfile
import unittest
import urllib.error

PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
sys.path.insert(0, os.path.join(PROJECT_ROOT, "core"))
sys.path.insert(0, os.path.join(PROJECT_ROOT, "linux", "ibus"))

import viskey_settings_model as model  # noqa: E402
from viskey_engine import EngineConfig, VisKeyEngine  # noqa: E402
from viskey_version import VERSION  # noqa: E402


class TempDirTestCase(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.dir = self._tmp.name

    def tearDown(self):
        self._tmp.cleanup()

    def path(self, *parts):
        return os.path.join(self.dir, *parts)


class ConfigTests(TempDirTestCase):
    def test_defaults_keep_legacy_keys_and_add_new(self):
        cfg = model.default_config()
        for key in ("vietnamese_enabled", "input_method", "no_underline_mode", "auto_restore_english",
                    "modern_orthography", "quick_telex", "enable_macros", "macros"):
            self.assertIn(key, cfg)
        self.assertEqual(cfg["direct_commit_method"], "auto")
        self.assertEqual(cfg["macros"]["ko"], "không")

    def test_default_config_is_a_deep_copy(self):
        cfg = model.default_config()
        cfg["macros"]["zz"] = "x"
        self.assertNotIn("zz", model.DEFAULT_CONFIG["macros"])

    def test_merge_keeps_valid_values_and_drops_wrong_types(self):
        cfg = model.merge_config({
            "input_method": "vni",
            "quick_telex": True,
            "no_underline_mode": "yes",          # wrong type -> default
            "direct_commit_method": "magic",     # not an allowed value -> default
            "macros": {"tks": "cảm ơn"},
            "future_key": [1, 2],                # unknown keys survive
        })
        self.assertEqual(cfg["input_method"], "vni")
        self.assertTrue(cfg["quick_telex"])
        self.assertTrue(cfg["no_underline_mode"])
        self.assertEqual(cfg["direct_commit_method"], "auto")
        self.assertEqual(cfg["macros"], {"tks": "cảm ơn"})
        self.assertEqual(cfg["future_key"], [1, 2])

    def test_merge_rejects_unknown_input_method(self):
        self.assertEqual(model.merge_config({"input_method": "viqr"})["input_method"], "telex")
        self.assertEqual(model.merge_config(["not", "a", "dict"]), model.default_config())

    def test_load_missing_or_corrupt_file_gives_defaults(self):
        self.assertEqual(model.load_config(self.path("nope.json")), model.default_config())
        with open(self.path("bad.json"), "w", encoding="utf-8") as f:
            f.write("{not json")
        self.assertEqual(model.load_config(self.path("bad.json")), model.default_config())

    def test_atomic_save_round_trip_creates_dirs_and_leaves_no_temp_files(self):
        target = self.path("sub", "viskey", "config.json")
        cfg = model.default_config()
        cfg["input_method"] = "vni"
        cfg["macros"]["vn"] = "Việt Nam"
        model.save_config(cfg, target)
        self.assertEqual(os.listdir(os.path.dirname(target)), ["config.json"])
        with open(target, encoding="utf-8") as f:
            raw = f.read()
        self.assertIn("Việt Nam", raw)  # UTF-8, not \u escapes
        self.assertEqual(model.load_config(target), cfg)

    def test_failed_save_keeps_previous_file(self):
        target = self.path("config.json")
        model.save_config(model.default_config(), target)
        with open(target, encoding="utf-8") as f:
            before = f.read()
        broken = model.default_config()
        broken["macros"] = {"x": object()}  # not JSON serialisable
        with self.assertRaises(TypeError):
            model.save_config(broken, target)
        with open(target, encoding="utf-8") as f:
            self.assertEqual(f.read(), before)
        self.assertEqual(os.listdir(self.dir), ["config.json"])

    def test_store_set_returns_previous_and_get_returns_copies(self):
        target = self.path("config.json")
        store = model.ConfigStore(target)
        self.assertEqual(store.set("quick_telex", True), False)
        macros = store.get("macros")
        macros["new"] = "mới"
        self.assertNotIn("new", store.get("macros"))
        store.save()
        self.assertTrue(model.load_config(target)["quick_telex"])

    def test_in_memory_store_never_writes(self):
        store = model.ConfigStore(path=None, initial={"macros": {"a": "b"}})
        store.set("input_method", "vni")
        store.save()
        self.assertEqual(os.listdir(self.dir), [])
        self.assertEqual(store.get("macros"), {"a": "b"})


class MacroValidationTests(unittest.TestCase):
    def test_valid_keys(self):
        for key in ("ko", "VN", "a1", "x" * 32, "  tks  "):
            self.assertIsNone(model.validate_macro_key(key), key)

    def test_invalid_keys_have_vietnamese_messages(self):
        self.assertEqual(model.validate_macro_key(""), "Viết tắt không được để trống")
        self.assertIn("32", model.validate_macro_key("x" * 33))
        for key in ("đc", "a b", "a-b", "kô", "a.b"):
            self.assertIn("chữ cái không dấu", model.validate_macro_key(key), key)

    def test_values(self):
        self.assertIsNone(model.validate_macro_value("cảm ơn nhiều"))
        self.assertIsNotNone(model.validate_macro_value("   "))
        self.assertIsNotNone(model.validate_macro_value("a\nb"))
        self.assertIsNotNone(model.validate_macro_value("x" * (model.MACRO_VALUE_MAX + 1)))

    def test_normalize_key(self):
        self.assertEqual(model.normalize_macro_key("  KO "), "ko")


class MacroImportExportTests(unittest.TestCase):
    def test_versioned_json(self):
        text = json.dumps({"version": 1, "macros": {"TKS": "cảm ơn nhiều", "vn": "Việt Nam"}}, ensure_ascii=False)
        macros, errors = model.parse_macro_import(text)
        self.assertEqual(errors, [])
        self.assertEqual(macros, {"tks": "cảm ơn nhiều", "vn": "Việt Nam"})

    def test_bare_json_object_and_bom(self):
        macros, errors = model.parse_macro_import("﻿" + json.dumps({"dc": "được"}))
        self.assertEqual((macros, errors), ({"dc": "được"}, []))

    def test_json_with_bad_entries(self):
        text = json.dumps({"version": 1, "macros": {"ok": "ổn", "bad key": "x", "n": 5}})
        macros, errors = model.parse_macro_import(text)
        self.assertEqual(macros, {"ok": "ổn"})
        self.assertEqual(len(errors), 2)

    def test_unsupported_version_and_invalid_json(self):
        self.assertEqual(model.parse_macro_import('{"version": 2, "macros": {}}')[0], {})
        self.assertIn("Phiên bản", model.parse_macro_import('{"version": 2, "macros": {}}')[1][0])
        self.assertEqual(model.parse_macro_import("{oops")[1], ["Tệp JSON không hợp lệ"])
        self.assertEqual(model.parse_macro_import('{"macros": [1]}')[1], ["Tệp JSON không có danh sách gõ tắt"])
        self.assertTrue(model.parse_macro_import("   ")[1])

    def test_tab_separated_lines(self):
        text = "# danh sách\nko\tkhông\n\nVN\tViệt Nam\r\nno tab here\nđc\tđược\nmn\t mọi người \n"
        macros, errors = model.parse_macro_import(text)
        self.assertEqual(macros, {"ko": "không", "vn": "Việt Nam", "mn": "mọi người"})
        self.assertEqual(len(errors), 2)
        self.assertTrue(errors[0].startswith("Dòng 5:"), errors)
        self.assertTrue(errors[1].startswith("Dòng 6:"), errors)

    def test_value_may_contain_tabs_after_first(self):
        macros, _ = model.parse_macro_import("a\tb\tc\n")
        self.assertEqual(macros, {"a": "b\tc"})

    def test_export_round_trip(self):
        src = {"vn": "Việt Nam", "bt": "bình thường"}
        text = model.export_macros_json(src)
        data = json.loads(text)
        self.assertEqual(data["version"], 1)
        self.assertEqual(list(data["macros"]), ["bt", "vn"])  # sorted
        self.assertIn("Việt Nam", text)
        self.assertEqual(model.parse_macro_import(text), (src, []))

    def test_filter_ignores_case_and_diacritics(self):
        macros = {"dc": "được", "vn": "Việt Nam", "ko": "không"}
        self.assertEqual(model.filter_macros(macros, "duoc"), {"dc": "được"})
        self.assertEqual(model.filter_macros(macros, "VIET"), {"vn": "Việt Nam"})
        self.assertEqual(model.filter_macros(macros, "k"), {"ko": "không"})
        self.assertEqual(model.filter_macros(macros, "  "), macros)

    def test_sorting(self):
        macros = {"b": "1", "a": "2", "c": "3"}
        usage = {"c": 10, "b": 10, "a": 1}
        self.assertEqual([k for k, _ in model.sort_macro_items(macros, usage, "az")], ["a", "b", "c"])
        self.assertEqual([k for k, _ in model.sort_macro_items(macros, usage, "usage")], ["b", "c", "a"])
        self.assertEqual([k for k, _ in model.sort_macro_items(macros, None, "usage")], ["a", "b", "c"])

    def test_format_count(self):
        self.assertEqual(model.format_count(97), "97")
        self.assertEqual(model.format_count(1038), "1.038")
        self.assertEqual(model.format_count(1234567), "1.234.567")


class VersionTests(unittest.TestCase):
    def test_parse(self):
        self.assertEqual(model.parse_version("v1.2.3"), (1, 2, 3))
        self.assertEqual(model.parse_version("1.2"), (1, 2, 0))
        self.assertEqual(model.parse_version("V2.0.1-beta.1"), (2, 0, 1))
        self.assertIsNone(model.parse_version("nightly"))
        self.assertIsNone(model.parse_version(None))

    def test_compare(self):
        self.assertEqual(model.compare_versions("1.0.0", "v1.0"), 0)
        self.assertEqual(model.compare_versions("1.0.10", "1.0.9"), 1)
        self.assertEqual(model.compare_versions("1.9.9", "2.0.0"), -1)
        with self.assertRaises(ValueError):
            model.compare_versions("x", "1.0")

    def test_evaluate_releases(self):
        releases = [
            {"tag_name": "v1.2.0", "draft": True, "html_url": "u-draft"},
            {"tag_name": "v1.1.0", "prerelease": True, "html_url": "u-110"},
            {"tag_name": "v1.0.0", "html_url": "u-100"},
            {"tag_name": "latest-build"},
        ]
        r = model.evaluate_releases("1.0.0", releases)
        self.assertEqual((r.status, r.version, r.url), ("newer", "1.1.0", "u-110"))
        self.assertEqual(model.evaluate_releases("1.1.0", releases).status, "latest")
        self.assertEqual(model.evaluate_releases("2.0.0", releases).status, "latest")
        self.assertEqual(model.evaluate_releases("1.0.0", []).status, "error")
        self.assertEqual(model.evaluate_releases("1.0.0", {"message": "x"}).status, "error")

    def test_check_for_update_errors_are_vietnamese(self):
        def raiser(exc):
            def fetch(_v):
                raise exc
            return fetch

        cases = [
            (urllib.error.HTTPError("u", 403, "rate", {}, None), "giới hạn"),
            (urllib.error.HTTPError("u", 500, "boom", {}, None), "lỗi 500"),
            (urllib.error.URLError("dns"), "Không kết nối"),
            (urllib.error.URLError(socket.timeout("t")), "Hết thời gian"),
            (socket.timeout("t"), "Hết thời gian"),
            (ValueError("json"), "không hợp lệ"),
        ]
        for exc, needle in cases:
            r = model.check_for_update("1.0.0", fetch=raiser(exc))
            self.assertEqual(r.status, "error")
            self.assertIn(needle, r.message, repr(exc))

    def test_check_for_update_success(self):
        r = model.check_for_update("1.0.0", fetch=lambda v: [{"tag_name": "v1.0.1", "html_url": "x"}])
        self.assertEqual((r.status, r.version), ("newer", "1.0.1"))

    def test_version_single_source_matches_cmake(self):
        with open(os.path.join(PROJECT_ROOT, "CMakeLists.txt"), encoding="utf-8") as f:
            m = re.search(r"project\(VisKey VERSION (\d+\.\d+\.\d+)", f.read())
        self.assertIsNotNone(m)
        self.assertEqual(VERSION, m.group(1))
        with open(os.path.join(PROJECT_ROOT, "scripts", "build_linux_deb.sh"), encoding="utf-8") as f:
            self.assertIn("viskey_version.py", f.read())


def type_like_ibus_adapter(engine, text, recorder_keys):
    """Mimic ibus_viskey._process_printable: capture raw keys before each key, then detect."""
    screen = []
    for ch in text:
        raw_before = "".join(engine.raw_keys)
        res = engine.process_key(ch)
        key = model.detect_macro_expansion(raw_before, ch, res.handled, res.commit_text,
                                           engine.config.macros, engine.config.enable_macros)
        if key:
            recorder_keys.append(key)
        if res.handled:
            if res.backspaces:
                del screen[len(screen) - res.backspaces:]
            screen.extend(res.commit_text)
        else:
            screen.append(ch)
    return "".join(screen)


class MacroUsageTests(TempDirTestCase):
    def make_engine(self, **kw):
        return VisKeyEngine(EngineConfig(input_method="telex", **kw))

    def test_detects_expansions_with_case_patterns(self):
        engine = self.make_engine()
        keys = []
        out = type_like_ibus_adapter(engine, "ko Ko KO dc, vn. ", keys)
        self.assertEqual(out, "không Không KHÔNG được, Việt Nam. ")
        self.assertEqual(keys, ["ko", "ko", "ko", "dc", "vn"])

    def test_ignores_normal_words_and_english_restore(self):
        engine = self.make_engine()
        keys = []
        out = type_like_ibus_adapter(engine, "Tieengs Vieetj window koo ", keys)
        self.assertEqual(out, "Tiếng Việt window koo ")
        self.assertEqual(keys, [])

    def test_disabled_macros_are_not_counted(self):
        engine = self.make_engine(enable_macros=False)
        keys = []
        self.assertEqual(type_like_ibus_adapter(engine, "ko ", keys), "ko ")
        self.assertEqual(keys, [])

    def test_detect_edge_cases(self):
        macros = {"ko": "không"}
        self.assertIsNone(model.detect_macro_expansion("", " ", True, "không ", macros, True))
        self.assertIsNone(model.detect_macro_expansion("ko", " ", False, "", macros, True))
        self.assertIsNone(model.detect_macro_expansion("ko", " ", True, "ko ", macros, True))
        self.assertEqual(model.detect_macro_expansion("KO", ".", True, "KHÔNG.", macros, True), "ko")

    def test_recorder_throttles_and_merges(self):
        now = [100.0]
        path = self.path("share", "macro_usage.json")
        rec = model.MacroUsageRecorder(path, min_interval=5.0, clock=lambda: now[0])
        self.assertFalse(rec.flush())               # nothing pending
        rec.record("ko")
        self.assertTrue(rec.flush())                 # first write is immediate
        rec.record("ko")
        rec.record("dc")
        now[0] += 2
        self.assertFalse(rec.flush())                # throttled
        self.assertTrue(rec.dirty)
        now[0] += 4
        self.assertTrue(rec.flush())
        self.assertEqual(model.load_usage(path), {"ko": 2, "dc": 1})
        rec.record("dc")
        self.assertTrue(rec.flush(force=True))       # focus-out
        self.assertEqual(model.load_usage(path), {"ko": 2, "dc": 2})
        rec2 = model.MacroUsageRecorder(path, clock=lambda: now[0])
        rec2.record("vn")
        rec2.flush(force=True)
        self.assertEqual(model.load_usage(path), {"ko": 2, "dc": 2, "vn": 1})
        with open(path, encoding="utf-8") as f:
            data = json.load(f)
        self.assertEqual(set(data), {"version", "counts"})  # nothing but counters

    def test_load_usage_is_defensive(self):
        path = self.path("u.json")
        self.assertEqual(model.load_usage(path), {})
        with open(path, "w", encoding="utf-8") as f:
            json.dump({"counts": {"ok": 3, "neg": -1, "s": "x", "b": True}}, f)
        self.assertEqual(model.load_usage(path), {"ok": 3})


class StatusAndEnvTests(TempDirTestCase):
    def test_status_file_contains_only_engine_state(self):
        path = self.path("cache", "status.json")
        model.write_status(True, path=path, pid=1234, now=1700000000)
        data = model.read_status(path)
        self.assertEqual(data, {"engine": "running", "pid": 1234, "surrounding_text": True, "updated": 1700000000})
        self.assertIsNone(model.read_status(self.path("missing.json")))

    def test_pid_alive(self):
        self.assertTrue(model.pid_alive(os.getpid()))
        self.assertFalse(model.pid_alive(0))
        self.assertFalse(model.pid_alive("abc"))

    def test_session_type(self):
        self.assertEqual(model.session_type({"XDG_SESSION_TYPE": "wayland"}), "Wayland")
        self.assertEqual(model.session_type({"XDG_SESSION_TYPE": "x11"}), "X11")
        self.assertEqual(model.session_type({"WAYLAND_DISPLAY": "wayland-0"}), "Wayland")
        self.assertEqual(model.session_type({"DISPLAY": ":0"}), "X11")
        self.assertEqual(model.session_type({}), "")

    def test_format_accelerator(self):
        self.assertEqual(model.format_accelerator("<Control>space"), "Ctrl+Space")
        self.assertEqual(model.format_accelerator("<Super>space"), "Super+Space")
        self.assertEqual(model.format_accelerator("<Shift><Alt>z"), "Shift+Alt+Z")
        self.assertEqual(model.format_accelerator("<Primary>F12"), "Ctrl+F12")
        self.assertEqual(model.format_accelerator(""), "")

    def test_system_info_text(self):
        text = model.format_system_info({"viskey": "1.0.0", "session": "Wayland", "adwaita": "1.5.0"})
        self.assertIn("VisKey: 1.0.0", text)
        self.assertIn("Phiên hiển thị: Wayland", text)
        self.assertIn("IBus: không rõ", text)


class SettingsModuleTests(unittest.TestCase):
    def test_settings_entry_point_imports_without_building_gui(self):
        import viskey_settings
        self.assertEqual(viskey_settings.APP_ID, "org.viskey.Settings")
        self.assertEqual(viskey_settings._parse_args(["--ui-snapshot", "out", "-v"]),
                         (os.path.abspath("out"), ["-v"]))
        for key in viskey_settings.DEMO_MACROS:
            self.assertIsNone(model.validate_macro_key(key), key)

    def test_ibus_adapter_honours_direct_commit_method(self):
        with open(os.path.join(PROJECT_ROOT, "linux", "ibus", "ibus_viskey.py"), encoding="utf-8") as f:
            src = f.read()
        self.assertIn('data.get("direct_commit_method"', src)
        self.assertIn('self.direct_commit_method != "backspace"', src)


if __name__ == "__main__":
    unittest.main(verbosity=1)
