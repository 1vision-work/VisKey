// VisKeyApp.mm
// Complete Native macOS Vietnamese Input Method Application
// Features:
//  - CGEventTap Low-Level Keyboard & Mouse Hook (Zero Preedit Underline)
//  - Spotlight / Chrome / Safari / Arc Omnibox Autocomplete Fix (U+202F injection)
//  - Per-app VI/EN rules (persisted; terminals default to EN) + policy for new apps
//  - Configurable hotkeys (toggle VI/EN, secondary toggle, Telex <-> VNI)
//  - EventTap Timeout Watchdog & SecureEventInput (Password field) protection
//  - Settings window, menu-bar popover, VI/EN HUD and onboarding (macos/ui/*)
//  - Launch at Login (LaunchAgents plist generator)
//  - `--ui-snapshot <dir>`: renders the UI offscreen to PNG files and exits (no event tap, no real settings)

#import <Cocoa/Cocoa.h>
#import <ApplicationServices/ApplicationServices.h>
#import <Carbon/Carbon.h>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "../core/viskey_engine.hpp"
#import "ui/VKUIKit.h"
#import "ui/VKModel.h"
#import "ui/VKSettingsWindow.h"
#import "ui/VKPopover.h"
#import "ui/VKHUD.h"
#import "ui/VKOnboarding.h"
#import "ui/VKUpdate.h"

static const int64_t kVisKeyEventMagic = 0x564B5821; // 'VKX!' synthetic event marker

@interface VisKeyController : NSObject <NSApplicationDelegate, NSPopoverDelegate, VKAppActions>
@property (strong, nonatomic) NSStatusItem *statusItem;
@property (strong, nonatomic) VKModel *model;
@property (strong, nonatomic) VKSettingsWindowController *settings;
@property (strong, nonatomic) VKOnboardingWindowController *onboarding;
@property (strong, nonatomic) NSPopover *popover;
@property (strong, nonatomic) VKPopoverController *popoverController;
@property (strong, nonatomic) VKHUDController *hud;
@property (strong, nonatomic) id popoverMonitor;
@property (strong, nonatomic) NSTimer *permissionTimer;
@property (strong, nonatomic) VKUpdateChecker *updater;
@property (assign, nonatomic) CFMachPortRef eventTap;
@property (assign, nonatomic) CFRunLoopSourceRef runLoopSource;
@property (strong, nonatomic) NSSet<NSString *> *autocompleteBundles;
@property (copy, nonatomic) NSString *snapshotDir;
@property (assign, nonatomic) BOOL snapshotMode;
@property (assign, nonatomic) BOOL lastTrusted;
@property (assign, nonatomic) BOOL loggedTapFailure;

- (void)toggleVietnameseMode:(id)sender;
- (void)toggleInputMethod:(id)sender;
- (void)openSettingsWindow:(id)sender;
- (void)recordMacroUse:(NSString *)key;
@end

static VisKeyController *gController = nil;
static viskey::VisKeyEngine gEngine;

// Hotkeys honoured by the event tap (mirrors VKModel, updated on the main thread).
struct VKHotkeyBinding {
    long keyCode;          // -1 = none
    unsigned long mods;    // NSEventModifierFlagControl | Option | Shift | Command
};
static VKHotkeyBinding gHotPrimary = {kVK_Space, (unsigned long)NSEventModifierFlagControl};
static VKHotkeyBinding gHotSecondary = {-1, 0};
static VKHotkeyBinding gHotMethod = {-1, 0};
static bool gHotkeyRecording = false;
// Raw keys of the word the engine is currently composing (used to detect macro expansions).
static std::string gLastRawWord;

static void resetEngine() {
    gEngine.reset();
    gLastRawWord.clear();
}

static unsigned long VKModsFromCGFlags(CGEventFlags flags) {
    unsigned long m = 0;
    if (flags & kCGEventFlagMaskControl) m |= NSEventModifierFlagControl;
    if (flags & kCGEventFlagMaskAlternate) m |= NSEventModifierFlagOption;
    if (flags & kCGEventFlagMaskShift) m |= NSEventModifierFlagShift;
    if (flags & kCGEventFlagMaskCommand) m |= NSEventModifierFlagCommand;
    return m;
}

static bool VKHotkeyMatches(const VKHotkeyBinding &hk, CGKeyCode keyCode, unsigned long mods) {
    return hk.keyCode >= 0 && hk.keyCode == (long)keyCode && hk.mods == mods;
}

static VKHotkeyBinding VKBindingFromHotkey(VKHotkey *hk) {
    VKHotkeyBinding b;
    b.keyCode = (hk && !hk.isEmpty) ? (long)hk.keyCode : -1;
    b.mods = (hk && !hk.isEmpty) ? (unsigned long)hk.modifiers : 0;
    return b;
}

static std::string VKAsciiLower(const std::string &s) {
    std::string r = s;
    for (auto &c : r) c = (char)std::tolower((unsigned char)c);
    return r;
}

