// VKOnboarding.mm — see VKOnboarding.h. MIT License, 1VISION.

#import "VKOnboarding.h"
#import "VKUIKit.h"

static const CGFloat kVKOnbW = 880.0;
static const CGFloat kVKOnbH = 600.0;
static const CGFloat kVKOnbLeftW = 320.0;

#pragma mark - Left brand panel

@interface VKOnboardingSteps : NSView
@property (nonatomic) NSInteger step;
@end

@implementation VKOnboardingSteps

- (BOOL)isFlipped { return YES; }

- (void)setStep:(NSInteger)step { _step = step; self.needsDisplay = YES; }

- (void)drawRect:(NSRect)dirtyRect {
    NSArray<NSString *> *titles = @[@"Chọn kiểu gõ", @"Cấp quyền Trợ năng", @"Gõ thử & hoàn tất"];
    for (NSInteger i = 0; i < (NSInteger)titles.count; i++) {
        NSInteger n = i + 1;
        CGFloat y = i * 34.0;
        NSRect circle = NSMakeRect(0, y, 22, 22);
        BOOL done = n < self.step, current = n == self.step;
        if (done) {
            VKDrawCheckBadge(circle, VKHex(0x3FE0C0));
        } else if (current) {
            [[NSColor whiteColor] setFill];
            [[NSBezierPath bezierPathWithOvalInRect:circle] fill];
        } else {
            NSBezierPath *ring = [NSBezierPath bezierPathWithOvalInRect:NSInsetRect(circle, 0.75, 0.75)];
            ring.lineWidth = 1.5;
            [VKHexA(0xFFFFFF, 0.5) setStroke];
            [ring stroke];
        }
        if (!done) {
            NSString *num = [NSString stringWithFormat:@"%ld", (long)n];
            NSDictionary *na = @{NSFontAttributeName: VKFont(12, NSFontWeightBold),
                                 NSForegroundColorAttributeName: current ? VKHex(0x2534B8) : VKHexA(0xFFFFFF, 0.55)};
            NSSize ns = [num sizeWithAttributes:na];
            [num drawAtPoint:NSMakePoint(NSMidX(circle) - ns.width / 2.0, NSMidY(circle) - ns.height / 2.0) withAttributes:na];
        }
        NSColor *tc = current ? [NSColor whiteColor] : (done ? VKHexA(0xFFFFFF, 0.85) : VKHexA(0xFFFFFF, 0.5));
        NSDictionary *ta = @{NSFontAttributeName: VKFont(14.5, current ? NSFontWeightSemibold : NSFontWeightRegular),
                             NSForegroundColorAttributeName: tc};
        NSSize ts = [titles[(NSUInteger)i] sizeWithAttributes:ta];
        [titles[(NSUInteger)i] drawAtPoint:NSMakePoint(34, NSMidY(circle) - ts.height / 2.0) withAttributes:ta];
    }
}

@end

#pragma mark - Controller

@interface VKOnboardingWindowController ()
@property (nonatomic, strong) VKModel *model;
@property (nonatomic) BOOL snapshot;
@property (nonatomic, readwrite) NSInteger step;
@property (nonatomic, strong) VKOnboardingSteps *stepsView;
@property (nonatomic, strong) VKBox *rightPanel;
@property (nonatomic, strong, nullable) NSTimer *pollTimer;
@property (nonatomic) BOOL wasTrusted;
@end

@implementation VKOnboardingWindowController

- (instancetype)initWithModel:(VKModel *)model snapshot:(BOOL)snapshot {
    NSWindowStyleMask style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskFullSizeContentView;
    NSWindow *w = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, kVKOnbW, kVKOnbH)
                                              styleMask:style
                                                backing:NSBackingStoreBuffered
                                                  defer:NO];
    w.title = @"Chào mừng đến với VisKey";
    w.titleVisibility = NSWindowTitleHidden;
    w.titlebarAppearsTransparent = YES;
    w.releasedWhenClosed = NO;
    [w center];
    self = [super initWithWindow:w];
    if (self) {
        _model = model;
        _snapshot = snapshot;
        _step = 1;
        w.delegate = self;
        [self buildFrame];
        [self showStep:1];
    }
    return self;
}

