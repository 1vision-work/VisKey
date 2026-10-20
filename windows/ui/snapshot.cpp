// VisKey for Windows - "--ui-snapshot <outdir>": renders every screen offscreen at 1.5x to PNG
// (Direct2D on a WIC bitmap, PNG encoder through COM). No hooks, no tray icon, no registry.
// MIT License, 1VISION.
#include "views.h"

#include <objbase.h>
#include <shellapi.h>
#include <shlobj.h>

#include <algorithm>
#include <functional>
#include <string>

namespace vk {

namespace {

const float kSnapshotDpi = 144.f;  // 1.5x
const float kCaptionH = 40.f;
const float kDesktopMargin = 28.f;  // desktop visible around the tray flyout
const float kFlyoutRadius = 8.f;    // DWMWCP_ROUND

// Neutral wallpaper-like background (soft blobs on a gradient) so the translucent flyout reads as
// translucent. Everything is already blurry, which stands in for the acrylic blur DWM would add.
void PaintDesktop(Gfx& g, bool dark, float w, float h) {
    // Solid fills only (gradient brushes render differently across D2D implementations).
    g.Fill(R(0, 0, w, h), dark ? Rgb(0x18223A) : Rgb(0xC4D3F2));
    struct Blob {
        float x, y, r;
        uint32_t light, dark;
    };
    const Blob blobs[] = {{0.10f, 0.20f, 0.55f, 0x6F8CFF, 0x3044D6}, {0.95f, 0.40f, 0.50f, 0x3FE0C0, 0x0E8C78},
                          {0.50f, 0.95f, 0.55f, 0xFFC38A, 0x8E3B6A}, {0.70f, 0.05f, 0.35f, 0xE6E9FF, 0x262D6B}};
    const float side = std::max(w, h);
    for (const Blob& b : blobs) {
        const int kRings = 24;  // stacked translucent discs = soft radial falloff
        for (int i = kRings; i >= 1; --i) {
            float r = b.r * side * (float)i / (float)kRings;
            g.FillCircle(b.x * w, b.y * h, r, Rgb(dark ? b.dark : b.light, dark ? 0.07f : 0.08f));
        }
    }
}

// The tray flyout as DWM would show it: soft shadow, acrylic material (luminosity + tint), our
// translucent layer on top, clipped to the rounded window shape, hairline border.
void PaintFlyoutOnDesktop(Gfx& g, const Theme& t, const Model& m, float fh) {
    const float x = kDesktopMargin, y = kDesktopMargin;
    RectF win = RW(x, y, kFlyoutW, fh);
    PaintDesktop(g, t.dark, kFlyoutW + 2 * kDesktopMargin, fh + 2 * kDesktopMargin);
    for (int i = 6; i >= 1; --i) {  // shadow
        float d = (float)i * 2.f;
        g.FillRound(R(win.left - d * .5f, win.top + d * .3f, win.right + d * .5f, win.bottom + d), kFlyoutRadius + d, Rgb(0x000000, 0.025f));
    }
    // Acrylic material approximation (Windows 11 transient acrylic: strong luminosity, light tint).
    g.FillRound(win, kFlyoutRadius, t.dark ? Rgb(0x2C2C2C, 0.50f) : Rgb(0xF3F3F3, 0.45f));

    ID2D1RenderTarget* rt = g.Target();
    ID2D1RoundedRectangleGeometry* shape = nullptr;
    ID2D1Layer* layer = nullptr;
    D2D1_ROUNDED_RECT rr;
    rr.rect = RW(0, 0, kFlyoutW, fh);
    rr.radiusX = rr.radiusY = kFlyoutRadius;
    D2D()->CreateRoundedRectangleGeometry(rr, &shape);
    rt->CreateLayer(nullptr, &layer);
    rt->SetTransform(D2D1::Matrix3x2F::Translation(x, y));
    if (shape && layer) rt->PushLayer(D2D1::LayerParameters(RW(0, 0, kFlyoutW, fh), shape), layer);
    Ui ui;
    ui.g = &g;
    ui.t = &t;
    ui.live = false;
    PaintFlyout(ui, m, kFlyoutW, fh, FlyoutSurface::Translucent);
    if (shape && layer) rt->PopLayer();
    g.StrokeRound(RW(0, 0, kFlyoutW, fh), kFlyoutRadius, t.dark ? Rgb(0xFFFFFF, 0.10f) : Rgb(0x000000, 0.10f));
    rt->SetTransform(D2D1::Matrix3x2F::Identity());
    SafeRelease(layer);
    SafeRelease(shape);
}

bool RenderToPng(const std::wstring& path, float w, float h, const std::function<void(Gfx&)>& draw) {
    OffscreenTarget off;
    bool ok = off.Create((UINT)(w * kSnapshotDpi / 96.f + .5f), (UINT)(h * kSnapshotDpi / 96.f + .5f), kSnapshotDpi);
    if (ok) {
        off.rt->BeginDraw();
        {
            Gfx g(off.rt);
            draw(g);
        }
        ok = SUCCEEDED(off.rt->EndDraw()) && off.SavePng(path);
    }
    off.Release();
    return ok;
}

}  // namespace

int RunUiSnapshot(const std::wstring& outDirIn) {
    std::wstring outDir = outDirIn;
    while (outDir.size() > 3 && (outDir.back() == L'\\' || outDir.back() == L'/')) outDir.pop_back();
    if (outDir.empty()) return 2;
    SHCreateDirectoryExW(nullptr, outDir.c_str(), nullptr);
    DWORD attr = GetFileAttributesW(outDir.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY)) return 3;

