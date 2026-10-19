// VisKey for Windows - painters for the settings pages and the tray flyout (MIT License, 1VISION).
// The same code renders the live windows and the --ui-snapshot PNGs.
#include "views.h"

#include <algorithm>

namespace vk {

namespace {

const wchar_t* kNavTitles[kPageCount] = {L"Chung", L"Gõ tắt", L"Theo ứng dụng", L"Phím tắt", L"Cập nhật", L"Giới thiệu"};
const Icon kNavIcons[kPageCount] = {Icon::Sliders, Icon::Bolt, Icon::Grid, Icon::Keyboard, Icon::Update, Icon::Info};

// Words searched by "Tìm cài đặt" for each page (titles + subtitles of its rows).
const wchar_t* kPageKeywords[kPageCount] = {
    L"Chung Tiếng Việt đang bật Cách gõ Kiểu gõ Telex VNI Vị trí đặt dấu Kiểu cũ Kiểu mới hòa hoà Tự khôi phục từ tiếng Anh "
    L"Sửa lỗi mất nhân đôi chữ Chrome Edge Windows Search Excel trình duyệt Ghi nhớ Anh/Việt theo ứng dụng Khởi động cùng Windows",
    L"Gõ tắt macro Bật gõ tắt Quick Telex Thêm cụm từ Nhập Xuất Lọc viết tắt",
    L"Theo ứng dụng Khi mở ứng dụng mới Nhớ lần cuối Luôn VI Luôn EN Thêm ứng dụng Khôi phục danh sách mặc định",
    L"Phím tắt Bật / tắt Tiếng Việt Phím tắt chính Phím tắt phụ Đổi Telex VNI Mở Bảng điều khiển hotkey",
    L"Cập nhật Kiểm tra cập nhật Tự động kiểm tra cập nhật Lần kiểm tra cuối phiên bản tải về",
    L"Giới thiệu Mã nguồn Báo lỗi Bản phát hành Giấy phép MIT Sao chép thông tin hệ thống Khôi phục toàn bộ cài đặt",
};

struct Ctx {
    Ui& ui;
    Gfx& g;
    const Theme& t;
    SettingsView& v;
    const Model& m;
    float x0, x1;
    std::wstring q;

