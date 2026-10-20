// VKPageShortcuts.mm — "Phím tắt" page (design 06) and the hotkey recorder.
// MIT License, 1VISION.

#import "VKPages.h"

static const NSInteger kVKKeyEscape = 53;

#pragma mark - Hotkey recorder

static __weak VKHotkeyRecorder *gActiveRecorder = nil;

@interface VKHotkeyRecorder ()
@property (nonatomic, readwrite) BOOL recording;
@property (nonatomic, strong, nullable) id eventMonitor;
@end

@implementation VKHotkeyRecorder

+ (void)stopActiveRecorder {
    [gActiveRecorder stopRecording];
}

- (instancetype)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        _hotkey = [VKHotkey none];
    }
    return self;
}

- (void)dealloc {
    if (_eventMonitor) [NSEvent removeMonitor:_eventMonitor];
}

- (void)setHotkey:(VKHotkey *)hotkey {
    _hotkey = hotkey ?: [VKHotkey none];
    self.needsDisplay = YES;
}

- (void)setPreviewRecording:(BOOL)previewRecording {
    _previewRecording = previewRecording;
    self.needsDisplay = YES;
}

- (void)handleClickAtPoint:(NSPoint)point {
    if (self.recording) [self stopRecording];
    else [self startRecording];
    [super handleClickAtPoint:point];
}

- (void)startRecording {
    if (self.recording) return;
    if (gActiveRecorder && gActiveRecorder != self) [gActiveRecorder stopRecording];
    gActiveRecorder = self;
    self.recording = YES;
    self.needsDisplay = YES;
    VKModel *m = [VKModel shared];
    m.hotkeyRecording = YES;
    [m.actions vk_hotkeyRecordingChanged:YES];
    __weak VKHotkeyRecorder *weakSelf = self;
    self.eventMonitor = [NSEvent addLocalMonitorForEventsMatchingMask:(NSEventMaskKeyDown | NSEventMaskLeftMouseDown)
                                                              handler:^NSEvent *(NSEvent *event) {
        VKHotkeyRecorder *s = weakSelf;
        return s ? [s handleRecordingEvent:event] : event;
    }];
}

- (NSEvent *)handleRecordingEvent:(NSEvent *)event {
    if (!self.recording) return event;
    if (event.type == NSEventTypeLeftMouseDown) {
        NSPoint p = [self convertPoint:event.locationInWindow fromView:nil];
        if (event.window != self.window || !NSPointInRect(p, self.bounds)) [self stopRecording];
        return event;
    }
    NSUInteger mods = event.modifierFlags & [VKHotkey relevantModifierMask];
    if (event.keyCode == kVKKeyEscape && mods == 0) {
        [self stopRecording];
        return nil;
    }
    NSUInteger required = NSEventModifierFlagControl | NSEventModifierFlagOption | NSEventModifierFlagCommand;
    if ((mods & required) == 0) {
        NSBeep();
        return nil;
    }
    VKHotkey *hk = [VKHotkey hotkeyWithKeyCode:event.keyCode modifiers:mods];
    [self stopRecording];
    self.hotkey = hk;
    if (self.onChange) self.onChange(hk);
    return nil;
}

- (void)stopRecording {
    if (!self.recording) return;
    self.recording = NO;
    if (self.eventMonitor) {
        [NSEvent removeMonitor:self.eventMonitor];
        self.eventMonitor = nil;
    }
    if (gActiveRecorder == self) gActiveRecorder = nil;
    VKModel *m = [VKModel shared];
    m.hotkeyRecording = NO;
    [m.actions vk_hotkeyRecordingChanged:NO];
    self.needsDisplay = YES;
}

