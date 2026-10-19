// VKPageGeneral.mm — "Chung" page (design 01). MIT License, 1VISION.

#import "VKPages.h"

@interface VKGeneralPage ()
@property (nonatomic, strong) VKLangTile *langTile;
@property (nonatomic, strong) NSTextField *statusTitle;
@property (nonatomic, strong) NSTextField *statusSubtitle;
@property (nonatomic, strong) VKKeycapsView *hotkeyCaps;
@property (nonatomic, strong) VKSwitch *mainSwitch;
@property (nonatomic, strong) NSTextField *tryField;
@property (nonatomic, strong) VKChip *exampleChip;
@property (nonatomic, strong) VKSelectCard *telexCard;
@property (nonatomic, strong) VKSelectCard *vniCard;
@property (nonatomic, strong) VKSelectCard *classicCard;
@property (nonatomic, strong) VKSelectCard *modernCard;
@property (nonatomic, strong) VKSwitch *restoreSwitch;
@property (nonatomic, strong) VKSwitch *acFixSwitch;
@property (nonatomic, strong) VKSwitch *perAppSwitch;
@property (nonatomic, strong) VKSwitch *quickTelexSwitch;
@property (nonatomic, strong) VKSwitch *loginSwitch;
@property (nonatomic, strong) VKSwitch *hudSwitch;
@property (nonatomic) CGFloat capsRightEdge;
@property (nonatomic) CGFloat capsCenterY;
@end

@implementation VKGeneralPage

