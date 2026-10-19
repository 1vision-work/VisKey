// VKPageInfo.mm — "Quyền & Bảo mật" and "Giới thiệu" pages. MIT License, 1VISION.

#import "VKPages.h"
#import "VKUpdate.h"

#pragma mark - Quyền & Bảo mật

@interface VKPrivacyPage ()
@property (nonatomic, strong) VKIconTile *axTile;
@property (nonatomic, strong) NSTextField *axSubtitle;
@property (nonatomic, strong) VKButton *axButton;
@property (nonatomic, strong) VKIconTile *secureTile;
@property (nonatomic, strong) NSTextField *secureSubtitle;
@end

@implementation VKPrivacyPage

- (VKControlView *)statusRowInCard:(VKBox *)card y:(CGFloat)y title:(NSString *)title symbol:(NSString *)symbol
                              tile:(VKIconTile *__strong *)tileOut subtitle:(NSTextField *__strong *)subOut
                        rightWidth:(CGFloat)rightWidth last:(BOOL)last {
    CGFloat W = card.bounds.size.width;
    VKControlView *row = [[VKControlView alloc] initWithFrame:NSMakeRect(1, y, W - 2, 76)];
    row.showsHandCursor = NO;
    row.bottomSeparator = !last;
    row.searchText = title;
    [card addSubview:row];
    [self.searchRows addObject:row];
    VKIconTile *tile = [[VKIconTile alloc] initWithFrame:NSMakeRect(20, 20, 36, 36)];
    tile.symbolName = symbol;
    tile.symbolSize = 17;
    [row addSubview:tile];
    NSTextField *t = VKLabel(title, VKFont(14.5, NSFontWeightSemibold), VKTheme.text);
    VKSetOrigin(t, 70, 18);
    [row addSubview:t];
    NSTextField *s = VKLabel(@"", VKFont(13, NSFontWeightRegular), VKTheme.textSecondary);
    s.frame = NSMakeRect(70, 40, W - 2 - 70 - rightWidth - 20, 18);
    [row addSubview:s];
    *tileOut = tile;
    *subOut = s;
    return row;
}

- (void)build {
    __weak VKPrivacyPage *weakSelf = self;
    CGFloat pad = kVKPagePad, W = self.contentWidth;
    self.keywords = @"quyền bảo mật trợ năng accessibility secure input riêng tư";
    CGFloat y = [self addTitle:@"Quyền & Bảo mật" subtitle:nil];

    VKBox *card = [self addCard:NSMakeRect(pad, y - 4, W, 76 * 2 + 2)];
    self.axButton = [VKButton buttonWithTitle:@"Mở Cài đặt hệ thống" symbol:nil style:VKButtonStylePrimary];
    self.axButton.trailingSymbolName = @"arrow.up.right";
    self.axButton.font = VKFont(13.5, NSFontWeightSemibold);
    CGFloat bw = [self.axButton fittingWidth];
    VKIconTile *tile = nil;
    NSTextField *sub = nil;
    VKControlView *axRow = [self statusRowInCard:card y:1 title:@"Quyền Trợ năng (Accessibility)" symbol:@"hand.raised"
                                            tile:&tile subtitle:&sub rightWidth:bw last:NO];
    self.axTile = tile;
    self.axSubtitle = sub;
    self.axButton.frame = NSMakeRect(axRow.frame.size.width - 22 - bw, 21, bw, 34);
    self.axButton.onClick = ^{ [weakSelf.model.actions vk_requestAccessibility]; };
    [axRow addSubview:self.axButton];

    [self statusRowInCard:card y:77 title:@"Secure Input (ô mật khẩu)" symbol:@"lock.shield"
                     tile:&tile subtitle:&sub rightWidth:0 last:YES];
    self.secureTile = tile;
    self.secureSubtitle = sub;

    y = NSMaxY(card.frame) + 24;
    [self addSectionLabel:@"QUYỀN RIÊNG TƯ" x:pad y:y];
    y += 24;
    NSArray<NSString *> *paras = @[
        @"Mọi xử lý diễn ra trên máy bạn. VisKey không ghi lại hay gửi phím gõ ra ngoài, tự nhường quyền khi bạn nhập mật khẩu. Mã nguồn mở, giấy phép MIT.",
        @"VisKey chỉ kết nối mạng để kiểm tra cập nhật — tự động tối đa 1 lần/ngày (tắt được trong mục Giới thiệu) hoặc khi bạn bấm “Kiểm tra cập nhật” — và chỉ đọc thông tin phiên bản công khai trên GitHub, không gửi kèm dữ liệu nào.",
        @"Nếu vừa cập nhật VisKey mà bộ gõ không hoạt động, hãy tắt rồi bật lại VisKey trong Cài đặt hệ thống → Quyền riêng tư & Bảo mật → Trợ năng."
    ];
    VKBox *pcard = [self addCard:NSMakeRect(pad, y, W, 100)];
    pcard.searchText = @"Quyền riêng tư không gửi phím gõ mạng";
    [self.searchRows addObject:pcard];
    VKIconTile *shield = [[VKIconTile alloc] initWithFrame:NSMakeRect(20, 18, 36, 36)];
    shield.symbolName = @"checkmark.shield";
    shield.symbolSize = 17;
    shield.fillColor = VKTheme.accentTint;
    shield.tintColor = VKTheme.accentText;
    [pcard addSubview:shield];
    CGFloat py = 18;
    for (NSUInteger i = 0; i < paras.count; i++) {
        NSTextField *l = VKWrapLabel(paras[i], VKFont(i == 0 ? 14 : 13, NSFontWeightRegular),
                                     i == 0 ? VKTheme.text : VKTheme.textSecondary, W - 70 - 24);
        VKSetOrigin(l, 70, py);
        [pcard addSubview:l];
        py = NSMaxY(l.frame) + 10;
    }
    NSRect pf = pcard.frame;
    pf.size.height = py + 8;
    pcard.frame = pf;
    [self setContentHeight:NSMaxY(pf) + 28];
}

