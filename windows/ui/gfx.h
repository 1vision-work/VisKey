// VisKey for Windows - Direct2D / DirectWrite drawing layer (MIT License, 1VISION).
// d2d1.dll and dwrite.dll are loaded at runtime (LoadLibraryW + GetProcAddress) so VisKey64.exe
// keeps a minimal static import table. If either is missing, GfxStartup() returns false and the
// app falls back to the classic tray menu.
#pragma once

#include "common.h"

#include <d2d1.h>
#include <dwrite.h>

namespace vk {

struct Color {
    float r = 0, g = 0, b = 0, a = 1;
};

inline Color Rgb(uint32_t hex, float alpha = 1.f) {
    Color c;
    c.r = (float)((hex >> 16) & 0xFF) / 255.f;
    c.g = (float)((hex >> 8) & 0xFF) / 255.f;
    c.b = (float)(hex & 0xFF) / 255.f;
    c.a = alpha;
    return c;
}
inline Color WithAlpha(Color c, float a) {
    c.a = a;
    return c;
}
inline Color Mix(Color a, Color b, float t) {
    Color c;
    c.r = a.r + (b.r - a.r) * t;
    c.g = a.g + (b.g - a.g) * t;
    c.b = a.b + (b.b - a.b) * t;
    c.a = a.a + (b.a - a.a) * t;
    return c;
}
inline COLORREF ToColorRef(Color c) {
    return RGB((int)(c.r * 255.f + .5f), (int)(c.g * 255.f + .5f), (int)(c.b * 255.f + .5f));
}

using RectF = D2D1_RECT_F;
inline RectF R(float l, float t, float r, float b) {
    RectF x;
    x.left = l;
    x.top = t;
    x.right = r;
    x.bottom = b;
    return x;
}
inline RectF RW(float x, float y, float w, float h) { return R(x, y, x + w, y + h); }
inline RectF Inset(const RectF& r, float dx, float dy) { return R(r.left + dx, r.top + dy, r.right - dx, r.bottom - dy); }
inline bool Contains(const RectF& r, float x, float y) { return x >= r.left && x < r.right && y >= r.top && y < r.bottom; }
inline float Wd(const RectF& r) { return r.right - r.left; }
inline float Ht(const RectF& r) { return r.bottom - r.top; }
inline float Cy(const RectF& r) { return (r.top + r.bottom) * .5f; }
inline float Cx(const RectF& r) { return (r.left + r.right) * .5f; }

// Colors from design/tokens.json (ink = primary, jade = accent, charcoal = neutrals).
struct Theme {
    bool dark = false;
    Color windowBg, navSel, navHover, card, cardHover, cardStroke, sectionText;
    Color text, text2, text3;
    Color accent, accentFill, onAccentFill, accentStrong, accentTint, accentTintText;
    Color control, controlHover, controlPressed, controlStroke, controlBottom, fieldActive;
    Color toggleOffStroke, toggleOffKnob, toggleOnKnob;
    Color jade, jadeText, jadeTint;
    Color warning, warningTint, error, errorTint;
    Color separator, keycap, keycapStroke, focus;
    Color flyoutBg, flyoutFooter, tileOff, tileOffHover, segmentBg, segmentSel;
    Color tileBlue;  // app tile (ink-600 in both modes)
};
const Theme& GetTheme(bool dark);

enum class Face { Ui, Display, Mono, Icons };
enum class Weight { Regular = 400, Medium = 500, Semibold = 600, Bold = 700 };
enum class HAlign { Left, Center, Right };

struct TextStyle {
    Face face = Face::Ui;
    float size = 14.f;
    Weight weight = Weight::Regular;
};
inline TextStyle TS(float size, Weight w = Weight::Regular, Face f = Face::Ui) {
    TextStyle s;
    s.face = f;
    s.size = size;
    s.weight = w;
    return s;
}

enum class Icon {
    None, Sliders, Bolt, Grid, Keyboard, Update, Info, Search, ChevronDown, ChevronUp, ChevronRight,
    Globe, Terminal, Power, Upload, Download, Edit, Trash, More, Plus, Close, Copy, External, Warning,
    Check, Arrow, Code, Bug, Document, Folder, Undo, Window
};

bool GfxStartup();
void GfxShutdown();
ID2D1Factory* D2D();
IDWriteFactory* DWrite();
bool HasIconFont();

class Gfx {
public:
    explicit Gfx(ID2D1RenderTarget* rt);
    ~Gfx();
    Gfx(const Gfx&) = delete;
    Gfx& operator=(const Gfx&) = delete;

    ID2D1RenderTarget* Target() const { return rt_; }

    void Clear(Color c);
    void Fill(const RectF& r, Color c);
    void FillRound(const RectF& r, float radius, Color c);
    void StrokeRound(const RectF& r, float radius, Color c, float width = 1.f);
    void StrokeRoundDashed(const RectF& r, float radius, Color c, float width = 1.f);
    void Line(float x1, float y1, float x2, float y2, Color c, float width = 1.f);
    void FillCircle(float cx, float cy, float radius, Color c);
    void StrokeCircle(float cx, float cy, float radius, Color c, float width = 1.f);

    // Single-line text, vertically centred in r, trimmed with an ellipsis.
    void Text(const std::wstring& s, const RectF& r, const TextStyle& st, Color c, HAlign h = HAlign::Left);
    // Wrapped text from the top of r; returns the height used.
    float TextWrapped(const std::wstring& s, const RectF& r, const TextStyle& st, Color c, HAlign h = HAlign::Left);
    float TextWidth(const std::wstring& s, const TextStyle& st);
    float WrappedHeight(const std::wstring& s, float width, const TextStyle& st);

    void DrawIcon(Icon icon, float cx, float cy, float size, Color c);
    // VisKey mark (design/icons/symbol.svg): caret + V, drawn inside box.
    void Mark(const RectF& box, Color caret, Color vee);

    void PushClip(const RectF& r);
    void PopClip();

private:
    ID2D1SolidColorBrush* Brush(Color c);
    void Poly(const D2D1_POINT_2F* pts, int n, bool closed, Color c, float width, bool fill = false);
    void Arc(float cx, float cy, float r, float a0, float a1, Color c, float w);
    void VectorIcon(Icon icon, float cx, float cy, float size, Color c);

    ID2D1RenderTarget* rt_ = nullptr;
    ID2D1SolidColorBrush* brush_ = nullptr;
    ID2D1StrokeStyle* roundStroke_ = nullptr;
    ID2D1StrokeStyle* dashStroke_ = nullptr;
};

// Creates a WIC bitmap render target (offscreen), used by --ui-snapshot.
struct OffscreenTarget {
    IUnknown* wicFactory = nullptr;  // IWICImagingFactory
    IUnknown* bitmap = nullptr;      // IWICBitmap
    ID2D1RenderTarget* rt = nullptr;
    bool Create(UINT pxW, UINT pxH, float dpi);
    bool SavePng(const std::wstring& path);
    void Release();
};

template <class T>
inline void SafeRelease(T*& p) {
    if (p) {
        p->Release();
        p = nullptr;
    }
}

}  // namespace vk
