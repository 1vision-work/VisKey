// VisKey for Windows - small immediate-mode widget kit on top of Gfx (MIT License, 1VISION).
#include "widgets.h"

#include <algorithm>

namespace vk {

void Ui::Add(int id, const RectF& r, bool focusable) {
    if (!id) return;
    if (r.bottom <= clip.top || r.top >= clip.bottom || r.right <= clip.left || r.left >= clip.right) return;
    Hit h;
    h.id = id;
    h.r = R(std::max(r.left, clip.left), std::max(r.top, clip.top), std::min(r.right, clip.right), std::min(r.bottom, clip.bottom));
    h.focusable = focusable;
    hits.push_back(h);
}

int Ui::HitTest(float x, float y) const {
    for (auto it = hits.rbegin(); it != hits.rend(); ++it) {
        if (Contains(it->r, x, y)) return it->id;
    }
    return 0;
}

const Hit* Ui::Find(int id) const {
    for (const auto& h : hits) {
        if (h.id == id) return &h;
    }
    return nullptr;
}

int Ui::NextFocus(int dir) const {
    std::vector<int> order;
    for (const auto& h : hits) {
        if (h.focusable && std::find(order.begin(), order.end(), h.id) == order.end()) order.push_back(h.id);
    }
    if (order.empty()) return 0;
    auto it = std::find(order.begin(), order.end(), focus);
    if (it == order.end()) return dir > 0 ? order.front() : order.back();
    int idx = (int)(it - order.begin()) + (dir > 0 ? 1 : -1);
    if (idx < 0) idx = (int)order.size() - 1;
    if (idx >= (int)order.size()) idx = 0;
    return order[idx];
}

void Ui::FocusRing(const RectF& r, float radius, int id) {
    if (!Focused(id)) return;
    g->StrokeRound(Inset(r, -3.f, -3.f), radius + 3.f, t->focus, 2.f);
}

void Ui::Card(const RectF& r, bool hover, bool highlight) {
    g->FillRound(r, 8.f, hover ? t->cardHover : t->card);
    g->StrokeRound(r, 8.f, highlight ? t->accent : t->cardStroke, highlight ? 1.5f : 1.f);
}

void Ui::Toggle(int id, float right, float cy, bool on, bool enabled, bool label) {
    RectF sw = R(right - 40.f, cy - 10.f, right, cy + 10.f);
    bool hov = Hot(id) && enabled;
    float alpha = enabled ? 1.f : 0.4f;
    if (on) {
        Color fill = t->accentFill;
        if (hov) fill = Mix(fill, t->dark ? Rgb(0xFFFFFF) : Rgb(0x000000), 0.08f);
        g->FillRound(sw, 10.f, WithAlpha(fill, alpha));
        float k = hov ? 7.f : 6.f;
        g->FillCircle(sw.right - 10.f, cy, k, WithAlpha(t->toggleOnKnob, alpha));
    } else {
        if (hov) g->FillRound(sw, 10.f, t->navHover);
        g->StrokeRound(sw, 10.f, WithAlpha(t->toggleOffStroke, alpha), 1.f);
        float k = hov ? 6.f : 5.f;
        g->FillCircle(sw.left + 10.f, cy, k, WithAlpha(t->toggleOffKnob, alpha));
    }
    RectF hit = sw;
    if (label) {
        RectF lr = R(right - 40.f - 12.f - 40.f, cy - 12.f, right - 40.f - 12.f, cy + 12.f);
        g->Text(on ? L"Bật" : L"Tắt", lr, TS(14.f), enabled ? t->text : t->text3, HAlign::Right);
        hit.left = lr.left;
    }
    FocusRing(sw, 10.f, id);
    if (enabled) Add(id, Inset(hit, 0, -6.f));
}

void Ui::Dropdown(int id, const RectF& r, const std::wstring& text) {
    Color bg = Pressed(id) ? t->controlPressed : Hot(id) ? t->controlHover : t->control;
    g->FillRound(r, 5.f, bg);
    g->StrokeRound(r, 5.f, t->controlStroke);
    g->Line(r.left + 4.f, r.bottom - .5f, r.right - 4.f, r.bottom - .5f, WithAlpha(t->controlBottom, 0.6f), 1.f);
    g->Text(text, R(r.left + 12.f, r.top, r.right - 34.f, r.bottom), TS(14.f), t->text);
    g->DrawIcon(Icon::ChevronDown, r.right - 18.f, Cy(r), 12.f, t->text2);
    FocusRing(r, 5.f, id);
    Add(id, r);
}

float Ui::ButtonWidth(const std::wstring& text, Icon icon, float size) {
    float w = g->TextWidth(text, TS(size, Weight::Medium)) + 32.f;
    if (icon != Icon::None) w += text.empty() ? 0.f : 24.f;
    if (text.empty()) w = 36.f;
    return w;
}

void Ui::Button(int id, const RectF& r, const std::wstring& text, Btn kind, Icon icon, bool enabled) {
    bool hov = enabled && Hot(id), down = enabled && Pressed(id);
    Color bg = t->control, fg = t->text, stroke = t->controlStroke;
    switch (kind) {
    case Btn::Accent:
        bg = t->accentFill;
        fg = t->onAccentFill;
        stroke = WithAlpha(bg, 0);
        if (hov) bg = Mix(bg, t->dark ? Rgb(0xFFFFFF) : Rgb(0x000000), 0.08f);
        if (down) bg = Mix(bg, t->dark ? Rgb(0x000000) : Rgb(0xFFFFFF), 0.15f);
        break;
    case Btn::Subtle:
        bg = down ? t->controlPressed : hov ? t->navSel : WithAlpha(t->control, 0);
        stroke = WithAlpha(stroke, 0);
        fg = t->text;
        break;
    case Btn::Danger:
        bg = t->errorTint;
        fg = t->error;
        stroke = WithAlpha(stroke, 0);
        if (hov) bg = Mix(bg, t->error, 0.12f);
        break;
    case Btn::DangerSubtle:
        bg = down ? t->controlPressed : hov ? t->errorTint : t->control;
        fg = t->error;
        stroke = WithAlpha(stroke, 0);
        break;
    case Btn::Standard:
    default:
        bg = down ? t->controlPressed : hov ? t->controlHover : t->control;
        break;
    }
    if (!enabled) {
        bg = WithAlpha(bg, bg.a * 0.5f);
        fg = t->text3;
    }
    g->FillRound(r, 5.f, bg);
    g->StrokeRound(r, 5.f, stroke);
    float iconW = (icon != Icon::None) ? 24.f : 0.f;
    float tw = text.empty() ? 0.f : g->TextWidth(text, TS(14.f, Weight::Medium));
    float total = iconW + tw;
    float x = Cx(r) - total * .5f;
    if (icon != Icon::None) g->DrawIcon(icon, x + (text.empty() ? 12.f : 8.f), Cy(r), 16.f, fg);
    if (!text.empty()) g->Text(text, R(x + iconW, r.top, x + iconW + tw + 2.f, r.bottom), TS(14.f, Weight::Medium), fg);
    FocusRing(r, 5.f, id);
    if (enabled) Add(id, r);
}

void Ui::IconButton(int id, const RectF& r, Icon icon, Color c, float iconSize, bool focusable) {
    if (Pressed(id)) g->FillRound(r, 5.f, t->controlPressed);
    else if (Hot(id)) g->FillRound(r, 5.f, t->navSel);
    g->DrawIcon(icon, Cx(r), Cy(r), iconSize, c);
    FocusRing(r, 5.f, id);
    Add(id, r, focusable);
}

void Ui::Segmented(int idBase, const RectF& r, const std::vector<std::wstring>& items, int sel, bool accentSel, float textSize) {
    g->FillRound(r, 6.f, t->segmentBg);
    if (!t->dark) g->StrokeRound(r, 6.f, t->controlStroke);
    int n = (int)items.size();
    if (!n) return;
    float w = (Wd(r) - 6.f) / (float)n;
    for (int i = 0; i < n; ++i) {
        RectF cell = R(r.left + 3.f + w * (float)i, r.top + 3.f, r.left + 3.f + w * (float)(i + 1), r.bottom - 3.f);
        int id = idBase + i;
        bool selected = (i == sel);
        Color fg = t->text2;
        if (selected) {
            if (accentSel) {
                g->FillRound(cell, 4.f, t->accentFill);
                fg = t->onAccentFill;
            } else {
                g->FillRound(cell, 4.f, t->segmentSel);
                if (!t->dark) g->StrokeRound(cell, 4.f, t->controlStroke);
                fg = t->text;
            }
        } else if (Hot(id)) {
            g->FillRound(cell, 4.f, t->navHover);
            fg = t->text;
        }
        g->Text(items[i], cell, TS(textSize, selected ? Weight::Semibold : Weight::Regular), fg, HAlign::Center);
        FocusRing(cell, 4.f, id);
        Add(id, cell);
    }
}

float Ui::KeycapsWidth(const std::vector<std::wstring>& parts, float size) {
    float w = 0;
    for (size_t i = 0; i < parts.size(); ++i) {
        w += g->TextWidth(parts[i], TS(size, Weight::Regular, Face::Mono)) + 12.f;
        if (i + 1 < parts.size()) w += 16.f;
    }
    return w;
}

float Ui::Keycaps(float x, float cy, const std::vector<std::wstring>& parts, float size, bool accent) {
    for (size_t i = 0; i < parts.size(); ++i) {
        float tw = g->TextWidth(parts[i], TS(size, Weight::Regular, Face::Mono));
        RectF cap = R(x, cy - (size + 10.f) * .5f, x + tw + 12.f, cy + (size + 10.f) * .5f);
        g->FillRound(cap, 4.f, accent ? t->accentTint : t->keycap);
        g->StrokeRound(cap, 4.f, accent ? WithAlpha(t->accentTint, 0) : t->keycapStroke);
        g->Text(parts[i], cap, TS(size, Weight::Regular, Face::Mono), accent ? t->accentTintText : t->text, HAlign::Center);
        x = cap.right;
        if (i + 1 < parts.size()) {
            g->Text(L"+", R(x, cap.top, x + 16.f, cap.bottom), TS(size), t->text2, HAlign::Center);
            x += 16.f;
        }
    }
    return x;
}

RectF Ui::FieldTextRect(const RectF& r, Icon trailing) {
    return R(r.left + 12.f, r.top + 2.f, r.right - (trailing != Icon::None ? 36.f : 10.f), r.bottom - 3.f);
}

void Ui::Field(int id, const RectF& r, const std::wstring& text, const std::wstring& placeholder, bool active, bool mono, Icon trailing) {
    Color bg = active ? t->fieldActive : Hot(id) ? t->controlHover : t->control;
    g->FillRound(r, 5.f, bg);
    g->StrokeRound(r, 5.f, t->controlStroke);
    if (active) {
        g->FillRound(R(r.left + 1.f, r.bottom - 2.f, r.right - 1.f, r.bottom), 1.f, t->accent);
    } else {
        g->Line(r.left + 4.f, r.bottom - .5f, r.right - 4.f, r.bottom - .5f, t->controlBottom, 1.f);
    }
    RectF tr = FieldTextRect(r, trailing);
    TextStyle st = TS(14.f, Weight::Regular, mono ? Face::Mono : Face::Ui);
    if (!active || !live) {
        if (text.empty()) g->Text(placeholder, tr, TS(14.f), t->text2);
        else g->Text(text, tr, st, t->text);
    }
    if (trailing != Icon::None) g->DrawIcon(trailing, r.right - 20.f, Cy(r), 14.f, t->text2);
    FocusRing(r, 5.f, id);
    Add(id, r);
}

float Ui::ChipWidth(const std::wstring& left, const std::wstring& right, bool mono) {
    float w = g->TextWidth(left, TS(13.f, Weight::Regular, mono ? Face::Mono : Face::Ui)) + 28.f;
    if (!right.empty()) w += g->TextWidth(right, TS(12.f, Weight::Bold)) + 10.f;
    return w;
}

void Ui::Chip(const RectF& r, const std::wstring& left, const std::wstring& right, Color bg, Color fg, bool mono) {
    g->FillRound(r, Ht(r) * .5f, bg);
    TextStyle st = TS(13.f, Weight::Regular, mono ? Face::Mono : Face::Ui);
    float lw = g->TextWidth(left, st);
    g->Text(left, R(r.left + 14.f, r.top, r.left + 14.f + lw + 2.f, r.bottom), st, fg);
    if (!right.empty()) g->Text(right, R(r.left + 14.f + lw + 8.f, r.top, r.right - 8.f, r.bottom), TS(12.f, Weight::Bold), fg);
}

// ---------------------------------------------------------------------------------------------
// Popup list window
namespace {

struct PopupState {
    HWND hwnd = nullptr;
    HWND owner = nullptr;
    int tag = 0;
    std::vector<PopupItem> items;
    int selected = -1;
    int hot = -1;
    bool dark = false;
    bool done = false;
    ID2D1HwndRenderTarget* rt = nullptr;
    DWORD closedTick = 0;
    int closedTag = 0;
};
PopupState g_popup;

const float kItemH = 36.f;
const float kPad = 4.f;

float PopupScale() { return (float)DpiForWindow(g_popup.hwnd) / 96.f; }

void PopupFinish(int index) {
    if (g_popup.done) return;
    g_popup.done = true;
    g_popup.closedTick = GetTickCount();
    g_popup.closedTag = g_popup.tag;
    PostMessageW(g_popup.owner, WM_VK_POPUP, (WPARAM)g_popup.tag, (LPARAM)index);
    PostMessageW(g_popup.hwnd, WM_CLOSE, 0, 0);
}

void PopupPaint(HWND hwnd) {
    PAINTSTRUCT ps;
    BeginPaint(hwnd, &ps);
    RECT rc;
    GetClientRect(hwnd, &rc);
    if (!g_popup.rt) {
        float dpi = (float)DpiForWindow(hwnd);
        D2D1_RENDER_TARGET_PROPERTIES p = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT,
                                                                       D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_UNKNOWN), dpi, dpi);
        D2D()->CreateHwndRenderTarget(p, D2D1::HwndRenderTargetProperties(hwnd, D2D1::SizeU((UINT32)rc.right, (UINT32)rc.bottom)), &g_popup.rt);
    }
    if (g_popup.rt) {
        const Theme& t = GetTheme(g_popup.dark);
        g_popup.rt->BeginDraw();
        {
            Gfx g(g_popup.rt);
            float s = PopupScale();
            float w = (float)rc.right / s, h = (float)rc.bottom / s;
            g.Clear(t.dark ? Rgb(0x2C2E34) : Rgb(0xFBFBFC));
            g.StrokeRound(R(0, 0, w, h), 0.f, t.dark ? Rgb(0x3A3E46) : Rgb(0xDADDE4));
            for (int i = 0; i < (int)g_popup.items.size(); ++i) {
                RectF ir = R(kPad, kPad + kItemH * (float)i, w - kPad, kPad + kItemH * (float)(i + 1));
                bool sel = i == g_popup.selected;
                if (i == g_popup.hot) g.FillRound(Inset(ir, 0, 2.f), 4.f, t.navSel);
                else if (sel) g.FillRound(Inset(ir, 0, 2.f), 4.f, t.navHover);
                if (sel) g.FillRound(R(ir.left + 1.f, Cy(ir) - 8.f, ir.left + 4.f, Cy(ir) + 8.f), 1.5f, t.accent);
                g.Text(g_popup.items[i].text, R(ir.left + 14.f, ir.top, ir.right - 10.f, ir.bottom), TS(14.f),
                       g_popup.items[i].danger ? t.error : t.text);
            }
        }
        if (g_popup.rt->EndDraw() == (HRESULT)D2DERR_RECREATE_TARGET) SafeRelease(g_popup.rt);
    }
    EndPaint(hwnd, &ps);
}