- (void)build {
    __weak VKGeneralPage *weakSelf = self;
    VKModel *m = self.model;
    CGFloat pad = kVKPagePad, W = self.contentWidth;
    self.keywords = @"kiểu gõ telex vni dấu khởi động hud gõ thử";

    CGFloat y = [self addTitle:@"Chung" subtitle:nil];
    VKSavedIndicator *saved = [VKSavedIndicator indicator];
    VKSetOrigin(saved, pad + W - saved.frame.size.width, 32);
    [self addSubview:saved];

    // Status card ---------------------------------------------------------
    VKBox *status = [self addCard:NSMakeRect(pad, y - 4, W, 160)];
    self.langTile = [[VKLangTile alloc] initWithFrame:NSMakeRect(22, 20, 58, 58)];
    [status addSubview:self.langTile];

    self.statusTitle = VKLabel(@"Đang gõ Tiếng Việt", VKFont(19, NSFontWeightSemibold), VKTheme.text);
    [status addSubview:self.statusTitle];
    self.statusSubtitle = VKLabel(@"", VKFont(13.5, NSFontWeightRegular), VKTheme.textSecondary);
    [status addSubview:self.statusSubtitle];

    self.mainSwitch = [VKSwitch largeSwitchWithOn:m.vietnameseEnabled];
    VKSetOrigin(self.mainSwitch, W - 22 - self.mainSwitch.frame.size.width, 34);
    self.mainSwitch.onToggle = ^(BOOL on) {
        [weakSelf.model.actions vk_setVietnamese:on];
    };
    [status addSubview:self.mainSwitch];

    self.hotkeyCaps = [[VKKeycapsView alloc] initWithKeys:@[] font:VKFont(13, NSFontWeightMedium) height:28];
    self.capsRightEdge = self.mainSwitch.frame.origin.x - 18;
    self.capsCenterY = NSMidY(self.mainSwitch.frame);
    [status addSubview:self.hotkeyCaps];

    VKBox *tryBox = [[VKBox alloc] initWithFrame:NSMakeRect(20, 98, W - 40, 44) fill:VKTheme.surface radius:10];
    tryBox.borderColor = VKTheme.separator;
    [status addSubview:tryBox];
    NSTextField *tryLabel = VKAttrLabel(VKAttrKern(@"GÕ THỬ", VKFont(12.5, NSFontWeightSemibold), VKTheme.primary, 1.0));
    VKPlaceLabel(tryLabel, 16, 22);
    [tryBox addSubview:tryLabel];
    self.exampleChip = [VKChip chipWithText:@"hoas → hóa" font:VKMono(12.5, NSFontWeightMedium)
                                       text:VKTheme.accentText fill:VKTheme.accentTint height:26];
    VKSetOrigin(self.exampleChip, tryBox.frame.size.width - 10 - self.exampleChip.frame.size.width, 9);
    [tryBox addSubview:self.exampleChip];
    self.tryField = VKPlainField(@"Tiếng Việt chuẩn, không mất chữ trên Chrome", VKFont(15, NSFontWeightRegular));
    CGFloat fx = NSMaxX(tryLabel.frame) + 16;
    self.tryField.frame = NSMakeRect(fx, 12, self.exampleChip.frame.origin.x - 12 - fx, 21);
    if (self.snapshot) self.tryField.stringValue = @"Tiếng Việt chuẩn, không mất chữ trên Chrome";
    [tryBox addSubview:self.tryField];
    status.searchText = @"Gõ thử";
    [self.searchRows addObject:status];

    // Kiểu gõ / Vị trí đặt dấu --------------------------------------------
    y = NSMaxY(status.frame) + 22;
    CGFloat gap = 12, cw = floor((W - 3 * gap) / 4.0);
    [self addSectionLabel:@"KIỂU GÕ" x:pad y:y];
    [self addSectionLabel:@"VỊ TRÍ ĐẶT DẤU" x:pad + 2 * (cw + gap) y:y];
    y += 24;
    CGFloat ch = 80;
    self.telexCard = [self selectCard:@"Telex" sub:@"aa→â · w→ư · s f r x j" mono:YES x:pad y:y w:cw h:ch];
    self.vniCard = [self selectCard:@"VNI" sub:@"a6→â · u7→ư · 1-5" mono:YES x:pad + (cw + gap) y:y w:cw h:ch];
    self.classicCard = [self selectCard:@"Kiểu cũ" sub:@"hòa · thúy · khỏe" mono:NO x:pad + 2 * (cw + gap) y:y w:cw h:ch];
    self.modernCard = [self selectCard:@"Kiểu mới" sub:@"hoà · thuý · khoẻ" mono:NO x:pad + 3 * (cw + gap) y:y w:cw h:ch];
    self.telexCard.onClick = ^{ [weakSelf setMethod:@"telex"]; };
    self.vniCard.onClick = ^{ [weakSelf setMethod:@"vni"]; };
    self.classicCard.onClick = ^{ [weakSelf setModern:NO]; };
    self.modernCard.onClick = ^{ [weakSelf setModern:YES]; };

    // Gõ thông minh -------------------------------------------------------
    y += ch + 24;
    [self addSectionLabel:@"GÕ THÔNG MINH" x:pad y:y];
    y += 24;
    CGFloat rh = 62;
    VKBox *smart = [self addCard:NSMakeRect(pad, y, W, rh * 4)];

    self.restoreSwitch = [self boundSwitch:^(VKModel *mm, BOOL on) { mm.autoRestoreEnglish = on; }];
    [self addRowToCard:smart y:0 height:rh title:@"Tự khôi phục từ tiếng Anh"
              subtitle:@"Gõ “window”, “software”, “book” không bị biến thành “ưindơ”, “sòtare”"
          monoSubtitle:NO symbol:@"arrow.counterclockwise" accessory:self.restoreSwitch last:NO];

    self.acFixSwitch = [self boundSwitch:^(VKModel *mm, BOOL on) { mm.fixAutocomplete = on; }];
    [self addRowToCard:smart y:rh height:rh title:@"Sửa lỗi mất chữ trên trình duyệt & Spotlight"
              subtitle:@"Chống “ttiếng” khi thanh địa chỉ tự gợi ý — Safari, Chrome, Arc, Excel"
          monoSubtitle:NO symbol:@"globe" accessory:self.acFixSwitch last:NO];

    self.perAppSwitch = [self boundSwitch:^(VKModel *mm, BOOL on) { mm.smartPerAppMode = on; }];
    VKButton *manage = [VKButton buttonWithTitle:@"Quản lý" symbol:nil style:VKButtonStyleLink];
    manage.font = VKFont(14, NSFontWeightMedium);
    NSRect mf = manage.frame;
    mf.size = NSMakeSize([manage fittingWidth], 24);
    manage.frame = mf;
    manage.onClick = ^{ [weakSelf.model.actions vk_openSettingsPage:VKSettingsPageApps]; };
    CGFloat accW = mf.size.width + 18 + self.perAppSwitch.frame.size.width;
    NSView *perAppAcc = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, accW, 24)];
    VKSetOrigin(manage, 0, 0);
    VKSetOrigin(self.perAppSwitch, accW - self.perAppSwitch.frame.size.width, 0);
    [perAppAcc addSubview:manage];
    [perAppAcc addSubview:self.perAppSwitch];
    [self addRowToCard:smart y:rh * 2 height:rh title:@"Ghi nhớ Anh/Việt theo từng ứng dụng"
              subtitle:@"Tự chuyển EN khi mở Terminal, iTerm2, Warp, Alacritty, Kitty"
          monoSubtitle:NO symbol:@"terminal" accessory:perAppAcc last:NO];

    self.quickTelexSwitch = [self boundSwitch:^(VKModel *mm, BOOL on) { mm.quickTelex = on; }];
    [self addRowToCard:smart y:rh * 3 height:rh title:@"Quick Telex"
              subtitle:@"cc=ch · gg=gi · kk=kh · nn=ng · qq=qu · pp=ph · tt=th"
          monoSubtitle:YES symbol:@"textformat" accessory:self.quickTelexSwitch last:YES];

    // Bottom split card ---------------------------------------------------
    y = NSMaxY(smart.frame) + 16;
    VKBox *bottom = [self addCard:NSMakeRect(pad, y, W, 54)];
    CGFloat half = floor(W / 2.0);
    self.loginSwitch = [VKSwitch switchWithOn:m.launchAtLogin];
    self.loginSwitch.onToggle = ^(BOOL on) { [weakSelf.model.actions vk_setLaunchAtLogin:on]; };
    VKControlView *left = [self addRowToCard:bottom y:0 height:54 title:@"Khởi động cùng macOS" subtitle:nil
                                monoSubtitle:NO symbol:nil accessory:nil last:YES];
    left.frame = NSMakeRect(1, 0, half - 1, 54);
    left.rightSeparator = YES;
    VKSetOrigin(self.loginSwitch, half - 1 - 22 - self.loginSwitch.frame.size.width, 15);
    [left addSubview:self.loginSwitch];

    self.hudSwitch = [self boundSwitch:^(VKModel *mm, BOOL on) { mm.showHUD = on; }];
    VKControlView *right = [self addRowToCard:bottom y:0 height:54 title:@"Hiện HUD khi chuyển VI/EN" subtitle:nil
                                 monoSubtitle:NO symbol:nil accessory:nil last:YES];
    right.frame = NSMakeRect(half, 0, W - half - 1, 54);
    VKSetOrigin(self.hudSwitch, right.frame.size.width - 22 - self.hudSwitch.frame.size.width, 15);
    [right addSubview:self.hudSwitch];

    [self setContentHeight:NSMaxY(bottom.frame) + 28];
}

