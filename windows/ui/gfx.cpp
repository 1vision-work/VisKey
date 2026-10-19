// VisKey for Windows - Direct2D / DirectWrite drawing layer (MIT License, 1VISION).
#include "gfx.h"

#include <objbase.h>
#include <wincodec.h>

#include <algorithm>
#include <cmath>
#include <map>

namespace vk {

namespace {

// GUIDs used with CoCreateInstance / WIC. Defined here so nothing depends on how a given SDK or
// MinGW import library exposes them (values from the Windows SDK headers).
const GUID kClsidWicImagingFactory = {0xcacaf262, 0x9370, 0x4615, {0xa1, 0x3b, 0x9f, 0x55, 0x39, 0xda, 0x4c, 0x0a}};
const GUID kContainerFormatPng = {0x1b7cfaf4, 0x713f, 0x473c, {0xbb, 0xcd, 0x61, 0x37, 0x42, 0x5f, 0xae, 0xaf}};
const GUID kPixelFormat32bppPBGRA = {0x6fddc324, 0x4e03, 0x4bfe, {0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x10}};
const GUID kPixelFormat32bppBGRA = {0x6fddc324, 0x4e03, 0x4bfe, {0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x0f}};

template <class T>
T LoadProc(HMODULE mod, const char* name) {
    FARPROC p = mod ? GetProcAddress(mod, name) : nullptr;
    T f = nullptr;
    static_assert(sizeof(f) == sizeof(p), "function pointer size");
    std::memcpy(&f, &p, sizeof(f));
    return f;
}

typedef HRESULT(WINAPI* PfnD2D1CreateFactory)(D2D1_FACTORY_TYPE, REFIID, const D2D1_FACTORY_OPTIONS*, void**);
typedef HRESULT(WINAPI* PfnDWriteCreateFactory)(DWRITE_FACTORY_TYPE, REFIID, IUnknown**);

ID2D1Factory* g_d2d = nullptr;
IDWriteFactory* g_dwrite = nullptr;
std::wstring g_faceUi = L"Segoe UI";
std::wstring g_faceDisplay = L"Segoe UI";
std::wstring g_faceMono = L"Consolas";
std::wstring g_faceIcons;
std::map<uint64_t, IDWriteTextFormat*> g_formats;
IDWriteInlineObject* g_ellipsisSign = nullptr;

bool FamilyExists(IDWriteFontCollection* fc, const wchar_t* name) {
    UINT32 index = 0;
    BOOL exists = FALSE;
    return fc && SUCCEEDED(fc->FindFamilyName(name, &index, &exists)) && exists;
}

void PickFaces() {
    IDWriteFontCollection* fc = nullptr;
    if (FAILED(g_dwrite->GetSystemFontCollection(&fc, FALSE)) || !fc) return;
    auto pick = [&](std::initializer_list<const wchar_t*> names, std::wstring& out) {
        for (const wchar_t* n : names) {
            if (FamilyExists(fc, n)) {
                out = n;
                return true;
            }
        }
        return false;
    };
    // Windows always has Segoe UI; the other names only matter elsewhere (e.g. Wine) and are
    // chosen for full Vietnamese coverage.
    pick({L"Segoe UI Variable Text", L"Segoe UI", L"Noto Sans", L"Inter", L"DejaVu Sans", L"Tahoma", L"Arial"}, g_faceUi);
    if (!pick({L"Segoe UI Variable Display", L"Segoe UI Variable Text", L"Segoe UI"}, g_faceDisplay)) g_faceDisplay = g_faceUi;
    if (!pick({L"Cascadia Mono", L"Consolas", L"JetBrains Mono", L"Noto Sans Mono", L"DejaVu Sans Mono", L"Courier New"}, g_faceMono)) g_faceMono = g_faceUi;
    if (!pick({L"Segoe Fluent Icons", L"Segoe MDL2 Assets"}, g_faceIcons)) g_faceIcons.clear();
    fc->Release();
}

IDWriteTextFormat* Format(const TextStyle& st) {
    uint64_t key = ((uint64_t)st.face << 40) | ((uint64_t)st.weight << 24) | (uint64_t)(st.size * 16.f);
    auto it = g_formats.find(key);
    if (it != g_formats.end()) return it->second;
    const std::wstring* face = &g_faceUi;
    if (st.face == Face::Display) face = &g_faceDisplay;
    if (st.face == Face::Mono) face = &g_faceMono;
    if (st.face == Face::Icons) face = &g_faceIcons;
    IDWriteTextFormat* fmt = nullptr;
    if (FAILED(g_dwrite->CreateTextFormat(face->c_str(), nullptr, (DWRITE_FONT_WEIGHT)st.weight, DWRITE_FONT_STYLE_NORMAL,
                                          DWRITE_FONT_STRETCH_NORMAL, st.size, L"vi-VN", &fmt))) {
        return nullptr;
    }
    fmt->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
    g_formats[key] = fmt;
    return fmt;
}

IDWriteTextLayout* Layout(const std::wstring& s, const TextStyle& st, float w, float h) {
    IDWriteTextFormat* fmt = Format(st);
    if (!fmt) return nullptr;
    IDWriteTextLayout* layout = nullptr;
    if (FAILED(g_dwrite->CreateTextLayout(s.c_str(), (UINT32)s.size(), fmt, w < 1 ? 1 : w, h < 1 ? 1 : h, &layout))) return nullptr;
    return layout;
}

D2D1_COLOR_F ToD2D(Color c) {
    D2D1_COLOR_F d;
    d.r = c.r;
    d.g = c.g;
    d.b = c.b;
    d.a = c.a;
    return d;
}

D2D1_POINT_2F P(float x, float y) {
    D2D1_POINT_2F p;
    p.x = x;
    p.y = y;
    return p;
}

Theme MakeLight() {
    Theme t;
    t.dark = false;
    t.windowBg = Rgb(0xF3F4F7);
    t.navSel = Rgb(0x16181C, 0.065f);
    t.navHover = Rgb(0x16181C, 0.04f);
    t.card = Rgb(0xFFFFFF);
    t.cardHover = Rgb(0xFAFAFC);
    t.cardStroke = Rgb(0xE3E5EA);
    t.sectionText = Rgb(0x16181C);
    t.text = Rgb(0x16181C);
    t.text2 = Rgb(0x666D7A);
    t.text3 = Rgb(0xA3A9B5);
    t.accent = Rgb(0x3044D6);
    t.accentFill = Rgb(0x3044D6);
    t.onAccentFill = Rgb(0xFFFFFF);
    t.accentStrong = Rgb(0x3044D6);
    t.accentTint = Rgb(0xE6E9FF);
    t.accentTintText = Rgb(0x2534B8);
    t.control = Rgb(0xFFFFFF);
    t.controlHover = Rgb(0xF6F7F9);
    t.controlPressed = Rgb(0xECEEF2);
    t.controlStroke = Rgb(0xDADDE4);
    t.controlBottom = Rgb(0xA3A9B5);
    t.fieldActive = Rgb(0xFFFFFF);
    t.toggleOffStroke = Rgb(0x666D7A);
    t.toggleOffKnob = Rgb(0x4B515C);
    t.toggleOnKnob = Rgb(0xFFFFFF);
    t.jade = Rgb(0x14B89A);
    t.jadeText = Rgb(0x0B7F6D);
    t.jadeTint = Rgb(0xD5F8F0);
    t.warning = Rgb(0xB45309);
    t.warningTint = Rgb(0xFFF4E6);
    t.error = Rgb(0xC2302A);
    t.errorTint = Rgb(0xFDECEA);
    t.separator = Rgb(0xE3E5EA);
    t.keycap = Rgb(0xF6F7F9);
    t.keycapStroke = Rgb(0xDADDE4);
    t.focus = Rgb(0x16181C);
    t.flyoutBg = Rgb(0xF9F9FB);
    t.flyoutFooter = Rgb(0xEEF0F4);
    t.tileOff = Rgb(0xE9EBF0);
    t.tileOffHover = Rgb(0xE0E3EA);
    t.segmentBg = Rgb(0xE6E8ED);
    t.segmentSel = Rgb(0xFFFFFF);
    t.tileBlue = Rgb(0x3044D6);
    t.flyoutLayer = WithAlpha(t.flyoutBg, 0.62f);
    t.flyoutFooterLayer = WithAlpha(t.flyoutFooter, 0.55f);
    t.flyoutAcrylicTint = WithAlpha(t.flyoutBg, 0.80f);
    return t;
}

Theme MakeDark() {
    Theme t;
    t.dark = true;
    t.windowBg = Rgb(0x1C1E23);
    t.navSel = Rgb(0xFFFFFF, 0.075f);
    t.navHover = Rgb(0xFFFFFF, 0.045f);
    t.card = Rgb(0x25272D);
    t.cardHover = Rgb(0x2B2E35);
    t.cardStroke = Rgb(0x2E3238);
    t.sectionText = Rgb(0xECEEF2);
    t.text = Rgb(0xECEEF2);
    t.text2 = Rgb(0xA3A9B5);
    t.text3 = Rgb(0x666D7A);
    t.accent = Rgb(0x8193FF);
    t.accentFill = Rgb(0x8193FF);
    t.onAccentFill = Rgb(0x16181C);
    t.accentStrong = Rgb(0x4C60F5);
    t.accentTint = Rgb(0x262D6B);
    t.accentTintText = Rgb(0xAEB9FF);
    t.control = Rgb(0x2E3138);
    t.controlHover = Rgb(0x353840);
    t.controlPressed = Rgb(0x2A2D33);
    t.controlStroke = Rgb(0x3A3E46);
    t.controlBottom = Rgb(0x666D7A);
    t.fieldActive = Rgb(0x1A1C20);
    t.toggleOffStroke = Rgb(0xA3A9B5);
    t.toggleOffKnob = Rgb(0xC9CCD4);
    t.toggleOnKnob = Rgb(0x16181C);
    t.jade = Rgb(0x3FE0C0);
    t.jadeText = Rgb(0x3FE0C0);
    t.jadeTint = Rgb(0x0E3B35);
    t.warning = Rgb(0xF5A65B);
    t.warningTint = Rgb(0x3A2A17);
    t.error = Rgb(0xFF8A80);
    t.errorTint = Rgb(0x3B2120);
    t.separator = Rgb(0x2E3238);
    t.keycap = Rgb(0x2E3138);
    t.keycapStroke = Rgb(0x41454E);
    t.focus = Rgb(0xFFFFFF);
    t.flyoutBg = Rgb(0x2A2B2F);
    t.flyoutFooter = Rgb(0x242528);
    t.tileOff = Rgb(0x3A3B40);
    t.tileOffHover = Rgb(0x44464C);
    t.segmentBg = Rgb(0x1F2023);
    t.segmentSel = Rgb(0x48494F);
    t.tileBlue = Rgb(0x3044D6);
    t.flyoutLayer = WithAlpha(t.flyoutBg, 0.70f);
    t.flyoutFooterLayer = WithAlpha(t.flyoutFooter, 0.60f);
    t.flyoutAcrylicTint = WithAlpha(t.flyoutBg, 0.84f);
    return t;
}

}  // namespace

const Theme& GetTheme(bool dark) {
    static const Theme light = MakeLight();
    static const Theme darkTheme = MakeDark();
    return dark ? darkTheme : light;
}

bool GfxStartup() {
    if (g_d2d && g_dwrite) return true;
    HMODULE d2d = LoadLibraryW(L"d2d1.dll");
    HMODULE dw = LoadLibraryW(L"dwrite.dll");
    auto createD2D = LoadProc<PfnD2D1CreateFactory>(d2d, "D2D1CreateFactory");
    auto createDW = LoadProc<PfnDWriteCreateFactory>(dw, "DWriteCreateFactory");
    if (!createD2D || !createDW) return false;
    D2D1_FACTORY_OPTIONS opts;
    opts.debugLevel = D2D1_DEBUG_LEVEL_NONE;
    void* f = nullptr;
    if (FAILED(createD2D(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory), &opts, &f)) || !f) return false;
    g_d2d = static_cast<ID2D1Factory*>(f);
    IUnknown* u = nullptr;
    if (FAILED(createDW(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), &u)) || !u) {
        SafeRelease(g_d2d);
        return false;
    }
    g_dwrite = static_cast<IDWriteFactory*>(u);
    PickFaces();
    return true;
}