static void sendReplacementEvents(int backspaces, const std::string &commitText, bool needAutocompleteFix) {
    CGEventSourceRef source = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
    if (!source) return;

    auto postKey = [&](CGKeyCode keyCode, bool keyDown) {
        CGEventRef ev = CGEventCreateKeyboardEvent(source, keyCode, keyDown);
        CGEventSetFlags(ev, 0); // a held Shift/Option must not turn Delete into another combo
        CGEventSetIntegerValueField(ev, kCGEventSourceUserData, kVisKeyEventMagic);
        CGEventPost(kCGSessionEventTap, ev);
        CFRelease(ev);
    };

    auto postUnicode = [&](const UniChar *chars, UniCharCount len) {
        CGEventRef down = CGEventCreateKeyboardEvent(source, 0, true);
        CGEventRef up   = CGEventCreateKeyboardEvent(source, 0, false);
        CGEventSetFlags(down, 0);
        CGEventSetFlags(up, 0);
        CGEventSetIntegerValueField(down, kCGEventSourceUserData, kVisKeyEventMagic);
        CGEventSetIntegerValueField(up,   kCGEventSourceUserData, kVisKeyEventMagic);
        CGEventKeyboardSetUnicodeString(down, len, chars);
        CGEventKeyboardSetUnicodeString(up,   len, chars);
        CGEventPost(kCGSessionEventTap, down);
        CGEventPost(kCGSessionEventTap, up);
        CFRelease(down);
        CFRelease(up);
    };

    if (needAutocompleteFix && backspaces > 0) {
        UniChar emptyFix = 0x202F;
        postUnicode(&emptyFix, 1);
        postKey((CGKeyCode)kVK_Delete, true);
        postKey((CGKeyCode)kVK_Delete, false);
    }

    for (int i = 0; i < backspaces; ++i) {
        postKey((CGKeyCode)kVK_Delete, true);
        postKey((CGKeyCode)kVK_Delete, false);
    }

    if (!commitText.empty()) {
        NSString *nsText = [NSString stringWithUTF8String:commitText.c_str()];
        if (nsText && nsText.length > 0) {
            std::vector<UniChar> buffer(nsText.length);
            [nsText getCharacters:buffer.data() range:NSMakeRange(0, nsText.length)];
            postUnicode(buffer.data(), (UniCharCount)buffer.size());
        }
    }

    CFRelease(source);
}

static CGEventRef VisKeyEventCallback(CGEventTapProxy /*proxy*/, CGEventType type, CGEventRef event, void * /*refcon*/) {
    if (type == kCGEventTapDisabledByTimeout || type == kCGEventTapDisabledByUserInput) {
        if (gController && gController.eventTap) {
            CGEventTapEnable(gController.eventTap, true);
        }
        resetEngine();
        return event;
    }

    if (type == kCGEventLeftMouseDown || type == kCGEventRightMouseDown || type == kCGEventOtherMouseDown) {
        resetEngine();
        return event;
    }

    if (type != kCGEventKeyDown) {
        return event;
    }

    if (CGEventGetIntegerValueField(event, kCGEventSourceUserData) == kVisKeyEventMagic) {
        return event;
    }

    // While the user records a hotkey in Settings, let every key through untouched.
    if (gHotkeyRecording) {
        resetEngine();
        return event;
    }

    if (IsSecureEventInputEnabled()) {
        resetEngine();
        return event;
    }

    CGEventFlags flags = CGEventGetFlags(event);
    CGKeyCode keyCode  = (CGKeyCode)CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);

    bool ctrlPressed  = (flags & kCGEventFlagMaskControl) != 0;
    bool cmdPressed   = (flags & kCGEventFlagMaskCommand) != 0;
    bool optPressed   = (flags & kCGEventFlagMaskAlternate) != 0;
    unsigned long mods = VKModsFromCGFlags(flags);

    if (VKHotkeyMatches(gHotPrimary, keyCode, mods) || VKHotkeyMatches(gHotSecondary, keyCode, mods)) {
        dispatch_async(dispatch_get_main_queue(), ^{
            [gController toggleVietnameseMode:nil];
        });
        return NULL;
    }
    if (VKHotkeyMatches(gHotMethod, keyCode, mods)) {
        dispatch_async(dispatch_get_main_queue(), ^{
            [gController toggleInputMethod:nil];
        });
        return NULL;
    }

    if (cmdPressed || ctrlPressed || optPressed) {
        resetEngine();
        return event;
    }

    if (!gController || !gController.model.vietnameseEnabled) {
        return event;
    }

    if (keyCode == kVK_Delete) {
        auto res = gEngine.process_backspace();
        gLastRawWord = res.raw_word;
        if (res.handled) {
            sendReplacementEvents(res.backspaces, res.commit_text, res.autocomplete_fix_needed);
            return NULL;
        }
        return event;
    }

    if (keyCode == kVK_LeftArrow || keyCode == kVK_RightArrow ||
        keyCode == kVK_UpArrow   || keyCode == kVK_DownArrow  ||
        keyCode == kVK_Home      || keyCode == kVK_End        ||
        keyCode == kVK_PageUp    || keyCode == kVK_PageDown   ||
        keyCode == kVK_Escape    || keyCode == kVK_Tab        ||
        keyCode == kVK_Return    || keyCode == kVK_ANSI_KeypadEnter) {
        resetEngine();
        return event;
    }

    UniChar chars[4] = {0};
    UniCharCount actualLen = 0;
    CGEventKeyboardGetUnicodeString(event, 4, &actualLen, chars);
    if (actualLen != 1 || chars[0] > 127) {
        resetEngine();
        return event;
    }

    char ch = static_cast<char>(chars[0]);
    const std::string prevRaw = gLastRawWord;
    auto res = gEngine.process_key(ch);
    gLastRawWord = res.raw_word;

    // Macro usage counter: the engine expands a macro when a non-word key ends a word whose
    // raw keys match a macro. Mirror that condition here without touching the engine.
    const auto &cfg = gEngine.config();
    bool isWordKey = std::isalpha((unsigned char)ch) != 0 ||
                     (cfg.input_method == "vni" && std::isdigit((unsigned char)ch) != 0 && !prevRaw.empty());
    if (res.handled && !isWordKey && cfg.enable_macros && !prevRaw.empty()) {
        std::string lower = VKAsciiLower(prevRaw);
        if (cfg.macros.find(lower) != cfg.macros.end()) {
            NSString *key = [NSString stringWithUTF8String:lower.c_str()];
            if (key) {
                dispatch_async(dispatch_get_main_queue(), ^{
                    [gController recordMacroUse:key];
                });
            }
        }
    }

    if (!res.handled) {
        return event;
    }

    NSString *frontBundle = NSWorkspace.sharedWorkspace.frontmostApplication.bundleIdentifier ?: @"";
    bool isAutocompleteApp = [gController.autocompleteBundles containsObject:frontBundle];
    bool useFix = res.autocomplete_fix_needed || (gEngine.config().fix_autocomplete && isAutocompleteApp && res.backspaces == 1);

    sendReplacementEvents(res.backspaces, res.commit_text, useFix);
    return NULL;
}

