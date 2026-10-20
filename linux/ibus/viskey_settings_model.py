# -*- coding: utf-8 -*-
"""
GUI-free logic shared by the VisKey Linux settings app (GTK4/libadwaita and Tk fallback)
and the IBus engine adapter.

Everything here is plain Python (stdlib only) so it can be unit-tested on machines without
GTK: config load/merge/atomic save, macro validation and import/export, release version
comparison, macro-usage counters and the small engine status file.

Privacy: the only typed text this module ever persists is a per-macro counter keyed by the
macro abbreviation the user configured themselves. No other keystrokes are stored.
"""
import copy
import json
import os
import re
import tempfile
import time
import unicodedata

# ---------------------------------------------------------------------------
# Paths (the IBus engine uses the same hard-coded config path)
# ---------------------------------------------------------------------------
CONFIG_DIR = os.path.expanduser("~/.config/viskey")
CONFIG_PATH = os.path.join(CONFIG_DIR, "config.json")
CONFIG_PATH_DISPLAY = "~/.config/viskey/config.json"
USAGE_PATH = os.path.expanduser("~/.local/share/viskey/macro_usage.json")
STATUS_PATH = os.path.expanduser("~/.cache/viskey/status.json")
UPDATE_STATE_PATH = os.path.expanduser("~/.local/state/viskey/update_check.json")

REPO_URL = "https://github.com/1vision-work/VisKey"
ISSUES_URL = REPO_URL + "/issues"
RELEASES_PAGE_URL = REPO_URL + "/releases"
LICENSE_URL = REPO_URL + "/blob/master/LICENSE"
RELEASES_API_URL = "https://api.github.com/repos/1vision-work/VisKey/releases?per_page=5"

# ---------------------------------------------------------------------------
# Config
# ---------------------------------------------------------------------------
DEFAULT_MACROS = {
    "vn": "Việt Nam",
    "ko": "không",
    "dc": "được",
    "uh": "ừ",
    "ntn": "như thế nào",
    "bt": "bình thường",
    "mn": "mọi người",
}

DEFAULT_CONFIG = {
    "vietnamese_enabled": True,
    "input_method": "telex",
    "no_underline_mode": True,
    "auto_restore_english": True,
    "modern_orthography": False,
    "quick_telex": False,
    "enable_macros": True,
    "macros": DEFAULT_MACROS,
    # Added in UI v2: how "no underline" mode deletes the previous characters.
    #   "auto"      -> Surrounding Text when the app supports it, otherwise fake BackSpace
    #   "backspace" -> always fake BackSpace key events
    "direct_commit_method": "auto",
    # Added in UI v2: let the settings app check GitHub releases at most once per 24 h.
    # Only read by the settings app; the IBus engine never touches the network.
    "auto_check_updates": True,
}

ENUM_VALUES = {
    "input_method": ("telex", "vni"),
    "direct_commit_method": ("auto", "backspace"),
}


def default_config() -> dict:
    return copy.deepcopy(DEFAULT_CONFIG)


def merge_config(data) -> dict:
    """Merge user data over the defaults.

    Known keys keep their default when the stored value has the wrong type; unknown keys are
    preserved untouched so newer/older VisKey versions can share the file.
    """
    cfg = default_config()
    if not isinstance(data, dict):
        return cfg
    for key, value in data.items():
        if key not in DEFAULT_CONFIG:
            cfg[key] = value
            continue
        default = DEFAULT_CONFIG[key]
        if key == "macros":
            if isinstance(value, dict):
                cfg[key] = {str(k): str(v) for k, v in value.items()}
        elif key in ENUM_VALUES:
            if isinstance(value, str) and value in ENUM_VALUES[key]:
                cfg[key] = value
        elif isinstance(default, bool):
            if isinstance(value, bool):
                cfg[key] = value
        else:
            cfg[key] = value
    return cfg


def load_config(path: str = CONFIG_PATH) -> dict:
    try:
        with open(path, "r", encoding="utf-8") as f:
            data = json.load(f)
    except (OSError, ValueError):
        return default_config()
    return merge_config(data)