void GfxShutdown() {
    for (auto& kv : g_formats) kv.second->Release();
    g_formats.clear();
    SafeRelease(g_ellipsisSign);
    SafeRelease(g_dwrite);
    SafeRelease(g_d2d);
}

ID2D1Factory* D2D() { return g_d2d; }
IDWriteFactory* DWrite() { return g_dwrite; }
bool HasIconFont() { return !g_faceIcons.empty(); }

// ---------------------------------------------------------------------------------------------
Gfx::Gfx(ID2D1RenderTarget* rt) : rt_(rt) {
    rt_->CreateSolidColorBrush(ToD2D(Rgb(0)), &brush_);
    D2D1_STROKE_STYLE_PROPERTIES sp = D2D1::StrokeStyleProperties(D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND,
                                                                  D2D1_LINE_JOIN_ROUND, 10.f, D2D1_DASH_STYLE_SOLID, 0.f);
    g_d2d->CreateStrokeStyle(sp, nullptr, 0, &roundStroke_);
    D2D1_STROKE_STYLE_PROPERTIES dp = D2D1::StrokeStyleProperties(D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_FLAT,
                                                                  D2D1_LINE_JOIN_MITER, 10.f, D2D1_DASH_STYLE_CUSTOM, 0.f);
    const float dashes[] = {3.f, 2.5f};
    g_d2d->CreateStrokeStyle(dp, dashes, 2, &dashStroke_);
}