- (void)drawRect:(NSRect)dirtyRect {
    NSRect b = self.bounds;
    if (self.recording || self.previewRecording) {
        [VKDynamicA(0x4C60F5, 0.22, 0x8193FF, 0.3) setFill];
        [[NSBezierPath bezierPathWithRoundedRect:b xRadius:10 yRadius:10] fill];
        NSRect inner = NSInsetRect(b, 3, 3);
        NSBezierPath *p = [NSBezierPath bezierPathWithRoundedRect:inner xRadius:8 yRadius:8];
        [VKTheme.controlBg setFill];
        [p fill];
        p.lineWidth = 1.5;
        [VKTheme.primary setStroke];
        [p stroke];
        NSDictionary *a = @{NSFontAttributeName: VKFont(14, NSFontWeightMedium), NSForegroundColorAttributeName: VKTheme.primary};
        NSString *t = @"Nhấn tổ hợp phím...";
        NSSize ts = [t sizeWithAttributes:a];
        CGFloat total = 8 + 10 + ts.width;
        CGFloat x = round(NSMidX(b) - total / 2.0);
        [VKTheme.error setFill];
        [[NSBezierPath bezierPathWithOvalInRect:NSMakeRect(x, NSMidY(b) - 4, 8, 8)] fill];
        [t drawAtPoint:NSMakePoint(x + 18, round(NSMidY(b) - ts.height / 2.0)) withAttributes:a];
        return;
    }
    NSBezierPath *p = [NSBezierPath bezierPathWithRoundedRect:NSInsetRect(b, 3.5, 3.5) xRadius:8 yRadius:8];
    if (self.hotkey.isEmpty) {
        if (self.hovering) {
            [VKTheme.hoverBg setFill];
            [p fill];
        }
        CGFloat dash[2] = {4.0, 3.0};
        [p setLineDash:dash count:2 phase:0];
        p.lineWidth = 1.0;
        [VKTheme.controlBorder setStroke];
        [p stroke];
        NSDictionary *a = @{NSFontAttributeName: VKFont(14, NSFontWeightRegular), NSForegroundColorAttributeName: VKTheme.textSecondary};
        NSString *t = @"Ghi phím tắt";
        NSSize ts = [t sizeWithAttributes:a];
        [t drawAtPoint:NSMakePoint(round(NSMidX(b) - ts.width / 2.0), round(NSMidY(b) - ts.height / 2.0)) withAttributes:a];
        return;
    }
    [VKTheme.controlBg setFill];
    [p fill];
    p.lineWidth = 1.0;
    [(self.hovering ? VKTheme.primary : VKTheme.controlBorder) setStroke];
    [p stroke];

    NSFont *font = VKFont(14, NSFontWeightMedium);
    NSArray<NSString *> *keys = [self.hotkey keycapLabels];
    CGFloat capH = 24, gap = 6;
    NSMutableArray<NSNumber *> *widths = [NSMutableArray array];
    CGFloat total = 0;
    for (NSString *k in keys) {
        CGFloat w = MAX(capH, VKTextWidth(k, font) + 16);
        [widths addObject:@(w)];
        total += w + gap;
    }
    total -= gap;
    CGFloat x = round(NSMidX(b) - total / 2.0);
    for (NSUInteger i = 0; i < keys.count; i++) {
        CGFloat w = widths[i].doubleValue;
        NSRect r = NSMakeRect(x, round(NSMidY(b) - capH / 2.0), w, capH);
        [VKTheme.primaryTint setFill];
        [[NSBezierPath bezierPathWithRoundedRect:r xRadius:5 yRadius:5] fill];
        NSDictionary *a = @{NSFontAttributeName: font, NSForegroundColorAttributeName: VKTheme.primary};
        NSSize ts = [keys[i] sizeWithAttributes:a];
        [keys[i] drawAtPoint:NSMakePoint(round(NSMidX(r) - ts.width / 2.0), round(NSMidY(r) - ts.height / 2.0)) withAttributes:a];
        x += w + gap;
    }
}

@end

#pragma mark - Keyboard visual

@interface VKKeyboardVisual : NSView
@property (nonatomic, strong) VKHotkey *hotkey;
@end

@implementation VKKeyboardVisual

