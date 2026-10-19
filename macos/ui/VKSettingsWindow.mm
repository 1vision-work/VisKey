// VKSettingsWindow.mm — see VKSettingsWindow.h. MIT License, 1VISION.

#import "VKSettingsWindow.h"
#import "VKPages.h"

static const CGFloat kVKWinW = 1000.0;
static const CGFloat kVKWinH = 740.0;
static const CGFloat kVKSidebarW = 220.0;
static const CGFloat kVKNavTop = 92.0;
static const CGFloat kVKNavStep = 38.0;

#pragma mark - Sidebar item

@interface VKNavItem : VKControlView
@property (nonatomic, copy) NSString *symbolName;
@property (nonatomic, copy) NSString *title;
@property (nonatomic, copy, nullable) NSString *badge;
@property (nonatomic, strong, nullable) NSColor *dotColor;
@property (nonatomic) BOOL selected;
@end

@implementation VKNavItem

- (void)setSelected:(BOOL)selected { _selected = selected; self.needsDisplay = YES; }
- (void)setBadge:(NSString *)badge { _badge = [badge copy]; self.needsDisplay = YES; }
- (void)setDotColor:(NSColor *)dotColor { _dotColor = dotColor; self.needsDisplay = YES; }

- (void)drawRect:(NSRect)dirtyRect {
    NSRect b = self.bounds;
    if (self.selected) {
        [VKTheme.sidebarSelected setFill];
        [[NSBezierPath bezierPathWithRoundedRect:b xRadius:8 yRadius:8] fill];
    } else if (self.hovering) {
        [VKDynamicA(0x000000, 0.04, 0xFFFFFF, 0.05) setFill];
        [[NSBezierPath bezierPathWithRoundedRect:b xRadius:8 yRadius:8] fill];
    }
    NSColor *fg = self.selected ? VKTheme.primary : VKTheme.text;
    VKDrawSymbol(self.symbolName, 14, NSFontWeightMedium, self.selected ? VKTheme.primary : VKTheme.textSecondary,
                 NSMakeRect(10, 0, 22, b.size.height));
    NSDictionary *ta = @{NSFontAttributeName: VKFont(14, self.selected ? NSFontWeightSemibold : NSFontWeightRegular),
                         NSForegroundColorAttributeName: fg};
    NSSize ts = [self.title sizeWithAttributes:ta];
    [self.title drawAtPoint:NSMakePoint(40, round(NSMidY(b) - ts.height / 2.0)) withAttributes:ta];
    if (self.badge.length) {
        NSDictionary *ba = @{NSFontAttributeName: VKFont(13, NSFontWeightMedium),
                             NSForegroundColorAttributeName: self.selected ? VKTheme.primary : VKTheme.textSecondary};
        NSSize bs = [self.badge sizeWithAttributes:ba];
        [self.badge drawAtPoint:NSMakePoint(b.size.width - 12 - bs.width, round(NSMidY(b) - bs.height / 2.0)) withAttributes:ba];
    }
    if (self.dotColor) {
        [self.dotColor setFill];
        [[NSBezierPath bezierPathWithOvalInRect:NSMakeRect(b.size.width - 20, NSMidY(b) - 4, 8, 8)] fill];
    }
}

@end

#pragma mark - Window controller

@interface VKSettingsWindowController ()
@property (nonatomic, strong) VKModel *model;
@property (nonatomic) BOOL snapshot;
@property (nonatomic, readwrite) VKSettingsPage currentPage;
@property (nonatomic, strong) NSArray<VKPage *> *pages;
@property (nonatomic, strong) NSArray<VKNavItem *> *navItems;
@property (nonatomic, strong) NSScrollView *scroll;
@property (nonatomic, strong) VKFieldBox *searchBox;
@property (nonatomic, strong, nullable) NSTimer *envTimer;
@end

@implementation VKSettingsWindowController

- (instancetype)initWithModel:(VKModel *)model snapshot:(BOOL)snapshot {
    NSWindowStyleMask style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                              NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskFullSizeContentView;
    NSWindow *w = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, kVKWinW, kVKWinH)
                                              styleMask:style
                                                backing:NSBackingStoreBuffered
                                                  defer:NO];
    w.title = @"Bảng điều khiển";
    w.titleVisibility = NSWindowTitleHidden;
    w.titlebarAppearsTransparent = YES;
    w.releasedWhenClosed = NO;
    [w center];
    self = [super initWithWindow:w];
    if (self) {
        _model = model;
        _snapshot = snapshot;
        w.delegate = self;
        [self buildContent];
        NSNotificationCenter *nc = [NSNotificationCenter defaultCenter];
        [nc addObserver:self selector:@selector(modelChanged:) name:VKModelDidChangeNotification object:model];
        [nc addObserver:self selector:@selector(usageChanged:) name:VKMacroUsageDidChangeNotification object:model];
        [nc addObserver:self selector:@selector(environmentChanged:) name:VKEnvironmentDidChangeNotification object:model];
        [self showPage:VKSettingsPageGeneral];
    }
    return self;
}

- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [_envTimer invalidate];
}

- (void)buildContent {
    VKBox *root = [[VKBox alloc] initWithFrame:NSMakeRect(0, 0, kVKWinW, kVKWinH) fill:VKTheme.windowBg radius:0];
    self.window.contentView = root;

    VKBox *sidebar = [[VKBox alloc] initWithFrame:NSMakeRect(0, 0, kVKSidebarW, kVKWinH) fill:VKTheme.sidebarBg radius:0];
    sidebar.rightSeparator = YES;
    [root addSubview:sidebar];

    if (self.snapshot) {
        VKFakeTrafficLights *lights = [[VKFakeTrafficLights alloc] initWithFrame:NSMakeRect(20, 18, 52, 12)];
        [sidebar addSubview:lights];
    }

    self.searchBox = [[VKFieldBox alloc] initWithFrame:NSMakeRect(14, 44, kVKSidebarW - 28, 30)
                                           placeholder:@"Tìm kiếm" font:VKFont(13.5, NSFontWeightRegular) symbol:@"magnifyingglass"];
    self.searchBox.fillColor = VKDynamic(0xE1E3E9, 0x2A2D33);
    self.searchBox.borderColor = nil;
    self.searchBox.field.delegate = self;
    [sidebar addSubview:self.searchBox];

    NSArray *defs = @[
        @[@"Chung", @"slider.horizontal.3"],
        @[@"Gõ tắt", @"bolt"],
        @[@"Theo ứng dụng", @"square.grid.2x2"],
        @[@"Phím tắt", @"keyboard"],
        @[@"Quyền & Bảo mật", @"checkmark.shield"],
        @[@"Giới thiệu", @"info.circle"]
    ];
    NSMutableArray<VKNavItem *> *items = [NSMutableArray array];
    __weak VKSettingsWindowController *weakSelf = self;
    for (NSUInteger i = 0; i < defs.count; i++) {
        VKNavItem *item = [[VKNavItem alloc] initWithFrame:NSMakeRect(12, kVKNavTop + kVKNavStep * i, kVKSidebarW - 24, 34)];
        item.title = defs[i][0];
        item.symbolName = defs[i][1];
        item.showsHandCursor = NO;
        VKSettingsPage page = (VKSettingsPage)i;
        item.onClick = ^{ [weakSelf showPage:page]; };
        [sidebar addSubview:item];
        [items addObject:item];
    }
    self.navItems = items;

    VKBox *footer = [[VKBox alloc] initWithFrame:NSMakeRect(12, kVKWinH - 16 - 54, kVKSidebarW - 24, 54)
                                            fill:VKDynamic(0xF6F7F9, 0x25282D) radius:10];
    [sidebar addSubview:footer];
    VKMarkView *mark = [[VKMarkView alloc] initWithFrame:NSMakeRect(10, 11, 32, 32)];
    [footer addSubview:mark];
    NSTextField *name = VKLabel(@"VisKey", VKFont(14, NSFontWeightSemibold), VKTheme.text);
    VKSetOrigin(name, 50, 9);
    [footer addSubview:name];
    NSTextField *ver = VKLabel([NSString stringWithFormat:@"Phiên bản %@", [self.model versionString]],
                               VKFont(12.5, NSFontWeightRegular), VKTheme.textSecondary);
    VKSetOrigin(ver, 50, 28);
    [footer addSubview:ver];

    self.scroll = [[NSScrollView alloc] initWithFrame:NSMakeRect(kVKSidebarW, 0, kVKWinW - kVKSidebarW, kVKWinH)];
    self.scroll.drawsBackground = NO;
    self.scroll.hasVerticalScroller = YES;
    self.scroll.autohidesScrollers = YES;
    self.scroll.borderType = NSNoBorder;
    self.scroll.contentView.drawsBackground = NO;
    [root addSubview:self.scroll];

    CGFloat pw = kVKWinW - kVKSidebarW;
    self.pages = @[
        [[VKGeneralPage alloc] initWithModel:self.model width:pw height:kVKWinH snapshot:self.snapshot],
        [[VKMacrosPage alloc] initWithModel:self.model width:pw height:kVKWinH snapshot:self.snapshot],
        [[VKAppsPage alloc] initWithModel:self.model width:pw height:kVKWinH snapshot:self.snapshot],
        [[VKShortcutsPage alloc] initWithModel:self.model width:pw height:kVKWinH snapshot:self.snapshot],
        [[VKPrivacyPage alloc] initWithModel:self.model width:pw height:kVKWinH snapshot:self.snapshot],
        [[VKAboutPage alloc] initWithModel:self.model width:pw height:kVKWinH snapshot:self.snapshot]
    ];
    [self refreshSidebar];
}