Gfx::~Gfx() {
    SafeRelease(dashStroke_);
    SafeRelease(roundStroke_);
    SafeRelease(brush_);
}

ID2D1SolidColorBrush* Gfx::Brush(Color c) {
    brush_->SetColor(ToD2D(c));
    return brush_;
}

void Gfx::Clear(Color c) { rt_->Clear(ToD2D(c)); }

void Gfx::Fill(const RectF& r, Color c) {
    if (c.a <= 0) return;
    rt_->FillRectangle(r, Brush(c));
}

void Gfx::FillRound(const RectF& r, float radius, Color c) {
    if (c.a <= 0) return;
    D2D1_ROUNDED_RECT rr;
    rr.rect = r;
    rr.radiusX = rr.radiusY = radius;
    rt_->FillRoundedRectangle(rr, Brush(c));
}

void Gfx::StrokeRound(const RectF& r, float radius, Color c, float width) {
    if (c.a <= 0) return;
    D2D1_ROUNDED_RECT rr;
    rr.rect = Inset(r, width * .5f, width * .5f);
    rr.radiusX = rr.radiusY = radius;
    rt_->DrawRoundedRectangle(rr, Brush(c), width);
}

void Gfx::StrokeRoundDashed(const RectF& r, float radius, Color c, float width) {
    D2D1_ROUNDED_RECT rr;
    rr.rect = Inset(r, width * .5f, width * .5f);
    rr.radiusX = rr.radiusY = radius;
    rt_->DrawRoundedRectangle(rr, Brush(c), width, dashStroke_);
}

void Gfx::Line(float x1, float y1, float x2, float y2, Color c, float width) {
    rt_->DrawLine(P(x1, y1), P(x2, y2), Brush(c), width, roundStroke_);
}

void Gfx::FillCircle(float cx, float cy, float radius, Color c) {
    rt_->FillEllipse(D2D1::Ellipse(P(cx, cy), radius, radius), Brush(c));
}

