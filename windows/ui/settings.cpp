// VisKey for Windows - settings window ("Bảng điều khiển"): custom-drawn with Direct2D, real EDIT
// controls for text input, keyboard navigation and per-monitor DPI (MIT License, 1VISION).
#include "views.h"

#include <algorithm>

namespace vk {

namespace {

struct SettingsWindow {
    HWND hwnd = nullptr;
    ID2D1HwndRenderTarget* rt = nullptr;
    Ui ui;
    SettingsView v;
    bool dark = false;
    UINT dpi = 96;
    std::map<int, HWND> edits;
    HFONT fontUi = nullptr, fontMono = nullptr;
    HBRUSH editBrush = nullptr;
    bool tracking = false;
};
SettingsWindow g_sw;

const UINT_PTR kFlashTimer = 1;

float Scale() { return (float)g_sw.dpi / 96.f; }

void Invalidate() {
    if (g_sw.hwnd) InvalidateRect(g_sw.hwnd, nullptr, FALSE);
}

RECT ToScreen(const RectF& r) {
    float s = Scale();
    RECT rc = {(LONG)(r.left * s), (LONG)(r.top * s), (LONG)(r.right * s), (LONG)(r.bottom * s)};
    MapWindowPoints(g_sw.hwnd, nullptr, reinterpret_cast<POINT*>(&rc), 2);
    return rc;
}

void ClampScroll() {
    float maxScroll = std::max(0.f, g_sw.v.contentHeight - g_sw.v.viewportHeight);
    g_sw.v.scroll = std::min(std::max(g_sw.v.scroll, 0.f), maxScroll);
}

void RebuildFonts() {
    if (g_sw.fontUi) DeleteObject(g_sw.fontUi);
    if (g_sw.fontMono) DeleteObject(g_sw.fontMono);
    int px = -(int)(14.f * Scale() + 0.5f);
    g_sw.fontUi = CreateFontW(px, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
    g_sw.fontMono = CreateFontW(px, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, FIXED_PITCH, L"Consolas");
    for (auto& kv : g_sw.edits) SendMessageW(kv.second, WM_SETFONT, (WPARAM)(kv.first == kIdMacroKey ? g_sw.fontMono : g_sw.fontUi), TRUE);
}

void RebuildEditBrush() {
    if (g_sw.editBrush) DeleteObject(g_sw.editBrush);
    g_sw.editBrush = CreateSolidBrush(ToColorRef(GetTheme(g_sw.dark).fieldActive));
}

std::wstring* FieldText(int id) {
    switch (id) {
    case kIdSearch: return &g_sw.v.search;
    case kIdMacroKey: return &g_sw.v.macroKey;
    case kIdMacroValue: return &g_sw.v.macroValue;
    case kIdMacroFilter: return &g_sw.v.macroFilter;
    case kIdRuleFilter: return &g_sw.v.ruleFilter;
    default: return nullptr;
    }
}

bool IsField(int id) { return FieldText(id) != nullptr; }

// ---- EDIT subclass: Tab / Enter / Esc go back to the window --------------------------------
WNDPROC g_editProc = nullptr;

LRESULT CALLBACK EditSubclass(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    int id = GetDlgCtrlID(hwnd);
    if (msg == WM_KEYDOWN) {
        if (wp == VK_TAB) {
            PostMessageW(GetParent(hwnd), WM_VK_FIELD, (WPARAM)id, (GetKeyState(VK_SHIFT) & 0x8000) ? kFieldShiftTab : kFieldTab);
            return 0;
        }
        if (wp == VK_RETURN) {
            PostMessageW(GetParent(hwnd), WM_VK_FIELD, (WPARAM)id, kFieldEnter);
            return 0;
        }
        if (wp == VK_ESCAPE) {
            PostMessageW(GetParent(hwnd), WM_VK_FIELD, (WPARAM)id, kFieldEscape);
            return 0;
        }
        if (wp == 'A' && (GetKeyState(VK_CONTROL) & 0x8000)) {
            SendMessageW(hwnd, EM_SETSEL, 0, -1);
            return 0;
        }
    }
    if (msg == WM_CHAR && (wp == L'\t' || wp == L'\r' || wp == 27 || wp == 1)) return 0;
    return CallWindowProcW(g_editProc, hwnd, msg, wp, lp);
}

HWND EditFor(int id) {
    auto it = g_sw.edits.find(id);
    if (it != g_sw.edits.end()) return it->second;
    HWND e = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | ES_AUTOHSCROLL, 0, 0, 10, 10, g_sw.hwnd, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
    if (!e) return nullptr;
    SendMessageW(e, EM_SETLIMITTEXT, id == kIdMacroKey ? 32 : 1000, 0);
    SendMessageW(e, WM_SETFONT, (WPARAM)(id == kIdMacroKey ? g_sw.fontMono : g_sw.fontUi), TRUE);
    WNDPROC old = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(e, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(EditSubclass)));
    if (!g_editProc) g_editProc = old;
    g_sw.edits[id] = e;
    return e;
}

void PlaceActiveEdit() {
    int id = g_sw.v.activeField;
    if (!id) return;
    HWND e = EditFor(id);
    auto it = g_sw.v.fieldRects.find(id);
    if (!e) return;
    if (it == g_sw.v.fieldRects.end()) {
        g_sw.v.activeField = 0;
        ShowWindow(e, SW_HIDE);
        return;
    }
    float s = Scale();
    RectF tr = Ui::FieldTextRect(it->second, id == kIdSearch || id == kIdMacroFilter || id == kIdRuleFilter ? Icon::Search : Icon::None);
    int fontH = (int)(20.f * s);
    int x = (int)(tr.left * s), w = (int)(Wd(tr) * s);
    int y = (int)(Cy(tr) * s) - fontH / 2;
    RECT cur;
    GetWindowRect(e, &cur);
    MapWindowPoints(nullptr, g_sw.hwnd, reinterpret_cast<POINT*>(&cur), 2);
    if (cur.left != x || cur.top != y || cur.right - cur.left != w || !IsWindowVisible(e)) {
        SetWindowPos(e, HWND_TOP, x, y, w, fontH, SWP_SHOWWINDOW | SWP_NOACTIVATE);
    }
}

void DeactivateField() {
    int id = g_sw.v.activeField;
    if (!id) return;
    g_sw.v.activeField = 0;
    auto it = g_sw.edits.find(id);
    if (it != g_sw.edits.end()) {
        if (GetFocus() == it->second) SetFocus(g_sw.hwnd);
        ShowWindow(it->second, SW_HIDE);
    }
    Invalidate();
}

void ActivateField(int id) {
    if (g_sw.v.activeField == id) return;
    DeactivateField();
    HWND e = EditFor(id);
    if (!e) return;
    g_sw.v.activeField = id;
    g_sw.ui.focus = id;
    SetWindowTextW(e, FieldText(id)->c_str());
    // Lay out once so the field rectangle is known, then show the EDIT on top of it.
    InvalidateRect(g_sw.hwnd, nullptr, FALSE);
    UpdateWindow(g_sw.hwnd);
    PlaceActiveEdit();
    SetFocus(e);
    SendMessageW(e, EM_SETSEL, 0, -1);
}

void SetFieldText(int id, const std::wstring& text) {
    *FieldText(id) = text;
    auto it = g_sw.edits.find(id);
    if (it != g_sw.edits.end()) SetWindowTextW(it->second, text.c_str());
}

void EnsureVisible(int id) {
    const Hit* h = g_sw.ui.Find(id);
    if (!h || h->r.left < kNavW) return;
    float vh = g_sw.v.viewportHeight;
    if (h->r.top < 8.f) g_sw.v.scroll += h->r.top - 24.f;
    else if (h->r.bottom > vh - 8.f) g_sw.v.scroll += h->r.bottom - vh + 24.f;
    ClampScroll();
}

void SetPage(int page) {
    if (page == g_sw.v.page) return;
    DeactivateField();
    if (g_sw.v.recordingSlot >= 0) {
        g_sw.v.recordingSlot = -1;
        EndHotkeyRecord();
    }
    g_sw.v.page = page;
    g_sw.v.scroll = 0;
    Invalidate();
}

void Message(const wchar_t* text, UINT icon) { MessageBoxW(g_sw.hwnd, text, L"VisKey", MB_OK | icon); }

// ---- actions --------------------------------------------------------------------------------
void AddApp(bool fromGeneral) {
    std::wstring path;
    if (!PickOpenFile(g_sw.hwnd, L"Chọn ứng dụng", L"Ứng dụng (*.exe)", L"*.exe", path)) return;
    size_t p = path.find_last_of(L"\\/");
    std::wstring name = p == std::wstring::npos ? path : path.substr(p + 1);
    std::wstring lower = LowerW(name);
    if (lower.size() < 5 || lower.compare(lower.size() - 4, 4, L".exe") != 0) {
        Message(L"Hãy chọn một tệp .exe.", MB_ICONWARNING);
        return;
    }
    Model& m = M();
    for (auto& r : m.rules) {
        if (r.exe == lower) {
            r.mode = RuleMode::EN;
            r.source = RuleSource::User;
            Commit(kChangeRules);
            return;
        }
    }
    AppRule r;
    r.exe = lower;
    r.display = name;
    r.mode = RuleMode::EN;
    r.source = RuleSource::User;
    r.lastVi = false;
    m.rules.push_back(r);
    Commit(kChangeRules);
    if (fromGeneral) g_sw.v.perAppExpanded = true;
}

void SubmitMacro() {
    Model& m = M();
    std::string key = WideToUtf8(g_sw.v.macroKey);
    while (!key.empty() && key.back() == ' ') key.pop_back();
    while (!key.empty() && key.front() == ' ') key.erase(key.begin());
    for (char& ch : key) {
        if (ch >= 'A' && ch <= 'Z') ch = (char)(ch + 32);
    }
    std::string value = WideToUtf8(g_sw.v.macroValue);
    if (!IsValidMacroKey(key)) {
        Message(L"Viết tắt chỉ gồm chữ a–z và số 0–9 (không dấu), bắt đầu bằng một chữ cái, tối đa 32 ký tự.", MB_ICONWARNING);
        ActivateField(kIdMacroKey);
        return;
    }
    if (value.empty() || g_sw.v.macroValue.find_first_not_of(L' ') == std::wstring::npos) {
        Message(L"Hãy nhập nội dung thay thế cho cụm viết tắt.", MB_ICONWARNING);
        ActivateField(kIdMacroValue);
        return;
    }
    const std::string& editing = g_sw.v.editingMacro;
    auto it = m.macros.find(key);
    if (it != m.macros.end() && key != editing) {
        std::wstring q = L"“" + Utf8ToWide(key) + L"” đã có (→ " + Utf8ToWide(it->second) + L").\n\nThay bằng nội dung mới?";
        if (MessageBoxW(g_sw.hwnd, q.c_str(), L"VisKey", MB_YESNO | MB_ICONQUESTION) != IDYES) return;
    }
    if (!editing.empty() && editing != key) {
        auto u = m.usage.find(editing);
        if (u != m.usage.end()) {
            m.usage[key] = u->second;
            m.usage.erase(editing);
        }
        m.macros.erase(editing);
    }
    m.macros[key] = value;
    g_sw.v.editingMacro.clear();
    DeactivateField();
    SetFieldText(kIdMacroKey, L"");
    SetFieldText(kIdMacroValue, L"");
    Commit(kChangeMacros | kChangeUsage);
}

void EditMacro(const std::string& key) {
    auto it = M().macros.find(key);
    if (it == M().macros.end()) return;
    g_sw.v.editingMacro = key;
    SetFieldText(kIdMacroKey, Utf8ToWide(key));
    SetFieldText(kIdMacroValue, Utf8ToWide(it->second));
    g_sw.v.scroll = 0;
    ActivateField(kIdMacroValue);
}

void DeleteMacro(const std::string& key) {
    Model& m = M();
    m.macros.erase(key);
    m.usage.erase(key);
    if (g_sw.v.editingMacro == key) g_sw.v.editingMacro.clear();
    Commit(kChangeMacros | kChangeUsage);
}

void ImportMacros() {
    std::wstring path;
    if (!PickOpenFile(g_sw.hwnd, L"Nhập gõ tắt", L"Tệp gõ tắt (*.json; *.txt)", L"*.json;*.txt", path)) return;
    std::string data;
    if (!ReadFileBytes(path, data)) {
        Message(L"Không đọc được tệp đã chọn.", MB_ICONERROR);
        return;
    }
    std::map<std::string, std::string> parsed;
    int skipped = 0;
    if (!ParseMacroFile(data, parsed, skipped)) {
        Message(L"Tệp không đúng định dạng gõ tắt.\n\nVisKey nhận tệp JSON dạng {\"version\": 1, \"macros\": {\"ko\": \"không\"}} "
                L"hoặc tệp văn bản UTF-8, mỗi dòng “viết tắt<Tab>nội dung”.",
                MB_ICONERROR);
        return;
    }
    if (parsed.empty()) {
        Message(L"Không có cụm từ hợp lệ nào trong tệp. Viết tắt chỉ gồm chữ a–z và số 0–9, bắt đầu bằng chữ cái.", MB_ICONWARNING);
        return;
    }
    Model& m = M();
    int added = 0, replaced = 0;
    for (const auto& kv : parsed) {
        auto it = m.macros.find(kv.first);
        if (it == m.macros.end()) ++added;
        else if (it->second != kv.second) ++replaced;
        m.macros[kv.first] = kv.second;
    }
    Commit(kChangeMacros);
    std::wstring msg = L"Đã nhập " + std::to_wstring(added) + L" cụm từ mới";
    if (replaced) msg += L", cập nhật " + std::to_wstring(replaced) + L" cụm từ đã có";
    msg += L".";
    if (skipped) msg += L"\nBỏ qua " + std::to_wstring(skipped) + L" dòng không hợp lệ.";
    Message(msg.c_str(), MB_ICONINFORMATION);
}

void ExportMacros() {
    std::wstring path;
    if (!PickSaveFile(g_sw.hwnd, L"Xuất gõ tắt", L"JSON (*.json)", L"*.json", L"viskey-go-tat.json", L"json", path)) return;
    if (!WriteFileBytes(path, ExportMacrosJson(M().macros))) {
        Message(L"Không ghi được tệp. Hãy chọn thư mục khác.", MB_ICONERROR);
        return;
    }
    std::wstring msg = L"Đã xuất " + std::to_wstring(M().macros.size()) + L" cụm từ.";
    Message(msg.c_str(), MB_ICONINFORMATION);
}

void ResetAll() {
    int r = MessageBoxW(g_sw.hwnd,
                        L"Khôi phục toàn bộ cài đặt?\n\nMọi tuỳ chọn, phím tắt, cụm từ gõ tắt (kể cả số lần dùng) và quy tắc theo ứng dụng "
                        L"sẽ trở về mặc định. Không thể hoàn tác.",
                        L"VisKey", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
    if (r != IDYES) return;
    DeleteAllStoredSettings();
    Model& m = M();
    bool hooks = m.hooksActive, snap = m.snapshot;
    ResetModelToDefaults(m);
    m.hooksActive = hooks;
    m.snapshot = snap;
    g_sw.v.editingMacro.clear();
    Commit(kChangeAll);
    Message(L"Đã khôi phục cài đặt mặc định.", MB_ICONINFORMATION);
}

void RestoreDefaultRules() {
    if (MessageBoxW(g_sw.hwnd, L"Khôi phục danh sách ứng dụng mặc định?\n\nCác ứng dụng bạn đã thêm hoặc VisKey tự ghi nhớ sẽ bị xoá khỏi danh sách.",
                    L"VisKey", MB_YESNO | MB_ICONQUESTION) != IDYES) {
        return;
    }
    M().rules = DefaultRules();
    Commit(kChangeRules);
}

AppRule* RuleByExe(const std::wstring& exe) {
    for (auto& r : M().rules) {
        if (r.exe == exe) return &r;
    }
    return nullptr;
}

void ShowDropdown(int id) {
    if (PopupRecentlyClosed(id)) return;
    const Hit* h = g_sw.ui.Find(id);
    if (!h) return;
    RECT anchor = ToScreen(h->r);
    std::vector<PopupItem> items;
    int sel = 0;
    if (id == kIdMethodDrop) {
        items = {{L"Telex", false}, {L"VNI", false}};
        sel = M().vni ? 1 : 0;
    } else {
        items = {{L"Kiểu cũ (hòa)", false}, {L"Kiểu mới (hoà)", false}};
        sel = M().modernOrthography ? 1 : 0;
    }
    ShowPopupList(g_sw.hwnd, id, anchor, items, sel, g_sw.dark);
}

void StartRecording(int slot) {
    DeactivateField();
    g_sw.v.recordingSlot = slot;
    BeginHotkeyRecord(g_sw.hwnd);
    SetFocus(g_sw.hwnd);
    Invalidate();
}

void StopRecording() {
    if (g_sw.v.recordingSlot < 0) return;
    g_sw.v.recordingSlot = -1;
    EndHotkeyRecord();
    Invalidate();
}

void Activate(int id) {
    Model& m = M();
    SettingsView& v = g_sw.v;
    if (IsField(id)) {
        ActivateField(id);
        return;
    }
    if (id >= kIdNavBase && id < kIdNavBase + kPageCount) {
        SetPage(id - kIdNavBase);
        return;
    }
    if (id >= kIdHotkeyRecBase && id < kIdHotkeyRecBase + kHotkeyCount) {
        int slot = id - kIdHotkeyRecBase;
        if (v.recordingSlot == slot) StopRecording();
        else StartRecording(slot);
        return;
    }
    if (id >= kIdHotkeyClearBase && id < kIdHotkeyClearBase + kHotkeyCount) {
        int slot = id - kIdHotkeyClearBase;
        if (v.recordingSlot == slot) {
            StopRecording();
        } else {
            m.hotkeys[slot] = Hotkey();
            Commit(kChangeHotkeys);
        }
        return;
    }
    if (id >= kIdMacroRowBase && id < kIdRuleRowBase) {
        int idx = (id - kIdMacroRowBase) / 4, part = (id - kIdMacroRowBase) % 4;
        if (idx >= (int)v.visibleMacros.size()) return;
        std::string key = v.visibleMacros[idx];
        if (part == 1) EditMacro(key);
        else if (part == 2) DeleteMacro(key);
        else if (part == 3) {
            const Hit* h = g_sw.ui.Find(id);
            if (!h || PopupRecentlyClosed(id)) return;
            v.menuMacro = key;
            RECT a = ToScreen(h->r);
            a.left = a.right - (LONG)(140 * Scale());
            ShowPopupList(g_sw.hwnd, id, a, {{L"Sửa", false}, {L"Xoá", true}}, -1, g_sw.dark);
        }
        return;
    }
    if (id >= kIdRuleRowBase) {
        int idx = (id - kIdRuleRowBase) / 8, part = (id - kIdRuleRowBase) % 8;
        if (idx >= (int)v.visibleRules.size()) return;
        AppRule* r = RuleByExe(v.visibleRules[idx]);
        if (!r) return;
        if (part >= 1 && part <= 3) {
            r->mode = (RuleMode)(part - 1);
            r->source = RuleSource::User;
            Commit(kChangeRules);
        } else if (part == 4) {
            std::wstring exe = r->exe;
            auto& rules = m.rules;
            rules.erase(std::remove_if(rules.begin(), rules.end(), [&](const AppRule& x) { return x.exe == exe; }), rules.end());
            Commit(kChangeRules);
        }
        return;
    }
    if (id >= kIdChipBase && id < kIdChipBase + 100) {
        SetPage(kPagePerApp);
        return;
    }
    if (id >= kIdSortBase && id < kIdSortBase + 2) {
        v.macroSort = id - kIdSortBase;
        Invalidate();
        return;
    }
    if (id >= kIdPolicyBase && id < kIdPolicyBase + 3) {
        m.policy = (NewAppPolicy)(id - kIdPolicyBase);
        Commit(kChangeSettings);
        return;
    }
    if (id >= kIdLinkBase && id < kIdLinkBase + 4) {
        static const wchar_t* urls[4] = {L"https://github.com/1vision-work/VisKey", L"https://github.com/1vision-work/VisKey/issues",
                                         L"https://github.com/1vision-work/VisKey/releases", L"https://github.com/1vision-work/VisKey/blob/master/LICENSE"};
        OpenUrl(urls[id - kIdLinkBase]);
        return;
    }
    switch (id) {
    case kIdMasterVi: SetVietnamese(!m.vietnamese); break;
    case kIdMethodDrop:
    case kIdOrthoDrop: ShowDropdown(id); break;
    case kIdAutoRestore: m.autoRestore = !m.autoRestore; Commit(kChangeSettings); break;
    case kIdFixAc: m.fixAutocomplete = !m.fixAutocomplete; Commit(kChangeSettings); break;
    case kIdSmart:
    case kIdPerAppMaster: m.smartPerApp = !m.smartPerApp; Commit(kChangeSettings); break;
    case kIdSmartExpand: v.perAppExpanded = !v.perAppExpanded; Invalidate(); break;
    case kIdStartup: m.launchAtStartup = !m.launchAtStartup; Commit(kChangeSettings); break;
    case kIdChipAdd: AddApp(true); break;
    case kIdAddApp: AddApp(false); break;
    case kIdRestoreRules: RestoreDefaultRules(); break;
    case kIdImport: ImportMacros(); break;
    case kIdExport: ExportMacros(); break;
    case kIdMacrosOn: m.macrosEnabled = !m.macrosEnabled; Commit(kChangeSettings); break;
    case kIdQuickTelex: m.quickTelex = !m.quickTelex; Commit(kChangeSettings); break;
    case kIdMacroAdd: SubmitMacro(); break;
    case kIdMacroCancel:
        v.editingMacro.clear();
        DeactivateField();
        SetFieldText(kIdMacroKey, L"");
        SetFieldText(kIdMacroValue, L"");
        Invalidate();
        break;
    case kIdCheckUpdate: RequestManualUpdateCheck(); break;
    case kIdAutoUpdate: m.autoUpdateCheck = !m.autoUpdateCheck; Commit(kChangeSettings); break;
    case kIdOpenDownload: {
        std::wstring url = L"https://github.com/1vision-work/VisKey/releases";
        if (m.haveUpdateResult && m.update.ok && m.update.newer && !m.update.tag.empty()) url += L"/tag/" + m.update.tag;
        OpenUrl(url.c_str());
        break;
    }
    case kIdReleases: OpenUrl(L"https://github.com/1vision-work/VisKey/releases"); break;
    case kIdCopyInfo:
        if (CopyToClipboard(g_sw.hwnd, SystemInfoText(m))) {
            v.flashId = kIdCopyInfo;
            SetTimer(g_sw.hwnd, kFlashTimer, 1800, nullptr);
            Invalidate();
        }
        break;
    case kIdResetAll: ResetAll(); break;
    default: break;
    }
}

void OnSearchChanged() {
    std::wstring q = FoldForSearch(g_sw.v.search);
    while (!q.empty() && q.back() == L' ') q.pop_back();
    if (q.empty()) return;
    // Jump to the first page that matches if the current one does not.
    SettingsView& v = g_sw.v;
    if (SettingsPageMatches(v.page, q)) return;
    for (int p = 0; p < kPageCount; ++p) {
        if (SettingsPageMatches(p, q)) {
            v.page = p;
            v.scroll = 0;
            break;
        }
    }
}

void Paint() {
    PAINTSTRUCT ps;
    BeginPaint(g_sw.hwnd, &ps);
    RECT rc;
    GetClientRect(g_sw.hwnd, &rc);
    if (!g_sw.rt) {
        float dpi = (float)g_sw.dpi;
        D2D1_RENDER_TARGET_PROPERTIES p = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT,
                                                                       D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_UNKNOWN), dpi, dpi);
        D2D()->CreateHwndRenderTarget(p, D2D1::HwndRenderTargetProperties(g_sw.hwnd, D2D1::SizeU((UINT32)rc.right, (UINT32)rc.bottom)), &g_sw.rt);
    }
    if (g_sw.rt) {
        g_sw.rt->BeginDraw();
        {
            Gfx g(g_sw.rt);
            g_sw.ui.g = &g;
            g_sw.ui.t = &GetTheme(g_sw.dark);
            float s = Scale();
            PaintSettings(g_sw.ui, g_sw.v, M(), (float)rc.right / s, (float)rc.bottom / s);
            g_sw.ui.g = nullptr;
        }
        HRESULT hr = g_sw.rt->EndDraw();
        if (hr == (HRESULT)D2DERR_RECREATE_TARGET) {
            SafeRelease(g_sw.rt);
            InvalidateRect(g_sw.hwnd, nullptr, FALSE);
        }
    }
    EndPaint(g_sw.hwnd, &ps);
    float before = g_sw.v.scroll;
    ClampScroll();
    if (before != g_sw.v.scroll) InvalidateRect(g_sw.hwnd, nullptr, FALSE);
    PlaceActiveEdit();
}

void UpdateHot(float x, float y) {
    int hot = g_sw.ui.HitTest(x, y);
    if (hot != g_sw.ui.hot) {
        g_sw.ui.hot = hot;
        Invalidate();
    }
}

void ApplyTheme() {
    g_sw.dark = SystemPrefersDark();
    ApplyWindowFrameTheme(g_sw.hwnd, g_sw.dark, false);
    RebuildEditBrush();
    for (auto& kv : g_sw.edits) InvalidateRect(kv.second, nullptr, TRUE);
    Invalidate();
}

void RecordHotkey(UINT vk, DWORD mods) {
    int slot = g_sw.v.recordingSlot;
    if (slot < 0) return;
    StopRecording();
    if (vk == VK_ESCAPE && !mods) return;
    Hotkey h = Hotkey::Unpack((vk & 0xFFFF) | mods);
    M().hotkeys[slot] = h;
    Commit(kChangeHotkeys);
}

LRESULT CALLBACK SettingsProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        g_sw.hwnd = hwnd;
        g_sw.dpi = DpiForWindow(hwnd);
        g_sw.dark = SystemPrefersDark();
        ApplyWindowFrameTheme(hwnd, g_sw.dark, false);
        RebuildFonts();
        RebuildEditBrush();
        float s = Scale();
        MONITORINFO mi = {};
        mi.cbSize = sizeof(mi);
        GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY), &mi);
        int w = std::min((int)(kSettingsW * s), (int)(mi.rcWork.right - mi.rcWork.left));
        int h = std::min((int)(kSettingsH * s), (int)(mi.rcWork.bottom - mi.rcWork.top));
        SetWindowPos(hwnd, nullptr, mi.rcWork.left + ((mi.rcWork.right - mi.rcWork.left) - w) / 2, mi.rcWork.top + ((mi.rcWork.bottom - mi.rcWork.top) - h) / 2,
                     w, h, SWP_NOZORDER | SWP_NOACTIVATE);
        return 0;
    }
    case WM_PAINT:
        Paint();
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_SIZE:
        if (g_sw.rt) g_sw.rt->Resize(D2D1::SizeU(LOWORD(lp), HIWORD(lp)));
        Invalidate();
        return 0;
    case WM_GETMINMAXINFO: {
        auto* mmi = reinterpret_cast<MINMAXINFO*>(lp);
        float s = (float)DpiForWindow(hwnd) / 96.f;
        mmi->ptMinTrackSize.x = (LONG)(900 * s);
        mmi->ptMinTrackSize.y = (LONG)(520 * s);
        return 0;
    }
    case 0x02E0: {  // WM_DPICHANGED
        g_sw.dpi = HIWORD(wp);
        if (g_sw.rt) g_sw.rt->SetDpi((float)g_sw.dpi, (float)g_sw.dpi);
        RebuildFonts();
        const RECT* r = reinterpret_cast<const RECT*>(lp);
        SetWindowPos(hwnd, nullptr, r->left, r->top, r->right - r->left, r->bottom - r->top, SWP_NOZORDER | SWP_NOACTIVATE);
        Invalidate();
        return 0;
    }
    case WM_SETTINGCHANGE:
        if (lp && lstrcmpiW(reinterpret_cast<LPCWSTR>(lp), L"ImmersiveColorSet") == 0) {
            ApplyTheme();
            RefreshFlyout();
        }
        break;
    case WM_MOUSEMOVE: {
        float s = Scale();
        if (!g_sw.tracking) {
            TRACKMOUSEEVENT tme = {sizeof(tme), TME_LEAVE, hwnd, 0};
            TrackMouseEvent(&tme);
            g_sw.tracking = true;
        }
        UpdateHot((short)LOWORD(lp) / s, (short)HIWORD(lp) / s);
        return 0;
    }
    case WM_MOUSELEAVE:
        g_sw.tracking = false;
        g_sw.ui.hot = 0;
        Invalidate();
        return 0;
    case WM_LBUTTONDOWN: {
        float s = Scale();
        int id = g_sw.ui.HitTest((short)LOWORD(lp) / s, (short)HIWORD(lp) / s);
        g_sw.ui.focusVisible = false;
        if (g_sw.v.recordingSlot >= 0 && id != kIdHotkeyRecBase + g_sw.v.recordingSlot) StopRecording();
        if (IsField(id)) {
            ActivateField(id);
            return 0;
        }
        DeactivateField();
        SetFocus(hwnd);
        g_sw.ui.pressed = id;
        if (id) {
            const Hit* h = g_sw.ui.Find(id);
            if (h && h->focusable) g_sw.ui.focus = id;
            SetCapture(hwnd);
        }
        Invalidate();
        return 0;
    }
    case WM_LBUTTONUP: {
        int pressed = g_sw.ui.pressed;
        g_sw.ui.pressed = 0;
        if (GetCapture() == hwnd) ReleaseCapture();
        float s = Scale();
        int id = g_sw.ui.HitTest((short)LOWORD(lp) / s, (short)HIWORD(lp) / s);
        Invalidate();
        if (pressed && pressed == id) Activate(id);
        return 0;
    }
    case WM_MOUSEWHEEL: {
        int delta = GET_WHEEL_DELTA_WPARAM(wp);
        g_sw.v.scroll -= (float)delta / 120.f * 56.f;
        ClampScroll();
        POINT pt = {(short)LOWORD(lp), (short)HIWORD(lp)};
        ScreenToClient(hwnd, &pt);
        Invalidate();
        UpdateWindow(hwnd);
        UpdateHot((float)pt.x / Scale(), (float)pt.y / Scale());
        return 0;
    }
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN: {
        if (g_sw.v.recordingSlot >= 0) {
            // Fallback when the low-level hook is not running.
            UINT vk = (UINT)wp;
            if (vk == VK_SHIFT || vk == VK_CONTROL || vk == VK_MENU || vk == VK_LWIN || vk == VK_RWIN) return 0;
            DWORD mods = ((GetKeyState(VK_CONTROL) & 0x8000) ? kModCtrl : 0) | ((GetKeyState(VK_MENU) & 0x8000) ? kModAlt : 0) |
                         ((GetKeyState(VK_SHIFT) & 0x8000) ? kModShift : 0) |
                         (((GetKeyState(VK_LWIN) | GetKeyState(VK_RWIN)) & 0x8000) ? kModWin : 0);
            RecordHotkey(vk, mods);
            return 0;
        }
        if (msg == WM_SYSKEYDOWN) break;
        bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        switch (wp) {
        case VK_TAB: {
            g_sw.ui.focusVisible = true;
            int next = g_sw.ui.NextFocus((GetKeyState(VK_SHIFT) & 0x8000) ? -1 : 1);
            g_sw.ui.focus = next;
            EnsureVisible(next);
            if (IsField(next)) ActivateField(next);
            Invalidate();
            return 0;
        }
        case VK_RETURN:
        case VK_SPACE:
            if (g_sw.ui.focus) {
                g_sw.ui.focusVisible = true;
                Activate(g_sw.ui.focus);
            }
            return 0;
        case VK_UP: g_sw.v.scroll -= 48.f; break;
        case VK_DOWN: g_sw.v.scroll += 48.f; break;
        case VK_PRIOR: g_sw.v.scroll -= g_sw.v.viewportHeight * 0.8f; break;
        case VK_NEXT: g_sw.v.scroll += g_sw.v.viewportHeight * 0.8f; break;
        case VK_HOME: g_sw.v.scroll = 0; break;
        case VK_END: g_sw.v.scroll = 1e6f; break;
        case 'F':
            if (ctrl) ActivateField(kIdSearch);
            return 0;
        default: return 0;
        }
        ClampScroll();
        Invalidate();
        return 0;
    }
    case WM_SYSCHAR:
        if (g_sw.v.recordingSlot >= 0) return 0;
        break;
    case WM_COMMAND: {
        int id = LOWORD(wp);
        int code = HIWORD(wp);
        if (IsField(id)) {
            HWND e = reinterpret_cast<HWND>(lp);
            if (code == EN_CHANGE) {
                int len = GetWindowTextLengthW(e);
                std::wstring text((size_t)len, L'\0');
                if (len) GetWindowTextW(e, &text[0], len + 1);
                *FieldText(id) = text;
                if (id == kIdSearch) OnSearchChanged();
                Invalidate();
            } else if (code == EN_KILLFOCUS) {
                if (g_sw.v.activeField == id) DeactivateField();
            }
        }
        return 0;
    }
    case WM_CTLCOLOREDIT: {
        HDC dc = reinterpret_cast<HDC>(wp);
        const Theme& t = GetTheme(g_sw.dark);
        SetTextColor(dc, ToColorRef(t.text));
        SetBkColor(dc, ToColorRef(t.fieldActive));
        return reinterpret_cast<LRESULT>(g_sw.editBrush);
    }
    case WM_VK_FIELD: {
        int id = (int)wp;
        switch ((int)lp) {
        case kFieldTab:
        case kFieldShiftTab: {
            g_sw.ui.focus = id;
            g_sw.ui.focusVisible = true;
            int next = g_sw.ui.NextFocus(lp == kFieldTab ? 1 : -1);
            DeactivateField();
            g_sw.ui.focus = next;
            EnsureVisible(next);
            if (IsField(next)) ActivateField(next);
            else SetFocus(hwnd);
            Invalidate();
            break;
        }
        case kFieldEnter:
            if (id == kIdMacroKey) ActivateField(kIdMacroValue);
            else if (id == kIdMacroValue) SubmitMacro();
            break;
        case kFieldEscape:
            if (!FieldText(id)->empty() && id != kIdMacroKey && id != kIdMacroValue) {
                SetFieldText(id, L"");
                Invalidate();
            } else {
                if (id == kIdMacroKey || id == kIdMacroValue) g_sw.v.editingMacro.clear();
                DeactivateField();
            }
            break;
        }
        return 0;
    }
    case WM_VK_POPUP: {
        int tag = (int)wp, idx = (int)lp;
        if (idx < 0) return 0;
        if (tag == kIdMethodDrop) {
            M().vni = idx == 1;
            Commit(kChangeSettings);
        } else if (tag == kIdOrthoDrop) {
            M().modernOrthography = idx == 1;
            Commit(kChangeSettings);
        } else if (tag >= kIdMacroRowBase && tag < kIdRuleRowBase && !g_sw.v.menuMacro.empty()) {
            std::string key = g_sw.v.menuMacro;
            g_sw.v.menuMacro.clear();
            if (idx == 0) EditMacro(key);
            else DeleteMacro(key);
        }
        return 0;
    }
    case WM_VK_HOTKEY_RECORDED:
        RecordHotkey((UINT)wp, (DWORD)lp);
        return 0;
    case WM_TIMER:
        if (wp == kFlashTimer) {
            KillTimer(hwnd, kFlashTimer);
            g_sw.v.flashId = 0;
            Invalidate();
        }
        return 0;
    case WM_ACTIVATE:
        if (LOWORD(wp) == WA_INACTIVE) StopRecording();
        break;
    case WM_CLOSE:
        StopRecording();
        DeactivateField();
        ShowWindow(hwnd, SW_HIDE);
        return 0;
    case WM_DESTROY:
        SafeRelease(g_sw.rt);
        g_sw.hwnd = nullptr;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

}  // namespace

HWND SettingsWindow() { return g_sw.hwnd; }

void CreateSettingsWindow(HINSTANCE inst) {
    if (g_sw.hwnd) return;
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = SettingsProc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = static_cast<HICON>(LoadImageW(inst, MAKEINTRESOURCEW(1), IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0));
    wc.hIconSm = static_cast<HICON>(LoadImageW(inst, MAKEINTRESOURCEW(1), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0));
    wc.lpszClassName = L"VisKeySettingsWindow";
    RegisterClassExW(&wc);
    CreateWindowExW(0, wc.lpszClassName, L"VisKey", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, 980, 720, nullptr, nullptr, inst,
                    nullptr);
}

void ShowSettingsWindow(int page) {
    if (!g_sw.hwnd) CreateSettingsWindow(GetModuleHandleW(nullptr));
    if (!g_sw.hwnd) return;
    if (page >= 0 && page < kPageCount) SetPage(page);
    if (IsIconic(g_sw.hwnd)) ShowWindow(g_sw.hwnd, SW_RESTORE);
    else ShowWindow(g_sw.hwnd, SW_SHOW);
    SetForegroundWindow(g_sw.hwnd);
    Invalidate();
}

void RefreshSettingsWindow() { Invalidate(); }

}  // namespace vk