def atomic_write_json(path: str, data, indent=2) -> None:
    """Write JSON to a temp file in the same directory, fsync, then os.replace()."""
    directory = os.path.dirname(os.path.abspath(path))
    os.makedirs(directory, exist_ok=True)
    fd, tmp = tempfile.mkstemp(prefix="." + os.path.basename(path) + ".", suffix=".tmp", dir=directory)
    try:
        with os.fdopen(fd, "w", encoding="utf-8") as f:
            json.dump(data, f, ensure_ascii=False, indent=indent)
            f.write("\n")
            f.flush()
            os.fsync(f.fileno())
        os.replace(tmp, path)
    except BaseException:
        try:
            os.unlink(tmp)
        except OSError:
            pass
        raise


def save_config(cfg: dict, path: str = CONFIG_PATH) -> None:
    atomic_write_json(path, cfg)


class ConfigStore:
    """Holds the current config. With path=None it is purely in-memory (snapshot/demo mode)."""

    def __init__(self, path=CONFIG_PATH, initial=None):
        self.path = path
        if initial is not None:
            self.cfg = merge_config(initial)
        elif path is not None:
            self.cfg = load_config(path)
        else:
            self.cfg = default_config()

    def get(self, key):
        value = self.cfg.get(key, DEFAULT_CONFIG.get(key))
        return copy.deepcopy(value)

    def set(self, key, value):
        """Set a value and return the previous one."""
        old = self.get(key)
        self.cfg[key] = copy.deepcopy(value)
        return old

    def snapshot(self) -> dict:
        return copy.deepcopy(self.cfg)

    def replace(self, cfg: dict) -> None:
        self.cfg = merge_config(cfg)

    def save(self) -> None:
        if self.path is not None:
            save_config(self.cfg, self.path)


# ---------------------------------------------------------------------------
# Macros: validation, import / export, filtering, sorting
# ---------------------------------------------------------------------------
MACRO_KEY_MAX = 32
MACRO_VALUE_MAX = 1000
_MACRO_KEY_RE = re.compile(r"^[A-Za-z0-9]+$")


def normalize_macro_key(key: str) -> str:
    return (key or "").strip().lower()


def validate_macro_key(key: str):
    """Return a Vietnamese error message, or None if the key is valid."""
    key = (key or "").strip()
    if not key:
        return "Viết tắt không được để trống"
    if len(key) > MACRO_KEY_MAX:
        return "Viết tắt dài tối đa %d ký tự" % MACRO_KEY_MAX
    if not _MACRO_KEY_RE.match(key):
        return "Viết tắt chỉ gồm chữ cái không dấu (a–z) và chữ số"
    return None


def validate_macro_value(value: str):
    value = (value or "").strip()
    if not value:
        return "Nội dung thay bằng không được để trống"
    if len(value) > MACRO_VALUE_MAX:
        return "Nội dung dài tối đa %d ký tự" % MACRO_VALUE_MAX
    if "\n" in value or "\r" in value:
        return "Nội dung chỉ được nằm trên một dòng"
    return None


def parse_macro_import(text: str):
    """Parse an import file.

    Accepts JSON {"version": 1, "macros": {...}}, a bare JSON object {"key": "value"}, or plain
    text with one "key<TAB>value" per line (blank lines and lines starting with # are skipped).
    Returns (macros, errors) where errors is a list of Vietnamese messages.
    """
    macros = {}
    errors = []
    if text.startswith("﻿"):
        text = text[1:]
    stripped = text.strip()
    if not stripped:
        return macros, ["Tệp trống, không có cụm từ nào"]

    if stripped.startswith("{"):
        try:
            data = json.loads(stripped)
        except ValueError:
            return macros, ["Tệp JSON không hợp lệ"]
        if isinstance(data, dict) and "macros" in data:
            version = data.get("version", 1)
            if version != 1:
                return macros, ["Phiên bản tệp không được hỗ trợ: %s" % version]
            data = data["macros"]
        if not isinstance(data, dict):
            return macros, ["Tệp JSON không có danh sách gõ tắt"]
        for raw_key, raw_value in data.items():
            if not isinstance(raw_value, str):
                errors.append("“%s”: nội dung phải là chuỗi ký tự" % raw_key)
                continue
            _add_import_entry(macros, errors, str(raw_key), raw_value, "“%s”" % raw_key)
        return macros, errors

    for lineno, line in enumerate(text.splitlines(), start=1):
        if not line.strip() or line.lstrip().startswith("#"):
            continue
        if "\t" not in line:
            errors.append("Dòng %d: thiếu dấu Tab giữa viết tắt và nội dung" % lineno)
            continue
        key, value = line.split("\t", 1)
        _add_import_entry(macros, errors, key, value, "Dòng %d" % lineno)
    return macros, errors


