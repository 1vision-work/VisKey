// VKPageInfo.mm — "Quyền & Bảo mật" and "Giới thiệu" pages. MIT License, 1VISION.

#import "VKPages.h"

static NSString *const kVKRepoURL = @"https://github.com/1vision-work/VisKey";
static NSString *const kVKReleasesAPI = @"https://api.github.com/repos/1vision-work/VisKey/releases?per_page=5";

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
        @"VisKey chỉ kết nối mạng khi bạn bấm “Kiểm tra cập nhật” trong mục Giới thiệu, và chỉ để đọc thông tin phiên bản công khai trên GitHub.",
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

static NSArray<NSNumber *> *VKVersionParts(NSString *version) {
    NSString *v = [version stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
    if ([v hasPrefix:@"v"] || [v hasPrefix:@"V"]) v = [v substringFromIndex:1];
    NSRange cut = [v rangeOfCharacterFromSet:[NSCharacterSet characterSetWithCharactersInString:@"-+ "]];
    if (cut.location != NSNotFound) v = [v substringToIndex:cut.location];
    NSMutableArray<NSNumber *> *parts = [NSMutableArray array];
    for (NSString *p in [v componentsSeparatedByString:@"."]) [parts addObject:@(p.integerValue)];
    return parts;
}

static NSComparisonResult VKCompareVersions(NSString *a, NSString *b) {
    NSArray<NSNumber *> *pa = VKVersionParts(a), *pb = VKVersionParts(b);
    NSUInteger n = MAX(pa.count, pb.count);
    for (NSUInteger i = 0; i < n; i++) {
        NSInteger x = i < pa.count ? pa[i].integerValue : 0;
        NSInteger y = i < pb.count ? pb[i].integerValue : 0;
        if (x != y) return x < y ? NSOrderedAscending : NSOrderedDescending;
    }
    return NSOrderedSame;
}

@interface VKAboutPage ()
@property (nonatomic, strong) VKButton *checkButton;
@property (nonatomic, strong) NSTextField *statusLabel;
@property (nonatomic, strong) VKButton *releaseLink;
@property (nonatomic, copy, nullable) NSString *releaseURL;
@property (nonatomic) BOOL checking;
@end

@implementation VKAboutPage

- (void)build {
    __weak VKAboutPage *weakSelf = self;
    CGFloat cx = self.pageWidth / 2.0;
    self.keywords = @"giới thiệu phiên bản cập nhật giấy phép mã nguồn báo lỗi";
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

    self.releaseLink = [VKButton buttonWithTitle:@"Xem bản phát hành" symbol:nil style:VKButtonStyleLink];
    self.releaseLink.trailingSymbolName = @"arrow.up.right";
    CGFloat lw = [self.releaseLink fittingWidth];
    self.releaseLink.frame = NSMakeRect(cx - lw / 2.0, NSMaxY(self.statusLabel.frame) + 4, lw, 22);
    self.releaseLink.hidden = YES;
    self.releaseLink.onClick = ^{
        NSString *u = weakSelf.releaseURL;
        if (u.length) [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:u]];
    };
    [self addSubview:self.releaseLink];

    CGFloat cardW = 560;
    CGFloat y = NSMaxY(self.releaseLink.frame) + 14;
    NSArray *links = @[
        @[@"Mã nguồn", @"chevron.left.forwardslash.chevron.right", kVKRepoURL],
        @[@"Báo lỗi", @"ladybug", [kVKRepoURL stringByAppendingString:@"/issues"]],
        @[@"Bản phát hành & thay đổi", @"shippingbox", [kVKRepoURL stringByAppendingString:@"/releases"]],
        @[@"Giấy phép MIT", @"doc.text", [kVKRepoURL stringByAppendingString:@"/blob/master/LICENSE"]]
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
    NSTextField *note = VKLabel(@"VisKey không bao giờ tự kết nối mạng — chỉ kiểm tra khi bạn bấm nút.",
                                VKFont(12, NSFontWeightRegular), VKTheme.textTertiary);
    VKSetOrigin(note, cx - note.frame.size.width / 2.0, NSMaxY(card.frame) + 14);
    [self addSubview:note];
    [self setContentHeight:NSMaxY(note.frame) + 28];
}

- (void)setStatus:(NSString *)text color:(NSColor *)color {
    self.statusLabel.stringValue = text;
    self.statusLabel.textColor = color;
}

- (void)checkForUpdates {
    if (self.snapshot || self.checking) return;
    self.checking = YES;
    self.releaseLink.hidden = YES;
    self.checkButton.enabled = NO;
    [self setStatus:@"Đang kiểm tra…" color:VKTheme.textSecondary];

    NSURL *url = [NSURL URLWithString:kVKReleasesAPI];
    NSMutableURLRequest *req = [NSMutableURLRequest requestWithURL:url
                                                       cachePolicy:NSURLRequestReloadIgnoringLocalCacheData
                                                   timeoutInterval:8.0];
    [req setValue:[NSString stringWithFormat:@"VisKey/%@ (macOS)", [self.model versionString]] forHTTPHeaderField:@"User-Agent"];
    [req setValue:@"application/vnd.github+json" forHTTPHeaderField:@"Accept"];
    NSURLSessionConfiguration *cfg = [NSURLSessionConfiguration ephemeralSessionConfiguration];
    cfg.timeoutIntervalForRequest = 8.0;
    cfg.timeoutIntervalForResource = 8.0;
    NSURLSession *session = [NSURLSession sessionWithConfiguration:cfg];
    __weak VKAboutPage *weakSelf = self;
    NSURLSessionDataTask *task = [session dataTaskWithRequest:req completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
        dispatch_async(dispatch_get_main_queue(), ^{
            [weakSelf handleUpdateData:data response:response error:error];
        });
    }];
    [task resume];
    [session finishTasksAndInvalidate];
}

