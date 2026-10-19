// VisKey for Windows - tray flyout (left click on the tray icon), MIT License, 1VISION.
//
// Backdrop: on Windows 11 22H2+ DWM draws transient-window acrylic behind the whole client area
// (DWMWA_SYSTEMBACKDROP_TYPE + DwmExtendFrameIntoClientArea(-1)); older builds use the accent
// policy acrylic. In both cases the D2D HWND render target is created with
// B8G8R8A8 + D2D1_ALPHA_MODE_PREMULTIPLIED so the alpha we draw reaches DWM (no DirectComposition,
// no WS_EX_NOREDIRECTIONBITMAP needed), and text uses grayscale antialiasing because ClearType
// needs an opaque destination. Solid mode keeps the original opaque target with ClearType.
#include "views.h"

#include <shellapi.h>

#include <algorithm>

namespace vk {

namespace {

struct Flyout {
    HWND hwnd = nullptr;
    ID2D1HwndRenderTarget* rt = nullptr;
    Ui ui;
    bool dark = false;
    DWORD hiddenTick = 0;
    bool tracking = false;
    Backdrop backdrop = Backdrop::Solid;
};
Flyout g_fly;

typedef HRESULT(WINAPI* PfnNotifyIconGetRect)(const NOTIFYICONIDENTIFIER*, RECT*);

float Scale() { return (float)DpiForWindow(g_fly.hwnd) / 96.f; }

void Hide() {
    if (g_fly.hwnd && IsWindowVisible(g_fly.hwnd)) {
        ShowWindow(g_fly.hwnd, SW_HIDE);
        g_fly.hiddenTick = GetTickCount();
    }
}

void Activate(int id) {
    Model& m = M();
    switch (id) {
    case kIdFlyVi: SetVietnamese(true); break;
    case kIdFlyEn: SetVietnamese(false); break;
    case kIdFlyTelex: m.vni = false; Commit(kChangeSettings); break;
    case kIdFlyVni: m.vni = true; Commit(kChangeSettings); break;
    case kIdFlyTileBase + 0: m.autoRestore = !m.autoRestore; Commit(kChangeSettings); break;
    case kIdFlyTileBase + 1: m.macrosEnabled = !m.macrosEnabled; Commit(kChangeSettings); break;
    case kIdFlyTileBase + 2: m.smartPerApp = !m.smartPerApp; Commit(kChangeSettings); break;
    case kIdFlyTileBase + 3: m.fixAutocomplete = !m.fixAutocomplete; Commit(kChangeSettings); break;
    case kIdFlyTileBase + 4: m.modernOrthography = !m.modernOrthography; Commit(kChangeSettings); break;
    case kIdFlyTileBase + 5: m.quickTelex = !m.quickTelex; Commit(kChangeSettings); break;
    case kIdFlyOpen:
        Hide();
        OpenSettings(-1);
        break;
    case kIdFlyUpdate:
        Hide();
        OpenSettings(kPageUpdate);
        break;
    case kIdFlyQuit:
        Hide();
        QuitApp();
        break;
    default: break;
    }
}

void Paint(HWND hwnd) {
    PAINTSTRUCT ps;
    BeginPaint(hwnd, &ps);
    RECT rc;
    GetClientRect(hwnd, &rc);
    const bool glass = g_fly.backdrop != Backdrop::Solid;
    if (!g_fly.rt) {
        float dpi = (float)DpiForWindow(hwnd);
        D2D1_PIXEL_FORMAT pf = glass ? D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
                                     : D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_UNKNOWN);
        D2D1_RENDER_TARGET_PROPERTIES p = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, pf, dpi, dpi);
        D2D()->CreateHwndRenderTarget(p, D2D1::HwndRenderTargetProperties(hwnd, D2D1::SizeU((UINT32)rc.right, (UINT32)rc.bottom)), &g_fly.rt);
        if (!g_fly.rt && glass) {
            // Translucent target refused: go back to the solid look for this window.
            g_fly.backdrop = Backdrop::Solid;
            ApplyFlyoutBackdrop(hwnd, g_fly.dark, false);
            p.pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_UNKNOWN);
            D2D()->CreateHwndRenderTarget(p, D2D1::HwndRenderTargetProperties(hwnd, D2D1::SizeU((UINT32)rc.right, (UINT32)rc.bottom)), &g_fly.rt);
        }
        if (g_fly.rt && g_fly.backdrop != Backdrop::Solid) g_fly.rt->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
    }
    if (g_fly.rt) {
        g_fly.rt->BeginDraw();
        {
            Gfx g(g_fly.rt);
            g_fly.ui.g = &g;
            g_fly.ui.t = &GetTheme(g_fly.dark);
            float s = Scale();
            PaintFlyout(g_fly.ui, M(), (float)rc.right / s, (float)rc.bottom / s,
                        g_fly.backdrop == Backdrop::Solid ? FlyoutSurface::Solid : FlyoutSurface::Translucent);
            g_fly.ui.g = nullptr;
        }
        if (g_fly.rt->EndDraw() == (HRESULT)D2DERR_RECREATE_TARGET) SafeRelease(g_fly.rt);
    }
    EndPaint(hwnd, &ps);
}