- (void)dealloc {
    [_pollTimer invalidate];
}

- (void)buildFrame {
    VKBox *root = [[VKBox alloc] initWithFrame:NSMakeRect(0, 0, kVKOnbW, kVKOnbH) fill:VKDynamic(0xFFFFFF, 0x16181C) radius:0];
    self.window.contentView = root;

    VKBox *left = [[VKBox alloc] initWithFrame:NSMakeRect(0, 0, kVKOnbLeftW, kVKOnbH) fill:VKDynamic(0x2534B8, 0x1C278F) radius:0];
    [root addSubview:left];
    if (self.snapshot) {
        VKFakeTrafficLights *lights = [[VKFakeTrafficLights alloc] initWithFrame:NSMakeRect(28, 26, 52, 12)];
        lights.dimmed = YES;
        [left addSubview:lights];
    }
    VKMarkView *icon = [[VKMarkView alloc] initWithFrame:NSMakeRect(32, 120, 112, 112)];
    icon.appIcon = YES;
    [left addSubview:icon];
    NSTextField *head = VKWrapLabel(@"Gõ tiếng Việt đúng ngay lần đầu.", VKFont(29, NSFontWeightBold), [NSColor whiteColor], kVKOnbLeftW - 64);
    VKSetOrigin(head, 32, 258);
    [left addSubview:head];
    NSTextField *sub = VKWrapLabel(@"Không gạch chân, không mất chữ, không biến dạng từ tiếng Anh.",
                                   VKFont(14.5, NSFontWeightRegular), VKHexA(0xFFFFFF, 0.78), kVKOnbLeftW - 64);
    VKSetOrigin(sub, 32, NSMaxY(head.frame) + 14);
    [left addSubview:sub];
    self.stepsView = [[VKOnboardingSteps alloc] initWithFrame:NSMakeRect(32, kVKOnbH - 44 - 90, kVKOnbLeftW - 48, 90)];
    [left addSubview:self.stepsView];

    self.rightPanel = [[VKBox alloc] initWithFrame:NSMakeRect(kVKOnbLeftW, 0, kVKOnbW - kVKOnbLeftW, kVKOnbH) fill:nil radius:0];
    [root addSubview:self.rightPanel];
}

static NSAttributedString *VKRich(NSArray<NSArray *> *segments, CGFloat size, NSColor *color) {
    NSMutableAttributedString *s = [[NSMutableAttributedString alloc] init];
    for (NSArray *seg in segments) {
        BOOL bold = [seg[1] boolValue];
        [s appendAttributedString:VKAttr(seg[0], VKFont(size, bold ? NSFontWeightBold : NSFontWeightRegular), color)];
    }
    return s;
}

- (CGFloat)addHeaderStep:(NSInteger)n title:(NSString *)title subtitle:(NSString *)subtitle {
    CGFloat x = 48, w = self.rightPanel.frame.size.width - 96;
    NSTextField *label = VKAttrLabel(VKAttrKern([NSString stringWithFormat:@"BƯỚC %ld / 3", (long)n],
                                                VKFont(13, NSFontWeightSemibold), VKTheme.primary, 1.5));
    VKSetOrigin(label, x, 54);
    [self.rightPanel addSubview:label];
    NSTextField *t = VKWrapLabel(title, VKFont(26, NSFontWeightBold), VKTheme.text, w);
    VKSetOrigin(t, x, NSMaxY(label.frame) + 8);
    [self.rightPanel addSubview:t];
    NSTextField *s = VKWrapLabel(subtitle, VKFont(15, NSFontWeightRegular), VKTheme.textSecondary, w);
    VKSetOrigin(s, x, NSMaxY(t.frame) + 10);
    [self.rightPanel addSubview:s];
    return NSMaxY(s.frame) + 24;
}

