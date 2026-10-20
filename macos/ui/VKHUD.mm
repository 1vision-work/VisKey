// VKHUD.mm — see VKHUD.h. MIT License, 1VISION.

#import "VKHUD.h"
#import "VKUIKit.h"

@interface VKHUDController ()
@property (nonatomic, strong, nullable) NSPanel *panel;
@property (nonatomic) NSUInteger generation;
@end

@implementation VKHUDController

+ (NSView *)hudViewVietnamese:(BOOL)vietnamese hint:(NSString *)hint {
    NSString *title = vietnamese ? @"Tiếng Việt" : @"Tiếng Anh";
    NSFont *tf = VKFont(19, NSFontWeightSemibold);
    NSFont *hf = VKFont(14, NSFontWeightRegular);
    CGFloat textW = MAX(VKTextWidth(title, tf), VKTextWidth(hint, hf));
    CGFloat w = ceil(14 + 58 + 16 + textW + 26), h = 82;

    VKBox *box = [[VKBox alloc] initWithFrame:NSMakeRect(0, 0, w, h) fill:VKHexA(0x1B1E2B, 0.97) radius:20];
    box.borderColor = VKHexA(0xFFFFFF, 0.08);
    VKLangTile *tile = [[VKLangTile alloc] initWithFrame:NSMakeRect(12, 12, 58, 58)];
    tile.vietnamese = vietnamese;
    tile.forceDarkStyle = YES;
    [box addSubview:tile];
    NSTextField *t = VKLabel(title, tf, [NSColor whiteColor]);
    VKSetOrigin(t, 86, 15);
    [box addSubview:t];
    NSTextField *s = VKLabel(hint, hf, VKHexA(0xFFFFFF, 0.68));
    VKSetOrigin(s, 86, NSMaxY(t.frame) + 2);
    [box addSubview:s];
    return box;
}

- (NSPanel *)ensurePanel {
    if (self.panel) return self.panel;
    NSPanel *p = [[NSPanel alloc] initWithContentRect:NSMakeRect(0, 0, 280, 82)
                                            styleMask:(NSWindowStyleMaskBorderless | NSWindowStyleMaskNonactivatingPanel)
                                              backing:NSBackingStoreBuffered
                                                defer:YES];
    p.opaque = NO;
    p.backgroundColor = [NSColor clearColor];
    p.hasShadow = YES;
    p.level = NSStatusWindowLevel;
    p.ignoresMouseEvents = YES;
    p.hidesOnDeactivate = NO;
    p.floatingPanel = YES;
    p.becomesKeyOnlyIfNeeded = YES;
    p.releasedWhenClosed = NO;
    p.collectionBehavior = NSWindowCollectionBehaviorCanJoinAllSpaces | NSWindowCollectionBehaviorTransient |
                           NSWindowCollectionBehaviorFullScreenAuxiliary | NSWindowCollectionBehaviorIgnoresCycle;
    p.appearance = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
    self.panel = p;
    return p;
}

- (void)showVietnamese:(BOOL)vietnamese hint:(NSString *)hint {
    NSPanel *p = [self ensurePanel];
    NSView *content = [VKHUDController hudViewVietnamese:vietnamese hint:hint];
    NSSize size = content.frame.size;

    NSPoint mouse = [NSEvent mouseLocation];
    NSScreen *screen = [NSScreen mainScreen];
    for (NSScreen *s in [NSScreen screens]) {
        if (NSMouseInRect(mouse, s.frame, NO)) { screen = s; break; }
    }
    NSRect vis = screen ? screen.visibleFrame : NSMakeRect(0, 0, 1280, 800);
    NSRect frame = NSMakeRect(round(NSMidX(vis) - size.width / 2.0), round(NSMinY(vis) + 110), size.width, size.height);
    [p setFrame:frame display:NO];
    p.contentView = content;
    p.alphaValue = 1.0;
    [p orderFrontRegardless];

    self.generation += 1;
    NSUInteger gen = self.generation;
    __weak VKHUDController *weakSelf = self;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1.2 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        VKHUDController *s = weakSelf;
        if (!s || s.generation != gen) return;
        [NSAnimationContext runAnimationGroup:^(NSAnimationContext *context) {
            context.duration = 0.35;
            s.panel.animator.alphaValue = 0.0;
        } completionHandler:^{
            VKHUDController *s2 = weakSelf;
            if (s2 && s2.generation == gen) [s2.panel orderOut:nil];
        }];
    });
}

@end