- (BOOL)isFlipped { return YES; }

- (void)setHotkey:(VKHotkey *)hotkey {
    _hotkey = hotkey;
    self.needsDisplay = YES;
}

- (void)drawRect:(NSRect)dirtyRect {
    VKHotkey *hk = self.hotkey ?: [VKHotkey none];
    NSUInteger mods = hk.isEmpty ? 0 : hk.modifiers;
    NSString *bigLabel = hk.isEmpty ? @"space" : ([hk.keyName isEqualToString:@"Space"] ? @"space" : hk.keyName);
    BOOL bigOn = !hk.isEmpty;

    NSMutableArray<NSArray *> *keys = [NSMutableArray array];  // label, width, highlighted, centered
    if (mods & NSEventModifierFlagShift) [keys addObject:@[@"⇧ shift", @78, @YES, @NO]];
    [keys addObject:@[@"fn", @56, @NO, @NO]];
    [keys addObject:@[@"⌃ control", @66, @((mods & NSEventModifierFlagControl) != 0), @NO]];
    [keys addObject:@[@"⌥", @56, @((mods & NSEventModifierFlagOption) != 0), @NO]];
    [keys addObject:@[@"⌘", @64, @((mods & NSEventModifierFlagCommand) != 0), @NO]];
    [keys addObject:@[bigLabel, @(mods & NSEventModifierFlagShift ? 190 : 256), @(bigOn), @YES]];
    [keys addObject:@[@"⌘", @64, @NO, @NO]];
    [keys addObject:@[@"⌥", @56, @NO, @NO]];

    CGFloat gap = 6, total = 0;
    for (NSArray *k in keys) total += [k[1] doubleValue] + gap;
    total -= gap;
    CGFloat x = round((self.bounds.size.width - total) / 2.0);
    CGFloat h = self.bounds.size.height;
    for (NSArray *k in keys) {
        CGFloat w = [k[1] doubleValue];
        BOOL on = [k[2] boolValue];
        BOOL centered = [k[3] boolValue];
        NSRect r = NSMakeRect(x, 0, w, h);
        VKDrawKeycap(r, @"", VKFont(12, NSFontWeightRegular), on);
        NSString *label = k[0];
        NSDictionary *a = @{NSFontAttributeName: centered ? VKFont(15, NSFontWeightMedium) : VKMono(12, NSFontWeightMedium),
                            NSForegroundColorAttributeName: on ? [NSColor whiteColor] : VKTheme.textSecondary};
        NSSize ts = [label sizeWithAttributes:a];
        NSPoint p = centered ? NSMakePoint(round(NSMidX(r) - ts.width / 2.0), round((h - 2) / 2.0 - ts.height / 2.0))
                             : NSMakePoint(x + 9, h - 8 - ts.height);
        [label drawAtPoint:p withAttributes:a];
        x += w + gap;
    }
}

@end

#pragma mark - Page

@interface VKShortcutsPage ()
@property (nonatomic, strong) VKKeyboardVisual *keyboard;
@property (nonatomic, strong) NSTextField *caption;
@property (nonatomic, strong) VKBox *langCard;
@property (nonatomic, strong) VKControlView *primaryRow;
@property (nonatomic, strong) VKBox *warningRow;
@property (nonatomic, strong) VKControlView *secondaryRow;
@property (nonatomic, strong) VKControlView *methodRow;
@property (nonatomic, strong) VKHotkeyRecorder *primaryRecorder;
@property (nonatomic, strong) VKHotkeyRecorder *secondaryRecorder;
@property (nonatomic, strong) VKHotkeyRecorder *methodRecorder;
@property (nonatomic, strong) VKButton *primaryClear;
@property (nonatomic, strong) VKButton *secondaryClear;
@property (nonatomic, strong) VKButton *methodClear;
@property (nonatomic, strong) NSTextField *otherLabel;
@property (nonatomic, strong) VKBox *otherCard;
@property (nonatomic) CGFloat langCardTop;
@end