@implementation VisKeyController

- (NSDictionary<NSString *, NSString *> *)engineDefaultMacros {
    NSMutableDictionary *d = [NSMutableDictionary dictionary];
    viskey::EngineConfig defaults;
    for (const auto &kv : defaults.macros) {
        NSString *k = [NSString stringWithUTF8String:kv.first.c_str()];
        NSString *v = [NSString stringWithUTF8String:kv.second.c_str()];
        if (k && v) d[k] = v;
    }
    return d;
}

- (NSString *)ownBundleId {
    return [NSBundle mainBundle].bundleIdentifier ?: @"org.viskey.inputmethod";
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    gController = self;

    if (self.snapshotDir) {
        self.snapshotMode = YES;
        dispatch_async(dispatch_get_main_queue(), ^{
            [self runSnapshotsInto:self.snapshotDir];
        });
        return;
    }

    self.autocompleteBundles = [NSSet setWithArray:@[
        @"com.apple.Spotlight",
        @"com.apple.Safari",
        @"com.google.Chrome",
        @"company.thebrowser.Browser",
        @"com.brave.Browser",
        @"com.microsoft.edgemac",
        @"org.mozilla.firefox",
        @"com.microsoft.Excel"
    ]];

    self.model = [[VKModel alloc] initPersistent:YES defaultMacros:[self engineDefaultMacros]];
    [self.model load];
    self.model.actions = self;
    [VKModel setShared:self.model];
    NSRunningApplication *front = NSWorkspace.sharedWorkspace.frontmostApplication;
    if (front.bundleIdentifier && ![front.bundleIdentifier isEqualToString:[self ownBundleId]]) {
        self.model.currentAppBundleId = front.bundleIdentifier;
        self.model.currentAppName = front.localizedName;
    }
    [self applyModelToEngine];

    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(modelDidChange:)
                                                 name:VKModelDidChangeNotification object:self.model];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(environmentDidChange:)
                                                 name:VKEnvironmentDidChangeNotification object:self.model];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(environmentDidChange:)
                                                 name:VKUpdateStateDidChangeNotification object:self.model];

    self.hud = [[VKHUDController alloc] init];
    [self setupMainMenu];
    [self setupStatusBar];
    [self setupEventTap];

    self.lastTrusted = AXIsProcessTrusted() ? YES : NO;
    __weak VisKeyController *weakSelf = self;
    self.permissionTimer = [NSTimer scheduledTimerWithTimeInterval:1.5 repeats:YES block:^(NSTimer *timer) {
        [weakSelf permissionTick];
    }];

    [[[NSWorkspace sharedWorkspace] notificationCenter]
        addObserver:self
           selector:@selector(handleActiveAppChanged:)
               name:NSWorkspaceDidActivateApplicationNotification
             object:nil];

    if (!self.model.onboardingDone || !self.lastTrusted) {
        [self showOnboarding];
    }

    // Automatic update check: main-run-loop timers, the request itself runs on a background session.
    self.updater = [[VKUpdateChecker alloc] initWithModel:self.model];
    [self.updater start];
}

