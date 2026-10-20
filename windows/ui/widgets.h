// VisKey for Windows - small immediate-mode widget kit on top of Gfx (MIT License, 1VISION).
// Each frame the page code draws widgets and registers hit rectangles; mouse and keyboard input is
// resolved against the rectangles of the previous frame.
#pragma once

#include "gfx.h"

namespace vk {

struct Hit {
    int id = 0;
    RectF r{};
    bool focusable = true;
};

enum class Btn { Standard, Accent, Subtle, Danger, DangerSubtle };

class Ui {
public:
    Gfx* g = nullptr;
    const Theme* t = nullptr;
    bool live = true;  // false while rendering snapshots
    int hot = 0, pressed = 0, focus = 0;
    bool focusVisible = false;
    RectF clip = R(-1e6f, -1e6f, 1e6f, 1e6f);
    std::vector<Hit> hits;

    void Reset() { hits.clear(); }
    void Add(int id, const RectF& r, bool focusable = true);
    int HitTest(float x, float y) const;
    const Hit* Find(int id) const;
    int NextFocus(int dir) const;
    bool Hot(int id) const { return id && hot == id; }
    bool Pressed(int id) const { return id && pressed == id && hot == id; }
    bool Focused(int id) const { return live && focusVisible && id && focus == id; }
    void FocusRing(const RectF& r, float radius, int id);

    void Card(const RectF& r, bool hover = false, bool highlight = false);
    // "Bật"/"Tắt" label + switch, right-aligned at `right`. Registers id over label + switch.
    void Toggle(int id, float right, float cy, bool on, bool enabled = true, bool label = true);
    static float ToggleWidth() { return 40.f + 12.f + 34.f; }
    void Dropdown(int id, const RectF& r, const std::wstring& text);
    float ButtonWidth(const std::wstring& text, Icon icon, float size = 14.f);
    void Button(int id, const RectF& r, const std::wstring& text, Btn kind, Icon icon = Icon::None, bool enabled = true);
    void IconButton(int id, const RectF& r, Icon icon, Color c, float iconSize = 16.f, bool focusable = true);
    // Segmented control; item i gets id idBase + i.
    void Segmented(int idBase, const RectF& r, const std::vector<std::wstring>& items, int sel, bool accentSel = false,
                   float textSize = 13.f);
    float KeycapsWidth(const std::vector<std::wstring>& parts, float size = 12.f);
    float Keycaps(float x, float cy, const std::vector<std::wstring>& parts, float size = 12.f, bool accent = false);
    // Text field drawn by us; when `active` the real EDIT control covers FieldTextRect().
    void Field(int id, const RectF& r, const std::wstring& text, const std::wstring& placeholder, bool active, bool mono,
               Icon trailing = Icon::None);
    static RectF FieldTextRect(const RectF& r, Icon trailing);
    void Chip(const RectF& r, const std::wstring& left, const std::wstring& right, Color bg, Color fg, bool mono);
    float ChipWidth(const std::wstring& left, const std::wstring& right, bool mono);
};

// Owner-drawn popup list (dropdowns and the "..." row menu). Posts WM_VK_POPUP(tag, index) to
// owner; index -1 means dismissed.
struct PopupItem {
    std::wstring text;
    bool danger = false;
};
void ShowPopupList(HWND owner, int tag, const RECT& anchorPx, const std::vector<PopupItem>& items, int selected, bool dark);
bool PopupRecentlyClosed(int tag);

}  // namespace vk