@implementation VKShortcutsPage

- (NSView *)recorderAccessory:(VKHotkeyRecorder *__strong *)recorderOut clear:(VKButton *__strong *)clearOut slot:(NSInteger)slot {
    __weak VKShortcutsPage *weakSelf = self;
    NSView *acc = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 196 + 10 + 26, 44)];
    VKHotkeyRecorder *rec = [[VKHotkeyRecorder alloc] initWithFrame:NSMakeRect(0, 0, 196, 44)];
    rec.onChange = ^(VKHotkey *hk) { [weakSelf assignHotkey:hk slot:slot]; };
    [acc addSubview:rec];
    VKButton *clear = [VKButton buttonWithTitle:@"" symbol:@"xmark" style:VKButtonStylePlain];
    clear.font = VKFont(14, NSFontWeightMedium);
    clear.frame = NSMakeRect(196 + 10, 10, 26, 24);
    clear.toolTip = @"Xoá phím tắt";
    clear.onClick = ^{ [weakSelf assignHotkey:[VKHotkey none] slot:slot]; };
    [acc addSubview:clear];
    *recorderOut = rec;
    *clearOut = clear;
    return acc;
}

- (void)build {
    __weak VKShortcutsPage *weakSelf = self;
    CGFloat pad = kVKPagePad, W = self.contentWidth;
    self.keywords = @"phím tắt hotkey control space chuyển ngôn ngữ";

    CGFloat y = [self addTitle:@"Phím tắt" subtitle:nil];
    VKBox *kbCard = [self addCard:NSMakeRect(pad, y - 4, W, 150)];
    self.keyboard = [[VKKeyboardVisual alloc] initWithFrame:NSMakeRect(20, 22, W - 40, 50)];
    [kbCard addSubview:self.keyboard];
    self.caption = [NSTextField labelWithString:@""];
    self.caption.frame = NSMakeRect(20, 96, W - 40, 22);
    self.caption.alignment = NSTextAlignmentCenter;
    [kbCard addSubview:self.caption];

    y = NSMaxY(kbCard.frame) + 22;
    [self addSectionLabel:@"CHUYỂN NGÔN NGỮ" x:pad y:y];
    y += 24;
    self.langCardTop = y;
    self.langCard = [self addCard:NSMakeRect(pad, y, W, 200)];

    VKHotkeyRecorder *rec = nil;
    VKButton *clr = nil;
    NSView *acc = [self recorderAccessory:&rec clear:&clr slot:0];
    self.primaryRecorder = rec;
    self.primaryClear = clr;
    self.primaryRow = [self addRowToCard:self.langCard y:0 height:70 title:@"Bật / tắt Tiếng Việt" subtitle:@"Phím tắt chính"
                            monoSubtitle:NO symbol:nil accessory:acc last:NO];

    // Conflict warning row
    self.warningRow = [[VKBox alloc] initWithFrame:NSMakeRect(1, 70, W - 2, 72) fill:VKTheme.warningTint radius:0];
    self.warningRow.bottomSeparator = YES;
    [self.langCard addSubview:self.warningRow];
    VKIconTile *tri = [[VKIconTile alloc] initWithFrame:NSMakeRect(20, 16, 22, 22)];
    tri.fillColor = [NSColor clearColor];
    tri.tintColor = VKTheme.warning;
    tri.symbolName = @"exclamationmark.triangle";
    tri.symbolSize = 15;
    [self.warningRow addSubview:tri];
    VKButton *openKb = [VKButton buttonWithTitle:@"Mở Cài đặt bàn phím" symbol:nil style:VKButtonStyleLink];
    openKb.tintOverride = VKTheme.warning;
    openKb.font = VKFont(14, NSFontWeightSemibold);
    CGFloat linkW = [openKb fittingWidth];
    openKb.frame = NSMakeRect(W - 2 - 22 - linkW, 16, linkW, 22);
    openKb.onClick = ^{
        if (weakSelf.snapshot) return;
        [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"x-apple.systempreferences:com.apple.preference.keyboard?Shortcuts"]];
    };
    [self.warningRow addSubview:openKb];
    NSTextField *warnText = VKWrapLabel(@"Trùng với “Chọn nguồn nhập trước đó” của macOS. Tắt phím đó trong Cài đặt bàn phím để VisKey nhận phím ổn định.",
                                        VKFont(14, NSFontWeightRegular), VKTheme.warning, openKb.frame.origin.x - 24 - 52);
    VKSetOrigin(warnText, 52, 16);
    [self.warningRow addSubview:warnText];
    NSRect wf = self.warningRow.frame;
    wf.size.height = MAX(56.0, NSMaxY(warnText.frame) + 16);
    self.warningRow.frame = wf;
    self.warningRow.searchText = @"Trùng phím Chọn nguồn nhập trước đó";

    acc = [self recorderAccessory:&rec clear:&clr slot:1];
    self.secondaryRecorder = rec;
    self.secondaryClear = clr;
    self.secondaryRow = [self addRowToCard:self.langCard y:0 height:70 title:@"Phím tắt phụ"
                                  subtitle:@"Dùng khi phím chính bị ứng dụng khác chiếm"
                              monoSubtitle:NO symbol:nil accessory:acc last:NO];

    acc = [self recorderAccessory:&rec clear:&clr slot:2];
    self.methodRecorder = rec;
    self.methodClear = clr;
    self.methodRow = [self addRowToCard:self.langCard y:0 height:66 title:@"Đổi Telex ↔ VNI" subtitle:nil
                           monoSubtitle:NO symbol:nil accessory:acc last:YES];

    if (self.snapshot) self.secondaryRecorder.previewRecording = YES;

    // Khác
    self.otherLabel = [self addSectionLabel:@"KHÁC" x:pad y:0];
    self.otherCard = [self addCard:NSMakeRect(pad, 0, W, 64 + 72)];
    VKKeycapsView *prefsCaps = [[VKKeycapsView alloc] initWithKeys:@[@"⌘", @","] font:VKFont(13, NSFontWeightMedium) height:26];
    [self addRowToCard:self.otherCard y:0 height:64 title:@"Mở Bảng điều khiển" subtitle:nil
          monoSubtitle:NO symbol:nil accessory:prefsCaps last:NO];

    VKChip *soon = [VKChip chipWithText:@"Sắp có" font:VKFont(12, NSFontWeightSemibold)
                                   text:VKTheme.warning fill:VKTheme.warningTint height:22];
    VKKeycapsView *selCaps = [[VKKeycapsView alloc] initWithKeys:@[@"⌃", @"⌥", @"M"] font:VKFont(13, NSFontWeightMedium) height:26];
    NSView *selAcc = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, soon.frame.size.width + 14 + selCaps.frame.size.width, 26)];
    VKSetOrigin(soon, 0, 2);
    VKSetOrigin(selCaps, soon.frame.size.width + 14, 0);
    selCaps.alphaValue = 0.5;
    [selAcc addSubview:soon];
    [selAcc addSubview:selCaps];
    VKControlView *selRow = [self addRowToCard:self.otherCard y:64 height:72 title:@"Thêm gõ tắt từ chữ đang chọn"
                                      subtitle:@"Bôi đen một cụm từ rồi nhấn để lưu nhanh"
                                  monoSubtitle:NO symbol:nil accessory:selAcc last:YES];
    selRow.toolTip = @"Sắp có — tính năng này cần đọc chữ đang chọn trong ứng dụng khác, đang chờ quyết định.";
    for (NSView *v in selRow.subviews) {
        if ([v isKindOfClass:[NSTextField class]]) v.alphaValue = 0.55;
    }

    [self relayout];
}