// Re-evaluates theme + backdrop; the render target is recreated when the surface kind changes.
void UpdateLook() {
    if (!g_fly.hwnd) return;
    g_fly.dark = SystemPrefersDark();
    ApplyWindowFrameTheme(g_fly.hwnd, g_fly.dark, true);
    Backdrop b = ApplyFlyoutBackdrop(g_fly.hwnd, g_fly.dark);
    if ((b == Backdrop::Solid) != (g_fly.backdrop == Backdrop::Solid)) SafeRelease(g_fly.rt);
    g_fly.backdrop = b;
    InvalidateRect(g_fly.hwnd, nullptr, FALSE);
}

LRESULT CALLBACK FlyoutProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_PAINT:
        Paint(hwnd);
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_SIZE:
        if (g_fly.rt) g_fly.rt->Resize(D2D1::SizeU(LOWORD(lp), HIWORD(lp)));
        return 0;
    case 0x02E0:  // WM_DPICHANGED
        SafeRelease(g_fly.rt);
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_MOUSEMOVE: {
        if (!g_fly.tracking) {
            TRACKMOUSEEVENT tme = {sizeof(tme), TME_LEAVE, hwnd, 0};
            TrackMouseEvent(&tme);
            g_fly.tracking = true;
        }
        float s = Scale();
        int hot = g_fly.ui.HitTest((short)LOWORD(lp) / s, (short)HIWORD(lp) / s);
        if (hot != g_fly.ui.hot) {
            g_fly.ui.hot = hot;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    }
    case WM_MOUSELEAVE:
        g_fly.tracking = false;
        g_fly.ui.hot = 0;
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_LBUTTONDOWN: {
        float s = Scale();
        g_fly.ui.pressed = g_fly.ui.HitTest((short)LOWORD(lp) / s, (short)HIWORD(lp) / s);
        g_fly.ui.focusVisible = false;
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }
    case WM_LBUTTONUP: {
        float s = Scale();
        int id = g_fly.ui.HitTest((short)LOWORD(lp) / s, (short)HIWORD(lp) / s);
        int pressed = g_fly.ui.pressed;
        g_fly.ui.pressed = 0;
        InvalidateRect(hwnd, nullptr, FALSE);
        if (id && id == pressed) Activate(id);
        return 0;
    }
    case WM_KEYDOWN:
        if (wp == VK_ESCAPE) {
            Hide();
        } else if (wp == VK_TAB) {
            g_fly.ui.focusVisible = true;
            g_fly.ui.focus = g_fly.ui.NextFocus((GetKeyState(VK_SHIFT) & 0x8000) ? -1 : 1);
            InvalidateRect(hwnd, nullptr, FALSE);
        } else if ((wp == VK_RETURN || wp == VK_SPACE) && g_fly.ui.focus) {
            g_fly.ui.focusVisible = true;
            Activate(g_fly.ui.focus);
        }
        return 0;
    case WM_ACTIVATE:
        if (LOWORD(wp) == WA_INACTIVE) Hide();
        return 0;
    case WM_SETTINGCHANGE:      // theme, transparency effects (ImmersiveColorSet), high contrast
    case WM_THEMECHANGED:
    case WM_SYSCOLORCHANGE:
    case 0x031E:                // WM_DWMCOMPOSITIONCHANGED
        UpdateLook();
        break;
    case WM_CLOSE:
        Hide();
        return 0;
    case WM_DESTROY:
        SafeRelease(g_fly.rt);
        g_fly.hwnd = nullptr;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

}  // namespace