- (void)reload {
    VKModel *m = self.model;
    BOOL trusted = [m accessibilityTrusted];
    self.axTile.fillColor = trusted ? VKTheme.accentTint : VKTheme.warningTint;
    self.axTile.tintColor = trusted ? VKTheme.accentText : VKTheme.warning;
    self.axTile.symbolName = trusted ? @"checkmark.shield" : @"hand.raised";
    self.axTile.needsDisplay = YES;
    self.axSubtitle.stringValue = trusted ? @"Đã cấp — VisKey đang nhận phím gõ bình thường."
                                          : @"Chưa cấp — VisKey chưa thể gõ tiếng Việt trong các ứng dụng.";
    self.axSubtitle.textColor = trusted ? VKTheme.accentText : VKTheme.warning;
    self.axButton.style = trusted ? VKButtonStyleSecondary : VKButtonStylePrimary;
    self.axButton.needsDisplay = YES;

    BOOL secure = [m secureInputActive];
    self.secureTile.fillColor = secure ? VKTheme.warningTint : VKTheme.iconTileBg;
    self.secureTile.tintColor = secure ? VKTheme.warning : VKTheme.primary;
    self.secureTile.needsDisplay = YES;
    self.secureSubtitle.stringValue = secure ? @"Đang tạm dừng vì một ứng dụng bật Secure Input"
                                             : @"Khi bạn nhập mật khẩu, macOS bật Secure Input và VisKey tự nhường quyền.";
    self.secureSubtitle.textColor = secure ? VKTheme.warning : VKTheme.textSecondary;
}

- (void)environmentChanged {
    [self reload];
}

@end

#pragma mark - Giới thiệu

@interface VKAboutPage ()
@property (nonatomic, strong) VKButton *checkButton;
@property (nonatomic, strong) NSTextField *statusLabel;
@property (nonatomic, strong) NSTextField *lastCheckLabel;
@property (nonatomic, strong) VKButton *releaseLink;
@property (nonatomic, strong) VKSwitch *autoSwitch;
@end

@implementation VKAboutPage