LRESULT CALLBACK PopupProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_PAINT:
        PopupPaint(hwnd);
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_MOUSEMOVE: {
        float s = PopupScale();
        float y = (float)(short)HIWORD(lp) / s;
        int idx = (int)((y - kPad) / kItemH);
        if (y < kPad || idx >= (int)g_popup.items.size()) idx = -1;
        if (idx != g_popup.hot) {
            g_popup.hot = idx;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    }
    case WM_LBUTTONUP:
        if (g_popup.hot >= 0) PopupFinish(g_popup.hot);
        return 0;
    case WM_KEYDOWN: {
        int n = (int)g_popup.items.size();
        int cur = g_popup.hot >= 0 ? g_popup.hot : g_popup.selected;
        if (wp == VK_ESCAPE) PopupFinish(-1);
        else if (wp == VK_RETURN || wp == VK_SPACE) PopupFinish(cur);
        else if (wp == VK_DOWN || wp == VK_UP) {
            if (n) cur = (cur < 0) ? 0 : (cur + (wp == VK_DOWN ? 1 : n - 1)) % n;
            g_popup.hot = cur;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    }
    case WM_ACTIVATE:
        if (LOWORD(wp) == WA_INACTIVE) PopupFinish(-1);
        return 0;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        SafeRelease(g_popup.rt);
        g_popup.hwnd = nullptr;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

}  // namespace

bool PopupRecentlyClosed(int tag) { return g_popup.closedTag == tag && GetTickCount() - g_popup.closedTick < 250; }

void ShowPopupList(HWND owner, int tag, const RECT& anchorPx, const std::vector<PopupItem>& items, int selected, bool dark) {
    if (g_popup.hwnd) DestroyWindow(g_popup.hwnd);
    static bool registered = false;
    HINSTANCE inst = GetModuleHandleW(nullptr);
    if (!registered) {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.style = CS_DROPSHADOW;
        wc.lpfnWndProc = PopupProc;
        wc.hInstance = inst;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.lpszClassName = L"VisKeyPopupList";
        RegisterClassExW(&wc);
        registered = true;
    }
    g_popup.owner = owner;
    g_popup.tag = tag;
    g_popup.items = items;
    g_popup.selected = selected;
    g_popup.hot = -1;
    g_popup.dark = dark;
    g_popup.done = false;

    float s = (float)DpiForWindow(owner) / 96.f;
    // Width: at least the anchor, wide enough for the longest item.
    float maxText = 0;
    {
        HDC dc = GetDC(owner);
        HFONT font = CreateFontW(-(int)(14 * s), 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
        HGDIOBJ old = SelectObject(dc, font);
        for (const auto& it : items) {
            SIZE sz = {};
            GetTextExtentPoint32W(dc, it.text.c_str(), (int)it.text.size(), &sz);
            maxText = std::max(maxText, (float)sz.cx);
        }
        SelectObject(dc, old);
        DeleteObject(font);
        ReleaseDC(owner, dc);
    }
    int w = std::max((int)(anchorPx.right - anchorPx.left), (int)(maxText + 44 * s));
    int h = (int)((kItemH * (float)items.size() + kPad * 2) * s);
    int x = anchorPx.left, y = anchorPx.bottom + (int)(4 * s);
    MONITORINFO mi = {};
    mi.cbSize = sizeof(mi);
    GetMonitorInfoW(MonitorFromRect(&anchorPx, MONITOR_DEFAULTTONEAREST), &mi);
    if (y + h > mi.rcWork.bottom) y = anchorPx.top - h - (int)(4 * s);
    if (x + w > mi.rcWork.right) x = mi.rcWork.right - w;
    if (x < mi.rcWork.left) x = mi.rcWork.left;

    g_popup.hwnd = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST, L"VisKeyPopupList", L"", WS_POPUP, x, y, w, h, owner, nullptr, inst, nullptr);
    if (!g_popup.hwnd) return;
    ApplyWindowFrameTheme(g_popup.hwnd, dark, true);
    ShowWindow(g_popup.hwnd, SW_SHOW);
    SetForegroundWindow(g_popup.hwnd);
    SetFocus(g_popup.hwnd);
}

}  // namespace vk
