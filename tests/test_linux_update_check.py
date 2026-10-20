#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Tests for the automatic update check logic of the Linux settings app (stdlib only, no GTK).

Run: python3 tests/test_linux_update_check.py
"""
import json
import os
import sys
import tempfile
import time
import unittest

PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
sys.path.insert(0, os.path.join(PROJECT_ROOT, "linux", "ibus"))

import viskey_settings_model as model  # noqa: E402

DAY = 24 * 3600
NOW = 1_800_000_000


class SettingTests(unittest.TestCase):
    def test_auto_check_defaults_on_and_is_a_bool_setting(self):
        self.assertIs(model.default_config()["auto_check_updates"], True)
        self.assertIs(model.merge_config({"auto_check_updates": False})["auto_check_updates"], False)
        self.assertIs(model.merge_config({"auto_check_updates": "no"})["auto_check_updates"], True)

    def test_interval_is_24h_and_tick_is_shorter(self):
        self.assertEqual(model.AUTO_CHECK_INTERVAL, DAY)
        self.assertLess(model.AUTO_CHECK_TICK, model.AUTO_CHECK_INTERVAL)


class ThrottleTests(unittest.TestCase):
    def test_disabled_never_due(self):
        self.assertFalse(model.auto_check_due(False, 0, NOW))
        self.assertFalse(model.auto_check_due(False, NOW - 10 * DAY, NOW))

    def test_first_run_is_due(self):
        self.assertTrue(model.auto_check_due(True, 0, NOW))
        self.assertTrue(model.auto_check_due(True, None, NOW))
        self.assertTrue(model.auto_check_due(True, "garbage", NOW))

    def test_at_most_once_per_24h(self):
        self.assertFalse(model.auto_check_due(True, NOW - 1, NOW))
        self.assertFalse(model.auto_check_due(True, NOW - DAY + 1, NOW))
        self.assertTrue(model.auto_check_due(True, NOW - DAY, NOW))
        self.assertTrue(model.auto_check_due(True, NOW - 3 * DAY, NOW))

    def test_clock_moved_back_does_not_block_forever(self):
        self.assertTrue(model.auto_check_due(True, NOW + 3600, NOW))

    def test_failed_attempt_also_throttles(self):
        def failing(_version):
            return model.UpdateResult("error", message="Không kết nối được tới GitHub")
        _result, state = model.run_update_check("1.0.0", model.default_update_state(), None,
                                                now=NOW, check=failing)
        self.assertEqual(state["last_attempt"], NOW)
        self.assertEqual(state["last_status"], "error")
        self.assertFalse(model.auto_check_due(True, state["last_attempt"], NOW + 3600))
        self.assertTrue(model.auto_check_due(True, state["last_attempt"], NOW + DAY))


class NotifyOnceTests(unittest.TestCase):
    def newer_state(self, version, notified=""):
        state = model.record_update_result(
            model.default_update_state(),
            model.UpdateResult("newer", version=version, url="https://example.invalid/r"))
        state["last_notified_version"] = notified
        return state

    def test_notifies_new_version_once(self):
        state = self.newer_state("1.1.0")
        self.assertEqual(model.version_to_notify(state, "1.0.0"), "1.1.0")
        state["last_notified_version"] = "1.1.0"
        self.assertIsNone(model.version_to_notify(state, "1.0.0"))

    def test_notifies_again_for_an_even_newer_version(self):
        self.assertEqual(model.version_to_notify(self.newer_state("1.2.0", "1.1.0"), "1.0.0"), "1.2.0")

    def test_nothing_to_notify(self):
        self.assertIsNone(model.version_to_notify(model.default_update_state(), "1.0.0"))
        # user already upgraded past the stored version
        self.assertIsNone(model.version_to_notify(self.newer_state("1.1.0"), "1.1.0"))
        self.assertIsNone(model.version_to_notify(self.newer_state("1.1.0"), "1.2.0"))
        # an older notified marker does not suppress, garbage marker is ignored
        self.assertEqual(model.version_to_notify(self.newer_state("1.1.0", "junk"), "1.0.0"), "1.1.0")

    def test_known_newer_version(self):
        self.assertEqual(model.known_newer_version(self.newer_state("1.1.0"), "1.0.0"),
                         ("1.1.0", "https://example.invalid/r"))
        self.assertIsNone(model.known_newer_version(self.newer_state("1.1.0"), "1.1.0"))

    def test_errors_keep_known_newer_version_latest_clears_it(self):
        state = self.newer_state("1.1.0", "1.1.0")
        err = model.record_update_result(state, model.UpdateResult("error", message="x"))
        self.assertEqual(err["latest_version"], "1.1.0")
        self.assertEqual(err["last_message"], "x")
        self.assertEqual(err["last_notified_version"], "1.1.0")
        latest = model.record_update_result(err, model.UpdateResult("latest", version="1.1.0"))
        self.assertEqual(latest["latest_version"], "")
        self.assertEqual(latest["last_message"], "")


class SharedCheckTests(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.path = os.path.join(self._tmp.name, "state", "update_check.json")

    def tearDown(self):
        self._tmp.cleanup()

    def test_run_update_check_uses_the_same_request_and_persists(self):
        calls = []

        def fake_fetch(version):
            calls.append(version)
            # The attempt is recorded before the request goes out.
            with open(self.path, encoding="utf-8") as f:
                self.assertEqual(json.load(f)["last_attempt"], NOW)
            return [{"tag_name": "v1.3.0", "html_url": "https://example.invalid/v1.3.0"},
                    {"tag_name": "v9.0.0", "draft": True}]

        def check(version):
            return model.check_for_update(version, fetch=fake_fetch)

        result, state = model.run_update_check("1.0.0", model.default_update_state(), self.path,
                                               now=NOW, check=check)
        self.assertEqual(calls, ["1.0.0"])
        self.assertEqual((result.status, result.version), ("newer", "1.3.0"))
        stored = model.load_update_state(self.path)
        self.assertEqual(stored, state)
        self.assertEqual(stored["latest_url"], "https://example.invalid/v1.3.0")
        self.assertEqual(set(stored), set(model.default_update_state()))

    def test_run_update_check_survives_unwritable_state(self):
        blocker = os.path.join(self._tmp.name, "file")
        open(blocker, "w").close()
        result, state = model.run_update_check(
            "1.0.0", model.default_update_state(), os.path.join(blocker, "x.json"), now=NOW,
            check=lambda v: model.UpdateResult("latest", version=v))
        self.assertEqual(result.status, "latest")
        self.assertEqual(state["last_attempt"], NOW)

    def test_in_memory_state_writes_nothing(self):
        model.save_update_state({"last_attempt": 1}, None)
        model.run_update_check("1.0.0", model.default_update_state(), None, now=NOW,
                               check=lambda v: model.UpdateResult("latest", version=v))
        self.assertEqual(os.listdir(self._tmp.name), [])

    def test_load_update_state_is_defensive(self):
        self.assertEqual(model.load_update_state(self.path), model.default_update_state())
        os.makedirs(os.path.dirname(self.path))
        with open(self.path, "w", encoding="utf-8") as f:
            json.dump({"last_attempt": "x", "latest_version": 3, "last_notified_version": "1.1.0",
                       "extra": 1}, f)
        state = model.load_update_state(self.path)
        self.assertEqual(state["last_attempt"], 0)
        self.assertEqual(state["latest_version"], "")
        self.assertEqual(state["last_notified_version"], "1.1.0")
        self.assertNotIn("extra", state)
        with open(self.path, "w", encoding="utf-8") as f:
            f.write("[1, 2")
        self.assertEqual(model.load_update_state(self.path), model.default_update_state())

    def test_state_lives_outside_the_engine_config(self):
        self.assertNotEqual(os.path.dirname(model.UPDATE_STATE_PATH), model.CONFIG_DIR)


class FormatLastCheckTests(unittest.TestCase):
    def local(self, *ymdhm):
        return time.mktime(tuple(ymdhm) + (0, 0, 0, -1))

    def test_formats(self):
        now = self.local(2026, 10, 19, 15, 0)
        state = dict(model.default_update_state())
        self.assertEqual(model.format_last_check(state, now), "Lần kiểm tra cuối: chưa kiểm tra")
        state.update(last_attempt=self.local(2026, 10, 19, 9, 5), last_status="latest")
        self.assertEqual(model.format_last_check(state, now),
                         "Lần kiểm tra cuối: hôm nay, 09:05 · đã là bản mới nhất")
        state.update(last_attempt=self.local(2026, 10, 18, 23, 59), last_status="newer", latest_version="1.1.0")
        self.assertEqual(model.format_last_check(state, now),
                         "Lần kiểm tra cuối: hôm qua, 23:59 · có bản mới 1.1.0")
        state.update(last_attempt=self.local(2026, 9, 1, 7, 30), last_status="error",
                     last_message="Không kết nối được tới GitHub")
        self.assertEqual(model.format_last_check(state, now),
                         "Lần kiểm tra cuối: 01/09/2026, 07:30 · không kiểm tra được: Không kết nối được tới GitHub")

    def test_yesterday_across_month_boundary(self):
        now = self.local(2026, 11, 1, 8, 0)
        state = dict(model.default_update_state(), last_attempt=self.local(2026, 10, 31, 20, 0))
        self.assertEqual(model.format_last_check(state, now), "Lần kiểm tra cuối: hôm qua, 20:00")


class NoNetworkInEngineTests(unittest.TestCase):
    def test_ibus_engine_never_checks_for_updates(self):
        with open(os.path.join(PROJECT_ROOT, "linux", "ibus", "ibus_viskey.py"), encoding="utf-8") as f:
            src = f.read()
        for needle in ("urllib", "run_update_check", "check_for_update", "fetch_releases", "socket"):
            self.assertNotIn(needle, src)


if __name__ == "__main__":
    unittest.main(verbosity=1)