- (void)relayout {
    CGFloat W = self.contentWidth, y = 0;
    self.primaryRow.frame = NSMakeRect(1, y, W - 2, 70);
    y += 70;
    BOOL conflict = [self.model hasHotkeyConflict];
    self.warningRow.hidden = !conflict;
    if (conflict) {
        NSRect wf = self.warningRow.frame;
        wf.origin.y = y;
        self.warningRow.frame = wf;
        y += wf.size.height;
    }
    self.secondaryRow.frame = NSMakeRect(1, y, W - 2, 70);
    y += 70;
    self.methodRow.frame = NSMakeRect(1, y, W - 2, 66);
    y += 66;
    NSRect cf = self.langCard.frame;
    cf.size.height = y + 1;
    self.langCard.frame = cf;

    CGFloat oy = NSMaxY(cf) + 24;
    VKSetOrigin(self.otherLabel, kVKPagePad + 4, oy);
    NSRect of = self.otherCard.frame;
    of.origin.y = oy + 24;
    self.otherCard.frame = of;
    [self setContentHeight:NSMaxY(of) + 28];
}

- (void)assignHotkey:(VKHotkey *)hk slot:(NSInteger)slot {
    VKModel *m = self.model;
    if (!hk.isEmpty) {
        if (slot != 0 && [m.primaryHotkey isEqualToHotkey:hk]) m.primaryHotkey = [VKHotkey none];
        if (slot != 1 && [m.secondaryHotkey isEqualToHotkey:hk]) m.secondaryHotkey = [VKHotkey none];
        if (slot != 2 && [m.methodHotkey isEqualToHotkey:hk]) m.methodHotkey = [VKHotkey none];
    }
    if (slot == 0) m.primaryHotkey = hk;
    else if (slot == 1) m.secondaryHotkey = hk;
    else m.methodHotkey = hk;
    [m commit];
}

