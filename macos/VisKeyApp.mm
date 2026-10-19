// VisKeyApp.mm
// Complete Native macOS Vietnamese Input Method Application
// Features:
//  - CGEventTap Low-Level Keyboard & Mouse Hook (Zero Preedit Underline)
//  - Spotlight / Chrome / Safari / Arc Omnibox Autocomplete Fix (U+202F injection)
//  - Smart Per-App Mode (Remembers VI/EN per Bundle ID, defaults EN in Terminals)
//  - EventTap Timeout Watchdog & SecureEventInput (Password field) protection
//  - Native Cocoa Preferences Window (NSWindow) + Custom Macro Editor + NSUserDefaults persistence
//  - Launch at Login (LaunchAgents plist generator)

#import <Cocoa/Cocoa.h>
#import <ApplicationServices/ApplicationServices.h>
#import <Carbon/Carbon.h>
#include "../core/viskey_engine.hpp"

static const int64_t kVisKeyEventMagic = 0x564B5821; // 'VKX!' synthetic event marker

@interface VisKeyController : NSObject <NSApplicationDelegate, NSTableViewDataSource, NSTableViewDelegate>
@property (strong, nonatomic) NSStatusItem *statusItem;
@property (strong, nonatomic) NSWindow *settingsWindow;
@property (assign, nonatomic) CFMachPortRef eventTap;
@property (assign, nonatomic) CFRunLoopSourceRef runLoopSource;
@property (assign, nonatomic) BOOL vietnameseEnabled;
@property (assign, nonatomic) BOOL smartPerAppMode;
@property (assign, nonatomic) BOOL launchAtLogin;
@property (strong, nonatomic) NSMutableDictionary<NSString *, NSNumber *> *appModes;
@property (strong, nonatomic) NSSet<NSString *> *autocompleteBundles;
@property (strong, nonatomic) NSSet<NSString *> *defaultEnglishBundles;

// UI Controls for Preferences Window
@property (strong, nonatomic) NSPopUpButton *methodPopup;
@property (strong, nonatomic) NSButton *orthoCheckbox;
@property (strong, nonatomic) NSButton *restoreCheckbox;
@property (strong, nonatomic) NSButton *acFixCheckbox;
@property (strong, nonatomic) NSButton *perAppCheckbox;
@property (strong, nonatomic) NSButton *macroCheckbox;
@property (strong, nonatomic) NSButton *loginCheckbox;
@property (strong, nonatomic) NSTextField *macroKeyInput;
@property (strong, nonatomic) NSTextField *macroValInput;
@property (strong, nonatomic) NSTableView *macroTable;
@property (strong, nonatomic) NSMutableArray<NSDictionary<NSString *, NSString *> *> *macroList;

- (void)toggleVietnameseMode:(id)sender;
- (void)openSettingsWindow:(id)sender;
- (void)savePreferences;
- (void)loadPreferences;
@end

static VisKeyController *gController = nil;
static viskey::VisKeyEngine gEngine;

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
        gEngine.reset();
        return event;
    }

    if (type == kCGEventLeftMouseDown || type == kCGEventRightMouseDown || type == kCGEventOtherMouseDown) {
        gEngine.reset();
        return event;
    }

    if (type != kCGEventKeyDown) {
        return event;
    }

    if (CGEventGetIntegerValueField(event, kCGEventSourceUserData) == kVisKeyEventMagic) {
        return event;
    }

    if (IsSecureEventInputEnabled()) {
        gEngine.reset();
        return event;
    }

    CGEventFlags flags = CGEventGetFlags(event);
    CGKeyCode keyCode  = (CGKeyCode)CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);

    bool ctrlPressed  = (flags & kCGEventFlagMaskControl) != 0;
    bool cmdPressed   = (flags & kCGEventFlagMaskCommand) != 0;
    bool optPressed   = (flags & kCGEventFlagMaskAlternate) != 0;
    bool shiftPressed = (flags & kCGEventFlagMaskShift) != 0;

    if ((ctrlPressed && !cmdPressed && !optPressed && keyCode == kVK_Space) ||
        (cmdPressed && shiftPressed && !ctrlPressed && !optPressed && keyCode == kVK_ANSI_V)) {
        dispatch_async(dispatch_get_main_queue(), ^{
            [gController toggleVietnameseMode:nil];
        });
        return NULL;
    }

    if (cmdPressed || ctrlPressed || optPressed) {
        gEngine.reset();
        return event;
    }

    if (!gController || !gController.vietnameseEnabled) {
        return event;
    }

    if (keyCode == kVK_Delete) {
        auto res = gEngine.process_backspace();
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
        gEngine.reset();
        return event;
    }

    UniChar chars[4] = {0};
    UniCharCount actualLen = 0;
    CGEventKeyboardGetUnicodeString(event, 4, &actualLen, chars);
    if (actualLen != 1 || chars[0] > 127) {
        gEngine.reset();
        return event;
    }

    char ch = static_cast<char>(chars[0]);
    auto res = gEngine.process_key(ch);
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

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    gController = self;
    self.appModes = [NSMutableDictionary dictionary];
    self.macroList = [NSMutableArray array];

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

    self.defaultEnglishBundles = [NSSet setWithArray:@[
        @"com.apple.Terminal",
        @"com.googlecode.iterm2",
        @"dev.warp.Warp-Stable",
        @"io.alacritty",
        @"net.kovidgoyal.kitty"
    ]];

    [self loadPreferences];
    [self checkAccessibilityPermissions];
    [self setupStatusBar];
    [self buildSettingsWindow];
    [self setupEventTap];

    [[[NSWorkspace sharedWorkspace] notificationCenter]
        addObserver:self
           selector:@selector(handleActiveAppChanged:)
               name:NSWorkspaceDidActivateApplicationNotification
             object:nil];
}

