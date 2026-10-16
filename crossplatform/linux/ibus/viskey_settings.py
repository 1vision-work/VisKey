#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
VisKey Settings (Linux)
Tkinter GUI + headless CLI to edit ~/.config/viskey/config.json
(respects $XDG_CONFIG_HOME). The IBus engine reloads this file automatically
whenever its modification time changes.

Headless usage (no display required):
    viskey-settings --print-config
    viskey-settings --set input_method=vni --set quick_telex=true
    viskey-settings --set macros.ko=không      # add / edit a macro
    viskey-settings --set macros.ko=           # delete a macro
"""
import argparse
import dataclasses
import json
import os
import sys
import tempfile
from typing import Any, Dict, List, Optional

# Ensure core module is importable (source tree or installed layout)
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
CORE_DIR = os.path.abspath(os.path.join(SCRIPT_DIR, "..", "..", "core"))
if os.path.isdir(CORE_DIR):
    sys.path.insert(0, CORE_DIR)
elif os.path.isdir("/usr/share/viskey/core"):
    sys.path.insert(0, "/usr/share/viskey/core")

from viskey_engine import EngineConfig

APP_TITLE = "VisKey - Cấu hình Bộ gõ Tiếng Việt"
INPUT_METHODS = ("telex", "vni")

# Boolean options: (key, Vietnamese label). Mirrors EngineConfig + IBus-only no_underline_mode.
BOOL_OPTIONS = [
    ("modern_orthography", "Đặt dấu kiểu mới (hoà, thuý, khoẻ)"),
    ("auto_restore_english", "Tự khôi phục từ tiếng Anh (window, software, book...)"),
    ("quick_telex", "Quick Telex (cc=ch, gg=gi, kk=kh, nn=ng, qq=qu, pp=ph, tt=th)"),
    ("enable_macros", "Bật Gõ tắt (Macro)"),
    ("fix_autocomplete", "Sửa lỗi mất chữ trên ô có gợi ý tự động (Autocomplete)"),
    ("no_underline_mode", "Chế độ Không gạch chân (No Preedit)"),
]
BOOL_KEYS = [key for key, _ in BOOL_OPTIONS]


def config_dir() -> str:
    base = os.environ.get("XDG_CONFIG_HOME") or os.path.join(os.path.expanduser("~"), ".config")
    return os.path.join(base, "viskey")


def config_path() -> str:
    return os.path.join(config_dir(), "config.json")


def default_config() -> Dict[str, Any]:
    cfg = dataclasses.asdict(EngineConfig())
    cfg["no_underline_mode"] = True
    return cfg


def _to_bool(value: Any) -> Optional[bool]:
    if isinstance(value, bool):
        return value
    if isinstance(value, (int, float)):
        return bool(value)
    if isinstance(value, str):
        v = value.strip().lower()
        if v in ("1", "true", "yes", "on", "y"):
            return True
        if v in ("0", "false", "no", "off", "n"):
            return False
    return None


def _clean_macros(value: Any) -> Optional[Dict[str, str]]:
    if not isinstance(value, dict):
        return None
    macros: Dict[str, str] = {}
    for k, v in value.items():
        if not isinstance(k, str) or not isinstance(v, str):
            continue
        key = k.strip().lower()  # engine matches macros on the lowercased raw word
        if key and v:
            macros[key] = v
    return macros


def normalize_config(data: Any) -> Dict[str, Any]:
    """Merge user data over defaults, dropping invalid values. Unknown keys are preserved."""
    cfg = default_config()
    if not isinstance(data, dict):
        return cfg
    for k, v in data.items():
        if k not in cfg:
            cfg[k] = v
    method = data.get("input_method")
    if isinstance(method, str) and method.strip().lower() in INPUT_METHODS:
        cfg["input_method"] = method.strip().lower()
    for key in BOOL_KEYS:
        b = _to_bool(data.get(key))
        if b is not None:
            cfg[key] = b
    macros = _clean_macros(data.get("macros"))
    if macros is not None:
        cfg["macros"] = macros
    return cfg


def load_config(path: Optional[str] = None) -> Dict[str, Any]:
    """Load config.json; falls back to defaults if missing or corrupt."""
    path = path or config_path()
    try:
        with open(path, "r", encoding="utf-8") as f:
            data = json.load(f)
    except (OSError, ValueError):
        data = None
    return normalize_config(data)


def save_config(cfg: Dict[str, Any], path: Optional[str] = None) -> str:
    """Atomically write config.json (tmp file in the same dir + os.replace)."""
    path = path or config_path()
    directory = os.path.dirname(path)
    os.makedirs(directory, exist_ok=True)
    cfg = normalize_config(cfg)
    fd, tmp = tempfile.mkstemp(prefix=".config.", suffix=".json.tmp", dir=directory)
    try:
        os.fchmod(fd, 0o644)  # mkstemp creates 0600
        with os.fdopen(fd, "w", encoding="utf-8") as f:
            json.dump(cfg, f, ensure_ascii=False, indent=2, sort_keys=True)
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
    return path


def apply_setting(cfg: Dict[str, Any], assignment: str) -> None:
    """Apply one `key=value` assignment to cfg. Raises ValueError on invalid input."""
    if "=" not in assignment:
        raise ValueError(f"'{assignment}': expected key=value")
    key, value = assignment.split("=", 1)
    key = key.strip()

    if key.startswith("macros."):
        abbr = key[len("macros."):].strip().lower()
        if not abbr:
            raise ValueError("macro key must not be empty")
        macros = dict(cfg.get("macros") or {})
        if value:
            macros[abbr] = value
        else:
            macros.pop(abbr, None)
        cfg["macros"] = macros
    elif key == "input_method":
        method = value.strip().lower()
        if method not in INPUT_METHODS:
            raise ValueError(f"input_method must be one of: {', '.join(INPUT_METHODS)}")
        cfg["input_method"] = method
    elif key in BOOL_KEYS:
        b = _to_bool(value)
        if b is None:
            raise ValueError(f"{key}: expected a boolean (true/false), got '{value}'")
        cfg[key] = b
    elif key == "macros":
        try:
            macros = _clean_macros(json.loads(value))
        except ValueError:
            macros = None
        if macros is None:
            raise ValueError("macros: expected a JSON object of {\"key\": \"expansion\"}")
        cfg["macros"] = macros
    else:
        valid = ["input_method"] + BOOL_KEYS + ["macros", "macros.<key>"]
        raise ValueError(f"unknown option '{key}'. Valid options: {', '.join(valid)}")


# ---------------------------------------------------------------------------
# Tkinter GUI (tkinter is imported lazily so the module works without DISPLAY)
# ---------------------------------------------------------------------------
def run_gui() -> int:
    try:
        import tkinter as tk
        from tkinter import messagebox, ttk
    except ImportError:
        print("Error: python3-tk is required for the graphical settings window.", file=sys.stderr)
        return 1

    try:
        root = tk.Tk()
    except tk.TclError as exc:
        print(f"Error: cannot open display ({exc}). Use --print-config / --set instead.", file=sys.stderr)
        return 1

    root.title(APP_TITLE)
    root.minsize(560, 560)

    cfg = load_config()
    macros: Dict[str, str] = dict(cfg["macros"])

    method_var = tk.StringVar(value=cfg["input_method"])
    bool_vars = {key: tk.BooleanVar(value=bool(cfg[key])) for key in BOOL_KEYS}
    abbr_var = tk.StringVar()
    expansion_var = tk.StringVar()
    status_var = tk.StringVar(value=f"Tệp cấu hình: {config_path()}")

    main = ttk.Frame(root, padding=12)
    main.pack(fill="both", expand=True)

    ttk.Label(main, text=APP_TITLE, font=("TkDefaultFont", 13, "bold")).pack(anchor="w", pady=(0, 8))

    method_frame = ttk.LabelFrame(main, text="Kiểu gõ", padding=8)
    method_frame.pack(fill="x", pady=4)
    ttk.Radiobutton(method_frame, text="Telex", value="telex", variable=method_var).pack(side="left", padx=(0, 16))
    ttk.Radiobutton(method_frame, text="VNI", value="vni", variable=method_var).pack(side="left")

    opt_frame = ttk.LabelFrame(main, text="Tùy chọn", padding=8)
    opt_frame.pack(fill="x", pady=4)
    for key, label in BOOL_OPTIONS:
        ttk.Checkbutton(opt_frame, text=label, variable=bool_vars[key]).pack(anchor="w", pady=1)

    macro_frame = ttk.LabelFrame(main, text="Quản lý Gõ tắt (Macro)", padding=8)
    macro_frame.pack(fill="both", expand=True, pady=4)

    tree_frame = ttk.Frame(macro_frame)
    tree_frame.pack(fill="both", expand=True)
    tree = ttk.Treeview(tree_frame, columns=("abbr", "expansion"), show="headings", height=8, selectmode="browse")
    tree.heading("abbr", text="Từ gõ tắt")
    tree.heading("expansion", text="Nội dung thay thế")
    tree.column("abbr", width=120, stretch=False)
    tree.column("expansion", width=360)
    scroll = ttk.Scrollbar(tree_frame, orient="vertical", command=tree.yview)
    tree.configure(yscrollcommand=scroll.set)
    tree.pack(side="left", fill="both", expand=True)
    scroll.pack(side="right", fill="y")

    edit_frame = ttk.Frame(macro_frame)
    edit_frame.pack(fill="x", pady=(6, 0))
    ttk.Label(edit_frame, text="Gõ tắt:").grid(row=0, column=0, sticky="w")
    abbr_entry = ttk.Entry(edit_frame, textvariable=abbr_var, width=12)
    abbr_entry.grid(row=0, column=1, sticky="w", padx=(4, 12))
    ttk.Label(edit_frame, text="Thay bằng:").grid(row=0, column=2, sticky="w")
    ttk.Entry(edit_frame, textvariable=expansion_var).grid(row=0, column=3, sticky="ew", padx=(4, 0))
    edit_frame.columnconfigure(3, weight=1)

    def refresh_tree(select: Optional[str] = None) -> None:
        tree.delete(*tree.get_children())
        for abbr in sorted(macros):
            tree.insert("", "end", iid=abbr, values=(abbr, macros[abbr]))
        if select and tree.exists(select):
            tree.selection_set(select)
            tree.see(select)

    def on_select(_event=None) -> None:
        sel = tree.selection()
        if sel:
            abbr_var.set(sel[0])
            expansion_var.set(macros.get(sel[0], ""))

    def add_or_update() -> None:
        abbr = abbr_var.get().strip().lower()
        expansion = expansion_var.get()
        if not abbr or not expansion.strip():
            messagebox.showwarning(APP_TITLE, "Vui lòng nhập cả từ gõ tắt và nội dung thay thế.", parent=root)
            return
        if any(not ch.isalpha() for ch in abbr):
            messagebox.showwarning(APP_TITLE, "Từ gõ tắt chỉ được chứa chữ cái (a-z).", parent=root)
            return
        # Editing an entry under a new abbreviation renames it
        sel = tree.selection()
        if sel and sel[0] != abbr and sel[0] in macros and abbr not in macros:
            del macros[sel[0]]
        macros[abbr] = expansion
        refresh_tree(select=abbr)
        status_var.set(f"Đã cập nhật gõ tắt '{abbr}' (chưa lưu).")

    def delete_macro() -> None:
        sel = tree.selection()
        abbr = sel[0] if sel else abbr_var.get().strip().lower()
        if abbr not in macros:
            return
        del macros[abbr]
        abbr_var.set("")
        expansion_var.set("")
        refresh_tree()
        status_var.set(f"Đã xóa gõ tắt '{abbr}' (chưa lưu).")

    def clear_fields() -> None:
        tree.selection_remove(*tree.selection())
        abbr_var.set("")
        expansion_var.set("")
        abbr_entry.focus_set()

    tree.bind("<<TreeviewSelect>>", on_select)

    btn_row = ttk.Frame(macro_frame)
    btn_row.pack(fill="x", pady=(6, 0))
    ttk.Button(btn_row, text="Thêm / Cập nhật", command=add_or_update).pack(side="left")
    ttk.Button(btn_row, text="Xóa", command=delete_macro).pack(side="left", padx=6)
    ttk.Button(btn_row, text="Mới", command=clear_fields).pack(side="left")

    def collect() -> Dict[str, Any]:
        new_cfg = dict(cfg)
        new_cfg["input_method"] = method_var.get()
        for key in BOOL_KEYS:
            new_cfg[key] = bool(bool_vars[key].get())
        new_cfg["macros"] = dict(macros)
        return new_cfg

    def save() -> None:
        try:
            path = save_config(collect())
        except OSError as exc:
            messagebox.showerror(APP_TITLE, f"Không thể lưu cấu hình:\n{exc}", parent=root)
            return
        status_var.set(f"Đã lưu & áp dụng: {path}")

    def reset_defaults() -> None:
        if not messagebox.askyesno(APP_TITLE, "Khôi phục toàn bộ cấu hình mặc định?", parent=root):
            return
        defaults = default_config()
        method_var.set(defaults["input_method"])
        for key in BOOL_KEYS:
            bool_vars[key].set(defaults[key])
        macros.clear()
        macros.update(defaults["macros"])
        refresh_tree()
        status_var.set("Đã khôi phục mặc định (chưa lưu).")

    bottom = ttk.Frame(main)
    bottom.pack(fill="x", pady=(8, 0))
    ttk.Button(bottom, text="Lưu cấu hình & Áp dụng", command=save).pack(side="right")
    ttk.Button(bottom, text="Đóng", command=root.destroy).pack(side="right", padx=6)
    ttk.Button(bottom, text="Khôi phục mặc định", command=reset_defaults).pack(side="left")

    ttk.Label(main, textvariable=status_var, foreground="gray").pack(anchor="w", pady=(6, 0))

    refresh_tree()
    root.mainloop()
    return 0


def main(argv: Optional[List[str]] = None) -> int:
    parser = argparse.ArgumentParser(prog="viskey-settings", description=APP_TITLE)
    parser.add_argument("--print-config", action="store_true",
                        help="print the effective configuration as JSON and exit")
    parser.add_argument("--set", action="append", default=[], metavar="KEY=VALUE",
                        help="set an option (repeatable); use macros.<key>=<text> to add/edit "
                             "a macro and macros.<key>= to delete it")
    args = parser.parse_args(argv)

    if args.set:
        cfg = load_config()
        try:
            for assignment in args.set:
                apply_setting(cfg, assignment)
        except ValueError as exc:
            print(f"Error: {exc}", file=sys.stderr)
            return 2
        try:
            path = save_config(cfg)
        except OSError as exc:
            print(f"Error: cannot write config: {exc}", file=sys.stderr)
            return 1
        print(f"Saved {path}", file=sys.stderr)

    if args.print_config:
        print(json.dumps(load_config(), ensure_ascii=False, indent=2, sort_keys=True))

    if args.set or args.print_config:
        return 0
    return run_gui()


if __name__ == "__main__":
    sys.exit(main())