- (void)build {
    __weak VKAboutPage *weakSelf = self;
    CGFloat cx = self.pageWidth / 2.0;
    self.keywords = @"giới thiệu phiên bản cập nhật tự động giấy phép mã nguồn báo lỗi";
    self.pageTitle = @"Giới thiệu";

    VKMarkView *icon = [[VKMarkView alloc] initWithFrame:NSMakeRect(cx - 48, 44, 96, 96)];
    icon.appIcon = YES;
    [self addSubview:icon];

    NSTextField *name = VKLabel(@"VisKey", VKFont(28, NSFontWeightBold), VKTheme.text);
    VKSetOrigin(name, cx - name.frame.size.width / 2.0, 152);
    [self addSubview:name];
    NSTextField *tag = VKLabel(@"Bộ gõ tiếng Việt cho macOS, Linux & Windows · 1VISION", VKFont(14, NSFontWeightRegular), VKTheme.textSecondary);
    VKSetOrigin(tag, cx - tag.frame.size.width / 2.0, NSMaxY(name.frame) + 4);
    [self addSubview:tag];
    VKChip *ver = [VKChip chipWithText:[NSString stringWithFormat:@"Phiên bản %@", [self.model versionString]]
                                  font:VKFont(12.5, NSFontWeightSemibold) text:VKTheme.primary fill:VKTheme.primaryTint height:24];
    ver.hPadding = 12;
    [ver sizeToFitText];
    VKSetOrigin(ver, cx - ver.frame.size.width / 2.0, NSMaxY(tag.frame) + 12);
    [self addSubview:ver];

    self.checkButton = [VKButton buttonWithTitle:@"Kiểm tra cập nhật" symbol:@"arrow.triangle.2.circlepath" style:VKButtonStyleSecondary];
    CGFloat bw = [self.checkButton fittingWidth];
    self.checkButton.frame = NSMakeRect(cx - bw / 2.0, NSMaxY(ver.frame) + 18, bw, 34);
    self.checkButton.onClick = ^{ [weakSelf checkForUpdates]; };
    [self addSubview:self.checkButton];

    self.statusLabel = [NSTextField labelWithString:@""];
    self.statusLabel.font = VKFont(13, NSFontWeightRegular);
    self.statusLabel.textColor = VKTheme.textSecondary;
    self.statusLabel.alignment = NSTextAlignmentCenter;
    self.statusLabel.frame = NSMakeRect(cx - 280, NSMaxY(self.checkButton.frame) + 10, 560, 18);
    [self addSubview:self.statusLabel];

    self.lastCheckLabel = [NSTextField labelWithString:@""];
    self.lastCheckLabel.font = VKFont(12, NSFontWeightRegular);
    self.lastCheckLabel.textColor = VKTheme.textTertiary;
    self.lastCheckLabel.alignment = NSTextAlignmentCenter;
    self.lastCheckLabel.frame = NSMakeRect(cx - 280, NSMaxY(self.statusLabel.frame) + 2, 560, 16);
    [self addSubview:self.lastCheckLabel];

    self.releaseLink = [VKButton buttonWithTitle:@"Mở trang tải về" symbol:nil style:VKButtonStyleLink];
    self.releaseLink.trailingSymbolName = @"arrow.up.right";
    CGFloat lw = [self.releaseLink fittingWidth];
    self.releaseLink.frame = NSMakeRect(cx - lw / 2.0, NSMaxY(self.lastCheckLabel.frame) + 4, lw, 22);
    self.releaseLink.hidden = YES;
    self.releaseLink.onClick = ^{
        if (weakSelf.snapshot) return;
        [weakSelf.model.actions vk_openUpdateDownload];
    };
    [self addSubview:self.releaseLink];

    CGFloat cardW = 560;
    CGFloat y = NSMaxY(self.releaseLink.frame) + 14;
    VKBox *autoCard = [self addCard:NSMakeRect(cx - cardW / 2.0, y, cardW, 64)];
    self.autoSwitch = [VKSwitch switchWithOn:self.model.autoUpdateCheck];
    self.autoSwitch.onToggle = ^(BOOL on) {
        VKModel *mm = weakSelf.model;
        if (!mm) return;
        mm.autoUpdateCheck = on;
        [mm commit];
    };
    [self addRowToCard:autoCard y:1 height:62 title:@"Tự động kiểm tra cập nhật"
              subtitle:@"Tối đa 1 lần/ngày, chạy nền — chỉ báo nhẹ khi có bản mới"
          monoSubtitle:NO symbol:@"clock.arrow.circlepath" accessory:self.autoSwitch last:YES];

    y = NSMaxY(autoCard.frame) + 12;
    NSArray *links = @[
        @[@"Mã nguồn", @"chevron.left.forwardslash.chevron.right", VKRepoURL],
        @[@"Báo lỗi", @"ladybug", [VKRepoURL stringByAppendingString:@"/issues"]],
        @[@"Bản phát hành & thay đổi", @"shippingbox", [VKRepoURL stringByAppendingString:@"/releases"]],
        @[@"Giấy phép MIT", @"doc.text", [VKRepoURL stringByAppendingString:@"/blob/master/LICENSE"]]
    ];
    VKBox *card = [self addCard:NSMakeRect(cx - cardW / 2.0, y, cardW, 52.0 * links.count + 2)];
    for (NSUInteger i = 0; i < links.count; i++) {
        NSArray *l = links[i];
        VKIconTile *arrow = [[VKIconTile alloc] initWithFrame:NSMakeRect(0, 0, 20, 20)];
        arrow.fillColor = [NSColor clearColor];
        arrow.tintColor = VKTheme.textSecondary;
        arrow.symbolName = @"arrow.up.right";
        arrow.symbolSize = 12;
        VKControlView *row = [self addRowToCard:card y:1 + 52.0 * i height:52 title:l[0] subtitle:nil monoSubtitle:NO
                                         symbol:l[1] accessory:arrow last:(i + 1 == links.count)];
        row.showsHandCursor = YES;
        NSString *url = l[2];
        __weak VKControlView *weakRow = row;
        row.onHoverChanged = ^(BOOL hovering) { weakRow.fillColor = hovering ? VKTheme.hoverBg : nil; };
        row.onClick = ^{
            if (weakSelf.snapshot) return;
            [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:url]];
        };
    }
    NSTextField *note = VKLabel(@"Kiểm tra cập nhật chỉ đọc thông tin phiên bản công khai trên GitHub — không gửi dữ liệu nào.",
                                VKFont(12, NSFontWeightRegular), VKTheme.textTertiary);
    VKSetOrigin(note, cx - note.frame.size.width / 2.0, NSMaxY(card.frame) + 14);
    [self addSubview:note];
    [self setContentHeight:NSMaxY(note.frame) + 28];
}