- (void)loadPreferences {
    NSUserDefaults *defs = [NSUserDefaults standardUserDefaults];
    if ([defs objectForKey:@"vietnameseEnabled"] == nil) {
        self.vietnameseEnabled = YES;
        self.smartPerAppMode = YES;
        self.launchAtLogin = NO;
        gEngine.config().input_method = "telex";
        gEngine.config().modern_orthography = false;
        gEngine.config().auto_restore_english = true;
        gEngine.config().fix_autocomplete = true;
        gEngine.config().enable_macros = true;
        gEngine.config().quick_telex = false;
    } else {
        self.vietnameseEnabled = [defs boolForKey:@"vietnameseEnabled"];
        self.smartPerAppMode   = [defs boolForKey:@"smartPerAppMode"];
        self.launchAtLogin     = [defs boolForKey:@"launchAtLogin"];
        NSString *im = [defs stringForKey:@"inputMethod"] ?: @"telex";
        gEngine.config().input_method = [im UTF8String];
        gEngine.config().modern_orthography   = [defs boolForKey:@"modernOrthography"];
        gEngine.config().auto_restore_english = [defs boolForKey:@"autoRestoreEnglish"];
        gEngine.config().fix_autocomplete     = [defs boolForKey:@"fixAutocomplete"];
        gEngine.config().enable_macros        = [defs boolForKey:@"enableMacros"];
        gEngine.config().quick_telex          = [defs boolForKey:@"quickTelex"];

        NSDictionary *savedMacros = [defs dictionaryForKey:@"customMacros"];
        if (savedMacros) {
            gEngine.config().macros.clear();
            for (NSString *k in savedMacros) {
                NSString *v = savedMacros[k];
                gEngine.config().macros[[k UTF8String]] = [v UTF8String];
            }
        }
    }
    [self syncMacroListFromEngine];
}

- (void)syncMacroListFromEngine {
    [self.macroList removeAllObjects];
    for (const auto &kv : gEngine.config().macros) {
        NSString *k = [NSString stringWithUTF8String:kv.first.c_str()];
        NSString *v = [NSString stringWithUTF8String:kv.second.c_str()];
        if (k && v) {
            [self.macroList addObject:@{@"key": k, @"val": v}];
        }
    }
}

- (void)savePreferences {
    NSUserDefaults *defs = [NSUserDefaults standardUserDefaults];
    [defs setBool:self.vietnameseEnabled forKey:@"vietnameseEnabled"];
    [defs setBool:self.smartPerAppMode forKey:@"smartPerAppMode"];
    [defs setBool:self.launchAtLogin forKey:@"launchAtLogin"];
    [defs setObject:[NSString stringWithUTF8String:gEngine.config().input_method.c_str()] forKey:@"inputMethod"];
    [defs setBool:gEngine.config().modern_orthography forKey:@"modernOrthography"];
    [defs setBool:gEngine.config().auto_restore_english forKey:@"autoRestoreEnglish"];
    [defs setBool:gEngine.config().fix_autocomplete forKey:@"fixAutocomplete"];
    [defs setBool:gEngine.config().enable_macros forKey:@"enableMacros"];
    [defs setBool:gEngine.config().quick_telex forKey:@"quickTelex"];

    NSMutableDictionary *mDict = [NSMutableDictionary dictionary];
    for (const auto &kv : gEngine.config().macros) {
        NSString *k = [NSString stringWithUTF8String:kv.first.c_str()];
        NSString *v = [NSString stringWithUTF8String:kv.second.c_str()];
        if (k && v) mDict[k] = v;
    }
    [defs setObject:mDict forKey:@"customMacros"];
    [defs synchronize];
}