void Gfx::StrokeCircle(float cx, float cy, float radius, Color c, float width) {
    rt_->DrawEllipse(D2D1::Ellipse(P(cx, cy), radius, radius), Brush(c), width);
}

static DWRITE_TEXT_ALIGNMENT ToDw(HAlign h) {
    return h == HAlign::Center ? DWRITE_TEXT_ALIGNMENT_CENTER : h == HAlign::Right ? DWRITE_TEXT_ALIGNMENT_TRAILING : DWRITE_TEXT_ALIGNMENT_LEADING;
}

void Gfx::Text(const std::wstring& s, const RectF& r, const TextStyle& st, Color c, HAlign h) {
    if (s.empty()) return;
    IDWriteTextLayout* layout = Layout(s, st, Wd(r), Ht(r));
    if (!layout) return;
    layout->SetTextAlignment(ToDw(h));
    layout->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    if (!g_ellipsisSign) g_dwrite->CreateEllipsisTrimmingSign(layout, &g_ellipsisSign);
    DWRITE_TRIMMING trim;
    trim.granularity = DWRITE_TRIMMING_GRANULARITY_CHARACTER;
    trim.delimiter = 0;
    trim.delimiterCount = 0;
    layout->SetTrimming(&trim, g_ellipsisSign);
    rt_->DrawTextLayout(P(r.left, r.top), layout, Brush(c), D2D1_DRAW_TEXT_OPTIONS_NONE);
    layout->Release();
}

float Gfx::TextWrapped(const std::wstring& s, const RectF& r, const TextStyle& st, Color c, HAlign h) {
    if (s.empty()) return 0;
    IDWriteTextLayout* layout = Layout(s, st, Wd(r), 4096.f);
    if (!layout) return 0;
    layout->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
    layout->SetTextAlignment(ToDw(h));
    DWRITE_TEXT_METRICS m;
    float height = SUCCEEDED(layout->GetMetrics(&m)) ? m.height : 0.f;
    rt_->DrawTextLayout(P(r.left, r.top), layout, Brush(c), D2D1_DRAW_TEXT_OPTIONS_NONE);
    layout->Release();
    return height;
}

float Gfx::TextWidth(const std::wstring& s, const TextStyle& st) {
    if (s.empty()) return 0;
    IDWriteTextLayout* layout = Layout(s, st, 8192.f, 1024.f);
    if (!layout) return 0;
    DWRITE_TEXT_METRICS m;
    float w = SUCCEEDED(layout->GetMetrics(&m)) ? m.widthIncludingTrailingWhitespace : 0.f;
    layout->Release();
    return w;
}

float Gfx::WrappedHeight(const std::wstring& s, float width, const TextStyle& st) {
    if (s.empty()) return 0;
    IDWriteTextLayout* layout = Layout(s, st, width, 4096.f);
    if (!layout) return 0;
    layout->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
    DWRITE_TEXT_METRICS m;
    float h = SUCCEEDED(layout->GetMetrics(&m)) ? m.height : 0.f;
    layout->Release();
    return h;
}

void Gfx::PushClip(const RectF& r) { rt_->PushAxisAlignedClip(r, D2D1_ANTIALIAS_MODE_ALIASED); }
void Gfx::PopClip() { rt_->PopAxisAlignedClip(); }

void Gfx::Poly(const D2D1_POINT_2F* pts, int n, bool closed, Color c, float width, bool fill) {
    if (n < 2) return;
    ID2D1PathGeometry* geo = nullptr;
    if (FAILED(g_d2d->CreatePathGeometry(&geo))) return;
    ID2D1GeometrySink* sink = nullptr;
    if (SUCCEEDED(geo->Open(&sink))) {
        sink->BeginFigure(pts[0], fill ? D2D1_FIGURE_BEGIN_FILLED : D2D1_FIGURE_BEGIN_HOLLOW);
        sink->AddLines(pts + 1, (UINT32)(n - 1));
        sink->EndFigure(closed ? D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END_OPEN);
        sink->Close();
        sink->Release();
        if (fill) rt_->FillGeometry(geo, Brush(c));
        else rt_->DrawGeometry(geo, Brush(c), width, roundStroke_);
    }
    geo->Release();
}

void Gfx::Arc(float cx, float cy, float r, float a0, float a1, Color c, float w) {
    const int kSeg = 28;
    D2D1_POINT_2F pts[kSeg + 1];
    for (int i = 0; i <= kSeg; ++i) {
        float a = (a0 + (a1 - a0) * (float)i / (float)kSeg) * 3.14159265f / 180.f;
        pts[i] = P(cx + r * std::cos(a), cy + r * std::sin(a));
    }
    Poly(pts, kSeg + 1, false, c, w);
}

void Gfx::Mark(const RectF& box, Color caret, Color vee) {
    float s = Wd(box) / 64.f;
    auto pt = [&](float x, float y) { return P(box.left + x * s, box.top + y * s); };
    D2D1_POINT_2F a[] = {pt(22, 21), pt(32, 12), pt(42, 21)};
    D2D1_POINT_2F b[] = {pt(15, 28), pt(32, 53), pt(49, 28)};
    Poly(a, 3, false, caret, 6.f * s);
    Poly(b, 3, false, vee, 8.5f * s);
}