    std::string log = "VisKey " + WideToUtf8(AppVersion()) + " UI snapshots (scale 1.5x)\r\n";
    if (!GfxStartup()) {
        log += "ERROR: Direct2D/DirectWrite unavailable\r\n";
        WriteFileBytes(outDir + L"\\snapshot.txt", log);
        return 4;
    }

    Model& m = M();
    FillDemoModel(m);

    struct Shot {
        const wchar_t* file;
        int page;  // -1 = tray flyout
        bool dark;
        bool newer;  // demo: the automatic check found 1.1.0 (nav dot, banner, flyout line)
    };
    const Shot shots[] = {
        {L"win-general-light.png", kPageGeneral, false, false}, {L"win-general-dark.png", kPageGeneral, true, false},
        {L"win-macros-dark.png", kPageMacros, true, false},     {L"win-macros-light.png", kPageMacros, false, false},
        {L"win-perapp-light.png", kPagePerApp, false, false},   {L"win-hotkeys-light.png", kPageHotkeys, false, false},
        {L"win-update-light.png", kPageUpdate, false, false},   {L"win-update-newer-light.png", kPageUpdate, false, true},
        {L"win-about-light.png", kPageAbout, false, false},     {L"win-tray-dark.png", -1, true, false},
        {L"win-tray-light.png", -1, false, false},              {L"win-tray-newer-light.png", -1, false, true},
    };
    const UpdateResult upToDate = m.update;
    int okCount = 0, failCount = 0;
    for (const Shot& s : shots) {
        std::wstring path = outDir + L"\\" + s.file;
        const Theme& t = GetTheme(s.dark);
        m.update = upToDate;
        if (s.newer) {
            m.update.newer = true;
            m.update.latest = L"1.1.0";
            m.update.tag = L"v1.1.0";
        }
        bool ok;
        if (s.page < 0) {
            float fh = FlyoutHeight(m);
            ok = RenderToPng(path, kFlyoutW + 2 * kDesktopMargin, fh + 2 * kDesktopMargin, [&](Gfx& g) { PaintFlyoutOnDesktop(g, t, m, fh); });
        } else {
            ok = RenderToPng(path, kSettingsW, kSettingsH, [&](Gfx& g) {
                PaintFakeCaption(g, t, kSettingsW, kCaptionH);
                D2D1_MATRIX_3X2_F shift = D2D1::Matrix3x2F::Translation(0.f, kCaptionH);
                g.Target()->SetTransform(shift);
                Ui ui;
                ui.g = &g;
                ui.t = &t;
                ui.live = false;
                SettingsView v;
                v.page = s.page;
                if (s.page == kPageMacros) {
                    v.macroKey = L"tks";
                    v.macroValue = L"cảm ơn nhiều";
                    v.demoHoverMacro = "ko";
                    if (s.dark) v.macroSort = 1;  // "Hay dùng": most used first
                }
                if (s.page == kPageHotkeys) v.recordingSlot = kHotkeySecondary;
                g.PushClip(R(0, 0, kSettingsW, kSettingsH - kCaptionH));
                PaintSettings(ui, v, m, kSettingsW, kSettingsH - kCaptionH);
                g.PopClip();
                g.Target()->SetTransform(D2D1::Matrix3x2F::Identity());
            });
        }
        log += WideToUtf8(s.file) + (ok ? " ok\r\n" : " FAILED\r\n");
        ok ? ++okCount : ++failCount;
    }
    log += std::to_string(okCount) + " PNG written, " + std::to_string(failCount) + " failed\r\n";
    log += "Note: win-tray-*.png show the flyout over a sample desktop with an approximated acrylic material. "
           "On Windows the real backdrop (Windows 11 22H2+ system acrylic, or accent-policy acrylic on older builds) "
           "is drawn by DWM and cannot be captured offscreen; with transparency effects off, high contrast or "
           "battery saver the live flyout uses the solid background instead.\r\n";
    WriteFileBytes(outDir + L"\\snapshot.txt", log);
    GfxShutdown();
    return (okCount > 0 && failCount == 0) ? 0 : 5;
}

}  // namespace vk
