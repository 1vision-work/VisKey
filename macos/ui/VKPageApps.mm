// VKPageApps.mm — "Theo ứng dụng" page (design 05). MIT License, 1VISION.

#import "VKPages.h"

static NSString *const kVKAppRowId = @"VKAppRow";
static const CGFloat kVKSourceColX = 420.0;

#pragma mark - Row view

@interface VKAppRow : VKBox
@property (nonatomic, copy) NSString *bundleId;
@property (nonatomic, strong) VKIconTile *icon;
@property (nonatomic, strong) NSTextField *nameLabel;
@property (nonatomic, strong) VKChip *runningChip;
@property (nonatomic, strong) NSTextField *bundleLabel;
@property (nonatomic, strong) NSTextField *sourceLabel;
@property (nonatomic, strong) VKSegmented *modeControl;
- (void)configureWithRule:(VKAppRule *)rule model:(VKModel *)model;
@end

static NSString *VKFallbackSymbol(NSString *bundleId) {
    NSString *b = bundleId.lowercaseString;
    if ([b containsString:@"terminal"] || [b containsString:@"iterm"] || [b containsString:@"warp"] ||
        [b containsString:@"alacritty"] || [b containsString:@"kitty"]) return @"terminal";
    if ([b containsString:@"code"] || [b containsString:@"xcode"] || [b containsString:@"jetbrains"] ||
        [b containsString:@"sublime"] || [b containsString:@"zed"]) return @"chevron.left.forwardslash.chevron.right";
    if ([b containsString:@"safari"] || [b containsString:@"chrome"] || [b containsString:@"firefox"] ||
        [b containsString:@"browser"] || [b containsString:@"edge"]) return @"globe";
    if ([b containsString:@"notes"] || [b containsString:@"word"] || [b containsString:@"pages"]) return @"doc.text";
    if ([b containsString:@"slack"] || [b containsString:@"chat"] || [b containsString:@"messag"] ||
        [b containsString:@"zalo"] || [b containsString:@"telegram"] || [b containsString:@"discord"]) return @"bubble.left";
    return @"app";
}

@implementation VKAppRow

- (instancetype)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        self.bottomSeparator = YES;
        _icon = [[VKIconTile alloc] initWithFrame:NSMakeRect(20, 12, 32, 32)];
        _icon.symbolSize = 15;
        [self addSubview:_icon];
        _nameLabel = VKLabel(@"", VKFont(14.5, NSFontWeightSemibold), VKTheme.text);
        [self addSubview:_nameLabel];
        _runningChip = [VKChip chipWithText:@"Đang mở" font:VKFont(11.5, NSFontWeightSemibold)
                                       text:VKTheme.accentText fill:VKTheme.accentTint height:19];
        [self addSubview:_runningChip];
        _bundleLabel = VKLabel(@"", VKMono(11.5, NSFontWeightRegular), VKTheme.textSecondary);
        [self addSubview:_bundleLabel];
        _sourceLabel = VKLabel(@"", VKFont(13.5, NSFontWeightRegular), VKTheme.textSecondary);
        [self addSubview:_sourceLabel];
        _modeControl = [[VKSegmented alloc] initWithFrame:NSMakeRect(0, 0, 210, 34) items:@[@"VI", @"EN", @"Nhớ"]];
        _modeControl.style = VKSegmentStyleFilled;
        _modeControl.font = VKFont(13.5, NSFontWeightRegular);
        _modeControl.selectedFont = VKFont(13.5, NSFontWeightSemibold);
        _modeControl.fillForIndex = ^NSColor *(NSInteger index) {
            return index == 1 ? VKTheme.darkSegmentFill : VKTheme.primaryFill;
        };
        [self addSubview:_modeControl];
    }
    return self;
}

- (void)setFrameSize:(NSSize)newSize {
    [super setFrameSize:newSize];
    [self layoutRow];
}