- (void)reload {
    VKModel *m = self.model;
    if (!self.primaryRecorder.recording) self.primaryRecorder.hotkey = m.primaryHotkey;
    if (!self.secondaryRecorder.recording) self.secondaryRecorder.hotkey = m.secondaryHotkey;
    if (!self.methodRecorder.recording) self.methodRecorder.hotkey = m.methodHotkey;
    self.primaryClear.hidden = m.primaryHotkey.isEmpty;
    self.secondaryClear.hidden = m.secondaryHotkey.isEmpty && !self.secondaryRecorder.previewRecording;
    self.methodClear.hidden = m.methodHotkey.isEmpty;
    self.keyboard.hotkey = m.primaryHotkey;

    NSMutableAttributedString *cap = [[NSMutableAttributedString alloc] init];
    NSFont *f = VKFont(14.5, NSFontWeightRegular);
    if (m.primaryHotkey.isEmpty) {
        [cap appendAttributedString:VKAttr(@"Chưa đặt phím tắt chính — bấm “Ghi phím tắt” bên dưới.", f, VKTheme.textSecondary)];
    } else {
        [cap appendAttributedString:VKAttr(@"Nhấn ", f, VKTheme.textSecondary)];
        [cap appendAttributedString:VKAttr([m.primaryHotkey longString], VKFont(14.5, NSFontWeightSemibold), VKTheme.text)];
        [cap appendAttributedString:VKAttr(@" ở bất kỳ đâu để đổi giữa VI và EN", f, VKTheme.textSecondary)];
    }
    NSMutableParagraphStyle *ps = [[NSMutableParagraphStyle alloc] init];
    ps.alignment = NSTextAlignmentCenter;
    [cap addAttribute:NSParagraphStyleAttributeName value:ps range:NSMakeRange(0, cap.length)];
    self.caption.attributedStringValue = cap;
    [self relayout];
}

- (void)environmentChanged {
    BOOL conflict = [self.model hasHotkeyConflict];
    if (conflict == self.warningRow.hidden) [self relayout];
}

- (void)pageDidDisappear {
    [VKHotkeyRecorder stopActiveRecorder];
}

@end