static wchar_t GlyphFor(Icon icon) {
    switch (icon) {
    case Icon::Bolt: return 0xE945;
    case Icon::Grid: return 0xE71D;
    case Icon::Keyboard: return 0xE765;
    case Icon::Update: return 0xE72C;
    case Icon::Info: return 0xE946;
    case Icon::Search: return 0xE721;
    case Icon::ChevronDown: return 0xE70D;
    case Icon::ChevronUp: return 0xE70E;
    case Icon::ChevronRight: return 0xE76C;
    case Icon::Globe: return 0xE774;
    case Icon::Terminal: return 0xE756;
    case Icon::Power: return 0xE7E8;
    case Icon::Upload: return 0xE898;
    case Icon::Download: return 0xE896;
    case Icon::Edit: return 0xE70F;
    case Icon::Trash: return 0xE74D;
    case Icon::More: return 0xE712;
    case Icon::Plus: return 0xE710;
    case Icon::Close: return 0xE711;
    case Icon::Copy: return 0xE8C8;
    case Icon::External: return 0xE8A7;
    case Icon::Warning: return 0xE7BA;
    case Icon::Check: return 0xE73E;
    case Icon::Arrow: return 0xE72A;
    case Icon::Code: return 0xE943;
    case Icon::Bug: return 0xE783;
    case Icon::Document: return 0xE8A5;
    case Icon::Folder: return 0xE8B7;
    case Icon::Undo: return 0xE7A7;
    default: return 0;
    }
}

void Gfx::DrawIcon(Icon icon, float cx, float cy, float size, Color c) {
    if (icon == Icon::None) return;
    wchar_t glyph = HasIconFont() ? GlyphFor(icon) : 0;
    if (glyph) {
        std::wstring s(1, glyph);
        Text(s, R(cx - size, cy - size, cx + size, cy + size), TS(size, Weight::Regular, Face::Icons), c, HAlign::Center);
        return;
    }
    VectorIcon(icon, cx, cy, size, c);
}

