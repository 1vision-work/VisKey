// VisKey for Windows - view state + painters shared by the live windows and --ui-snapshot
// (MIT License, 1VISION).
#pragma once

#include "widgets.h"

namespace vk {

enum SettingsPage { kPageGeneral = 0, kPageMacros, kPagePerApp, kPageHotkeys, kPageUpdate, kPageAbout, kPageCount };

enum class UpdateState { Idle, Checking, UpToDate, Newer, Failed };

struct SettingsView {
    int page = kPageGeneral;
    float scroll = 0, contentHeight = 0, viewportHeight = 0;
    std::wstring search, macroKey, macroValue, macroFilter, ruleFilter;
    int activeField = 0;  // field whose real EDIT control is visible
    bool perAppExpanded = true;
    int macroSort = 0;  // 0 = A -> Z, 1 = most used
    int recordingSlot = -1;
    std::string editingMacro;  // key being edited ("" = adding)
    int flashId = 0;  // control showing a short confirmation ("Đã sao chép")
    std::string demoHoverMacro;  // snapshot only: row drawn as hovered

    // Filled while painting, used for input handling.
    std::map<int, RectF> fieldRects;
    std::vector<std::string> visibleMacros;
    std::vector<std::wstring> visibleRules;
    std::vector<std::wstring> chipRules;
    std::string menuMacro;  // row whose "..." menu is open
};

constexpr float kSettingsW = 980.f;
constexpr float kSettingsH = 720.f;
constexpr float kNavW = 272.f;
constexpr float kFlyoutW = 360.f;
constexpr float kFlyoutH = 412.f;
constexpr float kFlyoutUpdateH = 44.f;  // extra row "Có bản mới x.y.z" when an update is known

// How the flyout background is drawn: opaque (classic), or translucent over a DWM backdrop
// (Windows 11 22H2+ system acrylic, older builds SetWindowCompositionAttribute acrylic).
enum class FlyoutSurface { Solid, Translucent };

UpdateState UpdateStateOf(const Model& m);  // shown on the update page (silent automatic failures -> Idle)
float FlyoutHeight(const Model& m);

void PaintSettings(Ui& ui, SettingsView& v, const Model& m, float w, float h);
void PaintFlyout(Ui& ui, const Model& m, float w, float h, FlyoutSurface surface = FlyoutSurface::Solid);
// Draws a Windows 11 style caption strip (used only for snapshots: the live window keeps the
// real system caption).
void PaintFakeCaption(Gfx& g, const Theme& t, float w, float h);

// Control ids shared with the window procedures.
enum ControlId : int {
    kIdSearch = 1,
    kIdNavBase = 10,  // + page

    kIdMasterVi = 100, kIdMethodDrop, kIdOrthoDrop, kIdAutoRestore, kIdFixAc, kIdSmart, kIdSmartExpand, kIdStartup, kIdChipAdd,

    kIdImport = 200, kIdExport, kIdMacrosOn, kIdQuickTelex, kIdMacroKey, kIdMacroValue, kIdMacroAdd, kIdMacroCancel,
    kIdMacroFilter, kIdSortBase = 210,

    kIdPerAppMaster = 300, kIdPolicyBase = 310, kIdRuleFilter = 320, kIdAddApp, kIdRestoreRules,

    kIdHotkeyRecBase = 400, kIdHotkeyClearBase = 410,

    kIdCheckUpdate = 500, kIdOpenDownload, kIdReleases, kIdAutoUpdate,

    kIdLinkBase = 600, kIdCopyInfo = 610, kIdResetAll,

    kIdChipBase = 3000,
    kIdMacroRowBase = 10000,  // + i*4 + {0 row, 1 edit, 2 delete, 3 more}
    kIdRuleRowBase = 40000,   // + i*8 + {0 row, 1 VI, 2 EN, 3 remember, 4 delete}

    // flyout
    kIdFlyVi = 900, kIdFlyEn, kIdFlyTelex, kIdFlyVni, kIdFlyTileBase = 910, kIdFlyOpen = 920, kIdFlyQuit, kIdFlyUpdate,
};

std::wstring HotkeyWarning(const Model& m, int slot);
// q must already be folded with FoldForSearch().
bool SettingsPageMatches(int page, const std::wstring& q);

}  // namespace vk