- (VKButton *)addButton:(NSString *)title style:(VKButtonStyle)style trailing:(NSString *)trailing {
    VKButton *b = [VKButton buttonWithTitle:title symbol:nil style:style];
    b.trailingSymbolName = trailing;
    b.font = VKFont(14.5, style == VKButtonStylePrimary ? NSFontWeightSemibold : NSFontWeightMedium);
    NSRect f = b.frame;
    f.size = NSMakeSize(MAX(96.0, [b fittingWidth] + 8), 40);
    b.frame = f;
    [self.rightPanel addSubview:b];
    return b;
}

/// Lays out buttons right-aligned at the bottom (and an optional plain button on the left).
- (void)layoutButtons:(NSArray<VKButton *> *)right left:(VKButton *)leftButton {
    CGFloat W = self.rightPanel.frame.size.width, y = kVKOnbH - 44 - 40;
    CGFloat x = W - 48;
    for (VKButton *b in [right reverseObjectEnumerator]) {
        x -= b.frame.size.width;
        VKSetOrigin(b, x, y);
        x -= 12;
    }
    if (leftButton) VKSetOrigin(leftButton, 48, y);
}

- (void)showStep:(NSInteger)step {
    self.step = MAX((NSInteger)1, MIN((NSInteger)3, step));
    self.stepsView.step = self.step;
    for (NSView *v in [self.rightPanel.subviews copy]) [v removeFromSuperview];
    [self.pollTimer invalidate];
    self.pollTimer = nil;
    if (self.step == 1) [self buildStep1];
    else if (self.step == 2) [self buildStep2];
    else [self buildStep3];
}

- (void)finishLater {
    [self close];
}

- (void)buildStep1 {
    __weak VKOnboardingWindowController *weakSelf = self;
    CGFloat y = [self addHeaderStep:1 title:@"Chọn kiểu gõ bạn quen dùng"
                           subtitle:@"Bạn có thể đổi lại bất cứ lúc nào trong Bảng điều khiển."];
    CGFloat W = self.rightPanel.frame.size.width - 96, gap = 14, cw = floor((W - gap) / 2.0);
    VKSelectCard *telex = [[VKSelectCard alloc] initWithFrame:NSMakeRect(48, y, cw, 96)];
    telex.title = @"Telex";
    telex.subtitle = @"aa→â · w→ư · s f r x j";
    telex.monoSubtitle = YES;
    telex.selected = [self.model isTelex];
    VKSelectCard *vni = [[VKSelectCard alloc] initWithFrame:NSMakeRect(48 + cw + gap, y, cw, 96)];
    vni.title = @"VNI";
    vni.subtitle = @"a6→â · u7→ư · 1-5";
    vni.monoSubtitle = YES;
    vni.selected = ![self.model isTelex];
    __weak VKSelectCard *wt = telex, *wv = vni;
    telex.onClick = ^{
        weakSelf.model.inputMethod = @"telex";
        [weakSelf.model commit];
        wt.selected = YES;
        wv.selected = NO;
    };
    vni.onClick = ^{
        weakSelf.model.inputMethod = @"vni";
        [weakSelf.model commit];
        wt.selected = NO;
        wv.selected = YES;
    };
    [self.rightPanel addSubview:telex];
    [self.rightPanel addSubview:vni];

    NSTextField *hint = VKWrapLabel(@"Telex gõ dấu bằng chữ cái (s f r x j), VNI gõ dấu bằng số (1–5). Nếu chưa chắc, hãy chọn Telex.",
                                    VKFont(13.5, NSFontWeightRegular), VKTheme.textSecondary, W);
    VKSetOrigin(hint, 48, y + 96 + 18);
    [self.rightPanel addSubview:hint];

    VKButton *later = [self addButton:@"Để sau" style:VKButtonStylePlain trailing:nil];
    later.onClick = ^{ [weakSelf finishLater]; };
    VKButton *next = [self addButton:@"Tiếp tục" style:VKButtonStylePrimary trailing:nil];
    next.onClick = ^{ [weakSelf showStep:2]; };
    [self layoutButtons:@[next] left:later];
}