// Hand-drawn 24x24 line icons, used when no Segoe icon font is installed.
void Gfx::VectorIcon(Icon icon, float cx, float cy, float size, Color c) {
    const float s = size / 24.f;
    const float ox = cx - 12.f * s, oy = cy - 12.f * s;
    const float w = std::max(1.1f, 1.75f * s);
    auto p = [&](float x, float y) { return P(ox + x * s, oy + y * s); };
    auto line = [&](float x1, float y1, float x2, float y2) { Line(ox + x1 * s, oy + y1 * s, ox + x2 * s, oy + y2 * s, c, w); };
    auto poly = [&](std::initializer_list<D2D1_POINT_2F> pts, bool closed) {
        std::vector<D2D1_POINT_2F> v(pts);
        Poly(v.data(), (int)v.size(), closed, c, w);
    };
    auto rrect = [&](float l, float t, float r, float b, float rad) { StrokeRound(R(ox + l * s, oy + t * s, ox + r * s, oy + b * s), rad * s, c, w); };
    auto circle = [&](float x, float y, float r) { StrokeCircle(ox + x * s, oy + y * s, r * s, c, w); };
    auto dot = [&](float x, float y, float r) { FillCircle(ox + x * s, oy + y * s, r * s, c); };

    switch (icon) {
    case Icon::Sliders:
        line(3, 6, 12, 6); line(18, 6, 21, 6); circle(15, 6, 2.5f);
        line(3, 12, 5, 12); line(11, 12, 21, 12); circle(8, 12, 2.5f);
        line(3, 18, 13, 18); line(19, 18, 21, 18); circle(16, 18, 2.5f);
        break;
    case Icon::Bolt: poly({p(13, 2.5f), p(4.5f, 13.5f), p(11.5f, 13.5f), p(10.5f, 21.5f), p(19.5f, 10.5f), p(12.5f, 10.5f)}, true); break;
    case Icon::Grid: rrect(3.5f, 3.5f, 10.5f, 10.5f, 1.5f); rrect(13.5f, 3.5f, 20.5f, 10.5f, 1.5f); rrect(3.5f, 13.5f, 10.5f, 20.5f, 1.5f); rrect(13.5f, 13.5f, 20.5f, 20.5f, 1.5f); break;
    case Icon::Keyboard:
        rrect(2.5f, 6, 21.5f, 18, 2.5f);
        dot(6.5f, 10, 1); dot(10, 10, 1); dot(13.5f, 10, 1); dot(17, 10, 1);
        line(8, 14, 16, 14);
        break;
    case Icon::Update:
    case Icon::Undo: {
        Arc(cx, cy, 8.f * s, 215.f, 215.f + 300.f, c, w);
        float a = 215.f * 3.14159265f / 180.f;
        float sx = 12 + 8 * std::cos(a), sy = 12 + 8 * std::sin(a);
        poly({p(sx - 0.5f, sy - 5), p(sx, sy), p(sx + 5, sy + 0.5f)}, false);
        break;
    }
    case Icon::Info: circle(12, 12, 9); line(12, 11, 12, 16.5f); dot(12, 7.8f, 1.1f); break;
    case Icon::Search: circle(10.5f, 10.5f, 6.5f); line(15.5f, 15.5f, 20.5f, 20.5f); break;
    case Icon::ChevronDown: poly({p(6, 9), p(12, 15), p(18, 9)}, false); break;
    case Icon::ChevronUp: poly({p(6, 15), p(12, 9), p(18, 15)}, false); break;
    case Icon::ChevronRight: poly({p(9, 6), p(15, 12), p(9, 18)}, false); break;
    case Icon::Globe:
        circle(12, 12, 9);
        line(3, 12, 21, 12);
        rt_->DrawEllipse(D2D1::Ellipse(p(12, 12), 4.f * s, 9.f * s), Brush(c), w);
        break;
    case Icon::Terminal: rrect(3, 4, 21, 20, 2.5f); poly({p(7.5f, 9), p(10.5f, 12), p(7.5f, 15)}, false); line(12.5f, 15, 16.5f, 15); break;
    case Icon::Power: Arc(cx, oy + 13 * s, 8.f * s, -50.f, 230.f, c, w); line(12, 3, 12, 11.5f); break;
    case Icon::Upload: line(12, 15, 12, 3.5f); poly({p(7, 8.5f), p(12, 3.5f), p(17, 8.5f)}, false); poly({p(4, 15), p(4, 20), p(20, 20), p(20, 15)}, false); break;
    case Icon::Download: line(12, 3.5f, 12, 15); poly({p(7, 10), p(12, 15), p(17, 10)}, false); poly({p(4, 15), p(4, 20), p(20, 20), p(20, 15)}, false); break;
    case Icon::Edit: poly({p(4, 20), p(4, 16), p(15, 5), p(19, 9), p(8, 20)}, true); line(13, 7, 17, 11); break;
    case Icon::Trash:
        line(4, 6.5f, 20, 6.5f);
        poly({p(6, 6.5f), p(7, 20.5f), p(17, 20.5f), p(18, 6.5f)}, false);
        poly({p(9, 6.5f), p(9, 3.5f), p(15, 3.5f), p(15, 6.5f)}, false);
        line(10, 10.5f, 10, 16.5f); line(14, 10.5f, 14, 16.5f);
        break;
    case Icon::More: dot(5.5f, 12, 1.5f); dot(12, 12, 1.5f); dot(18.5f, 12, 1.5f); break;
    case Icon::Plus: line(12, 5, 12, 19); line(5, 12, 19, 12); break;
    case Icon::Close: line(6.5f, 6.5f, 17.5f, 17.5f); line(17.5f, 6.5f, 6.5f, 17.5f); break;
    case Icon::Copy: rrect(8.5f, 8.5f, 20.5f, 20.5f, 2); poly({p(15.5f, 8.5f), p(15.5f, 3.5f), p(3.5f, 3.5f), p(3.5f, 15.5f), p(8.5f, 15.5f)}, false); break;
    case Icon::External: line(7, 17, 17, 7); poly({p(9, 7), p(17, 7), p(17, 15)}, false); break;
    case Icon::Warning: poly({p(12, 3.5f), p(21.5f, 20), p(2.5f, 20)}, true); line(12, 9.5f, 12, 14); dot(12, 17, 1.1f); break;
    case Icon::Check: poly({p(5, 12.5f), p(10, 17.5f), p(19, 7)}, false); break;
    case Icon::Arrow: line(4, 12, 20, 12); poly({p(14, 6), p(20, 12), p(14, 18)}, false); break;
    case Icon::Code: poly({p(8.5f, 7), p(3.5f, 12), p(8.5f, 17)}, false); poly({p(15.5f, 7), p(20.5f, 12), p(15.5f, 17)}, false); break;
    case Icon::Bug: circle(12, 12, 9); line(12, 7.5f, 12, 13); dot(12, 16.3f, 1.1f); break;
    case Icon::Document:
        poly({p(6, 3), p(14, 3), p(19, 8), p(19, 21), p(6, 21)}, true);
        poly({p(14, 3), p(14, 8), p(19, 8)}, false);
        line(9, 13, 16, 13); line(9, 17, 16, 17);
        break;
    case Icon::Folder: poly({p(3, 6), p(9, 6), p(11, 8.5f), p(21, 8.5f), p(21, 19), p(3, 19)}, true); break;
    case Icon::Window: rrect(3, 4, 21, 20, 2.5f); line(3, 8.5f, 21, 8.5f); break;
    default: break;
    }
}

// ---------------------------------------------------------------------------------------------
bool OffscreenTarget::Create(UINT pxW, UINT pxH, float dpi) {
    IWICImagingFactory* wic = nullptr;
    if (FAILED(CoCreateInstance(kClsidWicImagingFactory, nullptr, CLSCTX_INPROC_SERVER, __uuidof(IWICImagingFactory),
                                reinterpret_cast<void**>(&wic))) || !wic) {
        return false;
    }
    wicFactory = wic;
    IWICBitmap* bmp = nullptr;
    if (FAILED(wic->CreateBitmap(pxW, pxH, kPixelFormat32bppPBGRA, WICBitmapCacheOnLoad, &bmp)) || !bmp) return false;
    bitmap = bmp;
    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), dpi, dpi);
    if (FAILED(g_d2d->CreateWicBitmapRenderTarget(bmp, props, &rt)) || !rt) return false;
    rt->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
    return true;
}