- (BOOL)applicationShouldHandleReopen:(NSApplication *)sender hasVisibleWindows:(BOOL)flag {
    if (!self.snapshotMode) [self openSettingsWindow:nil];
    return YES;
}

#pragma mark Model <-> engine

- (void)applyModelToEngine {
    VKModel *m = self.model;
    auto &cfg = gEngine.config();
    std::string im = [m isTelex] ? "telex" : "vni";
    bool methodChanged = cfg.input_method != im;
    cfg.input_method = im;
    cfg.modern_orthography = m.modernOrthography;
    cfg.auto_restore_english = m.autoRestoreEnglish;
    cfg.fix_autocomplete = m.fixAutocomplete;
    cfg.enable_macros = m.enableMacros;
    cfg.quick_telex = m.quickTelex;
    cfg.macros.clear();
    for (NSString *k in m.macros) {
        NSString *v = m.macros[k];
        const char *ck = k.UTF8String;
        const char *cv = v.UTF8String;
        if (ck && cv) cfg.macros[ck] = cv;
    }
    if (methodChanged) resetEngine();

    gHotPrimary = VKBindingFromHotkey(m.primaryHotkey);
    gHotSecondary = VKBindingFromHotkey(m.secondaryHotkey);
    gHotMethod = VKBindingFromHotkey(m.methodHotkey);
}

- (void)modelDidChange:(NSNotification *)n {
    [self applyModelToEngine];
    [self syncStatusItem];
    if (self.popover.shown) [self.popoverController reload];
}

- (void)environmentDidChange:(NSNotification *)n {
    if (self.popover.shown) [self.popoverController reload];
}

- (void)recordMacroUse:(NSString *)key {
    [self.model recordMacroUse:key];
}

#pragma mark Event tap & permission

- (void)setupEventTap {
    if (self.eventTap) return;
    CGEventMask mask = CGEventMaskBit(kCGEventKeyDown) |
                       CGEventMaskBit(kCGEventFlagsChanged) |
                       CGEventMaskBit(kCGEventLeftMouseDown) |
                       CGEventMaskBit(kCGEventRightMouseDown) |
                       CGEventMaskBit(kCGEventOtherMouseDown);

    self.eventTap = CGEventTapCreate(kCGHIDEventTap,
                                     kCGHeadInsertEventTap,
                                     kCGEventTapOptionDefault,
                                     mask,
                                     VisKeyEventCallback,
                                     NULL);
    if (!self.eventTap) {
        if (!self.loggedTapFailure) {
            NSLog(@"[VisKey] Failed to create CGEventTap. Grant Accessibility permission in System Settings.");
            self.loggedTapFailure = YES;
        }
        return;
    }

    self.runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, self.eventTap, 0);
    CFRunLoopAddSource(CFRunLoopGetMain(), self.runLoopSource, kCFRunLoopCommonModes);
    CGEventTapEnable(self.eventTap, true);
    NSLog(@"[VisKey] Event tap installed.");
}

- (void)permissionTick {
    BOOL trusted = AXIsProcessTrusted() ? YES : NO;
    if (trusted && !self.eventTap) [self setupEventTap];
    if (trusted != self.lastTrusted) {
        self.lastTrusted = trusted;
        [self.model notifyEnvironmentChanged];
    }
}

#pragma mark Menus & status item

- (void)setupMainMenu {
    NSMenu *main = [[NSMenu alloc] initWithTitle:@""];
    NSMenuItem *appItem = [[NSMenuItem alloc] initWithTitle:@"VisKey" action:nil keyEquivalent:@""];
    NSMenu *appMenu = [[NSMenu alloc] initWithTitle:@"VisKey"];
    NSMenuItem *prefs = [appMenu addItemWithTitle:@"Bảng điều khiển…" action:@selector(openSettingsWindow:) keyEquivalent:@","];
    prefs.target = self;
    [appMenu addItem:[NSMenuItem separatorItem]];
    [appMenu addItemWithTitle:@"Thoát VisKey" action:@selector(terminate:) keyEquivalent:@"q"];
    appItem.submenu = appMenu;
    [main addItem:appItem];

    NSMenuItem *editItem = [[NSMenuItem alloc] initWithTitle:@"Sửa" action:nil keyEquivalent:@""];
    NSMenu *edit = [[NSMenu alloc] initWithTitle:@"Sửa"];
    [edit addItemWithTitle:@"Hoàn tác" action:@selector(undo:) keyEquivalent:@"z"];
    [edit addItemWithTitle:@"Làm lại" action:@selector(redo:) keyEquivalent:@"Z"];
    [edit addItem:[NSMenuItem separatorItem]];
    [edit addItemWithTitle:@"Cắt" action:@selector(cut:) keyEquivalent:@"x"];
    [edit addItemWithTitle:@"Sao chép" action:@selector(copy:) keyEquivalent:@"c"];
    [edit addItemWithTitle:@"Dán" action:@selector(paste:) keyEquivalent:@"v"];
    [edit addItemWithTitle:@"Chọn tất cả" action:@selector(selectAll:) keyEquivalent:@"a"];
    editItem.submenu = edit;
    [main addItem:editItem];
    NSApp.mainMenu = main;
}