- (void)checkAccessibilityPermissions {
    NSDictionary *options = @{(__bridge id)kAXTrustedCheckOptionPrompt: @YES};
    Boolean trusted = AXIsProcessTrustedWithOptions((__bridge CFDictionaryRef)options);
    if (!trusted) {
        NSLog(@"[VisKey] Prompting user for Accessibility permissions.");
    }
}

- (void)setupStatusBar {
    self.statusItem = [[NSStatusBar systemStatusBar] statusItemWithLength:NSVariableStatusItemLength];
    NSMenu *menu = [[NSMenu alloc] initWithTitle:@"VisKey"];

    NSMenuItem *toggleItem = [[NSMenuItem alloc] initWithTitle:@"Bật Tiếng Việt (Ctrl+Space)"
                                                        action:@selector(toggleVietnameseMode:)
                                                 keyEquivalent:@""];
    toggleItem.target = self;
    toggleItem.tag = 100;
    [menu addItem:toggleItem];

    NSMenuItem *prefsItem = [[NSMenuItem alloc] initWithTitle:@"Bảng điều khiển VisKey..."
                                                       action:@selector(openSettingsWindow:)
                                                keyEquivalent:@","];
    prefsItem.target = self;
    [menu addItem:prefsItem];

    [menu addItem:[NSMenuItem separatorItem]];

    NSMenuItem *telexItem = [[NSMenuItem alloc] initWithTitle:@"Kiểu gõ Telex"
                                                       action:@selector(selectTelex:)
                                                keyEquivalent:@""];
    telexItem.target = self;
    telexItem.tag = 201;
    [menu addItem:telexItem];

    NSMenuItem *vniItem = [[NSMenuItem alloc] initWithTitle:@"Kiểu gõ VNI"
                                                     action:@selector(selectVNI:)
                                              keyEquivalent:@""];
    vniItem.target = self;
    vniItem.tag = 202;
    [menu addItem:vniItem];

    [menu addItem:[NSMenuItem separatorItem]];

    NSMenuItem *orthoItem = [[NSMenuItem alloc] initWithTitle:@"Đặt dấu kiểu mới (hoà, thuý)"
                                                       action:@selector(toggleOrthography:)
                                                keyEquivalent:@""];
    orthoItem.target = self;
    orthoItem.tag = 301;
    [menu addItem:orthoItem];

    NSMenuItem *restoreItem = [[NSMenuItem alloc] initWithTitle:@"Tự khôi phục từ tiếng Anh (Auto-Restore)"
                                                         action:@selector(toggleAutoRestore:)
                                                  keyEquivalent:@""];
    restoreItem.target = self;
    restoreItem.tag = 302;
    [menu addItem:restoreItem];

    NSMenuItem *acFixItem = [[NSMenuItem alloc] initWithTitle:@"Sửa lỗi mất chữ Spotlight & Trình duyệt"
                                                       action:@selector(toggleAutocompleteFix:)
                                                keyEquivalent:@""];
    acFixItem.target = self;
    acFixItem.tag = 303;
    [menu addItem:acFixItem];

    NSMenuItem *perAppItem = [[NSMenuItem alloc] initWithTitle:@"Ghi nhớ Anh/Việt theo từng ứng dụng"
                                                        action:@selector(togglePerAppMode:)
                                                 keyEquivalent:@""];
    perAppItem.target = self;
    perAppItem.tag = 304;
    [menu addItem:perAppItem];

    NSMenuItem *macroItem = [[NSMenuItem alloc] initWithTitle:@"Bật Gõ tắt (Macro & Quick Telex)"
                                                       action:@selector(toggleMacros:)
                                                keyEquivalent:@""];
    macroItem.target = self;
    macroItem.tag = 305;
    [menu addItem:macroItem];

    [menu addItem:[NSMenuItem separatorItem]];

    NSMenuItem *quitItem = [[NSMenuItem alloc] initWithTitle:@"Thoát VisKey"
                                                      action:@selector(terminate:)
                                               keyEquivalent:@"q"];
    [menu addItem:quitItem];

    self.statusItem.menu = menu;
    [self syncUIStates];
}

