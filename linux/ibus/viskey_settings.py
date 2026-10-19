#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
VisKey Settings & Macro Manager GUI for Linux
Saves configuration to ~/.config/viskey/config.json, which is automatically read by ibus_viskey.py.
"""
import json
import os
import sys
import tkinter as tk
from tkinter import messagebox, ttk

CONFIG_DIR = os.path.expanduser("~/.config/viskey")
CONFIG_PATH = os.path.join(CONFIG_DIR, "config.json")

DEFAULT_CONFIG = {
    "vietnamese_enabled": True,
    "input_method": "telex",
    "no_underline_mode": True,
    "auto_restore_english": True,
    "modern_orthography": False,
    "quick_telex": False,
    "enable_macros": True,
    "macros": {
        "vn": "Việt Nam",
        "ko": "không",
        "dc": "được",
        "uh": "ừ",
        "ntn": "như thế nào",
        "bt": "bình thường",
        "mn": "mọi người",
    },
}


def load_config() -> dict:
    if os.path.exists(CONFIG_PATH):
        try:
            with open(CONFIG_PATH, "r", encoding="utf-8") as f:
                data = json.load(f)
            merged = dict(DEFAULT_CONFIG)
            merged.update(data)
            return merged
        except Exception:
            pass
    return dict(DEFAULT_CONFIG)


def save_config(cfg: dict) -> None:
    os.makedirs(CONFIG_DIR, exist_ok=True)
    with open(CONFIG_PATH, "w", encoding="utf-8") as f:
        json.dump(cfg, f, ensure_ascii=False, indent=2)


class VisKeySettingsApp:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("VisKey — Bảng Điều Khiển Bộ Gõ Tiếng Việt (Linux)")
        self.root.geometry("540x480")
        self.cfg = load_config()

        main = ttk.Frame(root, padding=16)
        main.pack(fill=tk.BOTH, expand=True)

        title = ttk.Label(main, text="Cấu hình Bộ gõ VisKey (Linux IBus)", font=("Sans", 14, "bold"))
        title.pack(anchor=tk.W, pady=(0, 12))

        # Input Method
        im_frame = ttk.Frame(main)
        im_frame.pack(fill=tk.X, pady=4)
        ttk.Label(im_frame, text="Kiểu gõ:", width=18).pack(side=tk.LEFT)
        self.im_var = tk.StringVar(value=self.cfg["input_method"])
        ttk.Radiobutton(im_frame, text="Telex (Chuẩn)", variable=self.im_var, value="telex").pack(side=tk.LEFT, padx=8)
        ttk.Radiobutton(im_frame, text="VNI", variable=self.im_var, value="vni").pack(side=tk.LEFT, padx=8)

        # Checkboxes
        self.no_ul_var = tk.BooleanVar(value=self.cfg["no_underline_mode"])
        ttk.Checkbutton(
            main,
            text="Chế độ Không gạch chân (Surrounding Text / Fake Backspace)",
            variable=self.no_ul_var,
        ).pack(anchor=tk.W, pady=3)

        self.restore_var = tk.BooleanVar(value=self.cfg["auto_restore_english"])
        ttk.Checkbutton(
            main,
            text="Tự động khôi phục từ tiếng Anh khi gõ song ngữ (window, software...)",
            variable=self.restore_var,
        ).pack(anchor=tk.W, pady=3)

        self.ortho_var = tk.BooleanVar(value=self.cfg["modern_orthography"])
        ttk.Checkbutton(
            main,
            text="Đặt dấu kiểu mới (hoà, thuý thay vì hòa, thúy)",
            variable=self.ortho_var,
        ).pack(anchor=tk.W, pady=3)

        self.macro_var = tk.BooleanVar(value=self.cfg["enable_macros"])
        ttk.Checkbutton(
            main,
            text="Bật bảng Gõ tắt (Macro)",
            variable=self.macro_var,
        ).pack(anchor=tk.W, pady=3)

        self.quick_var = tk.BooleanVar(value=self.cfg["quick_telex"])
        ttk.Checkbutton(
            main,
            text="Bật Quick Telex (cc=ch, gg=gi, kk=kh, nn=ng, qq=qu, pp=ph, tt=th)",
            variable=self.quick_var,
        ).pack(anchor=tk.W, pady=3)

        # Macro Manager
        ttk.Label(main, text="Danh sách từ Gõ tắt (Macro):", font=("Sans", 11, "bold")).pack(anchor=tk.W, pady=(12, 4))

        tree_frame = ttk.Frame(main)
        tree_frame.pack(fill=tk.BOTH, expand=True)

        self.tree = ttk.Treeview(tree_frame, columns=("key", "val"), show="headings", height=6)
        self.tree.heading("key", text="Từ viết tắt")
        self.tree.heading("val", text="Nội dung thay thế")
        self.tree.column("key", width=130)
        self.tree.column("val", width=340)
        self.tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        scrollbar = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.tree.yview)
        self.tree.configure(yscroll=scrollbar.set)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        self._refresh_macro_tree()

        add_frame = ttk.Frame(main)
        add_frame.pack(fill=tk.X, pady=8)
        self.new_k = ttk.Entry(add_frame, width=14)
        self.new_k.pack(side=tk.LEFT, padx=(0, 6))
        self.new_v = ttk.Entry(add_frame, width=28)
        self.new_v.pack(side=tk.LEFT, padx=(0, 6))
        ttk.Button(add_frame, text="Thêm / Sửa", command=self.on_add_macro).pack(side=tk.LEFT, padx=4)
        ttk.Button(add_frame, text="Xóa mục chọn", command=self.on_del_macro).pack(side=tk.LEFT, padx=4)

        btn_frame = ttk.Frame(main)
        btn_frame.pack(fill=tk.X, pady=(8, 0))
        ttk.Button(btn_frame, text="Lưu cấu hình & Áp dụng", command=self.on_save).pack(side=tk.RIGHT)

    def _refresh_macro_tree(self):
        for item in self.tree.get_children():
            self.tree.delete(item)
        for k, v in sorted(self.cfg["macros"].items()):
            self.tree.insert("", tk.END, values=(k, v))

    def on_add_macro(self):
        k = self.new_k.get().strip().lower()
        v = self.new_v.get().strip()
        if not k or not v:
            return
        self.cfg["macros"][k] = v
        self.new_k.delete(0, tk.END)
        self.new_v.delete(0, tk.END)
        self._refresh_macro_tree()

    def on_del_macro(self):
        for sel in self.tree.selection():
            vals = self.tree.item(sel, "values")
            if vals:
                self.cfg["macros"].pop(str(vals[0]), None)
        self._refresh_macro_tree()

    def on_save(self):
        self.cfg["input_method"] = self.im_var.get()
        self.cfg["no_underline_mode"] = self.no_ul_var.get()
        self.cfg["auto_restore_english"] = self.restore_var.get()
        self.cfg["modern_orthography"] = self.ortho_var.get()
        self.cfg["enable_macros"] = self.macro_var.get()
        self.cfg["quick_telex"] = self.quick_var.get()
        save_config(self.cfg)
        messagebox.showinfo("VisKey", "Đã lưu cấu hình VisKey thành công!")


if __name__ == "__main__":
    root = tk.Tk()
    app = VisKeySettingsApp(root)
    root.mainloop()