- (void)setupStatusBar {
    self.statusItem = [[NSStatusBar systemStatusBar] statusItemWithLength:NSVariableStatusItemLength];
    NSStatusBarButton *button = self.statusItem.button;
    button.image = VKStatusMarkImage(16);
    button.imagePosition = NSImageLeft;
    button.font = [NSFont systemFontOfSize:13 weight:NSFontWeightSemibold];
    button.target = self;
    button.action = @selector(statusItemClicked:);
    [button sendActionOn:(NSEventMaskLeftMouseUp | NSEventMaskRightMouseUp)];

    self.popoverController = [[VKPopoverController alloc] initWithModel:self.model];
    self.popover = [[NSPopover alloc] init];
    self.popover.behavior = NSPopoverBehaviorTransient;
    self.popover.animates = YES;
    self.popover.contentViewController = self.popoverController;
    self.popover.delegate = self;
    [self syncStatusItem];
}

- (void)syncStatusItem {
    NSStatusBarButton *button = self.statusItem.button;
    if (!button) return;
    BOOL vi = self.model.vietnameseEnabled;
    button.title = vi ? @" VI" : @" EN";
    button.toolTip = vi ? @"VisKey — đang gõ Tiếng Việt" : @"VisKey — đang gõ Tiếng Anh";
}

- (void)statusItemClicked:(id)sender {
    NSEvent *event = NSApp.currentEvent;
    BOOL secondary = event.type == NSEventTypeRightMouseUp || (event.modifierFlags & NSEventModifierFlagControl) != 0;
    if (secondary) {
        [self.popover performClose:nil];
        NSMenu *menu = [[NSMenu alloc] initWithTitle:@"VisKey"];
        NSMenuItem *prefs = [menu addItemWithTitle:@"Bảng điều khiển…" action:@selector(openSettingsWindow:) keyEquivalent:@","];
        prefs.target = self;
        [menu addItem:[NSMenuItem separatorItem]];
        [menu addItemWithTitle:@"Thoát VisKey" action:@selector(terminate:) keyEquivalent:@"q"];
        self.statusItem.menu = menu;
        [self.statusItem.button performClick:nil];
        self.statusItem.menu = nil;
        return;
    }
    if (self.popover.shown) {
        [self.popover performClose:nil];
        return;
    }
    [self.popoverController reload];
    [self.popover showRelativeToRect:self.statusItem.button.bounds ofView:self.statusItem.button preferredEdge:NSRectEdgeMinY];
    __weak VisKeyController *weakSelf = self;
    if (!self.popoverMonitor) {
        self.popoverMonitor = [NSEvent addGlobalMonitorForEventsMatchingMask:(NSEventMaskLeftMouseDown | NSEventMaskRightMouseDown)
                                                                     handler:^(NSEvent *e) {
            [weakSelf.popover performClose:nil];
        }];
    }
}

- (void)popoverDidClose:(NSNotification *)notification {
    if (self.popoverMonitor) {
        [NSEvent removeMonitor:self.popoverMonitor];
        self.popoverMonitor = nil;
    }
}

#pragma mark Windows

- (void)activateApp {
    if (@available(macOS 14.0, *)) {
        [NSApp activate];
    } else {
        [NSApp activateIgnoringOtherApps:YES];
    }
}

- (void)openSettingsWindow:(id)sender {
    [self vk_openSettingsPage:VKSettingsPageGeneral];
}

- (void)showOnboarding {
    if (!self.onboarding) {
        self.onboarding = [[VKOnboardingWindowController alloc] initWithModel:self.model snapshot:NO];
        __weak VisKeyController *weakSelf = self;
        self.onboarding.onPermissionGranted = ^{
            [weakSelf setupEventTap];
        };
    }
    [self.onboarding showStep:self.model.onboardingDone ? 2 : 1];
    [self activateApp];
    [self.onboarding.window makeKeyAndOrderFront:nil];
}

#pragma mark VI / EN switching

