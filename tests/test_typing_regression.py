#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Typing regression suite (Python engine, used by Linux IBus).

Replays every case of tests/data/typing_regression.tsv key by key and applies the
engine results to a text buffer exactly like the macOS / Windows layers do:
  handled     -> [autocomplete fix: insert U+202F, delete 1] + delete `backspaces`
                 + insert `commit_text`; the original key is swallowed
  not handled -> the original key reaches the app (Backspace deletes one char)
Each case is typed followed by a space; the final buffer must equal expected + " ".
After every key the buffer must equal committed text + the engine's displayed word,
which catches doubled / lost characters mid-word.

In the TSV, the two characters "\\b" inside `keys` stand for a Backspace key press.
"""
import os
import sys

PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
sys.path.insert(0, os.path.join(PROJECT_ROOT, "core"))

from viskey_engine import EngineConfig, VisKeyEngine  # noqa: E402

TSV_PATH = os.path.join(PROJECT_ROOT, "tests", "data", "typing_regression.tsv")
AUTOCOMPLETE_FIX_CHAR = " "
BACKSPACE = "\b"


def make_engine(method):
    if method == "telex":
        return VisKeyEngine(EngineConfig(input_method="telex"))
    if method == "vni":
        return VisKeyEngine(EngineConfig(input_method="vni"))
    if method == "telex_norestore":
        return VisKeyEngine(EngineConfig(input_method="telex", auto_restore_english=False))
    raise ValueError(f"unknown method {method!r}")


def load_cases(path):
    cases = []
    with open(path, encoding="utf-8") as f:
        for lineno, line in enumerate(f, 1):
            line = line.rstrip("\n")
            if not line or line.startswith("#"):
                continue
            method, keys, expected = line.split("\t")
            cases.append((lineno, method, keys.replace("\\b", BACKSPACE), expected))
    return cases


def simulate(engine, keys):
    """Returns (final buffer, error or None)."""
    engine.reset()
    buf = []
    committed = ""  # buffer content before the word being composed
    for step, key in enumerate(keys):
        res = engine.process_backspace() if key == BACKSPACE else engine.process_key(key)
        if res.handled:
            if res.autocomplete_fix_needed and res.backspaces > 0:
                buf.append(AUTOCOMPLETE_FIX_CHAR)
                buf.pop()
            if res.backspaces > len(buf):
                return "".join(buf), f"step {step} ({key!r}): {res.backspaces} backspaces > buffer {len(buf)}"
            if res.backspaces:
                del buf[-res.backspaces:]
            buf.extend(res.commit_text)
        elif key == BACKSPACE:
            if not buf:
                return "", f"step {step}: Backspace on empty buffer"
            buf.pop()
        else:
            buf.append(key)

        word = engine.get_displayed_word()
        if not word:
            committed = "".join(buf)
        elif "".join(buf) != committed + word:
            return "".join(buf), (f"step {step} ({key!r}): screen {''.join(buf)!r} "
                                  f"!= committed {committed!r} + engine word {word!r}")
    return "".join(buf), None


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else TSV_PATH
    cases = load_cases(path)
    engines = {}
    failures = []
    per_method = {}
    for lineno, method, keys, expected in cases:
        engine = engines.setdefault(method, make_engine(method))
        actual, err = simulate(engine, keys + " ")
        ok = err is None and actual == expected + " "
        stats = per_method.setdefault(method, [0, 0])
        stats[0] += 1
        if not ok:
            stats[1] += 1
            failures.append(f"  line {lineno} [{method}] keys={keys!r} expected={expected + ' '!r} "
                            f"got={actual!r}" + (f"  ({err})" if err else ""))

    for method, (total, failed) in sorted(per_method.items()):
        print(f"  {method:16s} {total - failed}/{total} passed")
    if failures:
        print(f"{len(failures)} FAILURES:")
        print("\n".join(failures))
        print(f"PYTHON TYPING REGRESSION: {len(failures)}/{len(cases)} FAILED")
        sys.exit(1)
    print(f"ALL {len(cases)} PYTHON TYPING REGRESSION CASES PASSED")


if __name__ == "__main__":
    main()