- (void)checkForUpdates {
    if (self.snapshot || self.model.updateChecking) return;
    [self.model.actions vk_checkForUpdates];
}

- (void)reload {
    VKModel *m = self.model;
    self.autoSwitch.on = m.autoUpdateCheck;
    self.checkButton.enabled = !m.updateChecking;
    VKUpdateResult *r = m.lastUpdateResult;
    if (m.updateChecking) {
        self.statusLabel.stringValue = @"Đang kiểm tra…";
        self.statusLabel.textColor = VKTheme.textSecondary;
    } else if (r) {
        self.statusLabel.stringValue = [r message];
        self.statusLabel.textColor = [r isError] ? VKTheme.error
                                   : (r.status == VKUpdateStatusNoRelease ? VKTheme.textSecondary : VKTheme.accentText);
    } else {
        self.statusLabel.stringValue = @"";
    }
    self.lastCheckLabel.stringValue = [m lastUpdateCheckText] ?: @"";
    self.releaseLink.hidden = m.updateChecking || m.availableUpdateVersion.length == 0;
}

- (void)pageWillAppear {
    [super pageWillAppear];
    // Seeing the page acknowledges the "new version" notice (sidebar dot + popover line).
    VKModel *m = self.model;
    if (!self.snapshot && m.updateNoticePending) {
        m.updateNoticePending = NO;
        [m notifyUpdateStateChanged];
    }
}

@end