bool OffscreenTarget::SavePng(const std::wstring& path) {
    IWICImagingFactory* wic = static_cast<IWICImagingFactory*>(wicFactory);
    IWICBitmap* bmp = static_cast<IWICBitmap*>(bitmap);
    if (!wic || !bmp) return false;
    bool ok = false;
    IWICStream* stream = nullptr;
    IWICBitmapEncoder* enc = nullptr;
    IWICBitmapFrameEncode* frame = nullptr;
    UINT w = 0, h = 0;
    bmp->GetSize(&w, &h);
    if (SUCCEEDED(wic->CreateStream(&stream)) && SUCCEEDED(stream->InitializeFromFilename(path.c_str(), GENERIC_WRITE)) &&
        SUCCEEDED(wic->CreateEncoder(kContainerFormatPng, nullptr, &enc)) && SUCCEEDED(enc->Initialize(stream, WICBitmapEncoderNoCache)) &&
        SUCCEEDED(enc->CreateNewFrame(&frame, nullptr)) && SUCCEEDED(frame->Initialize(nullptr)) && SUCCEEDED(frame->SetSize(w, h))) {
        WICPixelFormatGUID fmt = kPixelFormat32bppBGRA;
        if (SUCCEEDED(frame->SetPixelFormat(&fmt)) && SUCCEEDED(frame->WriteSource(bmp, nullptr)) && SUCCEEDED(frame->Commit()) &&
            SUCCEEDED(enc->Commit())) {
            ok = true;
        }
    }
    SafeRelease(frame);
    SafeRelease(enc);
    SafeRelease(stream);
    return ok;
}

void OffscreenTarget::Release() {
    SafeRelease(rt);
    SafeRelease(bitmap);
    SafeRelease(wicFactory);
}

// ---------------------------------------------------------------------------------------------
typedef BOOL(WINAPI* PfnSetProcessDpiAwarenessContext)(HANDLE);
typedef UINT(WINAPI* PfnGetDpiForWindow)(HWND);
typedef HRESULT(WINAPI* PfnDwmSetWindowAttribute)(HWND, DWORD, LPCVOID, DWORD);

void EnablePerMonitorDpi() {
    HMODULE user = GetModuleHandleW(L"user32.dll");
    auto setCtx = LoadProc<PfnSetProcessDpiAwarenessContext>(user, "SetProcessDpiAwarenessContext");
    // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 == (HANDLE)-4
    if (setCtx && setCtx(reinterpret_cast<HANDLE>(static_cast<INT_PTR>(-4)))) return;
    SetProcessDPIAware();
}

UINT DpiForWindow(HWND hwnd) {
    static auto getDpi = LoadProc<PfnGetDpiForWindow>(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow");
    if (getDpi && hwnd) {
        UINT d = getDpi(hwnd);
        if (d) return d;
    }
    HDC dc = GetDC(nullptr);
    UINT d = dc ? (UINT)GetDeviceCaps(dc, LOGPIXELSX) : 96;
    if (dc) ReleaseDC(nullptr, dc);
    return d ? d : 96;
}

bool SystemPrefersDark() {
    DWORD value = 1, size = sizeof(value);
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0, KEY_READ, &key) == ERROR_SUCCESS) {
        if (RegQueryValueExW(key, L"AppsUseLightTheme", nullptr, nullptr, reinterpret_cast<LPBYTE>(&value), &size) != ERROR_SUCCESS) value = 1;
        RegCloseKey(key);
    }
    return value == 0;
}

void ApplyWindowFrameTheme(HWND hwnd, bool dark, bool popup) {
    static HMODULE dwm = LoadLibraryW(L"dwmapi.dll");
    static auto setAttr = LoadProc<PfnDwmSetWindowAttribute>(dwm, "DwmSetWindowAttribute");
    if (!setAttr || !hwnd) return;
    BOOL useDark = dark ? TRUE : FALSE;
    // DWMWA_USE_IMMERSIVE_DARK_MODE is 20 on Windows 10 20H1+ and 19 on earlier builds.
    if (FAILED(setAttr(hwnd, 20, &useDark, sizeof(useDark)))) setAttr(hwnd, 19, &useDark, sizeof(useDark));
    if (popup) {
        DWORD corner = 2;  // DWMWA_WINDOW_CORNER_PREFERENCE (33) = DWMWCP_ROUND
        setAttr(hwnd, 33, &corner, sizeof(corner));
        DWORD backdrop = 3;  // DWMWA_SYSTEMBACKDROP_TYPE (38) = DWMSBT_TRANSIENTWINDOW (acrylic)
        setAttr(hwnd, 38, &backdrop, sizeof(backdrop));
    } else {
        DWORD backdrop = 2;  // DWMSBT_MAINWINDOW (Mica) - visible in the caption area
        setAttr(hwnd, 38, &backdrop, sizeof(backdrop));
    }
}

// ---------------------------------------------------------------------------------------------
// Flyout backdrop
typedef LONG(WINAPI* PfnRtlGetVersion)(OSVERSIONINFOW*);