- (void)buildSettingsWindow {
    NSRect frame = NSMakeRect(0, 0, 480, 460);
    NSUInteger style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;
    self.settingsWindow = [[NSWindow alloc] initWithContentRect:frame
                                                      styleMask:style
                                                        backing:NSBackingStoreBuffered
                                                          defer:NO];
    self.settingsWindow.title = @"VisKey — Bảng Điều Khiển Bộ Gõ Tiếng Việt";
    [self.settingsWindow center];
    self.settingsWindow.releasedWhenClosed = NO;

    NSView *content = self.settingsWindow.contentView;

    NSTextField *titleLabel = [NSTextField labelWithString:@"Cấu hình Bộ gõ VisKey (macOS)"];
    titleLabel.font = [NSFont boldSystemFontOfSize:16];
    titleLabel.frame = NSMakeRect(24, 412, 400, 26);
    [content addSubview:titleLabel];

    NSTextField *methodLabel = [NSTextField labelWithString:@"Kiểu gõ:"];
    methodLabel.frame = NSMakeRect(24, 372, 100, 22);
    [content addSubview:methodLabel];

    self.methodPopup = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(130, 368, 200, 28) pullsDown:NO];
    [self.methodPopup addItemsWithTitles:@[@"Telex (Chuẩn)", @"VNI"]];
    self.methodPopup.target = self;
    self.methodPopup.action = @selector(onMethodPopupChanged:);
    [content addSubview:self.methodPopup];

    self.restoreCheckbox = [NSButton checkboxWithTitle:@"Tự động khôi phục từ tiếng Anh khi gõ song ngữ (window, software...)"
                                                target:self
                                                action:@selector(toggleAutoRestore:)];
    self.restoreCheckbox.frame = NSMakeRect(24, 332, 430, 22);
    [content addSubview:self.restoreCheckbox];

    self.acFixCheckbox = [NSButton checkboxWithTitle:@"Sửa lỗi mất/nhân đôi chữ trên Spotlight, Chrome, Safari, Arc"
                                              target:self
                                              action:@selector(toggleAutocompleteFix:)];
    self.acFixCheckbox.frame = NSMakeRect(24, 302, 430, 22);
    [content addSubview:self.acFixCheckbox];

    self.orthoCheckbox = [NSButton checkboxWithTitle:@"Đặt dấu kiểu mới (hoà, thuý thay vì hòa, thúy)"
                                              target:self
                                              action:@selector(toggleOrthography:)];
    self.orthoCheckbox.frame = NSMakeRect(24, 272, 430, 22);
    [content addSubview:self.orthoCheckbox];

    self.perAppCheckbox = [NSButton checkboxWithTitle:@"Ghi nhớ chế độ Anh/Việt theo từng ứng dụng (Tự tắt ở Terminal)"
                                               target:self
                                               action:@selector(togglePerAppMode:)];
    self.perAppCheckbox.frame = NSMakeRect(24, 242, 430, 22);
    [content addSubview:self.perAppCheckbox];

    self.macroCheckbox = [NSButton checkboxWithTitle:@"Bật bảng Gõ tắt (Macro) & Quick Telex (cc=ch, gg=gi, nn=ng...)"
                                              target:self
                                              action:@selector(toggleMacros:)];
    self.macroCheckbox.frame = NSMakeRect(24, 212, 430, 22);
    [content addSubview:self.macroCheckbox];

    self.loginCheckbox = [NSButton checkboxWithTitle:@"Tự khởi động VisKey cùng hệ điều hành macOS"
                                              target:self
                                              action:@selector(toggleLaunchAtLogin:)];
    self.loginCheckbox.frame = NSMakeRect(24, 182, 430, 22);
    [content addSubview:self.loginCheckbox];

    // Macro Table & Inputs
    NSTextField *macroHeader = [NSTextField labelWithString:@"Quản lý từ Gõ tắt (Macro):"];
    macroHeader.font = [NSFont boldSystemFontOfSize:13];
    macroHeader.frame = NSMakeRect(24, 148, 300, 20);
    [content addSubview:macroHeader];

    NSScrollView *scroll = [[NSScrollView alloc] initWithFrame:NSMakeRect(24, 56, 432, 86)];
    scroll.hasVerticalScroller = YES;
    scroll.borderType = NSBezelBorder;

    self.macroTable = [[NSTableView alloc] initWithFrame:scroll.bounds];
    NSTableColumn *colKey = [[NSTableColumn alloc] initWithIdentifier:@"key"];
    colKey.title = @"Từ viết tắt";
    colKey.width = 120;
    NSTableColumn *colVal = [[NSTableColumn alloc] initWithIdentifier:@"val"];
    colVal.title = @"Nội dung thay thế";
    colVal.width = 290;
    [self.macroTable addTableColumn:colKey];
    [self.macroTable addTableColumn:colVal];
    self.macroTable.dataSource = self;
    self.macroTable.delegate = self;
    scroll.documentView = self.macroTable;
    [content addSubview:scroll];

    self.macroKeyInput = [[NSTextField alloc] initWithFrame:NSMakeRect(24, 18, 90, 26)];
    self.macroKeyInput.placeholderString = @"Ví dụ: vn";
    [content addSubview:self.macroKeyInput];

    self.macroValInput = [[NSTextField alloc] initWithFrame:NSMakeRect(122, 18, 190, 26)];
    self.macroValInput.placeholderString = @"Ví dụ: Việt Nam";
    [content addSubview:self.macroValInput];

    NSButton *addBtn = [NSButton buttonWithTitle:@"Thêm / Cập nhật" target:self action:@selector(addMacroItem:)];
    addBtn.frame = NSMakeRect(320, 16, 136, 30);
    [content addSubview:addBtn];

    [self syncUIStates];
}

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    return (NSInteger)self.macroList.count;
}