- (void)buildStep2 {
    __weak VKOnboardingWindowController *weakSelf = self;
    BOOL trusted = [self.model accessibilityTrusted];
    self.wasTrusted = trusted;
    CGFloat y = [self addHeaderStep:2 title:@"Cho phép VisKey nhận phím gõ"
                           subtitle:@"macOS yêu cầu quyền Trợ năng để bộ gõ chuyển “aa” thành “â” trong mọi ứng dụng."];
    CGFloat W = self.rightPanel.frame.size.width - 96;

    VKBox *card = [[VKBox alloc] initWithFrame:NSMakeRect(48, y, W, 56 * 3 + 2) fill:VKTheme.groupBg radius:12];
    card.borderColor = VKDynamic(0xE3E5EA, 0x2E3238);
    [self.rightPanel addSubview:card];
    NSArray *rows = @[
        @[@[@"Bấm ", @NO], @[@"Mở Cài đặt hệ thống", @YES], @[@" bên dưới", @NO]],
        @[@[@"Tìm ", @NO], @[@"VisKey", @YES], @[@" trong danh sách Trợ năng", @NO]],
        @[@[@"Bật công tắc", @NO]]
    ];
    for (NSUInteger i = 0; i < rows.count; i++) {
        VKBox *row = [[VKBox alloc] initWithFrame:NSMakeRect(1, 1 + 56.0 * i, W - 2, 56) fill:nil radius:0];
        row.bottomSeparator = (i + 1 < rows.count);
        [card addSubview:row];
        VKIconTile *num = [[VKIconTile alloc] initWithFrame:NSMakeRect(16, 14, 28, 28)];
        num.text = [NSString stringWithFormat:@"%lu", (unsigned long)(i + 1)];
        num.symbolSize = 12;
        num.fillColor = VKTheme.primaryTint;
        num.tintColor = VKTheme.primary;
        [row addSubview:num];
        NSTextField *t = VKAttrLabel(VKRich(rows[i], 15, VKTheme.text));
        VKPlaceLabel(t, 58, 28);
        [row addSubview:t];
        if (i == 2) {
            VKSwitch *sw = [VKSwitch switchWithOn:YES];
            VKSetOrigin(sw, W - 2 - 18 - sw.frame.size.width, 16);
            sw.enabled = NO;
            sw.alphaValue = 1.0;
            [row addSubview:sw];
        }
    }
    y = NSMaxY(card.frame) + 18;

    VKBox *banner = [[VKBox alloc] initWithFrame:NSMakeRect(48, y, W, 44)
                                            fill:(trusted ? VKTheme.accentTint : VKTheme.warningTint) radius:10];
    [self.rightPanel addSubview:banner];
    NSColor *bc = trusted ? VKTheme.accentText : VKTheme.warning;
    VKBox *dot = [[VKBox alloc] initWithFrame:NSMakeRect(16, 18, 8, 8) fill:bc radius:4];
    [banner addSubview:dot];
    NSTextField *bt = VKLabel(trusted ? @"Đã cấp quyền Trợ năng. VisKey đã sẵn sàng."
                                      : @"Đang chờ quyền... VisKey sẽ tự tiếp tục khi bạn bật xong.",
                              VKFont(14, NSFontWeightRegular), bc);
    VKPlaceLabel(bt, 34, 22);
    [banner addSubview:bt];
    y = NSMaxY(banner.frame) + 18;

    VKIconTile *shield = [[VKIconTile alloc] initWithFrame:NSMakeRect(46, y - 1, 22, 22)];
    shield.fillColor = [NSColor clearColor];
    shield.tintColor = VKTheme.accentText;
    shield.symbolName = @"checkmark.shield";
    shield.symbolSize = 15;
    [self.rightPanel addSubview:shield];
    NSTextField *privacy = VKWrapLabel(@"Mọi xử lý diễn ra trên máy bạn. VisKey không ghi lại hay gửi phím gõ ra ngoài, tự nhường quyền khi bạn nhập mật khẩu. Mã nguồn mở, giấy phép MIT.",
                                       VKFont(13.5, NSFontWeightRegular), VKTheme.textSecondary, W - 30);
    VKSetOrigin(privacy, 78, y);
    [self.rightPanel addSubview:privacy];

    VKButton *later = [self addButton:@"Để sau" style:VKButtonStylePlain trailing:nil];
    later.onClick = ^{ [weakSelf finishLater]; };
    VKButton *back = [self addButton:@"Quay lại" style:VKButtonStyleSecondary trailing:nil];
    back.onClick = ^{ [weakSelf showStep:1]; };
    VKButton *primary;
    if (trusted) {
        primary = [self addButton:@"Tiếp tục" style:VKButtonStylePrimary trailing:nil];
        primary.onClick = ^{ [weakSelf showStep:3]; };
    } else {
        primary = [self addButton:@"Mở Cài đặt hệ thống" style:VKButtonStylePrimary trailing:@"arrow.up.right"];
        primary.onClick = ^{ [weakSelf.model.actions vk_requestAccessibility]; };
    }
    [self layoutButtons:@[back, primary] left:later];

    if (!trusted && !self.snapshot) {
        self.pollTimer = [NSTimer scheduledTimerWithTimeInterval:1.0 repeats:YES block:^(NSTimer *timer) {
            VKOnboardingWindowController *s = weakSelf;
            if (!s) { [timer invalidate]; return; }
            [s pollPermission];
        }];
    }
}