/// Frontmost app the VI/EN choice should be remembered for (never VisKey itself).
- (NSString *)targetAppBundleId {
    NSString *front = NSWorkspace.sharedWorkspace.frontmostApplication.bundleIdentifier;
    if (front && ![front isEqualToString:[self ownBundleId]]) return front;
    if (self.popover.shown) return self.model.currentAppBundleId;
    return nil;
}

- (void)rememberStateForCurrentApp {
    if (!self.model.smartPerAppMode) return;
    NSString *bundleId = [self targetAppBundleId];
    if (!bundleId) return;
    VKAppRule *rule = [self.model ruleForBundle:bundleId];
    if (!rule) {
        rule = [VKAppRule ruleWithBundleId:bundleId name:nil mode:VKAppModeRemember source:VKRuleSourceAuto];
        [self.model setRule:rule];
    }
    rule.lastVietnamese = @(self.model.vietnameseEnabled);
}

- (void)setVietnamese:(BOOL)vietnamese showHUD:(BOOL)showHUD {
    BOOL changed = (vietnamese != self.model.vietnameseEnabled);
    self.model.vietnameseEnabled = vietnamese;
    resetEngine();
    [self rememberStateForCurrentApp];
    [self.model commit];
    if (showHUD && changed && self.model.showHUD && !self.snapshotMode) {
        [self.hud showVietnamese:vietnamese hint:[self.model hudHint]];
    }
}

- (void)toggleVietnameseMode:(id)sender {
    [self setVietnamese:!self.model.vietnameseEnabled showHUD:YES];
}

- (void)toggleInputMethod:(id)sender {
    self.model.inputMethod = [self.model isTelex] ? @"vni" : @"telex";
    resetEngine();
    [self.model commit];
}

- (void)handleActiveAppChanged:(NSNotification *)notification {
    resetEngine();
    NSRunningApplication *app = notification.userInfo[NSWorkspaceApplicationKey];
    NSString *bundleId = app.bundleIdentifier;
    if (!bundleId || [bundleId isEqualToString:[self ownBundleId]]) return;
    self.model.currentAppBundleId = bundleId;
    self.model.currentAppName = app.localizedName;
    [self.model notifyEnvironmentChanged];
    if (!self.model.smartPerAppMode) return;

    VKAppRule *rule = [self.model ruleForBundle:bundleId];
    NSNumber *target = nil;
    if (rule) {
        target = [rule effectiveVietnamese];
    } else if (self.model.unknownAppPolicy == VKNewAppPolicyVI) {
        target = @YES;
    } else if (self.model.unknownAppPolicy == VKNewAppPolicyEN) {
        target = @NO;
    }
    if (target && target.boolValue != self.model.vietnameseEnabled) {
        self.model.vietnameseEnabled = target.boolValue;
        [self.model commit];
    }
}

#pragma mark VKAppActions

- (void)vk_setVietnamese:(BOOL)vietnamese {
    if (self.snapshotMode) return;
    [self setVietnamese:vietnamese showHUD:YES];
}

- (void)vk_openSettingsPage:(VKSettingsPage)page {
    if (self.snapshotMode) return;
    [self.popover performClose:nil];
    if (!self.settings) {
        self.settings = [[VKSettingsWindowController alloc] initWithModel:self.model snapshot:NO];
    }
    [self.settings showPage:page];
    [self activateApp];
    [self.settings.window makeKeyAndOrderFront:nil];
}

- (void)vk_setLaunchAtLogin:(BOOL)on {
    if (self.snapshotMode) return;
    self.model.launchAtLogin = on;
    NSString *agentsDir = [NSHomeDirectory() stringByAppendingPathComponent:@"Library/LaunchAgents"];
    NSString *plistPath = [agentsDir stringByAppendingPathComponent:@"org.viskey.inputmethod.plist"];
    NSFileManager *fm = [NSFileManager defaultManager];

    if (on) {
        [fm createDirectoryAtPath:agentsDir withIntermediateDirectories:YES attributes:nil error:nil];
        NSString *execPath = [[NSBundle mainBundle] executablePath] ?: @"/Applications/VisKey.app/Contents/MacOS/VisKey";
        NSDictionary *plist = @{
            @"Label": @"org.viskey.inputmethod",
            @"ProgramArguments": @[execPath],
            @"RunAtLoad": @YES,
            @"KeepAlive": @NO
        };
        [plist writeToFile:plistPath atomically:YES];
    } else {
        [fm removeItemAtPath:plistPath error:nil];
    }
    [self.model commit];
}

- (void)vk_setRuleForCurrentApp:(VKAppMode)mode {
    if (self.snapshotMode) return;
    NSString *bundleId = self.model.currentAppBundleId;
    if (!bundleId.length) return;
    VKAppRule *rule = [self.model ruleForBundle:bundleId];
    if (!rule) {
        rule = [VKAppRule ruleWithBundleId:bundleId name:nil mode:mode source:VKRuleSourceUser];
        [self.model setRule:rule];
    }
    rule.mode = mode;
    rule.source = VKRuleSourceUser;
    if (mode == VKAppModeVI || mode == VKAppModeEN) {
        [self setVietnamese:(mode == VKAppModeVI) showHUD:YES];
    } else {
        [self.model commit];
    }
}

