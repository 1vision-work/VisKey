// VKPopover.mm — see VKPopover.h. MIT License, 1VISION.

#import "VKPopover.h"
#import "VKUIKit.h"

static const CGFloat kVKPopW = 340.0;

#pragma mark - VI / EN switch

@interface VKLangSwitch : VKControlView
@property (nonatomic) BOOL vietnamese;
@property (nonatomic, copy, nullable) void (^onPick)(BOOL vietnamese);
@end

@implementation VKLangSwitch

- (void)handleClickAtPoint:(NSPoint)point {
    BOOL vi = point.x < self.bounds.size.width / 2.0;
    if (self.onPick) self.onPick(vi);
}

- (void)drawHalf:(NSRect)r big:(NSString *)big small:(NSString *)small selected:(BOOL)selected dot:(BOOL)dot {
    if (selected) {
        [NSGraphicsContext saveGraphicsState];
        NSShadow *sh = [[NSShadow alloc] init];
        sh.shadowOffset = NSMakeSize(0, -1);
        sh.shadowBlurRadius = 3;
        sh.shadowColor = [NSColor colorWithWhite:0 alpha:0.18];
        [sh set];
        [VKTheme.primaryFill setFill];
        [[NSBezierPath bezierPathWithRoundedRect:r xRadius:11 yRadius:11] fill];
        [NSGraphicsContext restoreGraphicsState];
    }
    NSDictionary *ba = @{NSFontAttributeName: VKFont(21, NSFontWeightBold),
                         NSForegroundColorAttributeName: selected ? [NSColor whiteColor] : VKTheme.text};
    NSDictionary *sa = @{NSFontAttributeName: VKFont(13.5, NSFontWeightRegular),
                         NSForegroundColorAttributeName: selected ? VKHexA(0xFFFFFF, 0.9) : VKTheme.textSecondary};
    NSSize bs = [big sizeWithAttributes:ba];
    NSSize ss = [small sizeWithAttributes:sa];
    CGFloat total = bs.height + ss.height - 2;
    CGFloat top = round(NSMidY(r) - total / 2.0);
    CGFloat dotW = dot ? 12 : 0;
    CGFloat bx = round(NSMidX(r) - (bs.width + dotW) / 2.0);
    [big drawAtPoint:NSMakePoint(bx, top) withAttributes:ba];
    if (dot) {
        [VKTheme.jadeDot setFill];
        [[NSBezierPath bezierPathWithOvalInRect:NSMakeRect(bx + bs.width + 5, top + bs.height / 2.0 - 3.5, 7, 7)] fill];
    }
    [small drawAtPoint:NSMakePoint(round(NSMidX(r) - ss.width / 2.0), top + bs.height - 2) withAttributes:sa];
}

- (void)drawRect:(NSRect)dirtyRect {
    NSRect b = self.bounds;
    [VKTheme.trackBg setFill];
    [[NSBezierPath bezierPathWithRoundedRect:b xRadius:14 yRadius:14] fill];
    CGFloat half = b.size.width / 2.0;
    NSRect left = NSMakeRect(4, 4, half - 4, b.size.height - 8);
    NSRect right = NSMakeRect(half, 4, half - 4, b.size.height - 8);
    [self drawHalf:left big:@"VI" small:@"Tiếng Việt" selected:self.vietnamese dot:self.vietnamese];
    [self drawHalf:right big:@"EN" small:@"English" selected:!self.vietnamese dot:NO];
}

@end

#pragma mark - Tile

@interface VKPopTile : VKControlView
@property (nonatomic, copy, nullable) NSString *symbolName;
@property (nonatomic, copy, nullable) NSString *glyph;
@property (nonatomic, copy) NSString *title;
@property (nonatomic, copy) NSString *subtitle;
@property (nonatomic) BOOL on;
@end

@implementation VKPopTile