- (id)tableView:(NSTableView *)tableView objectValueForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {
    if (row < 0 || row >= (NSInteger)self.macroList.count) return @"";
    return self.macroList[row][tableColumn.identifier] ?: @"";
}

- (void)addMacroItem:(id)sender {
    NSString *k = [[self.macroKeyInput.stringValue stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]] lowercaseString];
    NSString *v = [self.macroValInput.stringValue stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
    if (k.length == 0 || v.length == 0) return;

    gEngine.config().macros[[k UTF8String]] = [v UTF8String];
    [self syncMacroListFromEngine];
    [self.macroTable reloadData];
    self.macroKeyInput.stringValue = @"";
    self.macroValInput.stringValue = @"";
    [self savePreferences];
}

- (void)openSettingsWindow:(id)sender {
    [self syncUIStates];
    [NSApp activateIgnoringOtherApps:YES];
    [self.settingsWindow makeKeyAndOrderFront:nil];
}

- (void)setupEventTap {
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
        NSLog(@"[VisKey] Failed to create CGEventTap. Grant Accessibility permission in System Settings.");
        return;
    }

    self.runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, self.eventTap, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), self.runLoopSource, kCFRunLoopCommonModes);
    CGEventTapEnable(self.eventTap, true);
}

- (void)syncUIStates {
    if (self.statusItem && self.statusItem.button) {
        self.statusItem.button.title = self.vietnameseEnabled ? @"🇻🇳 VI" : @"🇬🇧 EN";
    }
    NSMenu *m = self.statusItem.menu;
    if (m) {
        [m itemWithTag:100].state = self.vietnameseEnabled ? NSControlStateValueOn : NSControlStateValueOff;
        bool isTelex = (gEngine.config().input_method == "telex");
        [m itemWithTag:201].state = isTelex ? NSControlStateValueOn : NSControlStateValueOff;
        [m itemWithTag:202].state = !isTelex ? NSControlStateValueOn : NSControlStateValueOff;
        [m itemWithTag:301].state = gEngine.config().modern_orthography ? NSControlStateValueOn : NSControlStateValueOff;
        [m itemWithTag:302].state = gEngine.config().auto_restore_english ? NSControlStateValueOn : NSControlStateValueOff;
        [m itemWithTag:303].state = gEngine.config().fix_autocomplete ? NSControlStateValueOn : NSControlStateValueOff;
        [m itemWithTag:304].state = self.smartPerAppMode ? NSControlStateValueOn : NSControlStateValueOff;
        [m itemWithTag:305].state = gEngine.config().enable_macros ? NSControlStateValueOn : NSControlStateValueOff;
    }
    if (self.methodPopup) {
        [self.methodPopup selectItemAtIndex:(gEngine.config().input_method == "telex" ? 0 : 1)];
        self.orthoCheckbox.state   = gEngine.config().modern_orthography ? NSControlStateValueOn : NSControlStateValueOff;
        self.restoreCheckbox.state = gEngine.config().auto_restore_english ? NSControlStateValueOn : NSControlStateValueOff;
        self.acFixCheckbox.state   = gEngine.config().fix_autocomplete ? NSControlStateValueOn : NSControlStateValueOff;
        self.perAppCheckbox.state  = self.smartPerAppMode ? NSControlStateValueOn : NSControlStateValueOff;
        self.macroCheckbox.state   = gEngine.config().enable_macros ? NSControlStateValueOn : NSControlStateValueOff;
        self.loginCheckbox.state   = self.launchAtLogin ? NSControlStateValueOn : NSControlStateValueOff;
    }
}

