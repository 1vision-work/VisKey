#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
VisKey for Windows — Zero-Compile Portable Edition (Win32 ctypes + Tkinter GUI)
Runs directly on Windows 10 / Windows 11 using standard Python without needing a C++ compiler!
Uses Win32 SetWindowsHookExW(WH_KEYBOARD_LL) and atomic SendInput(KEYEVENTF_UNICODE).
"""
import ctypes
from ctypes import wintypes
import json
import os
import sys
import threading
import tkinter as tk
from tkinter import messagebox, ttk

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
CORE_DIR = os.path.abspath(os.path.join(SCRIPT_DIR, "..", "core"))
if CORE_DIR not in sys.path:
    sys.path.insert(0, CORE_DIR)

from viskey_engine import EngineConfig, VisKeyEngine

CONFIG_DIR = os.path.join(os.environ.get("APPDATA", os.path.expanduser("~")), "VisKey")
CONFIG_PATH = os.path.join(CONFIG_DIR, "config.json")

# Win32 Constants
WH_KEYBOARD_LL = 13
WM_KEYDOWN = 0x0100
WM_SYSKEYDOWN = 0x0104
VK_BACK = 0x08
VK_TAB = 0x09
VK_RETURN = 0x0D
VK_SHIFT = 0x10
VK_CONTROL = 0x11
VK_MENU = 0x12
VK_CAPITAL = 0x14
VK_ESCAPE = 0x1B
VK_SPACE = 0x20
VK_PRIOR = 0x21
VK_NEXT = 0x22
VK_END = 0x23
VK_HOME = 0x24
VK_LEFT = 0x25
VK_UP = 0x26
VK_RIGHT = 0x27
VK_DOWN = 0x28
VK_LWIN = 0x5B
VK_RWIN = 0x5C

LLKHF_INJECTED = 0x00000010
INPUT_KEYBOARD = 1
KEYEVENTF_KEYUP = 0x0002
KEYEVENTF_UNICODE = 0x0004
VISKEY_MAGIC = 0x564B5821


class KBDLLHOOKSTRUCT(ctypes.Structure):
    _fields_ = [
        ("vkCode", wintypes.DWORD),
        ("scanCode", wintypes.DWORD),
        ("flags", wintypes.DWORD),
        ("time", wintypes.DWORD),
        ("dwExtraInfo", ctypes.c_ulonglong if ctypes.sizeof(ctypes.c_void_p) == 8 else wintypes.ULONG),
    ]


class KEYBDINPUT(ctypes.Structure):
    _fields_ = [
        ("wVk", wintypes.WORD),
        ("wScan", wintypes.WORD),
        ("dwFlags", wintypes.DWORD),
        ("time", wintypes.DWORD),
        ("dwExtraInfo", ctypes.c_ulonglong if ctypes.sizeof(ctypes.c_void_p) == 8 else wintypes.ULONG),
    ]


class INPUT_UNION(ctypes.Union):
    _fields_ = [("ki", KEYBDINPUT), ("padding", ctypes.c_byte * 32)]


class INPUT(ctypes.Structure):
    _fields_ = [("type", wintypes.DWORD), ("u", INPUT_UNION)]


HOOKPROC = ctypes.WINFUNCTYPE(ctypes.c_ssize_t, ctypes.c_int, wintypes.WPARAM, wintypes.LPARAM)


class WindowsHookRunner:
    def __init__(self, engine: VisKeyEngine, on_status_change=None):
        self.engine = engine
        self.vietnamese_enabled = True
        self.on_status_change = on_status_change
        self.hook_id = None
        self._hook_proc_ref = None

    def send_replacement(self, backspaces: int, commit_text: str, need_ac_fix: bool):
        user32 = ctypes.windll.user32
        inputs = []

        def append_vk(vk: int, down: bool):
            inp = INPUT()
            inp.type = INPUT_KEYBOARD
            inp.u.ki.wVk = vk
            inp.u.ki.wScan = 0
            inp.u.ki.dwFlags = 0 if down else KEYEVENTF_KEYUP
            inp.u.ki.dwExtraInfo = VISKEY_MAGIC
            inputs.append(inp)

        def append_unicode(ch_code: int, down: bool):
            inp = INPUT()
            inp.type = INPUT_KEYBOARD
            inp.u.ki.wVk = 0
            inp.u.ki.wScan = ch_code
            inp.u.ki.dwFlags = KEYEVENTF_UNICODE | (0 if down else KEYEVENTF_KEYUP)
            inp.u.ki.dwExtraInfo = VISKEY_MAGIC
            inputs.append(inp)

        if need_ac_fix and backspaces > 0:
            append_unicode(0x202F, True)
            append_unicode(0x202F, False)
            append_vk(VK_BACK, True)
            append_vk(VK_BACK, False)

        for _ in range(backspaces):
            append_vk(VK_BACK, True)
            append_vk(VK_BACK, False)

        for ch in commit_text:
            code = ord(ch)
            append_unicode(code, True)
            append_unicode(code, False)

        if inputs:
            arr = (INPUT * len(inputs))(*inputs)
            user32.SendInput(len(inputs), ctypes.byref(arr), ctypes.sizeof(INPUT))

    def _low_level_handler(self, nCode: int, wParam: int, lParam: int) -> int:
        user32 = ctypes.windll.user32
        if nCode != 0:
            return user32.CallNextHookEx(self.hook_id, nCode, wParam, lParam)

        kb = ctypes.cast(lParam, ctypes.POINTER(KBDLLHOOKSTRUCT)).contents
        if (kb.flags & LLKHF_INJECTED) or kb.dwExtraInfo == VISKEY_MAGIC:
            return user32.CallNextHookEx(self.hook_id, nCode, wParam, lParam)

        if wParam not in (WM_KEYDOWN, WM_SYSKEYDOWN):
            return user32.CallNextHookEx(self.hook_id, nCode, wParam, lParam)

        ctrl = (user32.GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0
        alt = (user32.GetAsyncKeyState(VK_MENU) & 0x8000) != 0
        win = ((user32.GetAsyncKeyState(VK_LWIN) | user32.GetAsyncKeyState(VK_RWIN)) & 0x8000) != 0
        shift = (user32.GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0
        caps = (user32.GetKeyState(VK_CAPITAL) & 0x0001) != 0

        # Ctrl + Space or Alt + Z toggles Vietnamese/English
        if (ctrl and not alt and not win and kb.vkCode == VK_SPACE) or (
            alt and not ctrl and not win and kb.vkCode == ord("Z")
        ):
            self.vietnamese_enabled = not self.vietnamese_enabled
            self.engine.reset()
            if self.on_status_change:
                self.on_status_change()
            return 1

        if ctrl or alt or win:
            self.engine.reset()
            return user32.CallNextHookEx(self.hook_id, nCode, wParam, lParam)

        if not self.vietnamese_enabled:
            return user32.CallNextHookEx(self.hook_id, nCode, wParam, lParam)

        if kb.vkCode == VK_BACK:
            res = self.engine.process_backspace()
            if res.handled:
                self.send_replacement(res.backspaces, res.commit_text, res.autocomplete_fix_needed)
                return 1
            return user32.CallNextHookEx(self.hook_id, nCode, wParam, lParam)

        if kb.vkCode in (
            VK_LEFT, VK_RIGHT, VK_UP, VK_DOWN, VK_HOME, VK_END,
            VK_PRIOR, VK_NEXT, VK_ESCAPE, VK_TAB, VK_RETURN
        ):
            self.engine.reset()
            return user32.CallNextHookEx(self.hook_id, nCode, wParam, lParam)

        ch = ""
        if 0x41 <= kb.vkCode <= 0x5A:
            is_upper = shift ^ caps
            ch = chr(kb.vkCode) if is_upper else chr(kb.vkCode + 32)
        elif not shift and 0x30 <= kb.vkCode <= 0x39:
            ch = chr(kb.vkCode)
        elif kb.vkCode == VK_SPACE:
            ch = " "
        else:
            self.engine.reset()
            return user32.CallNextHookEx(self.hook_id, nCode, wParam, lParam)

        res = self.engine.process_key(ch)
        if not res.handled:
            return user32.CallNextHookEx(self.hook_id, nCode, wParam, lParam)

        self.send_replacement(res.backspaces, res.commit_text, res.autocomplete_fix_needed)
        return 1

    def start_hook_thread(self):
        def _loop():
            user32 = ctypes.windll.user32
            kernel32 = ctypes.windll.kernel32
            self._hook_proc_ref = HOOKPROC(self._low_level_handler)
            self.hook_id = user32.SetWindowsHookExW(
                WH_KEYBOARD_LL, self._hook_proc_ref, kernel32.GetModuleHandleW(None), 0
            )
            msg = wintypes.MSG()
            while user32.GetMessageW(ctypes.byref(msg), None, 0, 0) != 0:
                user32.TranslateMessage(ctypes.byref(msg))
                user32.DispatchMessageW(ctypes.byref(msg))

        t = threading.Thread(target=_loop, daemon=True)
        t.start()


class VisKeyWindowsGUI:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("VisKey — Bộ Gõ Tiếng Việt Cho Windows")
        self.root.geometry("540x480")

        self.cfg = EngineConfig(
            input_method="telex",
            modern_orthography=False,
            auto_restore_english=True,
            quick_telex=False,
            enable_macros=True,
            fix_autocomplete=True,
        )
        self.engine = VisKeyEngine(self.cfg)
        self.hook = WindowsHookRunner(self.engine, on_status_change=self._update_status_badge)
        if sys.platform == "win32":
            self.hook.start_hook_thread()

        self._build_ui()

    def _build_ui(self):
        main = ttk.Frame(self.root, padding=16)
        main.pack(fill=tk.BOTH, expand=True)

        top = ttk.Frame(main)
        top.pack(fill=tk.X, pady=(0, 12))
        ttk.Label(top, text="Bảng Điều Khiển VisKey (Windows 10/11)", font=("Segoe UI", 14, "bold")).pack(side=tk.LEFT)
        self.status_btn = tk.Button(
            top, text="🇻🇳 ĐANG BẬT: TIẾNG VIỆT (VI)", bg="#1b7f37", fg="white",
            font=("Segoe UI", 10, "bold"), command=self.toggle_mode
        )
        self.status_btn.pack(side=tk.RIGHT)

        im_frame = ttk.Frame(main)
        im_frame.pack(fill=tk.X, pady=4)
        ttk.Label(im_frame, text="Kiểu gõ (Ctrl+Space đổi VI/EN):", width=28).pack(side=tk.LEFT)
        self.im_var = tk.StringVar(value=self.cfg.input_method)
        ttk.Radiobutton(im_frame, text="Telex (Chuẩn)", variable=self.im_var, value="telex", command=self.apply_settings).pack(side=tk.LEFT, padx=8)
        ttk.Radiobutton(im_frame, text="VNI", variable=self.im_var, value="vni", command=self.apply_settings).pack(side=tk.LEFT, padx=8)

        self.restore_var = tk.BooleanVar(value=self.cfg.auto_restore_english)
        ttk.Checkbutton(main, text="Tự động khôi phục từ tiếng Anh khi gõ song ngữ (window, software...)", variable=self.restore_var, command=self.apply_settings).pack(anchor=tk.W, pady=3)

        self.ac_var = tk.BooleanVar(value=self.cfg.fix_autocomplete)
        ttk.Checkbutton(main, text="Sửa lỗi mất/nhân đôi chữ trên Chrome, Edge, Windows Search, Excel", variable=self.ac_var, command=self.apply_settings).pack(anchor=tk.W, pady=3)

        self.ortho_var = tk.BooleanVar(value=self.cfg.modern_orthography)
        ttk.Checkbutton(main, text="Đặt dấu kiểu mới (hoà, thuý thay vì hòa, thúy)", variable=self.ortho_var, command=self.apply_settings).pack(anchor=tk.W, pady=3)

        self.macro_var = tk.BooleanVar(value=self.cfg.enable_macros)
        ttk.Checkbutton(main, text="Bật bảng Gõ tắt (Macro) & Quick Telex", variable=self.macro_var, command=self.apply_settings).pack(anchor=tk.W, pady=3)

        ttk.Label(main, text="Quản lý từ Gõ tắt (Macro):", font=("Segoe UI", 10, "bold")).pack(anchor=tk.W, pady=(10, 4))
        self.tree = ttk.Treeview(main, columns=("k", "v"), show="headings", height=6)
        self.tree.heading("k", text="Từ viết tắt")
        self.tree.heading("v", text="Nội dung thay thế")
        self.tree.column("k", width=130)
        self.tree.column("v", width=360)
        self.tree.pack(fill=tk.BOTH, expand=True)
        self._refresh_macros()

        add_f = ttk.Frame(main)
        add_f.pack(fill=tk.X, pady=8)
        self.ent_k = ttk.Entry(add_f, width=14)
        self.ent_k.pack(side=tk.LEFT, padx=(0, 6))
        self.ent_v = ttk.Entry(add_f, width=30)
        self.ent_v.pack(side=tk.LEFT, padx=(0, 6))
        ttk.Button(add_f, text="Thêm / Sửa", command=self.add_macro).pack(side=tk.LEFT, padx=4)
        ttk.Button(add_f, text="Xóa mục chọn", command=self.del_macro).pack(side=tk.LEFT, padx=4)

    def _update_status_badge(self):
        if self.hook.vietnamese_enabled:
            self.status_btn.configure(text="🇻🇳 ĐANG BẬT: TIẾNG VIỆT (VI)", bg="#1b7f37")
        else:
            self.status_btn.configure(text="🇬🇧 ĐANG BẬT: TIẾNG ANH (EN)", bg="#4a5568")

    def toggle_mode(self):
        self.hook.vietnamese_enabled = not self.hook.vietnamese_enabled
        self.engine.reset()
        self._update_status_badge()

    def apply_settings(self):
        self.cfg.input_method = self.im_var.get()
        self.cfg.auto_restore_english = self.restore_var.get()
        self.cfg.fix_autocomplete = self.ac_var.get()
        self.cfg.modern_orthography = self.ortho_var.get()
        self.cfg.enable_macros = self.macro_var.get()
        self.cfg.quick_telex = self.macro_var.get()
        self.engine.reset()

    def _refresh_macros(self):
        for item in self.tree.get_children():
            self.tree.delete(item)
        for k, v in sorted(self.cfg.macros.items()):
            self.tree.insert("", tk.END, values=(k, v))

    def add_macro(self):
        k = self.ent_k.get().strip().lower()
        v = self.ent_v.get().strip()
        if k and v:
            self.cfg.macros[k] = v
            self.ent_k.delete(0, tk.END)
            self.ent_v.delete(0, tk.END)
            self._refresh_macros()

    def del_macro(self):
        for sel in self.tree.selection():
            vals = self.tree.item(sel, "values")
            if vals:
                self.cfg.macros.pop(str(vals[0]), None)
        self._refresh_macros()


if __name__ == "__main__":
    root = tk.Tk()
    app = VisKeyWindowsGUI(root)
    root.mainloop()