def _add_import_entry(macros, errors, key, value, where):
    err = validate_macro_key(key) or validate_macro_value(value)
    if err:
        errors.append("%s: %s" % (where, err))
        return
    macros[normalize_macro_key(key)] = value.strip()


def export_macros_json(macros: dict) -> str:
    ordered = {k: macros[k] for k in sorted(macros)}
    return json.dumps({"version": 1, "macros": ordered}, ensure_ascii=False, indent=2) + "\n"


def fold_text(text: str) -> str:
    """Lower-case and strip Vietnamese diacritics so "duoc" matches "được"."""
    text = (text or "").lower().replace("đ", "d")
    decomposed = unicodedata.normalize("NFD", text)
    return "".join(ch for ch in decomposed if not unicodedata.combining(ch))


def filter_macros(macros: dict, query: str) -> dict:
    q = fold_text(query.strip())
    if not q:
        return dict(macros)
    return {k: v for k, v in macros.items() if q in fold_text(k) or q in fold_text(v)}


def sort_macro_items(macros: dict, usage: dict, mode: str = "az"):
    """mode "az": by abbreviation; mode "usage": most used first, ties A→Z."""
    usage = usage or {}
    if mode == "usage":
        return sorted(macros.items(), key=lambda kv: (-int(usage.get(kv[0], 0)), kv[0]))
    return sorted(macros.items(), key=lambda kv: kv[0])


def format_count(n: int) -> str:
    """1038 -> "1.038" (Vietnamese thousands separator)."""
    return "{:,}".format(int(n)).replace(",", ".")


# ---------------------------------------------------------------------------
# Macro usage counters
# ---------------------------------------------------------------------------
def detect_macro_expansion(raw_before: str, ch: str, handled: bool, commit_text: str,
                           macros: dict, enable_macros: bool):
    """Decide, from the outside of the engine, whether a key press expanded a macro.

    raw_before is the engine's raw keystroke buffer for the current word *before* `ch` was
    processed; handled/commit_text come from the engine's KeyProcessResult. The engine expands
    a macro on a word boundary by committing the (case-adapted) value followed by `ch`.
    Returns the macro key (lower case) or None.
    """
    if not enable_macros or not raw_before or not handled or not ch:
        return None
    key = raw_before.lower()
    value = macros.get(key)
    if value is None or not commit_text.endswith(ch):
        return None
    emitted = commit_text[: len(commit_text) - len(ch)]
    if emitted.lower() == value.lower():
        return key
    return None


def load_usage(path: str = USAGE_PATH) -> dict:
    try:
        with open(path, "r", encoding="utf-8") as f:
            data = json.load(f)
    except (OSError, ValueError):
        return {}
    counts = data.get("counts") if isinstance(data, dict) else None
    if not isinstance(counts, dict):
        return {}
    result = {}
    for k, v in counts.items():
        if isinstance(v, int) and not isinstance(v, bool) and v > 0:
            result[str(k)] = v
    return result


class MacroUsageRecorder:
    """Accumulates macro expansion counts in memory and writes them at most every
    `min_interval` seconds (or when flush(force=True) is called, e.g. on focus-out)."""

    def __init__(self, path: str = USAGE_PATH, min_interval: float = 5.0, clock=time.monotonic):
        self.path = path
        self.min_interval = min_interval
        self.clock = clock
        self.pending = {}
        self.last_write = None

    def record(self, key: str) -> None:
        self.pending[key] = self.pending.get(key, 0) + 1

    @property
    def dirty(self) -> bool:
        return bool(self.pending)

    def due(self) -> bool:
        if not self.pending:
            return False
        return self.last_write is None or self.clock() - self.last_write >= self.min_interval

    def flush(self, force: bool = False) -> bool:
        if not self.pending or not (force or self.due()):
            return False
        counts = load_usage(self.path)
        for k, n in self.pending.items():
            counts[k] = counts.get(k, 0) + n
        try:
            atomic_write_json(self.path, {"version": 1, "counts": counts}, indent=None)
        except OSError:
            return False
        self.pending = {}
        self.last_write = self.clock()
        return True