- (void)toggleVietnameseMode:(id)sender {
    self.vietnameseEnabled = !self.vietnameseEnabled;
    gEngine.reset();
    NSString *bundleId = NSWorkspace.sharedWorkspace.frontmostApplication.bundleIdentifier;
    if (self.smartPerAppMode && bundleId) {
        self.appModes[bundleId] = @(self.vietnameseEnabled);
    }
    [self syncUIStates];
    [self savePreferences];
}

- (void)onMethodPopupChanged:(NSPopUpButton *)sender {
    gEngine.config().input_method = (sender.indexOfSelectedItem == 0) ? "telex" : "vni";
    gEngine.reset();
    [self syncUIStates];
    [self savePreferences];
}

- (void)selectTelex:(id)sender {
    gEngine.config().input_method = "telex";
    gEngine.reset();
    [self syncUIStates];
    [self savePreferences];
}

- (void)selectVNI:(id)sender {
    gEngine.config().input_method = "vni";
    gEngine.reset();
    [self syncUIStates];
    [self savePreferences];
}

- (void)toggleOrthography:(id)sender {
    gEngine.config().modern_orthography = !gEngine.config().modern_orthography;
    [self syncUIStates];
    [self savePreferences];
}

- (void)toggleAutoRestore:(id)sender {
    gEngine.config().auto_restore_english = !gEngine.config().auto_restore_english;
    [self syncUIStates];
    [self savePreferences];
}

- (void)toggleAutocompleteFix:(id)sender {
    gEngine.config().fix_autocomplete = !gEngine.config().fix_autocomplete;
    [self syncUIStates];
    [self savePreferences];
}

- (void)togglePerAppMode:(id)sender {
    self.smartPerAppMode = !self.smartPerAppMode;
    [self syncUIStates];
    [self savePreferences];
}

- (void)toggleMacros:(id)sender {
    bool next = !gEngine.config().enable_macros;
    gEngine.config().enable_macros = next;
    gEngine.config().quick_telex = next;
    [self syncUIStates];
    [self savePreferences];
}

- (void)toggleLaunchAtLogin:(id)sender {
    self.launchAtLogin = !self.launchAtLogin;
    NSString *agentsDir = [NSHomeDirectory() stringByAppendingPathComponent:@"Library/LaunchAgents"];
    NSString *plistPath = [agentsDir stringByAppendingPathComponent:@"org.viskey.inputmethod.plist"];
    NSFileManager *fm = [NSFileManager defaultManager];

    if (self.launchAtLogin) {
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
    [self syncUIStates];
    [self savePreferences];
}

- (void)handleActiveAppChanged:(NSNotification *)notification {
    gEngine.reset();
    if (!self.smartPerAppMode) return;

    NSRunningApplication *app = notification.userInfo[NSWorkspaceApplicationKey];
    NSString *bundleId = app.bundleIdentifier;
    if (!bundleId) return;

    NSNumber *savedMode = self.appModes[bundleId];
    if (savedMode != nil) {
        self.vietnameseEnabled = savedMode.boolValue;
    } else if ([self.defaultEnglishBundles containsObject:bundleId]) {
        self.vietnameseEnabled = NO;
        self.appModes[bundleId] = @NO;
    }
    [self syncUIStates];
}

@end

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        [app setActivationPolicy:NSApplicationActivationPolicyAccessory];
        VisKeyController *delegate = [[VisKeyController alloc] init];
        app.delegate = delegate;
        [app run];
    }
    return 0;
}