    bool Match(const std::wstring& a, const std::wstring& b = std::wstring()) const {
        if (q.empty()) return false;
        return FoldForSearch(a + L" " + b).find(q) != std::wstring::npos;
    }
};

bool PageMatches(int page, const std::wstring& q) {
    if (q.empty()) return true;
    return FoldForSearch(kPageKeywords[page]).find(q) != std::wstring::npos;
}

void PageTitle(Ctx& c, float y, const std::wstring& title) {
    c.g.Text(title, R(c.x0, y, c.x1 - 260.f, y + 44.f), TS(28.f, Weight::Semibold, Face::Display), c.t.text);
}

void Section(Ctx& c, float y, const std::wstring& text, bool caps = false) {
    if (caps) c.g.Text(text, R(c.x0 + 4.f, y, c.x1, y + 22.f), TS(12.f, Weight::Semibold), c.t.text2);
    else c.g.Text(text, R(c.x0 + 2.f, y, c.x1, y + 24.f), TS(15.f, Weight::Semibold), c.t.sectionText);
}

void RowIcon(Ctx& c, float cx, float cy, Icon icon, const wchar_t* textIcon, Color col) {
    if (textIcon) {
        bool mono = textIcon[0] == L'c';
        c.g.Text(textIcon, R(cx - 20.f, cy - 12.f, cx + 20.f, cy + 12.f), TS(mono ? 14.f : 17.f, Weight::Semibold, mono ? Face::Mono : Face::Ui), col,
                 HAlign::Center);
    } else {
        c.g.DrawIcon(icon, cx, cy, 18.f, col);
    }
}

// Title + subtitle block of a row, vertically centred on cy.
void RowText(Ctx& c, float left, float right, float cy, const std::wstring& title, const std::wstring& sub, bool dim = false,
             bool monoSub = false) {
    Color tc = dim ? c.t.text3 : c.t.text;
    Color sc = dim ? c.t.text3 : c.t.text2;
    if (sub.empty()) {
        c.g.Text(title, R(left, cy - 12.f, right, cy + 12.f), TS(15.f), tc);
    } else {
        c.g.Text(title, R(left, cy - 21.f, right, cy + 1.f), TS(15.f), tc);
        c.g.Text(sub, R(left, cy + 1.f, right, cy + 21.f), TS(13.f, Weight::Regular, monoSub ? Face::Mono : Face::Ui), sc);
    }
}

// A standard card row: icon, title, subtitle; the right side is drawn by the caller.
RectF Row(Ctx& c, float y, float h, int hoverId, Icon icon, const wchar_t* textIcon, const std::wstring& title, const std::wstring& sub,
          float rightReserve, bool monoSub = false) {
    RectF r = R(c.x0, y, c.x1, y + h);
    c.ui.Card(r, c.ui.Hot(hoverId), c.Match(title, sub));
    if (hoverId) c.ui.Add(hoverId, r, false);
    float cy = y + std::min(h, 64.f) * .5f;
    RowIcon(c, c.x0 + 30.f, cy, icon, textIcon, c.t.text);
    RowText(c, c.x0 + 60.f, c.x1 - rightReserve, cy, title, sub, false, monoSub);
    return r;
}

float HintWithKeycaps(Ctx& c, float x, float cy) {
    TextStyle st = TS(14.f);
    auto word = [&](const std::wstring& s) {
        float w = c.g.TextWidth(s, st);
        c.g.Text(s, R(x, cy - 11.f, x + w + 2.f, cy + 11.f), st, c.t.text2);
        x += w + 6.f;
    };
    const Hotkey& a = c.m.hotkeys[kHotkeyPrimary];
    const Hotkey& b = c.m.hotkeys[kHotkeySecondary];
    if (a.Empty() && b.Empty()) {
        word(L"Chưa đặt phím tắt — bấm công tắc để đổi");
        return x;
    }
    word(L"Nhấn");
    if (!a.Empty()) x = c.ui.Keycaps(x, cy, HotkeyParts(a)) + 6.f;
    if (!a.Empty() && !b.Empty()) word(L"hoặc");
    if (!b.Empty()) x = c.ui.Keycaps(x, cy, HotkeyParts(b)) + 6.f;
    word(L"để đổi");
    return x;
}

void AppTile(Gfx& g, const Theme& t, const RectF& r, float radius) {
    g.FillRound(r, radius, t.tileBlue);
    g.Mark(Inset(r, Wd(r) * 0.16f, Wd(r) * 0.16f), Rgb(0x3FE0C0), Rgb(0xFFFFFF));
}

// ------------------------------------------------------------------------------------------
float PageGeneral(Ctx& c, float y) {
    const Model& m = c.m;
    PageTitle(c, y, L"Chung");
    y += 60.f;

    // Status card
    {
        RectF r = R(c.x0, y, c.x1, y + 92.f);
        c.ui.Card(r, c.ui.Hot(kIdMasterVi), c.Match(L"Tiếng Việt đang bật"));
        c.ui.Add(kIdMasterVi, r, false);
        RectF tile = RW(c.x0 + 20.f, y + 20.f, 52.f, 52.f);
        c.g.FillRound(tile, 10.f, m.vietnamese ? (c.t.dark ? c.t.accentStrong : c.t.tileBlue) : (c.t.dark ? Rgb(0x4B515C) : Rgb(0x666D7A)));
        c.g.Text(m.vietnamese ? L"VI" : L"EN", tile, TS(22.f, Weight::Bold), Rgb(0xFFFFFF), HAlign::Center);
        if (m.vietnamese) c.g.FillCircle(tile.right - 9.f, tile.top + 9.f, 4.5f, Rgb(0x3FE0C0));
        c.g.Text(m.vietnamese ? L"Tiếng Việt đang bật" : L"Tiếng Việt đang tắt", R(c.x0 + 92.f, y + 18.f, c.x1 - 150.f, y + 46.f),
                 TS(18.f, Weight::Semibold), c.t.text);
        HintWithKeycaps(c, c.x0 + 92.f, y + 64.f);
        c.ui.Toggle(kIdMasterVi, c.x1 - 24.f, y + 46.f, m.vietnamese);
        y += 92.f + 30.f;
    }

    Section(c, y, L"Cách gõ");
    y += 34.f;

    const float h = 66.f, gap = 4.f;
    Row(c, y, h, 0, Icon::Keyboard, nullptr, L"Kiểu gõ", L"Telex: aa → â, w → ư · VNI: a6 → â", 200.f);
    c.ui.Dropdown(kIdMethodDrop, RW(c.x1 - 24.f - 156.f, y + h * .5f - 17.f, 156.f, 34.f), m.vni ? L"VNI" : L"Telex");
    y += h + gap;

    Row(c, y, h, 0, Icon::None, L"à", L"Vị trí đặt dấu", L"Kiểu cũ: hòa, thúy · Kiểu mới: hoà, thuý", 200.f);
    c.ui.Dropdown(kIdOrthoDrop, RW(c.x1 - 24.f - 156.f, y + h * .5f - 17.f, 156.f, 34.f), m.modernOrthography ? L"Kiểu mới (hoà)" : L"Kiểu cũ (hòa)");
    y += h + gap;

    Row(c, y, h, kIdAutoRestore, Icon::Undo, nullptr, L"Tự khôi phục từ tiếng Anh", L"window, software, book giữ nguyên khi quên tắt bộ gõ",
        Ui::ToggleWidth() + 40.f);
    c.ui.Toggle(kIdAutoRestore, c.x1 - 24.f, y + h * .5f, m.autoRestore);
    y += h + gap;

    Row(c, y, h, kIdFixAc, Icon::Globe, nullptr, L"Sửa lỗi mất / nhân đôi chữ", L"Chrome, Edge, Windows Search, Excel", Ui::ToggleWidth() + 40.f);
    c.ui.Toggle(kIdFixAc, c.x1 - 24.f, y + h * .5f, m.fixAutocomplete);
    y += h + gap;

    // Per-app memory (expandable)
    {
        c.v.chipRules.clear();
        std::vector<const AppRule*> chips;
        for (const auto& r : m.rules) {
            if (r.mode != RuleMode::Remember) chips.push_back(&r);
        }
        const size_t kMaxChips = 10;
        float expandedH = 0;
        std::vector<RectF> chipRects;
        float addW = 0;
        if (c.v.perAppExpanded) {
            float cx = c.x0 + 60.f, cyTop = y + h + 16.f, rowH = 32.f;
            float maxX = c.x1 - 24.f;
            for (size_t i = 0; i < chips.size() && i < kMaxChips; ++i) {
                float w = c.ui.ChipWidth(chips[i]->Name(), chips[i]->mode == RuleMode::VI ? L"VI" : L"EN", true);
                if (cx + w > maxX && cx > c.x0 + 60.f) {
                    cx = c.x0 + 60.f;
                    cyTop += rowH + 10.f;
                }
                chipRects.push_back(RW(cx, cyTop, w, rowH));
                cx += w + 10.f;
            }
            addW = c.ui.ChipWidth(L"+ Thêm ứng dụng", L"", false) + 4.f;
            if (cx + addW > maxX && cx > c.x0 + 60.f) {
                cx = c.x0 + 60.f;
                cyTop += rowH + 10.f;
            }
            chipRects.push_back(RW(cx, cyTop, addW, rowH));
            expandedH = cyTop + rowH + 18.f - (y + h);
        }
        // Only the header part of this card toggles; the chip area below has its own targets.
        RectF r = Row(c, y, h + expandedH, 0, Icon::Terminal, nullptr, L"Ghi nhớ Anh/Việt theo ứng dụng", L"Tự chuyển EN khi vào cửa sổ dòng lệnh",
                      Ui::ToggleWidth() + 80.f);
        c.ui.Add(kIdSmart, R(c.x0, y, c.x1 - 24.f - 34.f, y + h), false);
        c.ui.Toggle(kIdSmart, c.x1 - 24.f - 48.f, y + h * .5f, m.smartPerApp);
        c.ui.IconButton(kIdSmartExpand, RW(c.x1 - 24.f - 30.f, y + h * .5f - 15.f, 30.f, 30.f), c.v.perAppExpanded ? Icon::ChevronUp : Icon::ChevronDown,
                        c.t.text2, 14.f);
        if (c.v.perAppExpanded) {
            Color tint = c.t.dark ? Rgb(0x212328) : Rgb(0xF8F9FB);
            c.g.FillRound(R(r.left + 1.f, y + h, r.right - 1.f, r.bottom - 1.f), 7.f, tint);
            c.g.Fill(R(r.left + 1.f, y + h, r.right - 1.f, y + h + 10.f), tint);
            c.g.Line(r.left + 1.f, y + h, r.right - 1.f, y + h, c.t.separator, 1.f);
            for (size_t i = 0; i + 1 < chipRects.size(); ++i) {
                const AppRule* rule = chips[i];
                bool vi = rule->mode == RuleMode::VI;
                int id = kIdChipBase + (int)i;
                Color bg = vi ? c.t.jadeTint : c.t.accentTint;
                if (c.ui.Hot(id)) bg = Mix(bg, c.t.text, 0.06f);
                c.ui.Chip(chipRects[i], rule->Name(), vi ? L"VI" : L"EN", bg, vi ? c.t.jadeText : c.t.accentTintText, true);
                c.ui.FocusRing(chipRects[i], 16.f, id);
                c.ui.Add(id, chipRects[i]);
                c.v.chipRules.push_back(rule->exe);
            }
            RectF add = chipRects.back();
            if (c.ui.Hot(kIdChipAdd)) c.g.FillRound(add, 16.f, c.t.navHover);
            c.g.StrokeRoundDashed(add, 16.f, c.t.dark ? Rgb(0x666D7A) : Rgb(0xA3A9B5), 1.f);
            c.g.Text(L"+ Thêm ứng dụng", add, TS(14.f), c.t.accent, HAlign::Center);
            c.ui.FocusRing(add, 16.f, kIdChipAdd);
            c.ui.Add(kIdChipAdd, add);
        }
        y = r.bottom + gap;
    }

    Row(c, y, 60.f, kIdStartup, Icon::Power, nullptr, L"Khởi động cùng Windows", L"", Ui::ToggleWidth() + 40.f);
    c.ui.Toggle(kIdStartup, c.x1 - 24.f, y + 30.f, m.launchAtStartup);
    y += 60.f;
    return y;
}

// ------------------------------------------------------------------------------------------
float PageMacros(Ctx& c, float y) {
    const Model& m = c.m;
    PageTitle(c, y, L"Gõ tắt");
    {
        float wx = c.ui.ButtonWidth(L"Xuất", Icon::Download), wn = c.ui.ButtonWidth(L"Nhập", Icon::Upload);
        RectF bx = RW(c.x1 - wx, y + 4.f, wx, 36.f);
        RectF bn = RW(bx.left - 8.f - wn, y + 4.f, wn, 36.f);
        c.ui.Button(kIdImport, bn, L"Nhập", Btn::Standard, Icon::Upload);
        c.ui.Button(kIdExport, bx, L"Xuất", Btn::Standard, Icon::Download);
    }
    y += 62.f;

    const float h = 66.f;
    Row(c, y, h, kIdMacrosOn, Icon::Bolt, nullptr, L"Bật gõ tắt", L"Tự giữ hoa/thường: ko → không, Ko → Không, KO → KHÔNG", Ui::ToggleWidth() + 40.f);
    c.ui.Toggle(kIdMacrosOn, c.x1 - 24.f, y + h * .5f, m.macrosEnabled);
    y += h + 4.f;
    Row(c, y, h, kIdQuickTelex, Icon::None, L"cc", L"Quick Telex", L"cc=ch gg=gi kk=kh nn=ng qq=qu pp=ph tt=th", Ui::ToggleWidth() + 40.f, true);
    c.ui.Toggle(kIdQuickTelex, c.x1 - 24.f, y + h * .5f, m.quickTelex);
    y += h + 26.f;

    bool editing = !c.v.editingMacro.empty();
    Section(c, y, editing ? L"Sửa cụm từ" : L"Thêm cụm từ");
    y += 34.f;
    {
        RectF card = R(c.x0, y, c.x1, y + 66.f);
        c.ui.Card(card, false, c.Match(L"Thêm cụm từ"));
        float cy = y + 33.f;
        RectF key = RW(c.x0 + 20.f, cy - 18.f, 150.f, 36.f);
        c.ui.Field(kIdMacroKey, key, c.v.macroKey, L"viết tắt", c.v.activeField == kIdMacroKey, true);
        c.v.fieldRects[kIdMacroKey] = key;
        c.g.DrawIcon(Icon::Arrow, key.right + 20.f, cy, 16.f, c.t.text2);
        float addW = 92.f;
        float right = c.x1 - 20.f;
        RectF add = RW(right - addW, cy - 18.f, addW, 36.f);
        RectF cancel = RW(add.left - 8.f - 72.f, cy - 18.f, 72.f, 36.f);
        RectF val = R(key.right + 40.f, cy - 18.f, (editing ? cancel.left : add.left) - 12.f, cy + 18.f);
        c.ui.Field(kIdMacroValue, val, c.v.macroValue, L"nội dung thay thế", c.v.activeField == kIdMacroValue, false);
        c.v.fieldRects[kIdMacroValue] = val;
        if (editing) c.ui.Button(kIdMacroCancel, cancel, L"Huỷ", Btn::Standard);
        c.ui.Button(kIdMacroAdd, add, editing ? L"Lưu" : L"Thêm", Btn::Accent);
        y += 66.f + 24.f;
    }

    // List header: count, sort, filter
    std::wstring fq = FoldForSearch(c.v.macroFilter);
    std::vector<std::pair<std::string, std::string>> rows;
    for (const auto& kv : m.macros) {
        if (!fq.empty()) {
            std::wstring hay = FoldForSearch(Utf8ToWide(kv.first) + L" " + Utf8ToWide(kv.second));
            if (hay.find(fq) == std::wstring::npos) continue;
        }
        rows.push_back(kv);
    }
    if (c.v.macroSort == 1) {
        auto uses = [&](const std::string& k) {
            auto it = m.usage.find(k);
            return it == m.usage.end() ? 0u : it->second;
        };
        std::stable_sort(rows.begin(), rows.end(), [&](const std::pair<std::string, std::string>& a, const std::pair<std::string, std::string>& b) {
            return uses(a.first) > uses(b.first);
        });
    }
    {
        c.g.Text(FormatCount((uint32_t)m.macros.size()) + L" cụm từ", R(c.x0 + 2.f, y, c.x0 + 200.f, y + 34.f), TS(15.f, Weight::Semibold), c.t.sectionText);
        RectF filter = RW(c.x1 - 220.f, y, 220.f, 34.f);
        c.ui.Field(kIdMacroFilter, filter, c.v.macroFilter, L"Lọc", c.v.activeField == kIdMacroFilter, false, Icon::Search);
        c.v.fieldRects[kIdMacroFilter] = filter;
        c.ui.Segmented(kIdSortBase, RW(filter.left - 12.f - 176.f, y, 176.f, 34.f), {L"A → Z", L"Hay dùng"}, c.v.macroSort);
        y += 46.f;
    }

    c.v.visibleMacros.clear();
    if (rows.empty()) {
        RectF r = R(c.x0, y, c.x1, y + 64.f);
        c.ui.Card(r);
        c.g.Text(m.macros.empty() ? L"Chưa có cụm từ nào. Thêm ở ô phía trên hoặc bấm Nhập." : L"Không có cụm từ phù hợp", r, TS(14.f), c.t.text2,
                 HAlign::Center);
        y += 64.f + 8.f;
    }
    for (size_t i = 0; i < rows.size(); ++i) {
        const std::string& key = rows[i].first;
        int base = kIdMacroRowBase + (int)i * 4;
        c.v.visibleMacros.push_back(key);
        RectF r = R(c.x0, y, c.x1, y + 50.f);
        bool focusInRow = c.ui.focusVisible && c.ui.focus >= base && c.ui.focus < base + 4;
        bool hover = c.ui.hot >= base && c.ui.hot < base + 4;
        if (!c.ui.live && c.v.demoHoverMacro == key) hover = true;
        bool showActions = hover || focusInRow;
        c.ui.Card(r, hover);
        c.ui.Add(base, r, false);
        float cy = y + 25.f;
        std::wstring wk = Utf8ToWide(key);
        TextStyle mono = TS(14.f, Weight::Medium, Face::Mono);
        float kw = c.g.TextWidth(wk, mono);
        RectF chip = RW(c.x0 + 20.f, cy - 14.f, kw + 18.f, 28.f);
        c.g.FillRound(chip, 4.f, hover ? c.t.accentStrong : c.t.accentTint);
        c.g.Text(wk, chip, mono, hover ? Rgb(0xFFFFFF) : c.t.accentTintText, HAlign::Center);
        float valueLeft = std::max(c.x0 + 150.f, chip.right + 16.f);
        auto it = m.usage.find(key);
        uint32_t used = it == m.usage.end() ? 0u : it->second;
        c.g.Text(Utf8ToWide(rows[i].second), R(valueLeft, cy - 12.f, c.x1 - (used ? 330.f : 180.f), cy + 12.f), TS(15.f), c.t.text);
        if (used) c.g.Text(L"Đã dùng " + FormatCount(used) + L" lần", R(c.x1 - 320.f, cy - 11.f, c.x1 - 170.f, cy + 11.f), TS(13.f), c.t.text2);
        if (showActions) {
            RectF del = RW(c.x1 - 20.f - 64.f, cy - 16.f, 64.f, 32.f);
            RectF edit = RW(del.left - 8.f - 64.f, cy - 16.f, 64.f, 32.f);
            c.ui.Button(base + 1, edit, L"Sửa", Btn::Standard);
            c.ui.Button(base + 2, del, L"Xoá", Btn::DangerSubtle);
        } else {
            c.ui.IconButton(base + 3, RW(c.x1 - 20.f - 34.f, cy - 16.f, 34.f, 32.f), Icon::More, c.t.text2, 16.f);
        }
        y += 50.f + 4.f;
    }
    if (!m.macros.empty()) {
        float cy = y + 14.f;
        std::wstring left = L"Hiển thị " + FormatCount((uint32_t)rows.size()) + L" / " + FormatCount((uint32_t)m.macros.size()) + L" cụm từ";
        c.g.Text(left, R(c.x0 + 4.f, cy - 10.f, c.x0 + 300.f, cy + 10.f), TS(13.f), c.t.text2);
        std::wstring saved = c.m.snapshot ? L"Bản xem thử" : L"Đã lưu tự động";
        float sw = c.g.TextWidth(saved, TS(13.f));
        c.g.FillCircle(c.x1 - 4.f - sw - 12.f, cy, 3.5f, c.t.jade);
        c.g.Text(saved, R(c.x1 - 4.f - sw - 2.f, cy - 10.f, c.x1, cy + 10.f), TS(13.f), c.t.text2);
        y += 30.f;
    }
    return y;
}

// ------------------------------------------------------------------------------------------
float PagePerApp(Ctx& c, float y) {
    const Model& m = c.m;
    PageTitle(c, y, L"Theo ứng dụng");
    c.ui.Toggle(kIdPerAppMaster, c.x1 - 4.f, y + 22.f, m.smartPerApp);
    c.g.Text(m.smartPerApp ? L"VisKey nhớ lần cuối bạn dùng VI hay EN trong từng ứng dụng."
                           : L"Đang tắt: VisKey giữ nguyên VI/EN khi bạn chuyển ứng dụng.",
             R(c.x0 + 2.f, y + 44.f, c.x1, y + 66.f), TS(14.f), c.t.text2);
    y += 84.f;

    {
        RectF r = Row(c, y, 74.f, 0, Icon::Window, nullptr, L"Khi mở ứng dụng mới", L"Áp dụng cho ứng dụng chưa có trong danh sách", 304.f);
        c.ui.Segmented(kIdPolicyBase, RW(c.x1 - 20.f - 280.f, Cy(r) - 17.f, 280.f, 34.f), {L"Nhớ lần cuối", L"Luôn VI", L"Luôn EN"}, (int)m.policy, false,
                       13.f);
        y += 74.f + 16.f;
    }

    std::wstring fq = FoldForSearch(c.v.ruleFilter);
    std::vector<const AppRule*> rows;
    for (const auto& r : m.rules) {
        if (!fq.empty() && FoldForSearch(r.Name()).find(fq) == std::wstring::npos) continue;
        rows.push_back(&r);
    }
    {
        float bw = c.ui.ButtonWidth(L"Thêm ứng dụng", Icon::Plus) + 8.f;
        RectF filter = R(c.x0, y, c.x1 - bw - 12.f, y + 36.f);
        c.ui.Field(kIdRuleFilter, filter, c.v.ruleFilter, L"Lọc " + FormatCount((uint32_t)m.rules.size()) + L" ứng dụng…",
                   c.v.activeField == kIdRuleFilter, false, Icon::Search);
        c.v.fieldRects[kIdRuleFilter] = filter;
        c.ui.Button(kIdAddApp, RW(c.x1 - bw, y, bw, 36.f), L"Thêm ứng dụng", Btn::Accent, Icon::Plus);
        y += 52.f;
    }

    const float headH = 40.f, rowH = 60.f, footH = 50.f;
    float tableH = headH + std::max<float>(1.f, (float)rows.size()) * rowH + footH;
    RectF card = R(c.x0, y, c.x1, y + tableH);
    c.ui.Card(card, false, c.Match(L"Khôi phục danh sách mặc định"));
    float colSource = c.x0 + (c.x1 - c.x0) * 0.42f;
    float segW = 200.f;
    float segL = c.x1 - 20.f - 36.f - segW;
    c.g.Text(L"ỨNG DỤNG", R(c.x0 + 20.f, y, colSource, y + headH), TS(12.f, Weight::Semibold), c.t.text2);
    c.g.Text(L"NGUỒN", R(colSource, y, segL, y + headH), TS(12.f, Weight::Semibold), c.t.text2);
    c.g.Text(L"CHẾ ĐỘ", R(segL, y, c.x1, y + headH), TS(12.f, Weight::Semibold), c.t.text2);
    float ry = y + headH;
    c.g.Line(c.x0 + 1.f, ry, c.x1 - 1.f, ry, c.t.separator);
    c.v.visibleRules.clear();
    if (rows.empty()) {
        c.g.Text(m.rules.empty() ? L"Chưa có ứng dụng nào" : L"Không có ứng dụng phù hợp", R(c.x0, ry, c.x1, ry + rowH), TS(14.f), c.t.text2, HAlign::Center);
        ry += rowH;
    }
    for (size_t i = 0; i < rows.size(); ++i) {
        const AppRule& r = *rows[i];
        int base = kIdRuleRowBase + (int)i * 8;
        c.v.visibleRules.push_back(r.exe);
        float cy = ry + rowH * .5f;
        bool hover = c.ui.hot >= base && c.ui.hot < base + 8;
        if (hover) c.g.Fill(R(c.x0 + 1.f, ry, c.x1 - 1.f, ry + rowH), c.t.navHover);
        c.ui.Add(base, R(c.x0, ry, c.x1, ry + rowH), false);
        bool console = IsConsoleExe(r.exe);
        RectF ic = RW(c.x0 + 20.f, cy - 17.f, 34.f, 34.f);
        c.g.FillRound(ic, 8.f, console ? Rgb(0x16181C) : c.t.accentTint);
        if (console && c.t.dark) c.g.StrokeRound(ic, 8.f, c.t.cardStroke);
        c.g.DrawIcon(console ? Icon::Terminal : Icon::Grid, Cx(ic), cy, 16.f, console ? Rgb(0x3FE0C0) : c.t.accent);
        c.g.Text(r.Name(), R(c.x0 + 68.f, cy - 12.f, colSource - 12.f, cy + 12.f), TS(15.f, Weight::Semibold, Face::Ui), c.t.text);
        const wchar_t* src = r.source == RuleSource::Default ? L"Mặc định" : r.source == RuleSource::User ? L"Bạn đặt" : L"Tự ghi nhớ";
        c.g.Text(src, R(colSource, cy - 11.f, segL - 8.f, cy + 11.f), TS(14.f, r.source == RuleSource::User ? Weight::Semibold : Weight::Regular),
                 r.source == RuleSource::User ? c.t.accent : c.t.text2);
        int sel = (int)r.mode;
        std::wstring remember = (r.mode == RuleMode::Remember) ? (r.lastVi ? L"Nhớ · VI" : L"Nhớ · EN") : L"Nhớ";
        c.ui.Segmented(base + 1, RW(segL, cy - 17.f, segW, 34.f), {L"VI", L"EN", remember}, sel, true);
        c.ui.IconButton(base + 4, RW(c.x1 - 20.f - 30.f, cy - 15.f, 30.f, 30.f), Icon::Close, c.t.text2, 12.f);
        ry += rowH;
        c.g.Line(c.x0 + 1.f, ry, c.x1 - 1.f, ry, c.t.separator);
    }
    {
        float cy = ry + footH * .5f;
        c.g.Text(L"Hiển thị " + FormatCount((uint32_t)rows.size()) + L" / " + FormatCount((uint32_t)m.rules.size()) + L" ứng dụng",
                 R(c.x0 + 20.f, cy - 10.f, c.x0 + 320.f, cy + 10.f), TS(13.f), c.t.text2);
        std::wstring link = L"Khôi phục danh sách mặc định";
        float lw = c.g.TextWidth(link, TS(14.f, Weight::Semibold));
        RectF lr = R(c.x1 - 20.f - lw - 12.f, cy - 15.f, c.x1 - 14.f, cy + 15.f);
        if (c.ui.Hot(kIdRestoreRules)) c.g.FillRound(lr, 5.f, c.t.navHover);
        c.g.Text(link, lr, TS(14.f, Weight::Semibold), c.t.accent, HAlign::Center);
        c.ui.FocusRing(lr, 5.f, kIdRestoreRules);
        c.ui.Add(kIdRestoreRules, lr);
    }
    return card.bottom;
}

// ------------------------------------------------------------------------------------------
void Recorder(Ctx& c, int slot, float cy) {
    const Hotkey& hk = c.m.hotkeys[slot];
    int id = kIdHotkeyRecBase + slot;
    RectF box = RW(c.x1 - 20.f - 36.f - 210.f, cy - 18.f, 210.f, 36.f);
    bool rec = c.v.recordingSlot == slot;
    if (rec) {
        c.g.FillRound(box, 6.f, c.t.accentTint);
        c.g.StrokeRound(box, 6.f, c.t.accent, 2.f);
        std::wstring s = L"Nhấn tổ hợp phím…";
        float w = c.g.TextWidth(s, TS(14.f)) + 18.f;
        float x = Cx(box) - w * .5f;
        c.g.FillCircle(x + 4.f, cy, 4.f, c.t.error);
        c.g.Text(s, R(x + 16.f, box.top, box.right - 4.f, box.bottom), TS(14.f), c.t.accent);
    } else if (hk.Empty()) {
        if (c.ui.Hot(id)) c.g.FillRound(box, 6.f, c.t.navHover);
        c.g.StrokeRoundDashed(box, 6.f, c.t.text3, 1.f);
        c.g.Text(L"Ghi phím tắt", box, TS(14.f), c.t.text2, HAlign::Center);
    } else {
        c.g.FillRound(box, 6.f, c.ui.Hot(id) ? c.t.controlHover : c.t.control);
        c.g.StrokeRound(box, 6.f, c.t.controlStroke);
        auto parts = HotkeyParts(hk);
        float w = c.ui.KeycapsWidth(parts, 13.f);
        c.ui.Keycaps(Cx(box) - w * .5f, cy, parts, 13.f, true);
    }
    c.ui.FocusRing(box, 6.f, id);
    c.ui.Add(id, box);
    if (!hk.Empty() || rec) {
        c.ui.IconButton(kIdHotkeyClearBase + slot, RW(c.x1 - 20.f - 30.f, cy - 15.f, 30.f, 30.f), Icon::Close, c.t.text2, 12.f);
    }
}

float PageHotkeys(Ctx& c, float y) {
    const Model& m = c.m;
    PageTitle(c, y, L"Phím tắt");
    y += 60.f;

    // Hero: the primary combo, large.
    {
        RectF r = R(c.x0, y, c.x1, y + 112.f);
        c.ui.Card(r);
        const Hotkey& hk = m.hotkeys[kHotkeyPrimary].Empty() ? m.hotkeys[kHotkeySecondary] : m.hotkeys[kHotkeyPrimary];
        if (hk.Empty()) {
            c.g.Text(L"Chưa có phím tắt — bật/tắt tiếng Việt bằng bảng nhanh ở khay hệ thống", r, TS(14.f), c.t.text2, HAlign::Center);
        } else {
            auto parts = HotkeyParts(hk);
            float w = c.ui.KeycapsWidth(parts, 18.f);
            c.ui.Keycaps(Cx(r) - w * .5f, y + 42.f, parts, 18.f, true);
            c.g.Text(L"Nhấn " + HotkeyText(hk) + L" ở bất kỳ đâu để đổi giữa VI và EN", R(c.x0 + 20.f, y + 70.f, c.x1 - 20.f, y + 96.f), TS(14.f), c.t.text2,
                     HAlign::Center);
        }
        y += 112.f + 24.f;
    }

    Section(c, y, L"CHUYỂN NGÔN NGỮ", true);
    y += 28.f;
    {
        struct RowDef {
            int slot;
            const wchar_t* title;
            const wchar_t* sub;
        };
        const RowDef defs[] = {{kHotkeyPrimary, L"Bật / tắt Tiếng Việt", L"Phím tắt chính"},
                               {kHotkeySecondary, L"Phím tắt phụ", L"Dùng khi phím chính bị ứng dụng khác chiếm"},
                               {kHotkeyMethod, L"Đổi Telex ↔ VNI", L"Chuyển nhanh giữa hai kiểu gõ"}};
        const float rowH = 70.f;
        float total = 0;
        std::wstring warn[3];
        float warnH[3] = {0, 0, 0};
        for (int i = 0; i < 3; ++i) {
            warn[i] = HotkeyWarning(m, defs[i].slot);
            if (!warn[i].empty()) warnH[i] = std::max(52.f, c.g.WrappedHeight(warn[i], c.x1 - c.x0 - 80.f, TS(14.f)) + 28.f);
            total += rowH + warnH[i];
        }
        bool hl = c.Match(L"Bật / tắt Tiếng Việt Phím tắt chính Phím tắt phụ Đổi Telex VNI");
        RectF card = R(c.x0, y, c.x1, y + total);
        c.ui.Card(card, false, hl);
        float ry = y;
        for (int i = 0; i < 3; ++i) {
            float cy = ry + rowH * .5f;
            c.g.Text(defs[i].title, R(c.x0 + 24.f, cy - 21.f, c.x1 - 300.f, cy + 1.f), TS(15.f, Weight::Semibold), c.t.text);
            c.g.Text(defs[i].sub, R(c.x0 + 24.f, cy + 1.f, c.x1 - 300.f, cy + 21.f), TS(13.f), c.t.text2);
            Recorder(c, defs[i].slot, cy);
            ry += rowH;
            if (warnH[i] > 0) {
                RectF band = R(c.x0 + 1.f, ry, c.x1 - 1.f, ry + warnH[i]);
                c.g.Fill(band, c.t.warningTint);
                c.g.DrawIcon(Icon::Warning, c.x0 + 36.f, ry + 24.f, 18.f, c.t.warning);
                c.g.TextWrapped(warn[i], R(c.x0 + 60.f, ry + 14.f, c.x1 - 24.f, band.bottom), TS(14.f), c.t.warning);
                ry += warnH[i];
            }
            if (i < 2) c.g.Line(c.x0 + 1.f, ry, c.x1 - 1.f, ry, c.t.separator);
        }
        y += total + 24.f;
    }

    Section(c, y, L"KHÁC", true);
    y += 28.f;
    {
        RectF card = R(c.x0, y, c.x1, y + 140.f);
        c.ui.Card(card, false, c.Match(L"Mở Bảng điều khiển Thêm gõ tắt từ chữ đang chọn"));
        float cy = y + 35.f;
        c.g.Text(L"Mở Bảng điều khiển", R(c.x0 + 24.f, cy - 21.f, c.x1 - 280.f, cy + 1.f), TS(15.f, Weight::Semibold), c.t.text);
        c.g.Text(L"Bấm đúp biểu tượng VisKey ở khay hệ thống, hoặc bấm một lần rồi chọn “Mở Bảng điều khiển”",
                 R(c.x0 + 24.f, cy + 1.f, c.x1 - 24.f, cy + 21.f), TS(13.f), c.t.text2);
        c.g.Line(c.x0 + 1.f, y + 70.f, c.x1 - 1.f, y + 70.f, c.t.separator);
        cy = y + 105.f;
        c.g.Text(L"Thêm gõ tắt từ chữ đang chọn", R(c.x0 + 24.f, cy - 21.f, c.x1 - 280.f, cy + 1.f), TS(15.f, Weight::Semibold), c.t.text3);
        c.g.Text(L"Bôi đen một cụm từ rồi nhấn để lưu nhanh", R(c.x0 + 24.f, cy + 1.f, c.x1 - 280.f, cy + 21.f), TS(13.f), c.t.text3);
        std::wstring soon = L"Sắp có";
        float sw = c.g.TextWidth(soon, TS(12.f, Weight::Semibold)) + 22.f;
        RectF badge = RW(c.x1 - 24.f - sw, cy - 12.f, sw, 24.f);
        c.g.FillRound(badge, 12.f, c.t.segmentBg);
        c.g.Text(soon, badge, TS(12.f, Weight::Semibold), c.t.text2, HAlign::Center);
        y += 140.f;
    }
    return y;
}

// ------------------------------------------------------------------------------------------
float PageUpdate(Ctx& c, float y) {
    const Model& m = c.m;
    UpdateState state = UpdateStateOf(m);
    PageTitle(c, y, L"Cập nhật");
    y += 60.f;
    {
        RectF r = R(c.x0, y, c.x1, y + 92.f);
        c.ui.Card(r, false, c.Match(L"Kiểm tra cập nhật phiên bản"));
        AppTile(c.g, c.t, RW(c.x0 + 20.f, y + 20.f, 52.f, 52.f), 12.f);
        c.g.Text(L"VisKey " + AppVersion(), R(c.x0 + 92.f, y + 20.f, c.x1 - 240.f, y + 46.f), TS(18.f, Weight::Semibold), c.t.text);
        c.g.Text(std::wstring(L"Phiên bản hiện tại · bản portable cho Windows ") + ArchText(), R(c.x0 + 92.f, y + 48.f, c.x1 - 240.f, y + 70.f), TS(13.f),
                 c.t.text2);
        bool checking = state == UpdateState::Checking;
        std::wstring label = checking ? L"Đang kiểm tra…" : L"Kiểm tra cập nhật";
        float bw = c.ui.ButtonWidth(label, Icon::Update) + 8.f;
        c.ui.Button(kIdCheckUpdate, RW(c.x1 - 24.f - bw, y + 28.f, bw, 36.f), label, Btn::Accent, Icon::Update, !checking);
        y += 92.f + 4.f;
    }
    {
        const float h = 66.f;
        std::wstring last = m.lastUpdateCheck > 0 ? FormatLocalTime(m.lastUpdateCheck) : std::wstring(L"chưa có");
        if (m.haveUpdateResult && !m.update.ok && !m.updateChecking) last += L" (không thành công)";
        Row(c, y, h, kIdAutoUpdate, Icon::Update, nullptr, L"Tự động kiểm tra cập nhật", L"Mỗi ngày tối đa một lần · Lần kiểm tra cuối: " + last,
            Ui::ToggleWidth() + 40.f);
        c.ui.Toggle(kIdAutoUpdate, c.x1 - 24.f, y + h * .5f, m.autoUpdateCheck);
        y += h + 12.f;
    }
    {
        Icon icon = Icon::Info;
        Color ic = c.t.text2;
        std::wstring title, body;
        std::wstring available = AvailableUpdateVersion(m);
        switch (state) {
        case UpdateState::Idle:
            if (m.autoUpdateCheck) {
                title = L"Tự động kiểm tra mỗi ngày một lần";
                body = L"VisKey chỉ đọc danh sách bản phát hành công khai trên GitHub — không gửi phím gõ, mã máy hay dữ liệu nào của bạn. "
                       L"Khi có bản mới, VisKey chỉ báo ở đây và ở khay hệ thống; không tự tải hay tự cài.";
            } else {
                title = L"Chỉ kiểm tra khi bạn bấm nút";
                body = L"Đã tắt tự động kiểm tra. Khi bạn bấm “Kiểm tra cập nhật”, VisKey chỉ đọc danh sách bản phát hành công khai trên "
                       L"GitHub — không gửi phím gõ hay dữ liệu nào của bạn.";
            }
            break;
        case UpdateState::Checking:
            icon = Icon::Update;
            title = L"Đang kiểm tra cập nhật…";
            body = L"Đang đọc danh sách bản phát hành trên GitHub.";
            break;
        case UpdateState::UpToDate:
            icon = Icon::Check;
            ic = c.t.jadeText;
            title = L"Đã là phiên bản mới nhất";
            body = L"Bản mới nhất trên GitHub: " + m.update.latest + L". Bạn đang dùng " + AppVersion() + L".";
            break;
        case UpdateState::Newer:
            icon = Icon::Download;
            ic = c.t.accent;
            title = L"Có bản mới " + available;
            body = L"Bạn đang dùng " + AppVersion() + L". Tải tệp zip mới từ trang phát hành rồi thay VisKey64.exe. VisKey không tự tải hay tự cài.";
            break;
        case UpdateState::Failed:
            icon = Icon::Warning;
            ic = c.t.error;
            title = L"Không kiểm tra được cập nhật";
            body = m.update.error;
            break;
        }
        bool newer = state == UpdateState::Newer;
        float textW = c.x1 - c.x0 - 60.f - 24.f - (newer ? 190.f : 0.f);
        float bh = c.g.WrappedHeight(body, textW, TS(13.f));
        float h = std::max(72.f, 44.f + bh + 16.f);
        RectF r = R(c.x0, y, c.x1, y + h);
        c.ui.Card(r);
        if (newer) {  // banner: tinted card with an accent edge
            c.g.FillRound(Inset(r, 1.f, 1.f), 7.f, c.t.accentTint);
            c.g.FillRound(RW(r.left + 1.f, r.top + 12.f, 3.f, h - 24.f), 1.5f, c.t.accent);
        }
        c.g.DrawIcon(icon, c.x0 + 30.f, y + 30.f, 18.f, ic);
        c.g.Text(title, R(c.x0 + 60.f, y + 16.f, c.x0 + 60.f + textW, y + 40.f), TS(15.f, Weight::Semibold),
                 state == UpdateState::Failed ? c.t.error : newer ? c.t.accentTintText : c.t.text);
        c.g.TextWrapped(body, R(c.x0 + 60.f, y + 42.f, c.x0 + 60.f + textW, r.bottom), TS(13.f), newer ? c.t.text : c.t.text2);
        if (newer) c.ui.Button(kIdOpenDownload, RW(c.x1 - 24.f - 170.f, Cy(r) - 18.f, 170.f, 36.f), L"Mở trang tải về", Btn::Accent);
        y += h + 26.f;
    }
    Section(c, y, L"Cách cập nhật bản portable");
    y += 34.f;
    {
        std::wstring body =
            L"1. Tải tệp zip mới từ trang Bản phát hành.\n2. Thoát VisKey (chuột phải biểu tượng ở khay → Thoát).\n3. Giải nén và thay VisKey64.exe cũ "
            L"bằng tệp mới, rồi chạy lại.\nCài đặt, gõ tắt và quy tắc theo ứng dụng nằm trong Registry (HKCU\\Software\\VisKey) nên được giữ nguyên.";
        float textW = c.x1 - c.x0 - 48.f;
        float bh = c.g.WrappedHeight(body, textW, TS(14.f));
        RectF r = R(c.x0, y, c.x1, y + bh + 36.f + 52.f);
        c.ui.Card(r);
        c.g.TextWrapped(body, R(c.x0 + 24.f, y + 18.f, c.x1 - 24.f, r.bottom), TS(14.f), c.t.text);
        float bw = c.ui.ButtonWidth(L"Bản phát hành", Icon::External) + 8.f;
        c.ui.Button(kIdReleases, RW(c.x0 + 24.f, r.bottom - 52.f, bw, 34.f), L"Bản phát hành", Btn::Standard, Icon::External);
        y = r.bottom;
    }
    return y;
}

// ------------------------------------------------------------------------------------------
float PageAbout(Ctx& c, float y) {
    float cx = (c.x0 + c.x1) * .5f;
    y += 8.f;
    AppTile(c.g, c.t, RW(cx - 44.f, y, 88.f, 88.f), 20.f);
    y += 100.f;
    c.g.Text(L"VisKey", R(c.x0, y, c.x1, y + 40.f), TS(28.f, Weight::Bold, Face::Display), c.t.text, HAlign::Center);
    y += 42.f;
    c.g.Text(L"Bộ gõ tiếng Việt cho macOS, Linux & Windows · 1VISION", R(c.x0, y, c.x1, y + 22.f), TS(14.f), c.t.text2, HAlign::Center);
    y += 32.f;
    {
        std::wstring ver = AppVersion();
        float vw = c.g.TextWidth(ver, TS(13.f, Weight::Semibold)) + 26.f;
        std::wstring status;
        Color sb = c.t.jadeTint, sf = c.t.jadeText;
        UpdateState state = UpdateStateOf(c.m);
        if (state == UpdateState::UpToDate) status = L"✓ Đã là phiên bản mới nhất";
        else if (state == UpdateState::Newer) {
            status = L"Có bản mới " + AvailableUpdateVersion(c.m);
            sb = c.t.accentTint;
            sf = c.t.accentTintText;
        } else {
            status = std::wstring(L"Windows · ") + ArchText();
            sb = c.t.segmentBg;
            sf = c.t.text2;
        }
        float sw = c.g.TextWidth(status, TS(13.f, Weight::Semibold)) + 28.f;
        float total = vw + 8.f + sw;
        RectF vb = RW(cx - total * .5f, y, vw, 28.f);
        RectF stb = RW(vb.right + 8.f, y, sw, 28.f);
        c.g.FillRound(vb, 14.f, c.t.accentTint);
        c.g.Text(ver, vb, TS(13.f, Weight::Semibold), c.t.accentTintText, HAlign::Center);
        c.g.FillRound(stb, 14.f, sb);
        c.g.Text(status, stb, TS(13.f, Weight::Semibold), sf, HAlign::Center);
        y += 48.f;
    }

    float gap = 20.f;
    float colW = (c.x1 - c.x0 - gap) * .5f;
    float lx = c.x0, rx = c.x0 + colW + gap;
    Section(c, y, L"Liên kết");
    c.g.Text(L"Chẩn đoán", R(rx + 2.f, y, rx + colW, y + 24.f), TS(15.f, Weight::Semibold), c.t.sectionText);
    y += 32.f;
    const float rowH = 46.f;
    {
        RectF card = RW(lx, y, colW, rowH * 4);
        c.ui.Card(card, false, c.Match(L"Mã nguồn Báo lỗi Bản phát hành Giấy phép MIT"));
        struct LinkDef {
            Icon icon;
            const wchar_t* text;
            const wchar_t* right;
        };
        const LinkDef links[] = {{Icon::Code, L"Mã nguồn", nullptr}, {Icon::Bug, L"Báo lỗi", nullptr}, {Icon::Download, L"Bản phát hành & thay đổi", nullptr},
                           {Icon::Document, L"Giấy phép", L"MIT"}};
        for (int i = 0; i < 4; ++i) {
            RectF r = RW(lx, y + rowH * (float)i, colW, rowH);
            int id = kIdLinkBase + i;
            if (c.ui.Hot(id)) c.g.FillRound(Inset(r, 2.f, 2.f), 6.f, c.t.navHover);
            c.g.DrawIcon(links[i].icon, lx + 26.f, Cy(r), 16.f, c.t.text2);
            c.g.Text(links[i].text, R(lx + 48.f, r.top, r.right - 60.f, r.bottom), TS(15.f), c.t.text);
            if (links[i].right) c.g.Text(links[i].right, R(r.right - 80.f, r.top, r.right - 42.f, r.bottom), TS(14.f), c.t.text2, HAlign::Right);
            c.g.DrawIcon(Icon::External, r.right - 24.f, Cy(r), 13.f, c.t.text2);
            if (i < 3) c.g.Line(lx + 1.f, r.bottom, lx + colW - 1.f, r.bottom, c.t.separator);
            c.ui.FocusRing(Inset(r, 2.f, 2.f), 6.f, id);
            c.ui.Add(id, r);
        }
    }
    {
        RectF card = RW(rx, y, colW, rowH * 4);
        c.ui.Card(card);
        struct D {
            bool ok;
            const wchar_t* label;
            std::wstring value;
        };
        std::wstring winVer = c.m.snapshot ? std::wstring(L"Windows 11") : WindowsVersionText();
        D rows[] = {{c.m.hooksActive || c.m.snapshot, L"Bộ gõ", c.m.snapshot ? L"bản xem thử" : (c.m.hooksActive ? L"đang chạy" : L"chưa chạy")},
                    {true, L"Hệ điều hành", winVer},
                    {true, L"Kiểu gõ", std::wstring(c.m.vni ? L"VNI" : L"Telex") + (c.m.vietnamese ? L" · VI" : L" · EN")},
                    {true, L"Cài đặt", L"HKCU\\Software\\VisKey"}};
        for (int i = 0; i < 4; ++i) {
            RectF r = RW(rx, y + rowH * (float)i, colW, rowH);
            if (i < 3) c.g.FillCircle(rx + 26.f, Cy(r), 4.f, rows[i].ok ? c.t.jade : c.t.warning);
            else c.g.DrawIcon(Icon::Folder, rx + 26.f, Cy(r), 16.f, c.t.text2);
            c.g.Text(rows[i].label, R(rx + 44.f, r.top, rx + 140.f, r.bottom), TS(15.f), c.t.text);
            c.g.Text(rows[i].value, R(rx + 120.f, r.top, r.right - 20.f, r.bottom), TS(i == 3 ? 12.f : 13.f, Weight::Regular, i == 3 ? Face::Mono : Face::Ui), c.t.text2,
                     HAlign::Right);
            if (i < 3) c.g.Line(rx + 1.f, r.bottom, rx + colW - 1.f, r.bottom, c.t.separator);
        }
    }
    y += rowH * 4 + 24.f;
    {
        std::wstring copy = c.v.flashId == kIdCopyInfo ? L"Đã sao chép" : L"Sao chép thông tin hệ thống";
        float bw = c.ui.ButtonWidth(L"Sao chép thông tin hệ thống", Icon::Copy) + 8.f;
        c.ui.Button(kIdCopyInfo, RW(lx, y, bw, 40.f), copy, Btn::Standard, c.v.flashId == kIdCopyInfo ? Icon::Check : Icon::Copy);
        std::wstring reset = L"Khôi phục toàn bộ cài đặt…";
        float rw = c.ui.ButtonWidth(reset, Icon::None) + 8.f;
        c.ui.Button(kIdResetAll, RW(c.x1 - rw, y, rw, 40.f), reset, Btn::Danger);
        y += 40.f;
    }
    return y;
}

}  // namespace

UpdateState UpdateStateOf(const Model& m) {
    if (m.updateChecking) return UpdateState::Checking;
    if (m.haveUpdateResult && m.update.ok) return m.update.newer ? UpdateState::Newer : UpdateState::UpToDate;
    if (m.haveUpdateResult && !m.updateResultAuto) return UpdateState::Failed;  // automatic failures stay silent
    return AvailableUpdateVersion(m).empty() ? UpdateState::Idle : UpdateState::Newer;
}

float FlyoutHeight(const Model& m) { return kFlyoutH + (AvailableUpdateVersion(m).empty() ? 0.f : kFlyoutUpdateH); }

bool SettingsPageMatches(int page, const std::wstring& q) { return page >= 0 && page < kPageCount && PageMatches(page, q); }

std::wstring HotkeyWarning(const Model& m, int slot) {
    const Hotkey& h = m.hotkeys[slot];
    if (h.Empty()) return std::wstring();
    for (int i = 0; i < kHotkeyCount; ++i) {
        if (i != slot && m.hotkeys[i] == h) return L"Trùng với một phím tắt khác của VisKey. Hãy chọn tổ hợp khác.";
    }
    if (h.win && h.vk == VK_SPACE && !h.ctrl && !h.alt) {
        return L"Trùng với Win + Space của Windows (đổi bàn phím / ngôn ngữ nhập). Windows có thể nhận phím này trước VisKey.";
    }
    if (!h.ctrl && !h.alt && !h.win) {
        bool typing = (h.vk >= 'A' && h.vk <= 'Z') || (h.vk >= '0' && h.vk <= '9') || h.vk == VK_SPACE || (h.vk >= VK_OEM_1 && h.vk <= VK_OEM_8) ||
                      h.vk == VK_OEM_102 || h.vk == VK_RETURN || h.vk == VK_TAB || h.vk == VK_BACK;
        if (typing) return L"Phím này không kèm Ctrl, Alt hay Win nên bạn sẽ không gõ được nó nữa. Nên thêm một phím bổ trợ.";
    }
    return std::wstring();
}

void PaintFakeCaption(Gfx& g, const Theme& t, float w, float h) {
    g.Fill(R(0, 0, w, h), t.windowBg);
    g.Mark(RW(14.f, h * .5f - 9.f, 18.f, 18.f), t.dark ? Rgb(0x3FE0C0) : Rgb(0x14B89A), t.dark ? Rgb(0x8193FF) : Rgb(0x3044D6));
    g.Text(L"VisKey", R(42.f, 0, 300.f, h), TS(12.f), t.text);
    float bw = 46.f;
    float x = w - bw * 3;
    g.Line(x + 18.f, h * .5f, x + 28.f, h * .5f, t.text, 1.f);
    x += bw;
    g.StrokeRound(RW(x + 18.f, h * .5f - 5.f, 10.f, 10.f), 1.5f, t.text, 1.f);
    x += bw;
    g.Line(x + 18.f, h * .5f - 5.f, x + 28.f, h * .5f + 5.f, t.text, 1.f);
    g.Line(x + 28.f, h * .5f - 5.f, x + 18.f, h * .5f + 5.f, t.text, 1.f);
}

void PaintSettings(Ui& ui, SettingsView& v, const Model& m, float w, float h) {
    Gfx& g = *ui.g;
    const Theme& t = *ui.t;
    ui.Reset();
    v.fieldRects.clear();
    g.Clear(t.windowBg);
    std::wstring q = FoldForSearch(v.search);
    // Trim the query.
    while (!q.empty() && q.back() == L' ') q.pop_back();
    while (!q.empty() && q.front() == L' ') q.erase(q.begin());

    // ---- navigation pane ----
    ui.clip = R(0, 0, kNavW, h);
    AppTile(g, t, RW(24.f, 20.f, 56.f, 56.f), 13.f);
    g.Text(L"VisKey", R(94.f, 22.f, kNavW - 8.f, 48.f), TS(18.f, Weight::Semibold), t.text);
    g.FillCircle(99.f, 62.f, 4.f, m.vietnamese ? t.jade : t.text3);
    g.Text(m.vietnamese ? L"Đang gõ Tiếng Việt" : L"Đang gõ Tiếng Anh", R(110.f, 51.f, kNavW - 8.f, 73.f), TS(13.f), t.text2);
    RectF search = RW(16.f, 94.f, kNavW - 32.f, 36.f);
    ui.Field(kIdSearch, search, v.search, L"Tìm cài đặt", v.activeField == kIdSearch, false, Icon::Search);
    v.fieldRects[kIdSearch] = search;
    bool updateDot = !AvailableUpdateVersion(m).empty();
    for (int i = 0; i < kPageCount; ++i) {
        RectF r = RW(16.f, 146.f + (float)i * 44.f, kNavW - 32.f, 40.f);
        int id = kIdNavBase + i;
        bool sel = v.page == i;
        bool match = PageMatches(i, q);
        if (sel) g.FillRound(r, 6.f, t.navSel);
        else if (ui.Hot(id)) g.FillRound(r, 6.f, t.navHover);
        if (sel) g.FillRound(RW(r.left, Cy(r) - 9.f, 3.f, 18.f), 1.5f, t.accent);
        Color fg = match ? t.text : t.text3;
        g.DrawIcon(kNavIcons[i], r.left + 24.f, Cy(r), 17.f, match ? t.accent : t.text3);
        g.Text(kNavTitles[i], R(r.left + 50.f, r.top, r.right - 8.f, r.bottom), TS(15.f, sel ? Weight::Medium : Weight::Regular), fg);
        if (i == kPageUpdate && updateDot) g.FillCircle(r.right - 18.f, Cy(r), 4.5f, t.accent);  // "Có bản mới"
        ui.FocusRing(r, 6.f, id);
        ui.Add(id, r);
    }
    g.Text(L"VisKey · Phiên bản " + AppVersion(), R(24.f, h - 40.f, kNavW - 16.f, h - 16.f), TS(12.f), t.text3);

    // ---- content ----
    float x0 = kNavW + 16.f, x1 = w - 28.f;
    ui.clip = R(kNavW, 0, w, h);
    g.PushClip(ui.clip);
    Ctx c{ui, g, t, v, m, x0, x1, q};
    float top = 22.f - v.scroll;
    float bottom = top;
    switch (v.page) {
    case kPageGeneral: bottom = PageGeneral(c, top); break;
    case kPageMacros: bottom = PageMacros(c, top); break;
    case kPagePerApp: bottom = PagePerApp(c, top); break;
    case kPageHotkeys: bottom = PageHotkeys(c, top); break;
    case kPageUpdate: bottom = PageUpdate(c, top); break;
    default: bottom = PageAbout(c, top); break;
    }
    v.contentHeight = bottom - top + 22.f + 32.f;
    v.viewportHeight = h;
    // Overlay scroll indicator.
    if (v.contentHeight > h + 1.f) {
        float trackH = h - 16.f;
        float thumbH = std::max(40.f, trackH * h / v.contentHeight);
        float maxScroll = v.contentHeight - h;
        float ty = 8.f + (trackH - thumbH) * (maxScroll > 0 ? v.scroll / maxScroll : 0.f);
        g.FillRound(RW(w - 8.f, ty, 3.f, thumbH), 1.5f, WithAlpha(t.text2, 0.45f));
    }
    g.PopClip();
    ui.clip = R(-1e6f, -1e6f, 1e6f, 1e6f);
}

// ------------------------------------------------------------------------------------------
// Tray flyout (08-win11-system-tray.png)
void PaintFlyout(Ui& ui, const Model& m, float w, float h, FlyoutSurface surface) {
    Gfx& g = *ui.g;
    const Theme& t = *ui.t;
    const bool glass = surface == FlyoutSurface::Translucent;
    ui.Reset();
    // Translucent: the backdrop (acrylic) is drawn by DWM behind us; this layer keeps text readable.
    // Live windows Clear (replace last frame's pixels); snapshots composite over a sample desktop.
    if (glass && !ui.live) g.Fill(R(0, 0, w, h), t.flyoutLayer);
    else g.Clear(glass ? t.flyoutLayer : t.flyoutBg);
    const float pad = 18.f;
    float y = 16.f;
    // Header
    g.Mark(RW(pad, y + 3.f, 22.f, 22.f), t.dark ? Rgb(0x3FE0C0) : Rgb(0x14B89A), t.dark ? Rgb(0x8193FF) : Rgb(0x3044D6));
    g.Text(L"VisKey", R(pad + 32.f, y, w / 2, y + 28.f), TS(16.f, Weight::Semibold), t.text);
    const Hotkey& prim = m.hotkeys[kHotkeyPrimary];
    const Hotkey& sec = m.hotkeys[kHotkeySecondary];
    g.Text(prim.Empty() ? HotkeyText(sec) : HotkeyText(prim), R(w / 2, y, w - pad, y + 28.f), TS(13.f), t.text2, HAlign::Right);
    y += 42.f;

    // VI / EN
    float bw = (w - pad * 2 - 10.f) * .5f;
    for (int i = 0; i < 2; ++i) {
        bool vi = i == 0;
        bool on = vi == m.vietnamese;
        int id = vi ? kIdFlyVi : kIdFlyEn;
        RectF r = RW(pad + (float)i * (bw + 10.f), y, bw, 64.f);
        Color bg = on ? t.accentStrong : (ui.Hot(id) ? t.tileOffHover : t.tileOff);
        if (on && ui.Hot(id)) bg = Mix(bg, Rgb(0xFFFFFF), 0.06f);
        g.FillRound(r, 6.f, bg);
        Color fg = on ? Rgb(0xFFFFFF) : t.text;
        g.Text(vi ? L"VI" : L"EN", R(r.left + 14.f, r.top, r.left + 60.f, r.bottom), TS(24.f, Weight::Bold), on ? fg : t.text2);
        g.Text(vi ? L"Tiếng Việt" : L"English", R(r.left + 64.f, r.top + 13.f, r.right - 6.f, r.top + 33.f), TS(14.f, Weight::Semibold), fg);
        std::wstring sub = on ? L"Đang bật" : HotkeyText(vi ? prim : (sec.Empty() ? prim : sec));
        g.Text(sub, R(r.left + 64.f, r.top + 33.f, r.right - 6.f, r.top + 52.f), TS(13.f), on ? WithAlpha(fg, 0.85f) : t.text2);
        ui.FocusRing(r, 6.f, id);
        ui.Add(id, r);
    }
    y += 64.f + 16.f;

    // Kiểu gõ
    g.Text(L"Kiểu gõ", R(pad, y, w / 2, y + 34.f), TS(14.f), t.text);
    ui.Segmented(kIdFlyTelex, RW(w - pad - 136.f, y, 136.f, 34.f), {L"Telex", L"VNI"}, m.vni ? 1 : 0, false, 14.f);
    y += 34.f + 16.f;

    // Toggle tiles (3 x 2)
    struct Tile {
        Icon icon;
        const wchar_t* glyph;
        const wchar_t* caption;
        bool on;
    };
    const Tile tiles[6] = {{Icon::Undo, nullptr, L"Khôi phục EN", m.autoRestore},     {Icon::Bolt, nullptr, L"Gõ tắt", m.macrosEnabled},
                           {Icon::Terminal, nullptr, L"Theo ứng dụng", m.smartPerApp}, {Icon::Globe, nullptr, L"Sửa trình duyệt", m.fixAutocomplete},
                           {Icon::None, L"à", L"Dấu kiểu mới", m.modernOrthography},  {Icon::None, L"cc", L"Quick Telex", m.quickTelex}};
    float tw = (w - pad * 2 - 16.f) / 3.f;
    for (int i = 0; i < 6; ++i) {
        int col = i % 3, row = i / 3;
        RectF r = RW(pad + (float)col * (tw + 8.f), y + (float)row * 82.f, tw, 50.f);
        int id = kIdFlyTileBase + i;
        Color bg = tiles[i].on ? t.accentStrong : (ui.Hot(id) ? t.tileOffHover : t.tileOff);
        if (tiles[i].on && ui.Hot(id)) bg = Mix(bg, Rgb(0xFFFFFF), 0.08f);
        g.FillRound(r, 6.f, bg);
        Color fg = tiles[i].on ? Rgb(0xFFFFFF) : t.text;
        if (tiles[i].glyph) {
            bool mono = tiles[i].glyph[0] == L'c';
            g.Text(tiles[i].glyph, r, TS(mono ? 15.f : 18.f, Weight::Bold, mono ? Face::Mono : Face::Ui), fg, HAlign::Center);
        } else {
            g.DrawIcon(tiles[i].icon, Cx(r), Cy(r), 18.f, fg);
        }
        g.Text(tiles[i].caption, R(r.left - 4.f, r.bottom + 4.f, r.right + 4.f, r.bottom + 26.f), TS(13.f), t.text, HAlign::Center);
        ui.FocusRing(r, 6.f, id);
        ui.Add(id, R(r.left, r.top, r.right, r.bottom + 26.f));
    }

    // Footer
    float fy = h - 54.f;
    std::wstring available = AvailableUpdateVersion(m);
    if (!available.empty()) {
        RectF r = RW(pad, fy - kFlyoutUpdateH + 2.f, w - pad * 2, 34.f);
        g.FillRound(r, 6.f, ui.Hot(kIdFlyUpdate) ? Mix(t.accentTint, t.text, 0.06f) : t.accentTint);
        g.FillCircle(r.left + 16.f, Cy(r), 4.f, t.accent);
        g.Text(L"Có bản mới " + available, R(r.left + 30.f, r.top, r.right - 90.f, r.bottom), TS(14.f, Weight::Semibold), t.accentTintText);
        g.Text(L"Xem", R(r.right - 90.f, r.top, r.right - 30.f, r.bottom), TS(14.f), t.accentTintText, HAlign::Right);
        g.DrawIcon(Icon::ChevronRight, r.right - 16.f, Cy(r), 12.f, t.accentTintText);
        ui.FocusRing(r, 6.f, kIdFlyUpdate);
        ui.Add(kIdFlyUpdate, r);
    }
    g.Fill(R(0, fy, w, h), glass ? t.flyoutFooterLayer : t.flyoutFooter);
    g.Line(0, fy, w, fy, t.separator, 1.f);
    std::wstring link = L"Mở Bảng điều khiển";
    float lw = g.TextWidth(link, TS(15.f));
    RectF lr = RW(pad - 6.f, fy + 11.f, lw + 12.f, 32.f);
    if (ui.Hot(kIdFlyOpen)) g.FillRound(lr, 5.f, t.navHover);
    g.Text(link, lr, TS(15.f), t.accent, HAlign::Center);
    ui.FocusRing(lr, 5.f, kIdFlyOpen);
    ui.Add(kIdFlyOpen, lr);
    ui.IconButton(kIdFlyQuit, RW(w - pad - 32.f, fy + 11.f, 32.f, 32.f), Icon::Power, t.text, 17.f);
}

}  // namespace vk