- (void)vk_requestAccessibility {
    if (self.snapshotMode) return;
    NSDictionary *options = @{(__bridge id)kAXTrustedCheckOptionPrompt: @YES};
    AXIsProcessTrustedWithOptions((__bridge CFDictionaryRef)options);
    NSURL *url = [NSURL URLWithString:@"x-apple.systempreferences:com.apple.preference.security?Privacy_Accessibility"];
    if (url) [[NSWorkspace sharedWorkspace] openURL:url];
}

- (void)vk_hotkeyRecordingChanged:(BOOL)recording {
    gHotkeyRecording = recording;
    resetEngine();
}

- (void)vk_closePopover {
    [self.popover performClose:nil];
}

- (void)vk_quit {
    if (self.snapshotMode) return;
    [NSApp terminate:nil];
}

- (void)vk_checkForUpdates {
    if (self.snapshotMode) return;
    [self.updater checkNow];
}

- (void)vk_openUpdateDownload {
    if (self.snapshotMode) return;
    NSString *u = self.model.availableUpdateURL;
    if (!u.length) u = [VKRepoURL stringByAppendingString:@"/releases"];
    NSURL *url = [NSURL URLWithString:u];
    if (url) [[NSWorkspace sharedWorkspace] openURL:url];
    [self.popover performClose:nil];
    if (self.model.updateNoticePending) {
        self.model.updateNoticePending = NO;
        [self.model notifyUpdateStateChanged];
    }
}

#pragma mark Snapshot mode

- (NSWindow *)hostWindowForView:(NSView *)view {
    NSWindow *w = [[NSWindow alloc] initWithContentRect:view.bounds
                                              styleMask:NSWindowStyleMaskBorderless
                                                backing:NSBackingStoreBuffered
                                                  defer:NO];
    w.releasedWhenClosed = NO;
    w.opaque = NO;
    w.backgroundColor = [NSColor clearColor];
    w.contentView = view;
    return w;
}

- (BOOL)renderView:(NSView *)view window:(NSWindow *)window appearance:(NSAppearance *)appearance path:(NSString *)path {
    window.appearance = appearance;
    window.alphaValue = 0.0;            // on screen for layout (tables load rows) but invisible
    window.ignoresMouseEvents = YES;
    [window orderFrontRegardless];
    [window.contentView layoutSubtreeIfNeeded];
    [window displayIfNeeded];
    [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.25]];

    __block BOOL ok = NO;
    [appearance performAsCurrentDrawingAppearance:^{
        NSRect r = view.bounds;
        NSBitmapImageRep *rep = [view bitmapImageRepForCachingDisplayInRect:r];
        if (!rep || rep.pixelsWide < (NSInteger)(r.size.width * 2.0)) {
            // Force 2x even on a 1x screen (CI runners).
            rep = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL
                                                          pixelsWide:(NSInteger)(r.size.width * 2.0)
                                                          pixelsHigh:(NSInteger)(r.size.height * 2.0)
                                                       bitsPerSample:8
                                                     samplesPerPixel:4
                                                            hasAlpha:YES
                                                            isPlanar:NO
                                                      colorSpaceName:NSDeviceRGBColorSpace
                                                         bytesPerRow:0
                                                        bitsPerPixel:0];
            rep.size = r.size;
        }
        if (!rep) return;
        [view cacheDisplayInRect:r toBitmapImageRep:rep];
        NSData *png = [rep representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
        ok = png && [png writeToFile:path atomically:YES];
    }];
    [window orderOut:nil];
    return ok;
}

/// Demo "new version found" state for the update snapshots (in memory only, nothing is fetched).
- (void)setDemoUpdate:(BOOL)on model:(VKModel *)m {
    NSString *current = [m versionString];
    if (on) {
        m.availableUpdateVersion = @"1.1.0";
        m.availableUpdateURL = [VKRepoURL stringByAppendingString:@"/releases"];
        m.updateNoticePending = YES;
        m.lastUpdateResult = [VKUpdateResult resultWithStatus:VKUpdateStatusAvailable currentVersion:current
                                                latestVersion:@"1.1.0" releaseURL:m.availableUpdateURL httpCode:200];
    } else {
        m.availableUpdateVersion = nil;
        m.availableUpdateURL = nil;
        m.updateNoticePending = NO;
        m.lastUpdateResult = [VKUpdateResult resultWithStatus:VKUpdateStatusUpToDate currentVersion:current
                                                latestVersion:nil releaseURL:nil httpCode:200];
    }
}