- (VKPage *)pageAt:(VKSettingsPage)page {
    NSUInteger i = (NSUInteger)page;
    return i < self.pages.count ? self.pages[i] : self.pages[0];
}

- (void)showPage:(VKSettingsPage)page {
    VKPage *old = [self pageAt:self.currentPage];
    VKPage *next = [self pageAt:page];
    if (old != next) [old pageDidDisappear];
    self.currentPage = page;
    self.scroll.documentView = next;
    [next pageWillAppear];
    [self.scroll.contentView scrollToPoint:NSZeroPoint];
    [self.scroll reflectScrolledClipView:self.scroll.contentView];
    for (NSUInteger i = 0; i < self.navItems.count; i++) {
        self.navItems[i].selected = (i == (NSUInteger)page);
    }
}

- (void)refreshSidebar {
    if (self.navItems.count < 6) return;
    self.navItems[VKSettingsPageMacros].badge = VKFormatCount((NSInteger)self.model.macros.count);
    self.navItems[VKSettingsPageShortcuts].dotColor = [self.model hasHotkeyConflict] ? VKTheme.warning : nil;
    self.navItems[VKSettingsPagePrivacy].dotColor = [self.model accessibilityTrusted] ? nil : VKTheme.warning;
}

#pragma mark Notifications

- (void)modelChanged:(NSNotification *)n {
    [self refreshSidebar];
    [[self pageAt:self.currentPage] reload];
}

- (void)usageChanged:(NSNotification *)n {
    if (self.currentPage == VKSettingsPageMacros) [[self pageAt:self.currentPage] reload];
}

- (void)environmentChanged:(NSNotification *)n {
    [self refreshSidebar];
    [[self pageAt:self.currentPage] environmentChanged];
}

#pragma mark Search

- (void)controlTextDidChange:(NSNotification *)notification {
    if (notification.object != self.searchBox.field) return;
    NSString *q = self.searchBox.field.stringValue;
    NSString *trimmed = [q stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
    NSStringCompareOptions opts = NSCaseInsensitiveSearch | NSDiacriticInsensitiveSearch;
    CGFloat y = kVKNavTop;
    NSInteger firstMatch = -1;
    BOOL currentMatches = NO;
    for (NSUInteger i = 0; i < self.navItems.count && i < self.pages.count; i++) {
        VKNavItem *item = self.navItems[i];
        BOOL match = [self.pages[i] applySearch:q];
        if (trimmed.length && [item.title rangeOfString:trimmed options:opts].location != NSNotFound) match = YES;
        item.hidden = !match;
        if (match) {
            VKSetOrigin(item, 12, y);
            y += kVKNavStep;
            if (firstMatch < 0) firstMatch = (NSInteger)i;
            if (i == (NSUInteger)self.currentPage) currentMatches = YES;
        }
    }
    if (!currentMatches && firstMatch >= 0) [self showPage:(VKSettingsPage)firstMatch];
}

#pragma mark NSWindowDelegate

- (void)windowDidBecomeKey:(NSNotification *)notification {
    if (self.snapshot || self.envTimer) return;
    [self refreshSidebar];
    [[self pageAt:self.currentPage] environmentChanged];
    __weak VKSettingsWindowController *weakSelf = self;
    self.envTimer = [NSTimer scheduledTimerWithTimeInterval:2.0 repeats:YES block:^(NSTimer *timer) {
        VKSettingsWindowController *s = weakSelf;
        if (!s) { [timer invalidate]; return; }
        [s refreshSidebar];
        [[s pageAt:s.currentPage] environmentChanged];
    }];
}

- (void)windowWillClose:(NSNotification *)notification {
    [VKHotkeyRecorder stopActiveRecorder];
    [self.envTimer invalidate];
    self.envTimer = nil;
}

@end
