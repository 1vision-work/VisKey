// SPDX-License-Identifier: GPL-3.0-or-later
//
// VisKey for Windows (by 1VISION) — based on OpenKey by Mai Vũ Tuyên.
//
// Settings and About dialogs. Brand (design/brand.md): ink #3044D6 accent, secondary text #666D7A,
// "VisKey" + tagline "Gõ đúng ở mọi nơi." in the header, Segoe UI.

#include "dialogs.h"

#include <commctrl.h>
#include <shellapi.h>

#include <string>
#include <vector>

#include "app.h"
#include "config.h"
#include "version.h"
#include "viskey_engine.h"
#include "win_util.h"
#include "../res/resource.h"

namespace viskey {

namespace {

constexpr COLORREF kInk = RGB(0x30, 0x44, 0xD6);
constexpr COLORREF kTextSecondary = RGB(0x66, 0x6D, 0x7A);

HWND g_settings = nullptr;
HWND g_about = nullptr;

// Code tables in the order of the "Bảng mã" combo box.
const int kTableOrder[] = {VK_TABLE_UNICODE, VK_TABLE_UNICODE_COMPOUND, VK_TABLE_TCVN3, VK_TABLE_VNI_WINDOWS,
                           VK_TABLE_CP1258};
const wchar_t* const kTableNames[] = {L"Unicode", L"Unicode tổ hợp", L"TCVN3 (ABC)", L"VNI Windows", L"CP1258"};
const wchar_t* const kInputNames[] = {L"Telex", L"VNI", L"Simple Telex"};

struct CheckBinding { int id; bool Config::*field; };
const CheckBinding kChecks[] = {
    {IDC_VIETNAMESE, &Config::vietnamese},
    {IDC_CHECK_SPELLING, &Config::checkSpelling},
    {IDC_RESTORE_WRONG, &Config::restoreIfWrongSpelling},
    {IDC_MODERN_ORTHO, &Config::modernOrthography},
    {IDC_FREE_MARK, &Config::freeMark},
    {IDC_QUICK_TELEX, &Config::quickTelex},
    {IDC_UPPER_FIRST, &Config::upperCaseFirstChar},
    {IDC_ALLOW_ZFWJ, &Config::allowConsonantZFWJ},
    {IDC_QUICK_START, &Config::quickStartConsonant},
    {IDC_QUICK_END, &Config::quickEndConsonant},
    {IDC_FIX_AUTOCOMPLETE, &Config::fixBrowserAutocomplete},
    {IDC_USE_MACRO, &Config::useMacro},
    {IDC_MACRO_ENGLISH, &Config::macroInEnglishMode},
    {IDC_MACRO_AUTOCAPS, &Config::autoCapsMacro},
};

HFONT MakeFont(HWND hwnd, int pointSize, int weight) {
    const int dpi = int(GetDpiForWindow(hwnd));
    return CreateFontW(-MulDiv(pointSize, dpi, 72), 0, 0, 0, weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                       OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
}

HICON LoadAppIcon(int size) {
    HICON icon = nullptr;
    LoadIconWithScaleDown(app::Instance(), MAKEINTRESOURCEW(IDI_APP), size, size, &icon);
    return icon;
}

std::wstring GetText(HWND dlg, int id) {
    HWND h = GetDlgItem(dlg, id);
    std::wstring s(size_t(GetWindowTextLengthW(h)) + 1, L'\0');
    s.resize(size_t(GetWindowTextW(h, &s[0], int(s.size()))));
    return s;
}

// ---- Settings -----------------------------------------------------------------------------------

struct SettingsState {
    Config draft;
    HFONT titleFont = nullptr;
    HFONT bodyFont = nullptr;
    HICON headerIcon = nullptr;
    HICON bigIcon = nullptr;
    HICON smallIcon = nullptr;

    void FreeHeaderResources() {
        if (titleFont) DeleteObject(titleFont);
        if (bodyFont) DeleteObject(bodyFont);
        if (headerIcon) DestroyIcon(headerIcon);
        titleFont = bodyFont = nullptr;
        headerIcon = nullptr;
    }

    void MakeHeaderResources(HWND dlg) {
        FreeHeaderResources();
        titleFont = MakeFont(dlg, 15, FW_SEMIBOLD);
        bodyFont = MakeFont(dlg, 9, FW_NORMAL);
        headerIcon = LoadAppIcon(MulDiv(32, int(GetDpiForWindow(dlg)), 96));
    }
};

SettingsState* StateOf(HWND dlg) { return reinterpret_cast<SettingsState*>(GetWindowLongPtrW(dlg, GWLP_USERDATA)); }

void FillMacroList(HWND dlg, const Config& c) {
    HWND list = GetDlgItem(dlg, IDC_MACRO_LIST);
    ListView_DeleteAllItems(list);
    for (const auto& m : c.macros) {
        std::wstring abbr = Utf8ToWide(m.abbreviation);
        std::wstring text = Utf8ToWide(m.expansion);
        LVITEMW item{};
        item.mask = LVIF_TEXT;
        item.iItem = ListView_GetItemCount(list);
        item.pszText = &abbr[0];
        const int row = ListView_InsertItem(list, &item);
        ListView_SetItemText(list, row, 1, &text[0]);
    }
}

void LoadControls(HWND dlg, const Config& c) {
    for (const auto& b : kChecks) CheckDlgButton(dlg, b.id, c.*b.field ? BST_CHECKED : BST_UNCHECKED);
    const int input = c.inputType >= VK_INPUT_SIMPLE_TELEX_1 ? 2 : c.inputType;
    SendDlgItemMessageW(dlg, IDC_INPUT_TYPE, CB_SETCURSEL, WPARAM(input), 0);
    for (int i = 0; i < 5; i++)
        if (kTableOrder[i] == c.codeTable) SendDlgItemMessageW(dlg, IDC_CODE_TABLE, CB_SETCURSEL, WPARAM(i), 0);
    CheckRadioButton(dlg, IDC_HOTKEY_CTRL_SHIFT, IDC_HOTKEY_ALT_Z,
                     c.hotkey == ToggleHotkey::AltZ ? IDC_HOTKEY_ALT_Z : IDC_HOTKEY_CTRL_SHIFT);
    CheckDlgButton(dlg, IDC_RUN_AT_STARTUP, IsRunAtStartupEnabled() ? BST_CHECKED : BST_UNCHECKED);
    FillMacroList(dlg, c);
}

// Reads every control into `c` (the macro list is edited in place in the draft).
void ReadControls(HWND dlg, Config& c) {
    for (const auto& b : kChecks) c.*b.field = IsDlgButtonChecked(dlg, b.id) == BST_CHECKED;
    const LRESULT input = SendDlgItemMessageW(dlg, IDC_INPUT_TYPE, CB_GETCURSEL, 0, 0);
    c.inputType = input == 2 ? VK_INPUT_SIMPLE_TELEX_1 : (input == 1 ? VK_INPUT_VNI : VK_INPUT_TELEX);
    const LRESULT table = SendDlgItemMessageW(dlg, IDC_CODE_TABLE, CB_GETCURSEL, 0, 0);
    if (table >= 0 && table < 5) c.codeTable = kTableOrder[table];
    c.hotkey = IsDlgButtonChecked(dlg, IDC_HOTKEY_ALT_Z) == BST_CHECKED ? ToggleHotkey::AltZ : ToggleHotkey::CtrlShift;
}

bool ApplySettings(HWND dlg) {
    SettingsState* st = StateOf(dlg);
    ReadControls(dlg, st->draft);
    app::Commit(st->draft);
    const bool startup = IsDlgButtonChecked(dlg, IDC_RUN_AT_STARTUP) == BST_CHECKED;
    if (startup != IsRunAtStartupEnabled() && !SetRunAtStartup(startup)) {
        MessageBoxW(dlg, L"Không ghi được cài đặt khởi động cùng Windows.", L"VisKey", MB_ICONWARNING);
        return false;
    }
    return true;
}

void OnMacroAdd(HWND dlg) {
    SettingsState* st = StateOf(dlg);
    const Macro m{WideToUtf8(GetText(dlg, IDC_MACRO_ABBR)), WideToUtf8(GetText(dlg, IDC_MACRO_TEXT))};
    if (m.abbreviation.find(' ') != std::string::npos || !UpsertMacro(st->draft.macros, m)) {
        MessageBoxW(dlg, L"Nhập từ viết tắt (không có dấu cách) và nội dung.", L"VisKey", MB_ICONINFORMATION);
        return;
    }
    FillMacroList(dlg, st->draft);
    SetDlgItemTextW(dlg, IDC_MACRO_ABBR, L"");
    SetDlgItemTextW(dlg, IDC_MACRO_TEXT, L"");
    SetFocus(GetDlgItem(dlg, IDC_MACRO_ABBR));
}

void OnMacroDelete(HWND dlg) {
    SettingsState* st = StateOf(dlg);
    HWND list = GetDlgItem(dlg, IDC_MACRO_LIST);
    const int row = ListView_GetNextItem(list, -1, LVNI_SELECTED);
    if (row < 0) return;
    wchar_t abbr[256] = {};
    ListView_GetItemText(list, row, 0, abbr, 256);
    RemoveMacro(st->draft.macros, WideToUtf8(abbr));
    FillMacroList(dlg, st->draft);
}

void OnMacroSelected(HWND dlg) {
    HWND list = GetDlgItem(dlg, IDC_MACRO_LIST);
    const int row = ListView_GetNextItem(list, -1, LVNI_SELECTED);
    if (row < 0) return;
    wchar_t buf[2048] = {};
    ListView_GetItemText(list, row, 0, buf, 2048);
    SetDlgItemTextW(dlg, IDC_MACRO_ABBR, buf);
    ListView_GetItemText(list, row, 1, buf, 2048);
    SetDlgItemTextW(dlg, IDC_MACRO_TEXT, buf);
}

void DrawHeader(HWND dlg, const DRAWITEMSTRUCT& di) {
    SettingsState* st = StateOf(dlg);
    HDC dc = di.hDC;
    RECT r = di.rcItem;
    const int dpi = int(GetDpiForWindow(dlg));
    auto px = [dpi](int v) { return MulDiv(v, dpi, 96); };

    FillRect(dc, &r, GetSysColorBrush(COLOR_WINDOW));
    RECT accent = {r.left, r.bottom - px(2), r.right, r.bottom};
    HBRUSH ink = CreateSolidBrush(kInk);
    FillRect(dc, &accent, ink);
    DeleteObject(ink);

    const int iconSize = px(32);
    const int pad = px(16);
    const int iconTop = (r.top + r.bottom - px(2) - iconSize) / 2;
    if (st->headerIcon) DrawIconEx(dc, r.left + pad, iconTop, st->headerIcon, iconSize, iconSize, 0, nullptr, DI_NORMAL);

    SetBkMode(dc, TRANSPARENT);
    const int textLeft = r.left + pad + iconSize + px(12);
    HGDIOBJ old = SelectObject(dc, st->titleFont);
    SetTextColor(dc, GetSysColor(COLOR_WINDOWTEXT));
    RECT title = {textLeft, iconTop - px(4), r.right - pad, iconTop + px(22)};
    DrawTextW(dc, L"VisKey", -1, &title, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);
    SelectObject(dc, st->bodyFont);
    SetTextColor(dc, kTextSecondary);
    RECT tagline = {textLeft, iconTop + px(20), r.right - pad, iconTop + px(40)};
    DrawTextW(dc, L"Gõ đúng ở mọi nơi.", -1, &tagline, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);
    RECT by = {textLeft, r.top, r.right - pad, r.bottom - px(2)};
    DrawTextW(dc, L"by 1VISION · v" VISKEY_VERSION_WSTR, -1, &by, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    SelectObject(dc, old);
}

void InitSettings(HWND dlg) {
    auto* st = new SettingsState();
    st->draft = app::CurrentConfig();
    SetWindowLongPtrW(dlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(st));
    st->MakeHeaderResources(dlg);
    st->bigIcon = LoadAppIcon(GetSystemMetrics(SM_CXICON));
    st->smallIcon = LoadAppIcon(GetSystemMetrics(SM_CXSMICON));
    SendMessageW(dlg, WM_SETICON, ICON_BIG, LPARAM(st->bigIcon));
    SendMessageW(dlg, WM_SETICON, ICON_SMALL, LPARAM(st->smallIcon));

    for (const wchar_t* name : kInputNames) SendDlgItemMessageW(dlg, IDC_INPUT_TYPE, CB_ADDSTRING, 0, LPARAM(name));
    for (const wchar_t* name : kTableNames) SendDlgItemMessageW(dlg, IDC_CODE_TABLE, CB_ADDSTRING, 0, LPARAM(name));

    HWND list = GetDlgItem(dlg, IDC_MACRO_LIST);
    ListView_SetExtendedListViewStyle(list, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    RECT cols = {0, 0, 54, 0};
    MapDialogRect(dlg, &cols);
    RECT lr;
    GetClientRect(list, &lr);
    LVCOLUMNW col{};
    col.mask = LVCF_TEXT | LVCF_WIDTH;
    col.cx = cols.right;
    col.pszText = const_cast<wchar_t*>(L"Viết tắt");
    ListView_InsertColumn(list, 0, &col);
    col.cx = lr.right - cols.right - GetSystemMetrics(SM_CXVSCROLL);
    col.pszText = const_cast<wchar_t*>(L"Nội dung");
    ListView_InsertColumn(list, 1, &col);

    SendDlgItemMessageW(dlg, IDC_MACRO_ABBR, EM_SETCUEBANNER, TRUE, LPARAM(L"vd: vk"));
    SendDlgItemMessageW(dlg, IDC_MACRO_TEXT, EM_SETCUEBANNER, TRUE, LPARAM(L"vd: VisKey"));
    SetDlgItemTextW(dlg, IDC_CONFIG_PATH, (L"Lưu tại: " + app::ConfigPath()).c_str());

    LoadControls(dlg, st->draft);
}

INT_PTR CALLBACK SettingsProc(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_INITDIALOG:
        InitSettings(dlg);
        return TRUE;
    case WM_DRAWITEM:
        if (wParam == IDC_HEADER) {
            DrawHeader(dlg, *reinterpret_cast<const DRAWITEMSTRUCT*>(lParam));
            return TRUE;
        }
        break;
    case WM_DPICHANGED:
        if (SettingsState* st = StateOf(dlg)) st->MakeHeaderResources(dlg);
        InvalidateRect(GetDlgItem(dlg, IDC_HEADER), nullptr, TRUE);
        break;
    case WM_CTLCOLORSTATIC:
        if (GetDlgCtrlID(reinterpret_cast<HWND>(lParam)) == IDC_CONFIG_PATH) {
            HDC dc = reinterpret_cast<HDC>(wParam);
            SetTextColor(dc, kTextSecondary);
            SetBkMode(dc, TRANSPARENT);
            return reinterpret_cast<INT_PTR>(GetSysColorBrush(COLOR_BTNFACE));
        }
        break;
    case WM_NOTIFY: {
        const auto* hdr = reinterpret_cast<const NMHDR*>(lParam);
        if (hdr->idFrom == IDC_MACRO_LIST && hdr->code == LVN_ITEMCHANGED) {
            const auto* lv = reinterpret_cast<const NMLISTVIEW*>(lParam);
            if ((lv->uNewState & LVIS_SELECTED) && !(lv->uOldState & LVIS_SELECTED)) OnMacroSelected(dlg);
        }
        break;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            if (ApplySettings(dlg)) DestroyWindow(dlg);
            return TRUE;
        case IDCANCEL:
            DestroyWindow(dlg);
            return TRUE;
        case IDC_APPLY:
            ApplySettings(dlg);
            return TRUE;
        case IDC_MACRO_ADD: OnMacroAdd(dlg); return TRUE;
        case IDC_MACRO_DELETE: OnMacroDelete(dlg); return TRUE;
        }
        break;
    case WM_DESTROY:
        if (SettingsState* st = StateOf(dlg)) {
            st->FreeHeaderResources();
            if (st->bigIcon) DestroyIcon(st->bigIcon);
            if (st->smallIcon) DestroyIcon(st->smallIcon);
            delete st;
            SetWindowLongPtrW(dlg, GWLP_USERDATA, 0);
        }
        g_settings = nullptr;
        return TRUE;
    }
    return FALSE;
}

// ---- About --------------------------------------------------------------------------------------

struct AboutState {
    HFONT nameFont = nullptr;
    HICON icon = nullptr;
};

INT_PTR CALLBACK AboutProc(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_INITDIALOG: {
        auto* st = new AboutState();
        SetWindowLongPtrW(dlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(st));
        RECT r;
        GetClientRect(GetDlgItem(dlg, IDC_ABOUT_ICON), &r);
        st->icon = LoadAppIcon(r.right - r.left);
        SendDlgItemMessageW(dlg, IDC_ABOUT_ICON, STM_SETICON, WPARAM(st->icon), 0);
        st->nameFont = MakeFont(dlg, 14, FW_SEMIBOLD);
        SendDlgItemMessageW(dlg, IDC_ABOUT_NAME, WM_SETFONT, WPARAM(st->nameFont), TRUE);
        SetDlgItemTextW(dlg, IDC_ABOUT_VERSION, L"Phiên bản " VISKEY_VERSION_WSTR L" cho Windows");
        return TRUE;
    }
    case WM_NOTIFY: {
        const auto* hdr = reinterpret_cast<const NMHDR*>(lParam);
        if (hdr->idFrom == IDC_ABOUT_LINK && (hdr->code == NM_CLICK || hdr->code == NM_RETURN)) {
            const auto* link = reinterpret_cast<const NMLINK*>(lParam);
            ShellExecuteW(dlg, L"open", link->item.szUrl, nullptr, nullptr, SW_SHOWNORMAL);
            return TRUE;
        }
        break;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            DestroyWindow(dlg);
            return TRUE;
        }
        break;
    case WM_DESTROY: {
        auto* st = reinterpret_cast<AboutState*>(GetWindowLongPtrW(dlg, GWLP_USERDATA));
        if (st) {
            if (st->nameFont) DeleteObject(st->nameFont);
            if (st->icon) DestroyIcon(st->icon);
            delete st;
        }
        g_about = nullptr;
        return TRUE;
    }
    }
    return FALSE;
}

void Present(HWND dlg) {
    ShowWindow(dlg, SW_SHOWNORMAL);
    SetForegroundWindow(dlg);
}

}  // namespace

void ShowSettingsDialog() {
    if (!g_settings)
        g_settings = CreateDialogParamW(app::Instance(), MAKEINTRESOURCEW(IDD_SETTINGS), nullptr, SettingsProc, 0);
    if (g_settings) Present(g_settings);
}

void ShowAboutDialog() {
    if (!g_about) g_about = CreateDialogParamW(app::Instance(), MAKEINTRESOURCEW(IDD_ABOUT), nullptr, AboutProc, 0);
    if (g_about) Present(g_about);
}

bool HandleDialogMessage(MSG* msg) {
    return (g_settings && IsDialogMessageW(g_settings, msg)) || (g_about && IsDialogMessageW(g_about, msg));
}

void RefreshSettingsDialog() {
    if (!g_settings) return;
    SettingsState* st = StateOf(g_settings);
    if (!st) return;
    std::vector<Macro> macros = std::move(st->draft.macros);  // keep unsaved macro edits
    st->draft = app::CurrentConfig();
    st->draft.macros = std::move(macros);
    LoadControls(g_settings, st->draft);
}

}  // namespace viskey