- (void)layoutRow {
    if (!self.modeControl) return;
    CGFloat w = self.bounds.size.width, h = self.bounds.size.height;
    VKSetOrigin(self.icon, 20, round((h - 32) / 2.0));
    VKSetOrigin(self.modeControl, w - 20 - self.modeControl.frame.size.width, round((h - 34) / 2.0));
    CGFloat srcX = MIN(kVKSourceColX, self.modeControl.frame.origin.x - 110);
    VKPlaceLabel(self.sourceLabel, srcX, h / 2.0);
    CGFloat maxText = srcX - 66 - 16;
    [self.nameLabel sizeToFit];
    NSRect nf = self.nameLabel.frame;
    CGFloat chipSpace = self.runningChip.hidden ? 0 : self.runningChip.frame.size.width + 8;
    nf.size.width = MIN(nf.size.width, maxText - chipSpace);
    nf.origin = NSMakePoint(66, round(h / 2.0 - nf.size.height - 0.5));
    self.nameLabel.frame = nf;
    VKSetOrigin(self.runningChip, NSMaxX(nf) + 8, round(NSMidY(nf) - 9.5));
    [self.bundleLabel sizeToFit];
    NSRect bf = self.bundleLabel.frame;
    bf.size.width = MIN(bf.size.width, maxText);
    bf.origin = NSMakePoint(66, round(h / 2.0 + 1));
    self.bundleLabel.frame = bf;
}

- (void)configureWithRule:(VKAppRule *)rule model:(VKModel *)model {
    self.bundleId = rule.bundleId;
    NSImage *img = [VKModel iconForBundle:rule.bundleId];
    self.icon.image = img;
    self.icon.symbolName = img ? nil : VKFallbackSymbol(rule.bundleId);
    self.icon.fillColor = img ? [NSColor clearColor] : VKTheme.iconTileBg;
    self.icon.needsDisplay = YES;
    self.nameLabel.stringValue = [VKModel displayNameForBundle:rule.bundleId fallback:rule.name];
    self.bundleLabel.stringValue = rule.bundleId;
    BOOL running = [model isRunningFrontmost:rule.bundleId];
    self.runningChip.hidden = !running;
    self.fillColor = running ? VKTheme.hoverBg : nil;

    switch (rule.source) {
        case VKRuleSourceDefault:
            self.sourceLabel.stringValue = @"Mặc định";
            self.sourceLabel.font = VKFont(13.5, NSFontWeightRegular);
            self.sourceLabel.textColor = VKTheme.textSecondary;
            break;
        case VKRuleSourceUser:
            self.sourceLabel.stringValue = @"Bạn đặt";
            self.sourceLabel.font = VKFont(13.5, NSFontWeightSemibold);
            self.sourceLabel.textColor = VKTheme.primary;
            break;
        case VKRuleSourceAuto:
            self.sourceLabel.stringValue = @"Tự ghi nhớ";
            self.sourceLabel.font = VKFont(13.5, NSFontWeightRegular);
            self.sourceLabel.textColor = VKTheme.textSecondary;
            break;
    }
    NSString *remember = @"Nhớ";
    if (rule.mode == VKAppModeRemember && rule.lastVietnamese) {
        remember = rule.lastVietnamese.boolValue ? @"Nhớ · VI" : @"Nhớ · EN";
    }
    self.modeControl.items = @[@"VI", @"EN", remember];
    self.modeControl.selectedIndex = (NSInteger)rule.mode;
    [self layoutRow];
}

@end

#pragma mark - Page

@interface VKAppsPage ()
@property (nonatomic, strong) VKSwitch *masterSwitch;
@property (nonatomic, strong) VKSegmented *policyControl;
@property (nonatomic, strong) VKFieldBox *filterBox;
@property (nonatomic, strong) NSTableView *table;
@property (nonatomic, strong) NSTextField *footerLabel;
@property (nonatomic, strong) NSArray<VKAppRule *> *rules;
@property (nonatomic, copy, nullable) NSString *lastCurrentApp;
@end

@implementation VKAppsPage