- (VKSelectCard *)selectCard:(NSString *)title sub:(NSString *)sub mono:(BOOL)mono x:(CGFloat)x y:(CGFloat)y w:(CGFloat)w h:(CGFloat)h {
    VKSelectCard *c = [[VKSelectCard alloc] initWithFrame:NSMakeRect(x, y, w, h)];
    c.title = title;
    c.subtitle = sub;
    c.monoSubtitle = mono;
    c.searchText = title;
    [self addSubview:c];
    [self.searchRows addObject:c];
    return c;
}

- (VKSwitch *)boundSwitch:(void (^)(VKModel *mm, BOOL on))apply {
    VKSwitch *s = [VKSwitch switchWithOn:NO];
    __weak VKGeneralPage *weakSelf = self;
    s.onToggle = ^(BOOL on) {
        VKModel *mm = weakSelf.model;
        if (!mm) return;
        apply(mm, on);
        [mm commit];
    };
    return s;
}

- (void)setMethod:(NSString *)method {
    if ([self.model.inputMethod isEqualToString:method]) return;
    self.model.inputMethod = method;
    [self.model commit];
}

- (void)setModern:(BOOL)modern {
    if (self.model.modernOrthography == modern) return;
    self.model.modernOrthography = modern;
    [self.model commit];
}

- (void)reload {
    VKModel *m = self.model;
    BOOL vi = m.vietnameseEnabled;
    self.langTile.vietnamese = vi;
    self.statusTitle.stringValue = vi ? @"Đang gõ Tiếng Việt" : @"Đang gõ Tiếng Anh";
    VKSetOrigin(self.statusTitle, 100, 24);
    [self.statusTitle sizeToFit];
    self.statusSubtitle.stringValue = [m statusSubtitle];
    [self.statusSubtitle sizeToFit];
    VKSetOrigin(self.statusSubtitle, 100, 54);

    self.hotkeyCaps.keys = [m.primaryHotkey keycapLabels];
    [self.hotkeyCaps sizeToFitKeys];
    self.hotkeyCaps.hidden = m.primaryHotkey.isEmpty;
    VKSetOrigin(self.hotkeyCaps, self.capsRightEdge - self.hotkeyCaps.frame.size.width, self.capsCenterY - 14);
    self.mainSwitch.on = vi;

    self.exampleChip.text = [m isTelex] ? (m.modernOrthography ? @"hoas → hoá" : @"hoas → hóa")
                                        : (m.modernOrthography ? @"hoa1 → hoá" : @"hoa1 → hóa");
    [self.exampleChip sizeToFitText];
    VKSetOrigin(self.exampleChip, self.exampleChip.superview.frame.size.width - 10 - self.exampleChip.frame.size.width, 9);

    self.telexCard.selected = [m isTelex];
    self.vniCard.selected = ![m isTelex];
    self.classicCard.selected = !m.modernOrthography;
    self.modernCard.selected = m.modernOrthography;

    self.restoreSwitch.on = m.autoRestoreEnglish;
    self.acFixSwitch.on = m.fixAutocomplete;
    self.perAppSwitch.on = m.smartPerAppMode;
    self.quickTelexSwitch.on = m.quickTelex;
    self.loginSwitch.on = m.launchAtLogin;
    self.hudSwitch.on = m.showHUD;
}

@end