# ---------------------------------------------------------------------------
# Engine status file (written by the IBus engine on focus-in, read by the settings app)
# ---------------------------------------------------------------------------
def write_status(surrounding_text: bool, path: str = STATUS_PATH, pid=None, now=None) -> None:
    data = {
        "engine": "running",
        "pid": int(pid if pid is not None else os.getpid()),
        "surrounding_text": bool(surrounding_text),
        "updated": int(now if now is not None else time.time()),
    }
    atomic_write_json(path, data, indent=None)


def read_status(path: str = STATUS_PATH):
    try:
        with open(path, "r", encoding="utf-8") as f:
            data = json.load(f)
    except (OSError, ValueError):
        return None
    return data if isinstance(data, dict) else None


def pid_alive(pid) -> bool:
    try:
        pid = int(pid)
    except (TypeError, ValueError):
        return False
    if pid <= 0:
        return False
    try:
        os.kill(pid, 0)
    except ProcessLookupError:
        return False
    except PermissionError:
        return True
    except OSError:
        return False
    return True


# ---------------------------------------------------------------------------
# Versions & update check (manual button + optional automatic check, same request)
# ---------------------------------------------------------------------------
def parse_version(text):
    """"v1.2.3" / "1.2" / "1.2.3-beta" -> (1, 2, 3); None if not a version."""
    if not isinstance(text, str):
        return None
    m = re.match(r"^\s*[vV]?(\d+(?:\.\d+){0,3})(?:[-+].*)?\s*$", text)
    if not m:
        return None
    parts = [int(p) for p in m.group(1).split(".")]
    while len(parts) < 3:
        parts.append(0)
    return tuple(parts)


def compare_versions(a, b) -> int:
    va, vb = parse_version(a), parse_version(b)
    if va is None or vb is None:
        raise ValueError("invalid version")
    width = max(len(va), len(vb))
    va = va + (0,) * (width - len(va))
    vb = vb + (0,) * (width - len(vb))
    return (va > vb) - (va < vb)


class UpdateResult:
    def __init__(self, status, version=None, url=None, message=None):
        self.status = status          # "latest" | "newer" | "error"
        self.version = version
        self.url = url
        self.message = message

    def __repr__(self):
        return "UpdateResult(%r, %r, %r, %r)" % (self.status, self.version, self.url, self.message)


def evaluate_releases(current: str, releases) -> UpdateResult:
    if not isinstance(releases, list):
        return UpdateResult("error", message="Dữ liệu phản hồi không hợp lệ")
    best = None
    for rel in releases:
        if not isinstance(rel, dict) or rel.get("draft"):
            continue
        tag = rel.get("tag_name") or rel.get("name")
        ver = parse_version(tag)
        if ver is None:
            continue
        if best is None or ver > best[0]:
            url = rel.get("html_url") or RELEASES_PAGE_URL
            best = (ver, ".".join(str(p) for p in ver), url)
    if best is None:
        return UpdateResult("error", message="Chưa có bản phát hành nào")
    if compare_versions(best[1], current) > 0:
        return UpdateResult("newer", version=best[1], url=best[2])
    return UpdateResult("latest", version=current)


def fetch_releases(current_version: str, timeout: float = 8.0, url: str = RELEASES_API_URL):
    import urllib.request
    req = urllib.request.Request(url, headers={
        "User-Agent": "VisKey/%s" % current_version,
        "Accept": "application/vnd.github+json",
    })
    with urllib.request.urlopen(req, timeout=timeout) as resp:
        return json.loads(resp.read().decode("utf-8"))


def check_for_update(current_version: str, fetch=fetch_releases) -> UpdateResult:
    import socket
    import urllib.error
    try:
        releases = fetch(current_version)
    except urllib.error.HTTPError as e:
        if e.code in (403, 429):
            return UpdateResult("error", message="GitHub tạm giới hạn truy cập, hãy thử lại sau")
        return UpdateResult("error", message="GitHub trả về lỗi %d" % e.code)
    except (socket.timeout, TimeoutError):
        return UpdateResult("error", message="Hết thời gian chờ phản hồi từ GitHub")
    except urllib.error.URLError as e:
        if isinstance(getattr(e, "reason", None), (socket.timeout, TimeoutError)):
            return UpdateResult("error", message="Hết thời gian chờ phản hồi từ GitHub")
        return UpdateResult("error", message="Không kết nối được tới GitHub")
    except ValueError:
        return UpdateResult("error", message="Dữ liệu phản hồi không hợp lệ")
    except OSError:
        return UpdateResult("error", message="Không kết nối được tới GitHub")
    return evaluate_releases(current_version, releases)