- (void)build {
    __weak VKAppsPage *weakSelf = self;
    CGFloat pad = kVKPagePad, W = self.contentWidth;
    self.rules = @[];
    self.keywords = @"theo ứng dụng terminal bundle khi mở ứng dụng mới";

    CGFloat y = [self addTitle:@"Theo ứng dụng"
                      subtitle:VKAttr(@"VisKey nhớ lần cuối bạn dùng VI hay EN trong từng ứng dụng.",
                                      VKFont(13.5, NSFontWeightRegular), VKTheme.textSecondary)];
    self.masterSwitch = [VKSwitch switchWithOn:YES];
    VKSetOrigin(self.masterSwitch, pad + W - self.masterSwitch.frame.size.width, 30);
    self.masterSwitch.onToggle = ^(BOOL on) {
        weakSelf.model.smartPerAppMode = on;
        [weakSelf.model commit];
    };
    [self addSubview:self.masterSwitch];

    // Policy card
    VKBox *policy = [self addCard:NSMakeRect(pad, y - 4, W, 66)];
    policy.searchText = @"Khi mở ứng dụng mới";
    [self.searchRows addObject:policy];
    NSTextField *pt = VKLabel(@"Khi mở ứng dụng mới", VKFont(14.5, NSFontWeightSemibold), VKTheme.text);
    VKSetOrigin(pt, 20, 13);
    [policy addSubview:pt];
    NSTextField *ps = VKLabel(@"Áp dụng cho ứng dụng chưa có trong danh sách", VKFont(13, NSFontWeightRegular), VKTheme.textSecondary);
    VKSetOrigin(ps, 20, 35);
    [policy addSubview:ps];
    self.policyControl = [[VKSegmented alloc] initWithFrame:NSMakeRect(W - 20 - 280, 15, 280, 36)
                                                      items:@[@"Nhớ lần cuối", @"Luôn VI", @"Luôn EN"]];
    self.policyControl.font = VKFont(13.5, NSFontWeightRegular);
    self.policyControl.selectedFont = VKFont(13.5, NSFontWeightSemibold);
    self.policyControl.onSelect = ^(NSInteger idx) {
        weakSelf.model.unknownAppPolicy = (VKNewAppPolicy)idx;
        [weakSelf.model commit];
    };
    [policy addSubview:self.policyControl];

    // Filter + add
    y = NSMaxY(policy.frame) + 16;
    VKButton *addBtn = [VKButton buttonWithTitle:@"Thêm ứng dụng" symbol:@"plus" style:VKButtonStylePrimary];
    addBtn.font = VKFont(14, NSFontWeightSemibold);
    CGFloat addW = [addBtn fittingWidth];
    addBtn.frame = NSMakeRect(pad + W - addW, y, addW, 36);
    addBtn.onClick = ^{ [weakSelf addApplication]; };
    [self addSubview:addBtn];
    self.filterBox = [[VKFieldBox alloc] initWithFrame:NSMakeRect(pad, y, W - addW - 12, 36)
                                           placeholder:@"Lọc ứng dụng..." font:VKFont(14, NSFontWeightRegular) symbol:@"magnifyingglass"];
    self.filterBox.field.delegate = self;
    [self addSubview:self.filterBox];

    // Table card
    y = NSMaxY(self.filterBox.frame) + 14;
    CGFloat cardH = self.viewportHeight - 24 - y;
    VKBox *card = [self addCard:NSMakeRect(pad, y, W, cardH)];
    VKBox *header = [[VKBox alloc] initWithFrame:NSMakeRect(1, 1, W - 2, 42) fill:nil radius:0];
    header.bottomSeparator = YES;
    [card addSubview:header];
    CGFloat modeX = (W - 2) - 20 - 210;
    CGFloat srcX = MIN(kVKSourceColX, modeX - 110);
    NSArray *cols = @[@[@"ỨNG DỤNG", @20], @[@"NGUỒN", @(srcX)], @[@"CHẾ ĐỘ", @(modeX)]];
    for (NSArray *c in cols) {
        NSTextField *l = VKAttrLabel(VKAttrKern(c[0], VKFont(12, NSFontWeightSemibold), VKTheme.textSecondary, 0.6));
        VKPlaceLabel(l, [c[1] doubleValue], 21);
        [header addSubview:l];
    }

    CGFloat footH = 44;
    NSScrollView *scroll = [[NSScrollView alloc] initWithFrame:NSMakeRect(1, 43, W - 2, cardH - 43 - footH)];
    scroll.drawsBackground = NO;
    scroll.hasVerticalScroller = YES;
    scroll.autohidesScrollers = YES;
    scroll.borderType = NSNoBorder;
    scroll.contentView.drawsBackground = NO;
    self.table = [[NSTableView alloc] initWithFrame:scroll.bounds];
    NSTableColumn *col = [[NSTableColumn alloc] initWithIdentifier:@"app"];
    col.width = W - 2;
    col.resizingMask = NSTableColumnAutoresizingMask;
    [self.table addTableColumn:col];
    self.table.headerView = nil;
    self.table.style = NSTableViewStyleFullWidth;
    self.table.rowHeight = 58;
    self.table.intercellSpacing = NSMakeSize(0, 0);
    self.table.backgroundColor = [NSColor clearColor];
    self.table.selectionHighlightStyle = NSTableViewSelectionHighlightStyleNone;
    self.table.focusRingType = NSFocusRingTypeNone;
    self.table.columnAutoresizingStyle = NSTableViewUniformColumnAutoresizingStyle;
    self.table.dataSource = self;
    self.table.delegate = self;
    NSMenu *menu = [[NSMenu alloc] initWithTitle:@""];
    NSMenuItem *removeItem = [[NSMenuItem alloc] initWithTitle:@"Xoá khỏi danh sách" action:@selector(removeClickedRule:) keyEquivalent:@""];
    removeItem.target = self;
    [menu addItem:removeItem];
    self.table.menu = menu;
    scroll.documentView = self.table;
    [card addSubview:scroll];

    VKBox *footer = [[VKBox alloc] initWithFrame:NSMakeRect(1, cardH - footH - 1, W - 2, footH) fill:nil radius:0];
    footer.topSeparator = YES;
    [card addSubview:footer];
    self.footerLabel = VKLabel(@"", VKFont(13, NSFontWeightRegular), VKTheme.textSecondary);
    [footer addSubview:self.footerLabel];
    VKButton *restore = [VKButton buttonWithTitle:@"Khôi phục danh sách mặc định" symbol:nil style:VKButtonStyleLink];
    restore.frame = NSMakeRect(footer.frame.size.width - 20 - [restore fittingWidth], 10, [restore fittingWidth], 24);
    restore.onClick = ^{ [weakSelf confirmRestoreDefaults]; };
    [footer addSubview:restore];
}