namespace {

struct Margins {  // MARGINS without pulling in uxtheme.h
    int left, right, top, bottom;
};
typedef HRESULT(WINAPI* PfnDwmExtendFrameIntoClientArea)(HWND, const Margins*);

// Undocumented but stable since Windows 10 1803 (user32!SetWindowCompositionAttribute).
struct AccentPolicy {
    DWORD accentState;  // 0 disabled, 4 ACCENT_ENABLE_ACRYLICBLURBEHIND
    DWORD accentFlags;
    DWORD gradientColor;  // AABBGGRR
    DWORD animationId;
};
struct WindowCompositionAttribData {
    DWORD attrib;  // 19 = WCA_ACCENT_POLICY
    void* data;
    SIZE_T size;
};
typedef BOOL(WINAPI* PfnSetWindowCompositionAttribute)(HWND, WindowCompositionAttribData*);

HMODULE DwmModule() {
    static HMODULE dwm = LoadLibraryW(L"dwmapi.dll");
    return dwm;
}

bool SetAccent(HWND hwnd, DWORD state, DWORD abgr) {
    static auto setWca = LoadProc<PfnSetWindowCompositionAttribute>(GetModuleHandleW(L"user32.dll"), "SetWindowCompositionAttribute");
    if (!setWca) return false;
    AccentPolicy policy = {state, state ? 2u : 0u, abgr, 0};
    WindowCompositionAttribData data = {19, &policy, sizeof(policy)};
    return setWca(hwnd, &data) != FALSE;
}

DWORD ToAbgr(Color c) {
    auto b = [](float v) { return (DWORD)(std::min(1.f, std::max(0.f, v)) * 255.f + .5f); };
    return (b(c.a) << 24) | (b(c.b) << 16) | (b(c.g) << 8) | b(c.r);
}

}  // namespace

DWORD WindowsBuildNumber() {
    static DWORD build = [] {
        auto rtlGetVersion = LoadProc<PfnRtlGetVersion>(GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion");
        OSVERSIONINFOW vi = {};
        vi.dwOSVersionInfoSize = sizeof(vi);
        if (!rtlGetVersion || rtlGetVersion(&vi) != 0 || vi.dwMajorVersion < 10) return (DWORD)0;
        return vi.dwBuildNumber;
    }();
    return build;
}

bool TranslucencyAllowed() {
    // Settings > Personalisation > Colours > Transparency effects.
    DWORD value = 1, size = sizeof(value);
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0, KEY_READ, &key) == ERROR_SUCCESS) {
        if (RegQueryValueExW(key, L"EnableTransparency", nullptr, nullptr, reinterpret_cast<LPBYTE>(&value), &size) != ERROR_SUCCESS) value = 1;
        RegCloseKey(key);
    }
    if (value == 0) return false;
    HIGHCONTRASTW hc = {};
    hc.cbSize = sizeof(hc);
    if (SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(hc), &hc, 0) && (hc.dwFlags & HCF_HIGHCONTRASTON)) return false;
    // Battery saver: SYSTEM_POWER_STATUS.SystemStatusFlag (4th byte; older headers call it Reserved1).
    SYSTEM_POWER_STATUS ps = {};
    if (GetSystemPowerStatus(&ps) && (reinterpret_cast<const BYTE*>(&ps)[3] & 1)) return false;
    return true;
}

Backdrop ApplyFlyoutBackdrop(HWND hwnd, bool dark, bool allowTranslucent) {
    if (!hwnd) return Backdrop::Solid;
    static auto setAttr = LoadProc<PfnDwmSetWindowAttribute>(DwmModule(), "DwmSetWindowAttribute");
    static auto extend = LoadProc<PfnDwmExtendFrameIntoClientArea>(DwmModule(), "DwmExtendFrameIntoClientArea");
    const DWORD build = WindowsBuildNumber();
    const bool allowed = allowTranslucent && TranslucencyAllowed();
    BOOL useDark = dark ? TRUE : FALSE;
    if (setAttr) {
        if (FAILED(setAttr(hwnd, 20, &useDark, sizeof(useDark)))) setAttr(hwnd, 19, &useDark, sizeof(useDark));
        DWORD corner = 2;  // DWMWA_WINDOW_CORNER_PREFERENCE (33) = DWMWCP_ROUND (Windows 11; ignored before)
        setAttr(hwnd, 33, &corner, sizeof(corner));
    }

    // 1. Windows 11 22H2+: system acrylic for a transient window behind a frame extended over the
    //    whole client area; we then draw with per-pixel alpha (D2D premultiplied target).
    if (allowed && build >= 22621 && setAttr && extend) {
        DWORD backdrop = 3;  // DWMWA_SYSTEMBACKDROP_TYPE (38) = DWMSBT_TRANSIENTWINDOW
        Margins m = {-1, -1, -1, -1};
        if (SUCCEEDED(setAttr(hwnd, 38, &backdrop, sizeof(backdrop))) && SUCCEEDED(extend(hwnd, &m))) {
            SetAccent(hwnd, 0, 0);
            return Backdrop::SystemAcrylic;
        }
    }
    // Anything else: no system backdrop, frame not extended.
    if (setAttr) {
        DWORD none = 1;  // DWMSBT_NONE
        setAttr(hwnd, 38, &none, sizeof(none));
    }
    if (extend) {
        Margins zero = {0, 0, 0, 0};
        extend(hwnd, &zero);
    }
    // 2. Windows 10 1803+ / early Windows 11: acrylic blur behind through the accent policy.
    if (allowed && build >= 17134 && SetAccent(hwnd, 4, ToAbgr(GetTheme(dark).flyoutAcrylicTint))) return Backdrop::AccentAcrylic;
    // 3. Solid (classic rounded background).
    SetAccent(hwnd, 0, 0);
    return Backdrop::Solid;
}

}  // namespace vk