- (void)drawRect:(NSRect)dirtyRect {
    NSRect b = self.bounds;
    NSBezierPath *p = [NSBezierPath bezierPathWithRoundedRect:NSInsetRect(b, 0.5, 0.5) xRadius:12 yRadius:12];
    [VKTheme.groupBg setFill];
    [p fill];
    if (self.hovering) {
        [VKTheme.hoverBg setFill];
        [p fill];
    }
    p.lineWidth = 1;
    [VKDynamic(0xE3E5EA, 0x2E3238) setStroke];
    [p stroke];

    NSRect circle = NSMakeRect(14, 14, 32, 32);
    [(self.on ? VKTheme.primaryFill : VKTheme.trackBg) setFill];
    [[NSBezierPath bezierPathWithOvalInRect:circle] fill];
    NSColor *iconColor = self.on ? [NSColor whiteColor] : VKTheme.textSecondary;
    if (self.symbolName) {
        VKDrawSymbol(self.symbolName, 14, NSFontWeightSemibold, iconColor, circle);
    } else if (self.glyph) {
        NSDictionary *ga = @{NSFontAttributeName: VKFont(16, NSFontWeightSemibold), NSForegroundColorAttributeName: iconColor};
        NSSize gs = [self.glyph sizeWithAttributes:ga];
        [self.glyph drawAtPoint:NSMakePoint(NSMidX(circle) - gs.width / 2.0, NSMidY(circle) - gs.height / 2.0) withAttributes:ga];
    }
    NSDictionary *ta = @{NSFontAttributeName: VKFont(14, NSFontWeightSemibold), NSForegroundColorAttributeName: VKTheme.text};
    [self.title drawAtPoint:NSMakePoint(14, 56) withAttributes:ta];
    NSMutableParagraphStyle *ps = [[NSMutableParagraphStyle alloc] init];
    ps.lineBreakMode = NSLineBreakByTruncatingTail;
    NSDictionary *sa = @{NSFontAttributeName: VKFont(13, NSFontWeightRegular),
                         NSForegroundColorAttributeName: self.on ? VKTheme.accentText : VKTheme.textSecondary,
                         NSParagraphStyleAttributeName: ps};
    [self.subtitle drawInRect:NSMakeRect(14, 75, b.size.width - 24, 18) withAttributes:sa];
}

@end

#pragma mark - Menu row

@interface VKMenuRow : VKControlView
@property (nonatomic, copy) NSString *title;
@property (nonatomic, copy) NSString *shortcut;
@end

@implementation VKMenuRow

- (void)drawRect:(NSRect)dirtyRect {
    NSRect b = self.bounds;
    if (self.hovering) {
        [VKDynamicA(0x000000, 0.05, 0xFFFFFF, 0.07) setFill];
        [[NSBezierPath bezierPathWithRoundedRect:b xRadius:6 yRadius:6] fill];
    }
    NSDictionary *ta = @{NSFontAttributeName: VKFont(14, NSFontWeightRegular), NSForegroundColorAttributeName: VKTheme.text};
    NSSize ts = [self.title sizeWithAttributes:ta];
    [self.title drawAtPoint:NSMakePoint(8, round(NSMidY(b) - ts.height / 2.0)) withAttributes:ta];
    NSDictionary *sa = @{NSFontAttributeName: VKMono(12.5, NSFontWeightRegular), NSForegroundColorAttributeName: VKTheme.textSecondary};
    NSSize ss = [self.shortcut sizeWithAttributes:sa];
    [self.shortcut drawAtPoint:NSMakePoint(b.size.width - 8 - ss.width, round(NSMidY(b) - ss.height / 2.0)) withAttributes:sa];
}

@end

#pragma mark - Controller

@interface VKPopoverController ()
@property (nonatomic, strong) VKModel *model;
@end

@implementation VKPopoverController

- (instancetype)initWithModel:(VKModel *)model {
    self = [super initWithNibName:nil bundle:nil];
    if (self) {
        _model = model;
    }
    return self;
}

- (void)loadView {
    VKBox *root = [[VKBox alloc] initWithFrame:NSMakeRect(0, 0, kVKPopW, 560) fill:VKDynamic(0xEFF1F6, 0x1F2226) radius:0];
    self.view = root;
    [self reload];
}

- (VKPopTile *)tileAt:(NSRect)frame symbol:(NSString *)symbol glyph:(NSString *)glyph title:(NSString *)title
             subtitle:(NSString *)subtitle on:(BOOL)on {
    VKPopTile *t = [[VKPopTile alloc] initWithFrame:frame];
    t.symbolName = symbol;
    t.glyph = glyph;
    t.title = title;
    t.subtitle = subtitle;
    t.on = on;
    [self.view addSubview:t];
    return t;
}