- (void)reload {
    VKModel *m = self.model;
    self.masterSwitch.on = m.smartPerAppMode;
    self.policyControl.selectedIndex = (NSInteger)m.unknownAppPolicy;
    self.filterBox.field.placeholderString = [NSString stringWithFormat:@"Lọc %@ ứng dụng...", VKFormatCount((NSInteger)m.appRules.count)];
    [self reloadTable];
}

- (void)environmentChanged {
    NSString *cur = self.model.currentAppBundleId ?: @"";
    if ([cur isEqualToString:self.lastCurrentApp ?: @""]) return;
    self.lastCurrentApp = cur;
    [self reloadTable];
}

- (void)reloadTable {
    NSString *q = [self.filterBox.field.stringValue stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
    NSStringCompareOptions opts = NSCaseInsensitiveSearch | NSDiacriticInsensitiveSearch;
    NSMutableArray<VKAppRule *> *list = [NSMutableArray array];
    for (VKAppRule *r in [self.model sortedRules]) {
        if (q.length) {
            NSString *name = [VKModel displayNameForBundle:r.bundleId fallback:r.name];
            if ([name rangeOfString:q options:opts].location == NSNotFound &&
                [r.bundleId rangeOfString:q options:opts].location == NSNotFound) continue;
        }
        [list addObject:r];
    }
    self.rules = list;
    [self.table reloadData];
    self.footerLabel.stringValue = [NSString stringWithFormat:@"Hiển thị %@ / %@ ứng dụng",
                                    VKFormatCount((NSInteger)list.count), VKFormatCount((NSInteger)self.model.appRules.count)];
    VKPlaceLabel(self.footerLabel, 20, 22);
}

#pragma mark Actions

- (void)setMode:(VKAppMode)mode forBundle:(NSString *)bundleId {
    VKAppRule *r = [self.model ruleForBundle:bundleId];
    if (!r) return;
    r.mode = mode;
    r.source = VKRuleSourceUser;
    [self.model commit];
}

- (void)removeClickedRule:(id)sender {
    NSInteger row = self.table.clickedRow;
    if (row < 0 || row >= (NSInteger)self.rules.count) return;
    [self.model removeRuleForBundle:self.rules[(NSUInteger)row].bundleId];
    [self.model commit];
}

- (void)addApplication {
    if (self.snapshot || !self.window) return;
    NSOpenPanel *panel = [NSOpenPanel openPanel];
    panel.canChooseFiles = YES;
    panel.canChooseDirectories = NO;
    panel.allowsMultipleSelection = YES;
    panel.directoryURL = [NSURL fileURLWithPath:@"/Applications" isDirectory:YES];
    panel.prompt = @"Thêm";
    panel.message = @"Chọn ứng dụng để VisKey ghi nhớ chế độ VI/EN riêng.";
    __weak VKAppsPage *weakSelf = self;
    [panel beginSheetModalForWindow:self.window completionHandler:^(NSModalResponse result) {
        VKAppsPage *s = weakSelf;
        if (!s || result != NSModalResponseOK) return;
        NSString *own = [NSBundle mainBundle].bundleIdentifier;
        NSInteger added = 0;
        NSMutableArray<NSString *> *bad = [NSMutableArray array];
        for (NSURL *url in panel.URLs) {
            NSString *bid = [NSBundle bundleWithURL:url].bundleIdentifier;
            if (bid.length == 0) {
                [bad addObject:url.lastPathComponent ?: @"?"];
                continue;
            }
            if ((own && [bid isEqualToString:own]) || [s.model ruleForBundle:bid]) continue;
            VKAppRule *r = [VKAppRule ruleWithBundleId:bid name:nil mode:VKAppModeRemember source:VKRuleSourceUser];
            [s.model setRule:r];
            added++;
        }
        if (added) [s.model commit];
        if (bad.count) {
            NSString *info = [bad componentsJoinedByString:@", "];
            dispatch_async(dispatch_get_main_queue(), ^{
                [s showAlert:@"Không đọc được mã ứng dụng (bundle id)." info:info];
            });
        }
    }];
}

- (void)confirmRestoreDefaults {
    if (self.snapshot || !self.window) return;
    NSAlert *alert = [[NSAlert alloc] init];
    alert.messageText = @"Khôi phục danh sách mặc định?";
    alert.informativeText = @"Các quy tắc bạn đặt và các ứng dụng tự ghi nhớ sẽ bị xoá. Terminal, iTerm2, Warp, Alacritty, Kitty sẽ trở lại mặc định EN.";
    [alert addButtonWithTitle:@"Khôi phục"];
    [alert addButtonWithTitle:@"Huỷ"];
    __weak VKAppsPage *weakSelf = self;
    [alert beginSheetModalForWindow:self.window completionHandler:^(NSModalResponse code) {
        if (code != NSAlertFirstButtonReturn) return;
        [weakSelf.model restoreDefaultRules];
        [weakSelf.model commit];
    }];
}

#pragma mark Table

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    return (NSInteger)self.rules.count;
}

- (NSView *)tableView:(NSTableView *)tableView viewForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {
    VKAppRow *view = [tableView makeViewWithIdentifier:kVKAppRowId owner:self];
    if (!view) {
        view = [[VKAppRow alloc] initWithFrame:NSMakeRect(0, 0, tableView.bounds.size.width, 58)];
        view.identifier = kVKAppRowId;
    }
    if (row < 0 || row >= (NSInteger)self.rules.count) return view;
    VKAppRule *rule = self.rules[(NSUInteger)row];
    [view configureWithRule:rule model:self.model];
    __weak VKAppsPage *weakSelf = self;
    NSString *bundleId = rule.bundleId;
    view.modeControl.onSelect = ^(NSInteger idx) {
        [weakSelf setMode:(VKAppMode)idx forBundle:bundleId];
    };
    return view;
}

- (BOOL)tableView:(NSTableView *)tableView shouldSelectRow:(NSInteger)row {
    return NO;
}

- (void)controlTextDidChange:(NSNotification *)notification {
    if (notification.object == self.filterBox.field) [self reloadTable];
}

@end