# Automatic check: throttle + "notify once per version" state.
# Stored apart from config.json so recording an attempt never makes the IBus engine reload.
AUTO_CHECK_INTERVAL = 24 * 3600      # never more often than this since the last attempt
AUTO_CHECK_TICK = 3 * 3600           # how often a running settings app re-evaluates


def default_update_state() -> dict:
    return {
        "last_attempt": 0,             # unix time of the last attempt (success or failure)
        "last_status": "",             # "latest" | "newer" | "error" | ""
        "last_message": "",            # Vietnamese error text for "error"
        "latest_version": "",          # newest release seen, if newer than the running build
        "latest_url": "",
        "last_notified_version": "",   # newest version the user was already told about
    }


def load_update_state(path: str = UPDATE_STATE_PATH) -> dict:
    state = default_update_state()
    try:
        with open(path, "r", encoding="utf-8") as f:
            data = json.load(f)
    except (OSError, ValueError):
        return state
    if not isinstance(data, dict):
        return state
    for key, default in state.items():
        value = data.get(key)
        if isinstance(default, int):
            if isinstance(value, (int, float)) and not isinstance(value, bool) and value >= 0:
                state[key] = int(value)
        elif isinstance(value, str):
            state[key] = value
    return state


def save_update_state(state: dict, path: str = UPDATE_STATE_PATH) -> None:
    if path is not None:
        atomic_write_json(path, state, indent=None)


def auto_check_due(enabled, last_attempt, now, interval: float = AUTO_CHECK_INTERVAL) -> bool:
    """True when the automatic check may run: enabled and >= interval since the last attempt.
    A last_attempt in the future (clock moved back) counts as due so it cannot block forever."""
    if not enabled:
        return False
    try:
        last = float(last_attempt or 0)
    except (TypeError, ValueError):
        last = 0.0
    if last <= 0 or last > now:
        return True
    return now - last >= interval


def version_to_notify(state: dict, current_version: str):
    """The newer version the user still has to be told about, or None (notify once per version)."""
    latest = state.get("latest_version") or ""
    if parse_version(latest) is None or parse_version(current_version) is None:
        return None
    if compare_versions(latest, current_version) <= 0:
        return None
    notified = state.get("last_notified_version") or ""
    if parse_version(notified) is not None and compare_versions(latest, notified) <= 0:
        return None
    return latest


def known_newer_version(state: dict, current_version: str):
    """(version, url) of a newer release recorded by the last successful check, else None."""
    latest = state.get("latest_version") or ""
    if parse_version(latest) is None or parse_version(current_version) is None:
        return None
    if compare_versions(latest, current_version) <= 0:
        return None
    return latest, state.get("latest_url") or RELEASES_PAGE_URL


def record_update_result(state: dict, result: UpdateResult) -> dict:
    """Return a copy of state updated with a finished check. Errors keep the last known
    newer version so a flaky network does not hide an available update."""
    state = dict(state)
    state["last_status"] = result.status
    state["last_message"] = (result.message or "") if result.status == "error" else ""
    if result.status == "newer":
        state["latest_version"] = result.version or ""
        state["latest_url"] = result.url or RELEASES_PAGE_URL
    elif result.status == "latest":
        state["latest_version"] = ""
        state["latest_url"] = ""
    return state


def run_update_check(current_version: str, state: dict, state_path=UPDATE_STATE_PATH,
                     now=None, check=check_for_update):
    """The one update-check routine used by the manual button and the automatic check.

    Records the attempt time *before* the request (so a crash or kill still counts towards the
    24 h throttle), performs the same anonymous GET as always, then stores the outcome.
    Blocking: call it from a worker thread. Returns (result, new_state).
    """
    state = dict(state)
    state["last_attempt"] = int(time.time() if now is None else now)
    try:
        save_update_state(state, state_path)
    except OSError:
        pass
    result = check(current_version)
    state = record_update_result(state, result)
    try:
        save_update_state(state, state_path)
    except OSError:
        pass
    return result, state


