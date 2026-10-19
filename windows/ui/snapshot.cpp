// VisKey for Windows - "--ui-snapshot <outdir>": renders every screen offscreen at 1.5x to PNG
// (Direct2D on a WIC bitmap, PNG encoder through COM). No hooks, no tray icon, no registry.
// MIT License, 1VISION.
#include "views.h"

#include <objbase.h>
#include <shellapi.h>
#include <shlobj.h>

#include <functional>
#include <string>

namespace vk {

namespace {

const float kSnapshotDpi = 144.f;  // 1.5x
const float kCaptionH = 40.f;

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
    };
    const Shot shots[] = {
        {L"win-general-light.png", kPageGeneral, false}, {L"win-general-dark.png", kPageGeneral, true}, {L"win-macros-dark.png", kPageMacros, true},
        {L"win-macros-light.png", kPageMacros, false},   {L"win-perapp-light.png", kPagePerApp, false}, {L"win-hotkeys-light.png", kPageHotkeys, false},
        {L"win-update-light.png", kPageUpdate, false},   {L"win-about-light.png", kPageAbout, false},   {L"win-tray-dark.png", -1, true},
        {L"win-tray-light.png", -1, false},
    };
    int okCount = 0, failCount = 0;
    for (const Shot& s : shots) {
        std::wstring path = outDir + L"\\" + s.file;
        const Theme& t = GetTheme(s.dark);
        bool ok;
        if (s.page < 0) {
            ok = RenderToPng(path, kFlyoutW, kFlyoutH, [&](Gfx& g) {
                Ui ui;
                ui.g = &g;
                ui.t = &t;
                ui.live = false;
                PaintFlyout(ui, m, kFlyoutW, kFlyoutH);
            });
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
                if (s.page == kPageUpdate) {
                    v.update = UpdateState::UpToDate;
                    v.updateResult.ok = true;
                    v.updateResult.latest = AppVersion();
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
    WriteFileBytes(outDir + L"\\snapshot.txt", log);
    GfxShutdown();
    return (okCount > 0 && failCount == 0) ? 0 : 5;
}

}  // namespace vk