- (void)pollPermission {
    if (![self.model accessibilityTrusted]) return;
    [self.pollTimer invalidate];
    self.pollTimer = nil;
    if (self.onPermissionGranted) self.onPermissionGranted();
    [self.model notifyEnvironmentChanged];
    if (self.step == 2) [self showStep:3];
}

- (void)buildStep3 {
    __weak VKOnboardingWindowController *weakSelf = self;
    BOOL telex = [self.model isTelex];
    NSString *sub = telex ? @"Gõ vài từ để kiểm tra, ví dụ “Vieejt Nam” → “Việt Nam”, “hoas” → “hóa”."
                          : @"Gõ vài từ để kiểm tra, ví dụ “Vie65t Nam” → “Việt Nam”, “hoa1” → “hóa”.";
    CGFloat y = [self addHeaderStep:3 title:@"Gõ thử & hoàn tất" subtitle:sub];
    CGFloat W = self.rightPanel.frame.size.width - 96;
    VKFieldBox *field = [[VKFieldBox alloc] initWithFrame:NSMakeRect(48, y, W, 48)
                                              placeholder:@"Gõ thử tại đây…" font:VKFont(16, NSFontWeightRegular) symbol:@"keyboard"];
    [self.rightPanel addSubview:field];
    NSString *hk = [self.model.primaryHotkey shortString];
    NSString *tip = hk.length ? [NSString stringWithFormat:@"Nhấn %@ để bật / tắt tiếng Việt. Biểu tượng VisKey trên thanh menu cho biết đang gõ VI hay EN.", hk]
                              : @"Biểu tượng VisKey trên thanh menu cho biết đang gõ VI hay EN.";
    NSTextField *note = VKWrapLabel(tip, VKFont(13.5, NSFontWeightRegular), VKTheme.textSecondary, W);
    VKSetOrigin(note, 48, NSMaxY(field.frame) + 14);
    [self.rightPanel addSubview:note];

    VKButton *back = [self addButton:@"Quay lại" style:VKButtonStyleSecondary trailing:nil];
    back.onClick = ^{ [weakSelf showStep:2]; };
    VKButton *start = [self addButton:@"Bắt đầu" style:VKButtonStylePrimary trailing:nil];
    start.onClick = ^{
        VKOnboardingWindowController *s = weakSelf;
        if (!s) return;
        s.model.onboardingDone = YES;
        [s.model commit];
        [s close];
    };
    [self layoutButtons:@[back, start] left:nil];
    if (!self.snapshot) [self.window makeFirstResponder:field.field];
}

- (void)windowWillClose:(NSNotification *)notification {
    [self.pollTimer invalidate];
    self.pollTimer = nil;
}

@end