def format_last_check(state: dict, now=None) -> str:
    """"Lần kiểm tra cuối: hôm nay, 09:12 · đã là bản mới nhất" (local time)."""
    ts = state.get("last_attempt") or 0
    if not ts:
        return "Lần kiểm tra cuối: chưa kiểm tra"
    now = time.time() if now is None else now
    t, today = time.localtime(ts), time.localtime(now)
    yesterday = time.localtime(time.mktime((today.tm_year, today.tm_mon, today.tm_mday - 1,
                                            12, 0, 0, 0, 0, -1)))
    day = (t.tm_year, t.tm_yday)
    if day == (today.tm_year, today.tm_yday):
        when = "hôm nay, %02d:%02d" % (t.tm_hour, t.tm_min)
    elif day == (yesterday.tm_year, yesterday.tm_yday):
        when = "hôm qua, %02d:%02d" % (t.tm_hour, t.tm_min)
    else:
        when = "%02d/%02d/%d, %02d:%02d" % (t.tm_mday, t.tm_mon, t.tm_year, t.tm_hour, t.tm_min)
    status = state.get("last_status")
    if status == "latest":
        tail = "đã là bản mới nhất"
    elif status == "newer":
        tail = "có bản mới %s" % (state.get("latest_version") or "")
    elif status == "error":
        tail = "không kiểm tra được" + (": %s" % state["last_message"] if state.get("last_message") else "")
    else:
        tail = ""
    return "Lần kiểm tra cuối: %s%s" % (when, " · " + tail if tail else "")


# ---------------------------------------------------------------------------
# Environment helpers
# ---------------------------------------------------------------------------
def session_type(environ=None) -> str:
    """"Wayland", "X11" or "" (unknown)."""
    env = os.environ if environ is None else environ
    t = (env.get("XDG_SESSION_TYPE") or "").strip().lower()
    if t == "wayland":
        return "Wayland"
    if t == "x11":
        return "X11"
    if env.get("WAYLAND_DISPLAY"):
        return "Wayland"
    if env.get("DISPLAY"):
        return "X11"
    return ""


_MOD_NAMES = {
    "control": "Ctrl", "ctrl": "Ctrl", "primary": "Ctrl", "shift": "Shift", "alt": "Alt",
    "mod1": "Alt", "super": "Super", "meta": "Meta", "hyper": "Hyper", "mod4": "Super",
}


def format_accelerator(accel: str) -> str:
    """GTK accelerator ("<Control>space", "<Super><Shift>a") -> "Ctrl+Space", "Super+Shift+A"."""
    if not accel:
        return ""
    mods = re.findall(r"<([^>]+)>", accel)
    key = re.sub(r"<[^>]+>", "", accel).strip()
    parts = [_MOD_NAMES.get(m.lower(), m.capitalize()) for m in mods]
    if key:
        if len(key) == 1:
            parts.append(key.upper())
        elif key.lower() == "space":
            parts.append("Space")
        else:
            parts.append(key[0].upper() + key[1:])
    return "+".join(parts)


def ibus_daemon_running() -> bool:
    import shutil
    import subprocess
    if shutil.which("pgrep"):
        try:
            r = subprocess.run(["pgrep", "-x", "ibus-daemon"], stdout=subprocess.DEVNULL,
                               stderr=subprocess.DEVNULL, timeout=2)
            return r.returncode == 0
        except (OSError, subprocess.SubprocessError):
            pass
    if shutil.which("ibus"):
        try:
            r = subprocess.run(["ibus", "address"], capture_output=True, text=True, timeout=2)
            out = r.stdout.strip()
            return r.returncode == 0 and bool(out) and out != "(null)"
        except (OSError, subprocess.SubprocessError):
            pass
    return False


def ibus_version() -> str:
    import subprocess
    try:
        r = subprocess.run(["ibus", "version"], capture_output=True, text=True, timeout=2)
    except (OSError, subprocess.SubprocessError):
        return ""
    return r.stdout.strip() if r.returncode == 0 else ""


def os_pretty_name(path: str = "/etc/os-release") -> str:
    try:
        with open(path, "r", encoding="utf-8") as f:
            for line in f:
                if line.startswith("PRETTY_NAME="):
                    return line.split("=", 1)[1].strip().strip('"')
    except OSError:
        pass
    return ""


def format_system_info(info: dict) -> str:
    labels = [
        ("viskey", "VisKey"),
        ("distro", "Hệ điều hành"),
        ("desktop", "Môi trường"),
        ("session", "Phiên hiển thị"),
        ("ibus", "IBus"),
        ("gtk", "GTK"),
        ("adwaita", "libadwaita"),
        ("python", "Python"),
    ]
    lines = []
    for key, label in labels:
        value = info.get(key) or "không rõ"
        lines.append("%s: %s" % (label, value))
    return "\n".join(lines)