- (void)handleUpdateData:(NSData *)data response:(NSURLResponse *)response error:(NSError *)error {
    self.checking = NO;
    self.checkButton.enabled = YES;
    if (error || !data) {
        [self setStatus:@"Không kiểm tra được cập nhật. Hãy kiểm tra kết nối mạng rồi thử lại." color:VKTheme.error];
        return;
    }
    NSInteger code = [response isKindOfClass:[NSHTTPURLResponse class]] ? ((NSHTTPURLResponse *)response).statusCode : 0;
    if (code != 200) {
        [self setStatus:[NSString stringWithFormat:@"Máy chủ GitHub trả về lỗi %ld. Thử lại sau.", (long)code] color:VKTheme.error];
        return;
    }
    id obj = [NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
    if (![obj isKindOfClass:[NSArray class]]) {
        [self setStatus:@"Dữ liệu cập nhật không hợp lệ." color:VKTheme.error];
        return;
    }
    NSDictionary *latest = nil;
    for (id rel in (NSArray *)obj) {
        if (![rel isKindOfClass:[NSDictionary class]]) continue;
        if ([rel[@"draft"] respondsToSelector:@selector(boolValue)] && [rel[@"draft"] boolValue]) continue;
        if (![rel[@"tag_name"] isKindOfClass:[NSString class]]) continue;
        latest = rel;
        break;
    }
    if (!latest) {
        [self setStatus:@"Chưa có bản phát hành nào trên GitHub." color:VKTheme.textSecondary];
        return;
    }
    NSString *tag = latest[@"tag_name"];
    NSString *current = [self.model versionString];
    NSString *clean = [tag hasPrefix:@"v"] || [tag hasPrefix:@"V"] ? [tag substringFromIndex:1] : tag;
    if (VKCompareVersions(clean, current) == NSOrderedDescending) {
        [self setStatus:[NSString stringWithFormat:@"Có bản mới %@", clean] color:VKTheme.accentText];
        id html = latest[@"html_url"];
        self.releaseURL = [html isKindOfClass:[NSString class]] ? html : [kVKRepoURL stringByAppendingString:@"/releases"];
        self.releaseLink.hidden = NO;
    } else {
        [self setStatus:[NSString stringWithFormat:@"Đã là phiên bản mới nhất (%@).", current] color:VKTheme.accentText];
    }
}

@end
