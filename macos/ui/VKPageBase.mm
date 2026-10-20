// VKPageBase.mm — VKPage base class. MIT License, 1VISION.

#import "VKPages.h"

const CGFloat kVKPagePad = 32.0;

@interface VKPage ()
@property (nonatomic, strong, readwrite) VKModel *model;
@property (nonatomic, readwrite) CGFloat pageWidth;
@property (nonatomic, readwrite) CGFloat viewportHeight;
@property (nonatomic, readwrite) BOOL snapshot;
@property (nonatomic, strong, readwrite) NSMutableArray<VKBox *> *searchRows;
@end

@implementation VKPage

- (instancetype)initWithModel:(VKModel *)model width:(CGFloat)width height:(CGFloat)viewportHeight snapshot:(BOOL)snapshot {
    self = [super initWithFrame:NSMakeRect(0, 0, width, viewportHeight)];
    if (self) {
        _model = model;
        _pageWidth = width;
        _viewportHeight = viewportHeight;
        _snapshot = snapshot;
        _searchRows = [NSMutableArray array];
        _pageTitle = @"";
        _keywords = @"";
        [self build];
        [self reload];
    }
    return self;
}

- (BOOL)isFlipped {
    return YES;
}

- (CGFloat)contentWidth {
    return self.pageWidth - 2.0 * kVKPagePad;
}

- (void)build {
}

- (void)reload {
}

- (void)environmentChanged {
}

- (void)pageWillAppear {
    [self reload];
}

- (void)pageDidDisappear {
}

- (void)setContentHeight:(CGFloat)height {
    NSRect f = self.frame;
    f.size.height = MAX(self.viewportHeight, ceil(height));
    self.frame = f;
}

- (BOOL)applySearch:(NSString *)query {
    NSString *q = [query stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
    NSStringCompareOptions opts = NSCaseInsensitiveSearch | NSDiacriticInsensitiveSearch;
    BOOL any = NO;
    for (VKBox *row in self.searchRows) {
        BOOL hit = q.length > 0 && row.searchText.length > 0 && [row.searchText rangeOfString:q options:opts].location != NSNotFound;
        row.searchHighlighted = hit;
        any = any || hit;
    }
    if (q.length == 0) return YES;
    if ([self.pageTitle rangeOfString:q options:opts].location != NSNotFound) any = YES;
    if ([self.keywords rangeOfString:q options:opts].location != NSNotFound) any = YES;
    return any;
}

- (CGFloat)addTitle:(NSString *)title subtitle:(NSAttributedString *)subtitle {
    self.pageTitle = title;
    NSTextField *t = VKLabel(title, VKFont(26, NSFontWeightBold), VKTheme.text);
    VKSetOrigin(t, kVKPagePad, 22);
    [self addSubview:t];
    if (subtitle) {
        NSTextField *s = VKAttrLabel(subtitle);
        VKSetOrigin(s, kVKPagePad, NSMaxY(t.frame) + 2);
        [self addSubview:s];
        return NSMaxY(s.frame) + 18;
    }
    return 76;
}

- (NSTextField *)addSectionLabel:(NSString *)text x:(CGFloat)x y:(CGFloat)y {
    NSTextField *l = VKAttrLabel(VKAttrKern(text, VKFont(12, NSFontWeightSemibold), VKTheme.textSecondary, 0.6));
    VKSetOrigin(l, x + 4, y);
    [self addSubview:l];
    return l;
}

- (VKBox *)addCard:(NSRect)frame {
    VKBox *card = [[VKBox alloc] initWithFrame:frame fill:VKTheme.groupBg radius:12];
    card.borderColor = VKDynamic(0xE6E8ED, 0x2A2E35);
    card.borderWidth = 1.0;
    [self addSubview:card];
    return card;
}

- (VKControlView *)addRowToCard:(VKBox *)card
                              y:(CGFloat)y
                         height:(CGFloat)height
                          title:(NSString *)title
                       subtitle:(NSString *)subtitle
                   monoSubtitle:(BOOL)mono
                         symbol:(NSString *)symbol
                      accessory:(NSView *)accessory
                           last:(BOOL)last {
    CGFloat w = card.bounds.size.width;
    VKControlView *row = [[VKControlView alloc] initWithFrame:NSMakeRect(1, y, w - 2, height)];
    row.showsHandCursor = NO;
    row.bottomSeparator = !last;
    row.searchText = title;
    row.cornerRadius = 0;
    [card addSubview:row];
    [self.searchRows addObject:row];

    CGFloat x = 20;
    if (symbol) {
        VKIconTile *tile = [[VKIconTile alloc] initWithFrame:NSMakeRect(20, round((height - 32) / 2.0), 32, 32)];
        tile.symbolName = symbol;
        tile.symbolSize = 15;
        [row addSubview:tile];
        x = 66;
    }
    CGFloat right = w - 20;
    if (accessory) {
        NSRect af = accessory.frame;
        af.origin.x = round(w - 22 - af.size.width);
        af.origin.y = round((height - af.size.height) / 2.0);
        accessory.frame = af;
        [row addSubview:accessory];
        right = af.origin.x - 14;
    }
    NSTextField *t = VKLabel(title, VKFont(14, NSFontWeightSemibold), VKTheme.text);
    NSTextField *s = subtitle ? VKLabel(subtitle, mono ? VKMono(12.5, NSFontWeightRegular) : VKFont(13, NSFontWeightRegular), VKTheme.textSecondary) : nil;
    CGFloat textW = MAX(40.0, right - x);
    CGFloat th = t.frame.size.height, sh = s ? s.frame.size.height : 0;
    CGFloat total = th + (s ? sh + 3 : 0);
    CGFloat ty = round((height - total) / 2.0);
    t.frame = NSMakeRect(x, ty, MIN(textW, t.frame.size.width), th);
    [row addSubview:t];
    if (s) {
        s.frame = NSMakeRect(x, ty + th + 3, MIN(textW, s.frame.size.width), sh);
        [row addSubview:s];
    }
    return row;
}

- (void)showAlert:(NSString *)message info:(NSString *)info {
    if (self.snapshot) return;
    NSAlert *alert = [[NSAlert alloc] init];
    alert.messageText = message;
    alert.informativeText = info ?: @"";
    [alert addButtonWithTitle:@"Đóng"];
    if (self.window) {
        [alert beginSheetModalForWindow:self.window completionHandler:nil];
    } else {
        [alert runModal];
    }
}

@end