- (void)runSnapshotsInto:(NSString *)dir {
    NSString *outDir = [dir stringByStandardizingPath];
    NSError *err = nil;
    if (![[NSFileManager defaultManager] createDirectoryAtPath:outDir withIntermediateDirectories:YES attributes:nil error:&err]) {
        fprintf(stderr, "VisKey: cannot create %s: %s\n", outDir.UTF8String, err.localizedDescription.UTF8String ?: "?");
        fflush(stderr);
        exit(1);
    }

    // In-memory model only: defaults + demo data, never the real NSUserDefaults domain.
    VKModel *m = [[VKModel alloc] initPersistent:NO defaultMacros:[self engineDefaultMacros]];
    [m loadDemoData];
    m.actions = self;
    [VKModel setShared:m];
    self.model = m;

    NSAppearance *light = [NSAppearance appearanceNamed:NSAppearanceNameAqua];
    NSAppearance *dark = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
    NSMutableArray<NSString *> *written = [NSMutableArray array];
    NSInteger failures = 0;

    struct Shot { VKSettingsPage page; BOOL dark; BOOL update; const char *name; };
    const Shot shots[] = {
        {VKSettingsPageGeneral, NO, NO, "macos-general-light.png"},
        {VKSettingsPageGeneral, YES, NO, "macos-general-dark.png"},
        {VKSettingsPageMacros, YES, NO, "macos-macros-dark.png"},
        {VKSettingsPageApps, NO, NO, "macos-perapp-light.png"},
        {VKSettingsPageShortcuts, NO, NO, "macos-shortcuts-light.png"},
        {VKSettingsPagePrivacy, NO, NO, "macos-privacy-light.png"},
        {VKSettingsPageAbout, NO, NO, "macos-about-light.png"},
        {VKSettingsPageAbout, NO, YES, "macos-about-update-light.png"},
    };
    for (const Shot &s : shots) {
        @autoreleasepool {
            [self setDemoUpdate:s.update model:m];
            // A fresh window per shot so every view resolves colors for its own appearance.
            VKSettingsWindowController *wc = [[VKSettingsWindowController alloc] initWithModel:m snapshot:YES];
            NSAppearance *app = s.dark ? dark : light;
            wc.window.appearance = app;
            [wc showPage:s.page];
            NSString *path = [outDir stringByAppendingPathComponent:[NSString stringWithUTF8String:s.name]];
            if ([self renderView:wc.window.contentView window:wc.window appearance:app path:path]) [written addObject:path];
            else failures++;
            [wc close];
        }
    }

    for (int variant = 0; variant < 2; variant++) {
        @autoreleasepool {
            [self setDemoUpdate:(variant == 1) model:m];
            VKPopoverController *pc = [[VKPopoverController alloc] initWithModel:m];
            NSView *pv = pc.view;
            [pc reload];
            NSWindow *w = [self hostWindowForView:pv];
            NSString *path = [outDir stringByAppendingPathComponent:
                                         (variant == 1 ? @"macos-popover-update-light.png" : @"macos-popover-light.png")];
            if ([self renderView:pv window:w appearance:light path:path]) [written addObject:path];
            else failures++;
            [w close];
        }
    }
    [self setDemoUpdate:NO model:m];

    @autoreleasepool {
        NSView *hv = [VKHUDController hudViewVietnamese:YES hint:[m hudHint]];
        NSWindow *w = [self hostWindowForView:hv];
        NSString *path = [outDir stringByAppendingPathComponent:@"macos-hud-dark.png"];
        if ([self renderView:hv window:w appearance:dark path:path]) [written addObject:path];
        else failures++;
        [w close];
    }

    @autoreleasepool {
        VKOnboardingWindowController *oc = [[VKOnboardingWindowController alloc] initWithModel:m snapshot:YES];
        oc.window.appearance = light;
        [oc showStep:2];
        NSString *path = [outDir stringByAppendingPathComponent:@"macos-onboarding-light.png"];
        if ([self renderView:oc.window.contentView window:oc.window appearance:light path:path]) [written addObject:path];
        else failures++;
        [oc close];
    }

    for (NSString *p in written) printf("%s\n", p.UTF8String);
    fflush(stdout);
    if (failures > 0) {
        fprintf(stderr, "VisKey: %ld snapshot(s) failed\n", (long)failures);
        fflush(stderr);
    }
    exit(failures > 0 ? 1 : 0);
}

@end

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        NSString *snapshotDir = nil;
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "--ui-snapshot") == 0) {
                if (i + 1 >= argc) {
                    fprintf(stderr, "usage: VisKey --ui-snapshot <outdir>\n");
                    return 2;
                }
                snapshotDir = [NSString stringWithUTF8String:argv[i + 1]];
                break;
            }
        }
        NSApplication *app = [NSApplication sharedApplication];
        [app setActivationPolicy:NSApplicationActivationPolicyAccessory];
        VisKeyController *delegate = [[VisKeyController alloc] init];
        delegate.snapshotDir = snapshotDir;
        app.delegate = delegate;
        [app run];
    }
    return 0;
}