bool TrayFlyoutRecentlyClosed() { return GetTickCount() - g_fly.hiddenTick < 300; }

void HideTrayFlyout() { Hide(); }

void RefreshFlyout() {
    if (!g_fly.hwnd || !IsWindowVisible(g_fly.hwnd)) return;
    // An update notice may have appeared or gone: grow / shrink upwards, keeping the bottom edge.
    RECT wr;
    GetWindowRect(g_fly.hwnd, &wr);
    int h = (int)(FlyoutHeight(M()) * Scale());
    if (wr.bottom - wr.top != h) SetWindowPos(g_fly.hwnd, nullptr, wr.left, wr.bottom - h, wr.right - wr.left, h, SWP_NOZORDER | SWP_NOACTIVATE);
    InvalidateRect(g_fly.hwnd, nullptr, FALSE);
}

void ShowTrayFlyout(HWND trayHost, UINT trayId) {
    HINSTANCE inst = GetModuleHandleW(nullptr);
    if (!g_fly.hwnd) {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = FlyoutProc;
        wc.hInstance = inst;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.lpszClassName = L"VisKeyTrayFlyout";
        RegisterClassExW(&wc);
        g_fly.hwnd = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST, wc.lpszClassName, L"VisKey", WS_POPUP, 0, 0, 10, 10, nullptr, nullptr, inst, nullptr);
        if (!g_fly.hwnd) return;
    }
    UpdateLook();
    g_fly.ui.hot = g_fly.ui.pressed = g_fly.ui.focus = 0;
    g_fly.ui.focusVisible = false;

    // Anchor: the tray icon rectangle if the shell tells us, else the cursor.
    RECT anchor = {};
    bool haveAnchor = false;
    static auto getRect = [] {
        FARPROC p = GetProcAddress(GetModuleHandleW(L"shell32.dll"), "Shell_NotifyIconGetRect");
        PfnNotifyIconGetRect f = nullptr;
        std::memcpy(&f, &p, sizeof(f));
        return f;
    }();
    if (getRect) {
        NOTIFYICONIDENTIFIER nii = {};
        nii.cbSize = sizeof(nii);
        nii.hWnd = trayHost;
        nii.uID = trayId;
        haveAnchor = SUCCEEDED(getRect(&nii, &anchor)) && anchor.right > anchor.left;
    }
    if (!haveAnchor) {
        POINT pt;
        GetCursorPos(&pt);
        anchor = {pt.x, pt.y, pt.x + 1, pt.y + 1};
    }
    POINT center = {(anchor.left + anchor.right) / 2, (anchor.top + anchor.bottom) / 2};
    HMONITOR mon = MonitorFromPoint(center, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = {};
    mi.cbSize = sizeof(mi);
    GetMonitorInfoW(mon, &mi);
    // Move onto the monitor first so the window picks up that monitor's DPI.
    SetWindowPos(g_fly.hwnd, HWND_TOPMOST, mi.rcWork.left, mi.rcWork.top, 10, 10, SWP_NOACTIVATE);
    float s = Scale();
    int w = (int)(kFlyoutW * s), h = (int)(FlyoutHeight(M()) * s), margin = (int)(12 * s);
    const RECT& wk = mi.rcWork;
    const RECT& mr = mi.rcMonitor;
    int x, y;
    if (wk.top > mr.top) {  // taskbar at the top
        x = center.x - w / 2;
        y = wk.top + margin;
    } else if (wk.left > mr.left) {  // left
        x = wk.left + margin;
        y = center.y - h / 2;
    } else if (wk.right < mr.right) {  // right
        x = wk.right - w - margin;
        y = center.y - h / 2;
    } else {  // bottom (or auto-hide)
        x = center.x - w / 2;
        y = wk.bottom - h - margin;
    }
    x = std::max((int)wk.left + margin, std::min(x, (int)wk.right - w - margin));
    y = std::max((int)wk.top + margin, std::min(y, (int)wk.bottom - h - margin));
    SafeRelease(g_fly.rt);
    SetWindowPos(g_fly.hwnd, HWND_TOPMOST, x, y, w, h, SWP_SHOWWINDOW);
    SetForegroundWindow(g_fly.hwnd);
    SetFocus(g_fly.hwnd);
    InvalidateRect(g_fly.hwnd, nullptr, FALSE);
}

}  // namespace vk