- (void)reload {
    if (!self.isViewLoaded) return;
    NSView *root = self.view;
    for (NSView *v in [root.subviews copy]) [v removeFromSuperview];
    VKModel *m = self.model;
    __weak VKPopoverController *weakSelf = self;
    CGFloat pad = 16, W = kVKPopW, IW = W - 2 * pad;

    // Header
    VKMarkView *mark = [[VKMarkView alloc] initWithFrame:NSMakeRect(pad - 2, 12, 30, 30)];
    [root addSubview:mark];
    NSTextField *name = VKLabel(@"VisKey", VKFont(16, NSFontWeightSemibold), VKTheme.text);
    VKPlaceLabel(name, pad + 36, 27);
    [root addSubview:name];
    VKButton *gear = [VKButton buttonWithTitle:@"" symbol:@"gearshape" style:VKButtonStylePlain];
    gear.font = VKFont(17, NSFontWeightRegular);
    gear.frame = NSMakeRect(W - pad - 26, 14, 26, 26);
    gear.toolTip = @"Bảng điều khiển";
    gear.onClick = ^{ [weakSelf.model.actions vk_openSettingsPage:VKSettingsPageGeneral]; };
    [root addSubview:gear];

    // VI / EN
    CGFloat y = 52;
    VKLangSwitch *lang = [[VKLangSwitch alloc] initWithFrame:NSMakeRect(pad, y, IW, 66)];
    lang.vietnamese = m.vietnameseEnabled;
    lang.onPick = ^(BOOL vi) { [weakSelf.model.actions vk_setVietnamese:vi]; };
    [root addSubview:lang];
    y += 66 + 12;

    // Current app card
    VKBox *appCard = [[VKBox alloc] initWithFrame:NSMakeRect(pad, y, IW, 74) fill:VKTheme.groupBg radius:12];
    appCard.borderColor = VKDynamic(0xE3E5EA, 0x2E3238);
    [root addSubview:appCard];
    NSString *bundle = m.currentAppBundleId;
    VKIconTile *icon = [[VKIconTile alloc] initWithFrame:NSMakeRect(12, 19, 36, 36)];
    icon.symbolSize = 17;
    icon.fillColor = VKTheme.trackBg;
    icon.tintColor = VKTheme.text;
    NSImage *img = bundle ? [VKModel iconForBundle:bundle] : nil;
    if (img) {
        icon.image = img;
        icon.fillColor = [NSColor clearColor];
    } else {
        icon.symbolName = [bundle.lowercaseString containsString:@"safari"] ? @"globe" : @"app";
    }
    [appCard addSubview:icon];
    CGFloat textX = 58;
    if (bundle.length) {
        VKAppRule *rule = [m ruleForBundle:bundle];
        BOOL vi = m.vietnameseEnabled;
        NSString *desc;
        if (!m.smartPerAppMode) desc = @"Ghi nhớ theo ứng dụng đang tắt";
        else if (rule && rule.mode == VKAppModeVI) desc = @"Luôn dùng Tiếng Việt cho ứng dụng này";
        else if (rule && rule.mode == VKAppModeEN) desc = @"Luôn dùng Tiếng Anh cho ứng dụng này";
        else if (rule.lastVietnamese) desc = rule.lastVietnamese.boolValue ? @"Đã ghi nhớ: Tiếng Việt cho ứng dụng này"
                                                                         : @"Đã ghi nhớ: Tiếng Anh cho ứng dụng này";
        else desc = @"Chưa ghi nhớ — VisKey sẽ nhớ khi bạn đổi VI/EN";

        VKButton *action = [VKButton buttonWithTitle:(vi ? @"Luôn EN" : @"Luôn VI") symbol:nil style:VKButtonStyleLink];
        action.font = VKFont(14, NSFontWeightSemibold);
        CGFloat aw = [action fittingWidth];
        action.frame = NSMakeRect(IW - 14 - aw, 25, aw, 24);
        action.onClick = ^{ [weakSelf.model.actions vk_setRuleForCurrentApp:(vi ? VKAppModeEN : VKAppModeVI)]; };
        [appCard addSubview:action];

        CGFloat textW = action.frame.origin.x - 10 - textX;
        NSString *appName = m.currentAppName.length ? m.currentAppName : [VKModel displayNameForBundle:bundle fallback:nil];
        NSTextField *an = VKLabel(appName, VKFont(14.5, NSFontWeightSemibold), VKTheme.text);
        NSTextField *dl = VKWrapLabel(desc, VKFont(12.5, NSFontWeightRegular), VKTheme.textSecondary, textW);
        CGFloat total = an.frame.size.height + 1 + dl.frame.size.height;
        CGFloat ty = round((74 - total) / 2.0);
        an.frame = NSMakeRect(textX, ty, MIN(textW, an.frame.size.width), an.frame.size.height);
        VKSetOrigin(dl, textX, NSMaxY(an.frame) + 1);
        [appCard addSubview:an];
        [appCard addSubview:dl];
    } else {
        NSTextField *none = VKLabel(@"Chưa có ứng dụng nào đang mở", VKFont(13.5, NSFontWeightRegular), VKTheme.textSecondary);
        VKPlaceLabel(none, textX, 37);
        [appCard addSubview:none];
    }
    y += 74 + 14;

    // Kiểu gõ
    NSTextField *ml = VKLabel(@"Kiểu gõ", VKFont(14, NSFontWeightRegular), VKTheme.textSecondary);
    VKPlaceLabel(ml, pad + 4, y + 17);
    [root addSubview:ml];
    VKSegmented *method = [[VKSegmented alloc] initWithFrame:NSMakeRect(W - pad - 150, y, 150, 34) items:@[@"Telex", @"VNI"]];
    method.font = VKFont(14, NSFontWeightRegular);
    method.selectedFont = VKFont(14, NSFontWeightSemibold);
    method.selectedIndex = [m isTelex] ? 0 : 1;
    method.onSelect = ^(NSInteger idx) {
        VKModel *mm = weakSelf.model;
        mm.inputMethod = idx == 0 ? @"telex" : @"vni";
        [mm commit];
    };
    [root addSubview:method];
    y += 34 + 14;

    // Tiles
    CGFloat gap = 10, tw = floor((IW - gap) / 2.0), th = 98;
    VKPopTile *t1 = [self tileAt:NSMakeRect(pad, y, tw, th) symbol:@"arrow.counterclockwise" glyph:nil
                           title:@"Khôi phục tiếng Anh" subtitle:(m.autoRestoreEnglish ? @"Bật" : @"Tắt") on:m.autoRestoreEnglish];
    t1.onClick = ^{
        VKModel *mm = weakSelf.model;
        mm.autoRestoreEnglish = !mm.autoRestoreEnglish;
        [mm commit];
    };
    NSString *macroSub = [NSString stringWithFormat:@"%@ cụm từ", VKFormatCount((NSInteger)m.macros.count)];
    if (!m.enableMacros) macroSub = [@"Tắt · " stringByAppendingString:macroSub];
    VKPopTile *t2 = [self tileAt:NSMakeRect(pad + tw + gap, y, tw, th) symbol:@"bolt.fill" glyph:nil
                           title:@"Gõ tắt" subtitle:macroSub on:m.enableMacros];
    t2.onClick = ^{ [weakSelf.model.actions vk_openSettingsPage:VKSettingsPageMacros]; };
    y += th + gap;
    VKPopTile *t3 = [self tileAt:NSMakeRect(pad, y, tw, th) symbol:nil glyph:@"à"
                           title:@"Dấu kiểu mới" subtitle:(m.modernOrthography ? @"Bật · hoà" : @"Tắt · hòa") on:m.modernOrthography];
    t3.onClick = ^{
        VKModel *mm = weakSelf.model;
        mm.modernOrthography = !mm.modernOrthography;
        [mm commit];
    };
    VKPopTile *t4 = [self tileAt:NSMakeRect(pad + tw + gap, y, tw, th) symbol:@"terminal" glyph:nil
                           title:@"Theo ứng dụng" subtitle:[m perAppSummary] on:m.smartPerAppMode];
    t4.onClick = ^{ [weakSelf.model.actions vk_openSettingsPage:VKSettingsPageApps]; };
    y += th + 14;

    VKBox *divider = [[VKBox alloc] initWithFrame:NSMakeRect(pad + 4, y, IW - 8, 1) fill:VKTheme.separator radius:0];
    [root addSubview:divider];
    y += 9;

    VKMenuRow *prefs = [[VKMenuRow alloc] initWithFrame:NSMakeRect(pad, y, IW, 34)];
    prefs.title = @"Bảng điều khiển…";
    prefs.shortcut = @"⌘ ,";
    prefs.showsHandCursor = NO;
    prefs.onClick = ^{ [weakSelf.model.actions vk_openSettingsPage:VKSettingsPageGeneral]; };
    [root addSubview:prefs];
    y += 34;
    VKMenuRow *quit = [[VKMenuRow alloc] initWithFrame:NSMakeRect(pad, y, IW, 34)];
    quit.title = @"Thoát VisKey";
    quit.shortcut = @"⌘ Q";
    quit.showsHandCursor = NO;
    quit.onClick = ^{ [weakSelf.model.actions vk_quit]; };
    [root addSubview:quit];
    y += 34 + 10;

    root.frame = NSMakeRect(0, 0, W, y);
    self.preferredContentSize = NSMakeSize(W, y);
}

@end
