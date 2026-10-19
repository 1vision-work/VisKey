// VKUIKit.mm — see VKUIKit.h. MIT License, 1VISION.

#import "VKUIKit.h"

#pragma mark - Colors

NSColor *VKHexA(uint32_t rgb, CGFloat alpha) {
    CGFloat r = ((rgb >> 16) & 0xFF) / 255.0;
    CGFloat g = ((rgb >> 8) & 0xFF) / 255.0;
    CGFloat b = (rgb & 0xFF) / 255.0;
    return [NSColor colorWithSRGBRed:r green:g blue:b alpha:alpha];
}

NSColor *VKHex(uint32_t rgb) {
    return VKHexA(rgb, 1.0);
}

BOOL VKAppearanceIsDark(NSAppearance *appearance) {
    NSAppearance *a = appearance ?: [NSAppearance currentDrawingAppearance];
    if (!a) return NO;
    NSAppearanceName best = [a bestMatchFromAppearancesWithNames:@[NSAppearanceNameAqua, NSAppearanceNameDarkAqua]];
    return [best isEqualToString:NSAppearanceNameDarkAqua];
}

NSColor *VKDynamicA(uint32_t light, CGFloat lightAlpha, uint32_t dark, CGFloat darkAlpha) {
    NSColor *lc = VKHexA(light, lightAlpha);
    NSColor *dc = VKHexA(dark, darkAlpha);
    return [NSColor colorWithName:nil dynamicProvider:^NSColor *(NSAppearance *appearance) {
        return VKAppearanceIsDark(appearance) ? dc : lc;
    }];
}

NSColor *VKDynamic(uint32_t light, uint32_t dark) {
    return VKDynamicA(light, 1.0, dark, 1.0);
}

#define VK_COLOR(NAME, LIGHT, DARK) \
    + (NSColor *)NAME { \
        static NSColor *c = nil; \
        if (!c) c = VKDynamic(LIGHT, DARK); \
        return c; \
    }

@implementation VKTheme
VK_COLOR(windowBg,        0xF6F7F9, 0x16181C)
VK_COLOR(sidebarBg,       0xECEEF2, 0x1F2226)
VK_COLOR(groupBg,         0xFFFFFF, 0x1F2226)
VK_COLOR(surface,         0xF6F7F9, 0x16181C)
VK_COLOR(separator,       0xE3E5EA, 0x2E3238)
VK_COLOR(text,            0x16181C, 0xECEEF2)
VK_COLOR(textSecondary,   0x666D7A, 0xA3A9B5)
VK_COLOR(textTertiary,    0xA3A9B5, 0x666D7A)
VK_COLOR(primary,         0x3044D6, 0x8193FF)
VK_COLOR(primaryFill,     0x3044D6, 0x4C60F5)
VK_COLOR(primaryTint,     0xE6E9FF, 0x262D6B)
VK_COLOR(sidebarSelected, 0xDDE2FF, 0x262D6B)
VK_COLOR(accentText,      0x0B7F6D, 0x3FE0C0)
VK_COLOR(accentGraphic,   0x14B89A, 0x3FE0C0)
VK_COLOR(accentTint,      0xD5F8F0, 0x103A34)
VK_COLOR(jadeDot,         0x3FE0C0, 0x3FE0C0)
VK_COLOR(warning,         0xB45309, 0xF5A65B)
VK_COLOR(warningTint,     0xFFF5E9, 0x3A2A1C)
VK_COLOR(error,           0xC2302A, 0xFF8A80)
VK_COLOR(controlBg,       0xFFFFFF, 0x343841)
VK_COLOR(controlBorder,   0xC9CCD4, 0x4B515C)
VK_COLOR(toggleOff,       0xD3D6DD, 0x4B515C)
VK_COLOR(trackBg,         0xECEEF2, 0x2A2E35)
VK_COLOR(fieldBg,         0xFFFFFF, 0x2A2E35)
VK_COLOR(keycapBg,        0xFFFFFF, 0x343841)
VK_COLOR(hoverBg,         0xF3F4FA, 0x262A31)
VK_COLOR(darkSegmentFill, 0x343841, 0x5A606C)
VK_COLOR(iconTileBg,      0xEEF0FF, 0x262D6B)
@end

NSFont *VKFont(CGFloat size, NSFontWeight weight) {
    return [NSFont systemFontOfSize:size weight:weight];
}

NSFont *VKMono(CGFloat size, NSFontWeight weight) {
    return [NSFont monospacedSystemFontOfSize:size weight:weight];
}

static BOOL VKContextFlipped(void) {
    NSGraphicsContext *ctx = [NSGraphicsContext currentContext];
    return ctx ? ctx.isFlipped : NO;
}

/// Resolves a (possibly dynamic) color against the current drawing appearance.
static NSColor *VKResolved(NSColor *color) {
    CGColorRef cg = color.CGColor;
    NSColor *c = cg ? [NSColor colorWithCGColor:cg] : nil;
    return c ?: color;
}

#pragma mark - Brand drawing

void VKDrawMark(NSRect rect, NSColor *chevronColor, NSColor *vColor) {
    BOOL flipped = VKContextFlipped();
    CGFloat s = MIN(rect.size.width, rect.size.height) / 64.0;
    CGFloat ox = NSMidX(rect) - 32.0 * s;
    CGFloat oy = NSMidY(rect) - 32.0 * s;
    NSPoint (^P)(CGFloat, CGFloat) = ^NSPoint(CGFloat x, CGFloat y) {
        return NSMakePoint(ox + x * s, flipped ? (oy + y * s) : (oy + (64.0 - y) * s));
    };

    NSBezierPath *chev = [NSBezierPath bezierPath];
    [chev moveToPoint:P(22, 21)];
    [chev lineToPoint:P(32, 12)];
    [chev lineToPoint:P(42, 21)];
    chev.lineWidth = 6.0 * s;
    chev.lineCapStyle = NSLineCapStyleRound;
    chev.lineJoinStyle = NSLineJoinStyleRound;
    [chevronColor setStroke];
    [chev stroke];

    NSBezierPath *v = [NSBezierPath bezierPath];
    [v moveToPoint:P(15, 28)];
    [v lineToPoint:P(32, 53)];
    [v lineToPoint:P(49, 28)];
    v.lineWidth = 8.5 * s;
    v.lineCapStyle = NSLineCapStyleRound;
    v.lineJoinStyle = NSLineJoinStyleRound;
    [vColor setStroke];
    [v stroke];
}

void VKDrawAppIcon(NSRect rect) {
    BOOL flipped = VKContextFlipped();
    CGFloat side = MIN(rect.size.width, rect.size.height);
    CGFloat k = side / 80.4;
    NSRect body = NSMakeRect(NSMidX(rect) - side / 2.0, NSMidY(rect) - side / 2.0, side, side);

    NSBezierPath *bodyPath = [NSBezierPath bezierPathWithRoundedRect:body xRadius:18.0 * k yRadius:18.0 * k];
    NSGradient *grad = [[NSGradient alloc] initWithStartingColor:VKHex(0x5266F7) endingColor:VKHex(0x2433B5)];
    [grad drawInBezierPath:bodyPath angle:(flipped ? 90.0 : -90.0)];

    // Inner plate (white 7%): x 19..81, y 18..80 in the 100-unit icon box (body starts at 9.8).
    CGFloat innerX = (19.0 - 9.8) * k, innerTop = (18.0 - 9.8) * k, innerSide = 62.0 * k;
    NSRect inner = NSMakeRect(body.origin.x + innerX,
                              flipped ? body.origin.y + innerTop : NSMaxY(body) - innerTop - innerSide,
                              innerSide, innerSide);
    [VKHexA(0xFFFFFF, 0.07) setFill];
    [[NSBezierPath bezierPathWithRoundedRect:inner xRadius:13.0 * k yRadius:13.0 * k] fill];

    // Rim highlight.
    NSBezierPath *rim = [NSBezierPath bezierPathWithRoundedRect:NSInsetRect(body, 0.4 * k, 0.4 * k)
                                                        xRadius:17.6 * k yRadius:17.6 * k];
    rim.lineWidth = MAX(0.8 * k, 0.5);
    [VKHexA(0xFFFFFF, 0.18) setStroke];
    [rim stroke];

    // Mark: translate(19.6 19.1) scale(0.95) of the 64-unit symbol.
    CGFloat markSide = 64.0 * 0.95 * k;
    CGFloat markX = (19.6 - 9.8) * k, markTop = (19.1 - 9.8) * k;
    NSRect markRect = NSMakeRect(body.origin.x + markX,
                                 flipped ? body.origin.y + markTop : NSMaxY(body) - markTop - markSide,
                                 markSide, markSide);
    VKDrawMark(markRect, VKHex(0x3FE0C0), VKHex(0xFFFFFF));
}

NSImage *VKStatusMarkImage(CGFloat pointSize) {
    NSImage *img = [NSImage imageWithSize:NSMakeSize(pointSize, pointSize) flipped:NO drawingHandler:^BOOL(NSRect r) {
        CGFloat e = r.size.height * (64.0 / 44.0);
        NSRect markRect = NSMakeRect(NSMidX(r) - e / 2.0, NSMidY(r) - e / 2.0 - r.size.height * 0.02, e, e);
        VKDrawMark(markRect, [NSColor blackColor], [NSColor blackColor]);
        return YES;
    }];
    [img setTemplate:YES];
    return img;
}

NSImage *VKSymbol(NSString *name, CGFloat pointSize, NSFontWeight weight) {
    NSImage *img = [NSImage imageWithSystemSymbolName:name accessibilityDescription:nil];
    if (!img) return nil;
    NSImageSymbolConfiguration *cfg = [NSImageSymbolConfiguration configurationWithPointSize:pointSize weight:weight];
    NSImage *configured = [img imageWithSymbolConfiguration:cfg];
    return configured ?: img;
}

void VKDrawSymbol(NSString *name, CGFloat pointSize, NSFontWeight weight, NSColor *color, NSRect rect) {
    NSImage *img = VKSymbol(name, pointSize, weight);
    if (!img) return;
    NSSize sz = img.size;
    if (sz.width <= 0 || sz.height <= 0) return;
    CGFloat scale = MIN(1.0, MIN(rect.size.width / sz.width, rect.size.height / sz.height));
    NSSize dst = NSMakeSize(sz.width * scale, sz.height * scale);
    NSRect dstRect = NSMakeRect(NSMidX(rect) - dst.width / 2.0, NSMidY(rect) - dst.height / 2.0, dst.width, dst.height);
    NSColor *tint = VKResolved(color);
    NSImage *tinted = [NSImage imageWithSize:sz flipped:NO drawingHandler:^BOOL(NSRect b) {
        [img drawInRect:b];
        [tint set];
        NSRectFillUsingOperation(b, NSCompositingOperationSourceAtop);
        return YES;
    }];
    [tinted drawInRect:dstRect fromRect:NSZeroRect operation:NSCompositingOperationSourceOver fraction:1.0 respectFlipped:YES hints:nil];
}

void VKDrawKeycap(NSRect rect, NSString *label, NSFont *font, BOOL highlighted) {
    BOOL flipped = VKContextFlipped();
    CGFloat edge = 2.0;
    NSRect face = rect;
    face.size.height -= edge;
    NSRect base = face;
    base.origin.y += flipped ? edge : -edge;
    if (!flipped) face.origin.y += edge;
    CGFloat radius = MIN(6.0, face.size.height / 4.0);

    NSColor *edgeColor = highlighted ? VKHex(0x2534B8) : VKDynamic(0xD3D6DD, 0x0E1013);
    [edgeColor setFill];
    [[NSBezierPath bezierPathWithRoundedRect:base xRadius:radius yRadius:radius] fill];

    NSBezierPath *facePath = [NSBezierPath bezierPathWithRoundedRect:face xRadius:radius yRadius:radius];
    [(highlighted ? VKTheme.primaryFill : VKTheme.keycapBg) setFill];
    [facePath fill];
    if (!highlighted) {
        facePath.lineWidth = 0.75;
        [VKDynamic(0xD8DBE2, 0x4B515C) setStroke];
        [facePath stroke];
    }

    NSDictionary *attrs = @{NSFontAttributeName: font,
                            NSForegroundColorAttributeName: highlighted ? [NSColor whiteColor] : VKTheme.text};
    NSSize ts = [label sizeWithAttributes:attrs];
    [label drawAtPoint:NSMakePoint(NSMidX(face) - ts.width / 2.0, NSMidY(face) - ts.height / 2.0) withAttributes:attrs];
}

void VKDrawCheckBadge(NSRect rect, NSColor *fill) {
    BOOL flipped = VKContextFlipped();
    [fill setFill];
    [[NSBezierPath bezierPathWithOvalInRect:rect] fill];
    CGFloat w = rect.size.width, h = rect.size.height;
    NSPoint (^P)(CGFloat, CGFloat) = ^NSPoint(CGFloat fx, CGFloat fy) {
        return NSMakePoint(rect.origin.x + fx * w, flipped ? rect.origin.y + fy * h : NSMaxY(rect) - fy * h);
    };
    NSBezierPath *check = [NSBezierPath bezierPath];
    [check moveToPoint:P(0.29, 0.52)];
    [check lineToPoint:P(0.44, 0.67)];
    [check lineToPoint:P(0.72, 0.36)];
    check.lineWidth = MAX(1.5, w * 0.1);
    check.lineCapStyle = NSLineCapStyleRound;
    check.lineJoinStyle = NSLineJoinStyleRound;
    [[NSColor whiteColor] setStroke];
    [check stroke];
}

#pragma mark - Labels

NSAttributedString *VKAttr(NSString *text, NSFont *font, NSColor *color) {
    return [[NSAttributedString alloc] initWithString:text ?: @""
                                           attributes:@{NSFontAttributeName: font, NSForegroundColorAttributeName: color}];
}

NSAttributedString *VKAttrKern(NSString *text, NSFont *font, NSColor *color, CGFloat kern) {
    return [[NSAttributedString alloc] initWithString:text ?: @""
                                           attributes:@{NSFontAttributeName: font,
                                                        NSForegroundColorAttributeName: color,
                                                        NSKernAttributeName: @(kern)}];
}

NSTextField *VKLabel(NSString *text, NSFont *font, NSColor *color) {
    NSTextField *l = [NSTextField labelWithString:text ?: @""];
    l.font = font;
    l.textColor = color;
    l.lineBreakMode = NSLineBreakByTruncatingTail;
    [l sizeToFit];
    return l;
}

NSTextField *VKAttrLabel(NSAttributedString *text) {
    NSTextField *l = [NSTextField labelWithAttributedString:text];
    l.lineBreakMode = NSLineBreakByTruncatingTail;
    [l sizeToFit];
    return l;
}

static void VKFitWrapLabel(NSTextField *l, CGFloat width) {
    l.preferredMaxLayoutWidth = width;
    NSSize s = [l.cell cellSizeForBounds:NSMakeRect(0, 0, width, CGFLOAT_MAX)];
    l.frame = NSMakeRect(0, 0, width, ceil(s.height));
}

NSTextField *VKWrapLabel(NSString *text, NSFont *font, NSColor *color, CGFloat width) {
    NSTextField *l = [NSTextField wrappingLabelWithString:text ?: @""];
    l.selectable = NO;
    l.font = font;
    l.textColor = color;
    VKFitWrapLabel(l, width);
    return l;
}

NSTextField *VKWrapAttrLabel(NSAttributedString *text, CGFloat width) {
    NSTextField *l = [NSTextField wrappingLabelWithString:@""];
    l.selectable = NO;
    l.attributedStringValue = text;
    VKFitWrapLabel(l, width);
    return l;
}

CGFloat VKTextWidth(NSString *text, NSFont *font) {
    return ceil([text ?: @"" sizeWithAttributes:@{NSFontAttributeName: font}].width);
}

void VKSetOrigin(NSView *view, CGFloat x, CGFloat y) {
    [view setFrameOrigin:NSMakePoint(round(x), round(y))];
}

void VKPlaceLabel(NSTextField *label, CGFloat x, CGFloat centerY) {
    [label sizeToFit];
    NSRect f = label.frame;
    [label setFrameOrigin:NSMakePoint(round(x), round(centerY - f.size.height / 2.0))];
}

NSTextField *VKPlainField(NSString *placeholder, NSFont *font) {
    NSTextField *f = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 100, 20)];
    f.bezeled = NO;
    f.bordered = NO;
    f.drawsBackground = NO;
    f.editable = YES;
    f.selectable = YES;
    f.font = font;
    f.textColor = VKTheme.text;
    f.placeholderString = placeholder;
    f.focusRingType = NSFocusRingTypeNone;
    f.lineBreakMode = NSLineBreakByClipping;
    NSCell *cell = f.cell;
    cell.usesSingleLineMode = YES;
    cell.scrollable = YES;
    cell.wraps = NO;
    return f;
}

NSString *VKFormatCount(NSInteger value) {
    static NSNumberFormatter *fmt = nil;
    if (!fmt) {
        fmt = [[NSNumberFormatter alloc] init];
        fmt.locale = [NSLocale localeWithLocaleIdentifier:@"vi_VN"];
        fmt.numberStyle = NSNumberFormatterDecimalStyle;
        fmt.usesGroupingSeparator = YES;
        fmt.groupingSeparator = @".";
        fmt.groupingSize = 3;
        fmt.maximumFractionDigits = 0;
    }
    return [fmt stringFromNumber:@(value)] ?: [NSString stringWithFormat:@"%ld", (long)value];
}

#pragma mark - VKBox

@implementation VKBox

- (instancetype)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        _borderWidth = 1.0;
    }
    return self;
}

- (instancetype)initWithFrame:(NSRect)frame fill:(NSColor *)fill radius:(CGFloat)radius {
    self = [self initWithFrame:frame];
    if (self) {
        _fillColor = fill;
        _cornerRadius = radius;
    }
    return self;
}

- (BOOL)isFlipped {
    return YES;
}

- (void)setFillColor:(NSColor *)fillColor { _fillColor = fillColor; self.needsDisplay = YES; }
- (void)setBorderColor:(NSColor *)borderColor { _borderColor = borderColor; self.needsDisplay = YES; }
- (void)setSearchHighlighted:(BOOL)searchHighlighted { _searchHighlighted = searchHighlighted; self.needsDisplay = YES; }

- (CGFloat)hairline {
    CGFloat scale = self.window ? self.window.backingScaleFactor : 2.0;
    return scale >= 2.0 ? 0.5 : 1.0;
}

- (void)drawRect:(NSRect)dirtyRect {
    NSRect b = self.bounds;
    CGFloat bw = self.borderColor ? MAX(self.borderWidth, 0.0) : 0.0;
    NSRect r = bw > 0 ? NSInsetRect(b, bw / 2.0, bw / 2.0) : b;
    CGFloat radius = MAX(0.0, self.cornerRadius - bw / 2.0);
    NSBezierPath *path = radius > 0 ? [NSBezierPath bezierPathWithRoundedRect:r xRadius:radius yRadius:radius]
                                    : [NSBezierPath bezierPathWithRect:r];
    if (self.fillColor) {
        [self.fillColor setFill];
        [path fill];
    }
    if (self.searchHighlighted) {
        [VKDynamicA(0x4C60F5, 0.13, 0x8193FF, 0.18) setFill];
        [path fill];
    }
    if (bw > 0) {
        path.lineWidth = bw;
        if (self.dashedBorder) {
            CGFloat dash[2] = {4.0, 3.0};
            [path setLineDash:dash count:2 phase:0];
        }
        [self.borderColor setStroke];
        [path stroke];
    }
    CGFloat h = [self hairline];
    if (self.topSeparator || self.bottomSeparator || self.rightSeparator) {
        [VKTheme.separator setFill];
        if (self.topSeparator) NSRectFill(NSMakeRect(self.separatorInset, 0, b.size.width - self.separatorInset, h));
        if (self.bottomSeparator) NSRectFill(NSMakeRect(self.separatorInset, b.size.height - h, b.size.width - self.separatorInset, h));
        if (self.rightSeparator) NSRectFill(NSMakeRect(b.size.width - h, 0, h, b.size.height));
    }
    if (self.drawHandler) self.drawHandler(b);
}

@end

#pragma mark - VKControlView

@interface VKControlView ()
@property (nonatomic, readwrite) BOOL hovering;
@property (nonatomic, readwrite) BOOL pressed;
@property (nonatomic) NSPoint mouseDownPoint;
@property (nonatomic, strong, nullable) NSTrackingArea *trackingArea;
@end

@implementation VKControlView

- (instancetype)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        _enabled = YES;
        _showsHandCursor = YES;
    }
    return self;
}

- (void)setEnabled:(BOOL)enabled {
    _enabled = enabled;
    self.alphaValue = enabled ? 1.0 : 0.45;
    [self.window invalidateCursorRectsForView:self];
    self.needsDisplay = YES;
}

- (void)updateTrackingAreas {
    [super updateTrackingAreas];
    if (self.trackingArea) [self removeTrackingArea:self.trackingArea];
    self.trackingArea = [[NSTrackingArea alloc] initWithRect:NSZeroRect
                                                     options:(NSTrackingMouseEnteredAndExited | NSTrackingActiveAlways | NSTrackingInVisibleRect)
                                                       owner:self
                                                    userInfo:nil];
    [self addTrackingArea:self.trackingArea];
}

- (void)mouseEntered:(NSEvent *)event {
    self.hovering = YES;
    self.needsDisplay = YES;
    if (self.onHoverChanged) self.onHoverChanged(YES);
}

- (void)mouseExited:(NSEvent *)event {
    self.hovering = NO;
    self.needsDisplay = YES;
    if (self.onHoverChanged) self.onHoverChanged(NO);
}

- (BOOL)acceptsFirstMouse:(NSEvent *)event {
    return YES;
}

- (BOOL)mouseDownCanMoveWindow {
    return NO;
}

- (void)mouseDown:(NSEvent *)event {
    if (!self.enabled) return;
    self.mouseDownPoint = [self convertPoint:event.locationInWindow fromView:nil];
    self.pressed = YES;
    self.needsDisplay = YES;
}

- (void)mouseUp:(NSEvent *)event {
    if (!self.pressed) return;
    self.pressed = NO;
    self.needsDisplay = YES;
    NSPoint p = [self convertPoint:event.locationInWindow fromView:nil];
    if (self.enabled && NSPointInRect(p, self.bounds)) {
        [self handleClickAtPoint:self.mouseDownPoint];
    }
}

- (void)handleClickAtPoint:(NSPoint)point {
    if (self.onClick) self.onClick();
}

- (void)resetCursorRects {
    if (self.showsHandCursor && self.enabled) {
        [self addCursorRect:self.bounds cursor:[NSCursor pointingHandCursor]];
    }
}

@end

#pragma mark - VKSwitch

@implementation VKSwitch

+ (instancetype)switchWithOn:(BOOL)on {
    VKSwitch *s = [[VKSwitch alloc] initWithFrame:NSMakeRect(0, 0, 40, 24)];
    s.on = on;
    return s;
}

+ (instancetype)largeSwitchWithOn:(BOOL)on {
    VKSwitch *s = [[VKSwitch alloc] initWithFrame:NSMakeRect(0, 0, 50, 30)];
    s.on = on;
    return s;
}

- (void)setOn:(BOOL)on {
    _on = on;
    self.needsDisplay = YES;
}

- (void)handleClickAtPoint:(NSPoint)point {
    self.on = !self.on;
    if (self.onToggle) self.onToggle(self.on);
    [super handleClickAtPoint:point];
}

- (void)drawRect:(NSRect)dirtyRect {
    NSRect b = NSInsetRect(self.bounds, 1, 1);
    CGFloat r = b.size.height / 2.0;
    [(self.on ? VKTheme.primaryFill : VKTheme.toggleOff) setFill];
    [[NSBezierPath bezierPathWithRoundedRect:b xRadius:r yRadius:r] fill];

    CGFloat d = b.size.height - 4.0;
    CGFloat x = self.on ? NSMaxX(b) - 2.0 - d : b.origin.x + 2.0;
    NSRect knob = NSMakeRect(x, b.origin.y + 2.0, d, d);
    [NSGraphicsContext saveGraphicsState];
    NSShadow *sh = [[NSShadow alloc] init];
    sh.shadowOffset = NSMakeSize(0, -1);
    sh.shadowBlurRadius = 2.0;
    sh.shadowColor = [NSColor colorWithWhite:0 alpha:0.25];
    [sh set];
    [[NSColor whiteColor] setFill];
    [[NSBezierPath bezierPathWithOvalInRect:knob] fill];
    [NSGraphicsContext restoreGraphicsState];
}

@end

#pragma mark - VKSegmented

@implementation VKSegmented

- (instancetype)initWithFrame:(NSRect)frame items:(NSArray<NSString *> *)items {
    self = [super initWithFrame:frame];
    if (self) {
        _items = [items copy];
        _font = VKFont(13, NSFontWeightRegular);
        _selectedFont = VKFont(13, NSFontWeightSemibold);
        self.showsHandCursor = YES;
    }
    return self;
}

- (void)setItems:(NSArray<NSString *> *)items { _items = [items copy]; self.needsDisplay = YES; }
- (void)setSelectedIndex:(NSInteger)selectedIndex { _selectedIndex = selectedIndex; self.needsDisplay = YES; }

- (NSRect)rectForIndex:(NSInteger)i {
    NSInteger n = MAX((NSInteger)1, (NSInteger)self.items.count);
    CGFloat inset = 3.0;
    CGFloat w = (self.bounds.size.width - inset * 2.0) / n;
    return NSMakeRect(inset + i * w, inset, w, self.bounds.size.height - inset * 2.0);
}

- (void)handleClickAtPoint:(NSPoint)point {
    NSInteger n = (NSInteger)self.items.count;
    for (NSInteger i = 0; i < n; i++) {
        if (NSPointInRect(point, NSInsetRect([self rectForIndex:i], 0, -4))) {
            if (i != self.selectedIndex) {
                self.selectedIndex = i;
                if (self.onSelect) self.onSelect(i);
            }
            break;
        }
    }
    [super handleClickAtPoint:point];
}

- (void)drawRect:(NSRect)dirtyRect {
    NSRect b = self.bounds;
    CGFloat radius = MIN(9.0, b.size.height / 2.0);
    [VKTheme.trackBg setFill];
    [[NSBezierPath bezierPathWithRoundedRect:b xRadius:radius yRadius:radius] fill];

    NSInteger n = (NSInteger)self.items.count;
    for (NSInteger i = 0; i < n; i++) {
        NSRect r = [self rectForIndex:i];
        BOOL sel = (i == self.selectedIndex);
        NSColor *textColor = VKTheme.textSecondary;
        if (sel) {
            NSBezierPath *p = [NSBezierPath bezierPathWithRoundedRect:r xRadius:radius - 2.0 yRadius:radius - 2.0];
            if (self.style == VKSegmentStylePill) {
                [NSGraphicsContext saveGraphicsState];
                NSShadow *sh = [[NSShadow alloc] init];
                sh.shadowOffset = NSMakeSize(0, -0.5);
                sh.shadowBlurRadius = 1.5;
                sh.shadowColor = [NSColor colorWithWhite:0 alpha:0.18];
                [sh set];
                [VKDynamic(0xFFFFFF, 0x4B515C) setFill];
                [p fill];
                [NSGraphicsContext restoreGraphicsState];
                textColor = VKTheme.text;
            } else {
                NSColor *fill = self.fillForIndex ? self.fillForIndex(i) : VKTheme.primaryFill;
                [fill setFill];
                [p fill];
                textColor = [NSColor whiteColor];
            }
        } else if (self.hovering) {
            textColor = VKTheme.text;
        }
        NSDictionary *attrs = @{NSFontAttributeName: sel ? self.selectedFont : self.font,
                                NSForegroundColorAttributeName: textColor};
        NSString *t = self.items[(NSUInteger)i];
        NSSize ts = [t sizeWithAttributes:attrs];
        [t drawAtPoint:NSMakePoint(round(NSMidX(r) - ts.width / 2.0), round(NSMidY(r) - ts.height / 2.0)) withAttributes:attrs];
    }
}

@end

#pragma mark - VKButton

@implementation VKButton

+ (instancetype)buttonWithTitle:(NSString *)title symbol:(NSString *)symbol style:(VKButtonStyle)style {
    VKButton *b = [[VKButton alloc] initWithFrame:NSMakeRect(0, 0, 80, 30)];
    b.title = title ?: @"";
    b.symbolName = symbol;
    b.style = style;
    b.font = (style == VKButtonStyleLink) ? VKFont(13, NSFontWeightSemibold) : VKFont(13, NSFontWeightMedium);
    NSRect f = b.frame;
    f.size.width = [b fittingWidth];
    b.frame = f;
    return b;
}

- (void)setTitle:(NSString *)title { _title = [title copy]; self.needsDisplay = YES; }

- (BOOL)hasPadding {
    return !(self.style == VKButtonStyleLink || self.style == VKButtonStylePlain);
}

- (CGFloat)symbolSize {
    return MAX(11.0, self.font.pointSize);
}

- (CGFloat)fittingWidth {
    CGFloat w = 0;
    CGFloat iconW = self.symbolName ? [self symbolSize] + 4.0 : 0;
    CGFloat textW = self.title.length ? VKTextWidth(self.title, self.font) : 0;
    w += iconW;
    if (iconW > 0 && textW > 0) w += 6.0;
    w += textW;
    if (self.trailingSymbolName) w += 8.0 + [self symbolSize];
    if ([self hasPadding]) w += (self.style == VKButtonStyleIcon || self.style == VKButtonStyleDanger) ? 16.0 : 30.0;
    return ceil(w);
}

- (NSColor *)contentColor {
    if (self.tintOverride) return self.tintOverride;
    switch (self.style) {
        case VKButtonStylePrimary: return [NSColor whiteColor];
        case VKButtonStyleSecondary: return VKTheme.text;
        case VKButtonStyleLink: return VKTheme.primary;
        case VKButtonStylePlain: return VKTheme.textSecondary;
        case VKButtonStyleDashed: return VKTheme.textSecondary;
        case VKButtonStyleIcon: return VKTheme.textSecondary;
        case VKButtonStyleDanger: return VKTheme.error;
    }
    return VKTheme.text;
}

- (void)drawRect:(NSRect)dirtyRect {
    NSRect b = self.bounds;
    CGFloat radius = 8.0;
    NSBezierPath *p = [NSBezierPath bezierPathWithRoundedRect:NSInsetRect(b, 0.5, 0.5) xRadius:radius yRadius:radius];
    switch (self.style) {
        case VKButtonStylePrimary:
            [VKTheme.primaryFill setFill];
            [p fill];
            if (self.hovering || self.pressed) {
                [[NSColor colorWithWhite:0 alpha:self.pressed ? 0.18 : 0.08] setFill];
                [p fill];
            }
            break;
        case VKButtonStyleSecondary:
            [VKTheme.controlBg setFill];
            [p fill];
            if (self.hovering || self.pressed) {
                [VKTheme.hoverBg setFill];
                [p fill];
            }
            p.lineWidth = 1.0;
            [VKTheme.controlBorder setStroke];
            [p stroke];
            break;
        case VKButtonStyleDashed: {
            if (self.hovering) {
                [VKTheme.hoverBg setFill];
                [p fill];
            }
            CGFloat dash[2] = {4.0, 3.0};
            [p setLineDash:dash count:2 phase:0];
            p.lineWidth = 1.0;
            [VKTheme.controlBorder setStroke];
            [p stroke];
            break;
        }
        case VKButtonStyleIcon:
        case VKButtonStyleDanger: {
            NSColor *bg = VKDynamic(0xF0F1F5, 0x2E3238);
            if (self.hovering || self.pressed) bg = VKDynamic(0xE3E5EA, 0x3A3F47);
            [bg setFill];
            [[NSBezierPath bezierPathWithRoundedRect:b xRadius:6 yRadius:6] fill];
            break;
        }
        case VKButtonStyleLink:
        case VKButtonStylePlain:
            break;
    }

    NSColor *color = [self contentColor];
    if ((self.style == VKButtonStyleLink || self.style == VKButtonStylePlain) && self.hovering) {
        color = [color colorWithAlphaComponent:0.75];
    }
    CGFloat iconW = self.symbolName ? [self symbolSize] + 4.0 : 0;
    NSDictionary *attrs = @{NSFontAttributeName: self.font, NSForegroundColorAttributeName: color};
    CGFloat textW = self.title.length ? ceil([self.title sizeWithAttributes:attrs].width) : 0;
    CGFloat trailW = self.trailingSymbolName ? [self symbolSize] : 0;
    CGFloat total = iconW + (iconW > 0 && textW > 0 ? 6.0 : 0) + textW + (trailW > 0 ? 8.0 + trailW : 0);
    CGFloat x = [self hasPadding] ? round((b.size.width - total) / 2.0) : 0;
    if (self.symbolName) {
        VKDrawSymbol(self.symbolName, [self symbolSize] - 1.0, NSFontWeightMedium, color,
                     NSMakeRect(x, 0, iconW, b.size.height));
        x += iconW + (textW > 0 ? 6.0 : 0);
    }
    if (textW > 0) {
        NSSize ts = [self.title sizeWithAttributes:attrs];
        [self.title drawAtPoint:NSMakePoint(x, round(NSMidY(b) - ts.height / 2.0)) withAttributes:attrs];
        x += textW;
    }
    if (self.trailingSymbolName) {
        VKDrawSymbol(self.trailingSymbolName, [self symbolSize] - 2.0, NSFontWeightSemibold, color,
                     NSMakeRect(x + 8.0, 0, trailW, b.size.height));
    }
}

@end

#pragma mark - VKSelectCard

@implementation VKSelectCard

- (void)setSelected:(BOOL)selected { _selected = selected; self.needsDisplay = YES; }

- (void)drawRect:(NSRect)dirtyRect {
    NSRect b = self.bounds;
    CGFloat bw = self.selected ? 1.5 : 1.0;
    NSBezierPath *p = [NSBezierPath bezierPathWithRoundedRect:NSInsetRect(b, bw / 2.0, bw / 2.0) xRadius:12 yRadius:12];
    [(self.selected ? VKTheme.primaryTint : VKTheme.groupBg) setFill];
    [p fill];
    p.lineWidth = bw;
    [(self.selected ? VKTheme.primary : (self.hovering ? VKTheme.controlBorder : VKTheme.separator)) setStroke];
    [p stroke];

    NSDictionary *ta = @{NSFontAttributeName: VKFont(15, NSFontWeightSemibold), NSForegroundColorAttributeName: VKTheme.text};
    [self.title drawAtPoint:NSMakePoint(16, 13) withAttributes:ta];

    NSMutableParagraphStyle *ps = [[NSMutableParagraphStyle alloc] init];
    ps.lineBreakMode = NSLineBreakByWordWrapping;
    ps.lineSpacing = 2.0;
    NSDictionary *sa = @{NSFontAttributeName: self.monoSubtitle ? VKMono(12, NSFontWeightRegular) : VKFont(13, NSFontWeightRegular),
                         NSForegroundColorAttributeName: VKTheme.textSecondary,
                         NSParagraphStyleAttributeName: ps};
    [self.subtitle drawInRect:NSMakeRect(16, 38, b.size.width - 32, b.size.height - 40) withAttributes:sa];

    if (self.selected) {
        VKDrawCheckBadge(NSMakeRect(b.size.width - 16 - 20, 13, 20, 20), VKTheme.primaryFill);
    }
}

@end

#pragma mark - VKKeycapsView

@implementation VKKeycapsView

+ (CGFloat)capWidthForKey:(NSString *)key font:(NSFont *)font height:(CGFloat)height {
    return MAX(height - 2.0, VKTextWidth(key, font) + 16.0);
}

+ (CGFloat)widthForKeys:(NSArray<NSString *> *)keys font:(NSFont *)font {
    CGFloat w = 0;
    for (NSString *k in keys) w += [self capWidthForKey:k font:font height:26.0] + 6.0;
    return keys.count ? w - 6.0 : 0;
}

- (instancetype)initWithKeys:(NSArray<NSString *> *)keys font:(NSFont *)font height:(CGFloat)height {
    self = [super initWithFrame:NSMakeRect(0, 0, 10, height)];
    if (self) {
        _keys = [keys copy];
        _font = font;
        _capHeight = height;
        [self sizeToFitKeys];
    }
    return self;
}

- (BOOL)isFlipped { return YES; }

- (void)setKeys:(NSArray<NSString *> *)keys { _keys = [keys copy]; self.needsDisplay = YES; }

- (void)sizeToFitKeys {
    CGFloat w = 0;
    for (NSString *k in self.keys) w += [VKKeycapsView capWidthForKey:k font:self.font height:self.capHeight] + 6.0;
    if (self.keys.count) w -= 6.0;
    NSRect f = self.frame;
    f.size = NSMakeSize(MAX(1.0, w), self.capHeight);
    self.frame = f;
}

- (void)drawRect:(NSRect)dirtyRect {
    CGFloat x = 0;
    for (NSString *k in self.keys) {
        CGFloat w = [VKKeycapsView capWidthForKey:k font:self.font height:self.capHeight];
        NSRect r = NSMakeRect(x, 0, w, self.capHeight);
        if (self.tinted) {
            [VKTheme.primaryTint setFill];
            [[NSBezierPath bezierPathWithRoundedRect:r xRadius:5 yRadius:5] fill];
            NSDictionary *a = @{NSFontAttributeName: self.font, NSForegroundColorAttributeName: VKTheme.primary};
            NSSize ts = [k sizeWithAttributes:a];
            [k drawAtPoint:NSMakePoint(NSMidX(r) - ts.width / 2.0, NSMidY(r) - ts.height / 2.0) withAttributes:a];
        } else {
            VKDrawKeycap(r, k, self.font, self.highlighted);
        }
        x += w + 6.0;
    }
}

@end

#pragma mark - VKIconTile

@implementation VKIconTile

- (instancetype)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        _fillColor = VKTheme.iconTileBg;
        _tintColor = VKTheme.primary;
        _cornerRadius = 8.0;
        _symbolSize = 14.0;
    }
    return self;
}

- (BOOL)isFlipped { return YES; }

- (void)drawRect:(NSRect)dirtyRect {
    NSRect b = self.bounds;
    NSBezierPath *p = self.circle ? [NSBezierPath bezierPathWithOvalInRect:b]
                                  : [NSBezierPath bezierPathWithRoundedRect:b xRadius:self.cornerRadius yRadius:self.cornerRadius];
    [self.fillColor setFill];
    [p fill];
    if (self.image) {
        [self.image drawInRect:b fromRect:NSZeroRect operation:NSCompositingOperationSourceOver fraction:1.0 respectFlipped:YES hints:nil];
    } else if (self.symbolName) {
        VKDrawSymbol(self.symbolName, self.symbolSize, NSFontWeightMedium, self.tintColor, b);
    } else if (self.text) {
        NSDictionary *a = @{NSFontAttributeName: VKFont(self.symbolSize + 2.0, NSFontWeightSemibold),
                            NSForegroundColorAttributeName: self.tintColor};
        NSSize ts = [self.text sizeWithAttributes:a];
        [self.text drawAtPoint:NSMakePoint(NSMidX(b) - ts.width / 2.0, NSMidY(b) - ts.height / 2.0) withAttributes:a];
    }
}

@end

#pragma mark - VKChip

@implementation VKChip

+ (instancetype)chipWithText:(NSString *)text font:(NSFont *)font text:(NSColor *)fg fill:(NSColor *)bg height:(CGFloat)height {
    VKChip *c = [[VKChip alloc] initWithFrame:NSMakeRect(0, 0, 20, height)];
    c.text = text;
    c.font = font;
    c.textColor = fg;
    c.fillColor = bg;
    c.hPadding = 8.0;
    [c sizeToFitText];
    return c;
}

- (BOOL)isFlipped { return YES; }

- (void)setText:(NSString *)text { _text = [text copy]; self.needsDisplay = YES; }

- (void)sizeToFitText {
    NSRect f = self.frame;
    f.size.width = VKTextWidth(self.text, self.font) + self.hPadding * 2.0;
    self.frame = f;
}

- (void)drawRect:(NSRect)dirtyRect {
    NSRect b = self.bounds;
    CGFloat r = MIN(6.0, b.size.height / 2.0);
    [self.fillColor setFill];
    [[NSBezierPath bezierPathWithRoundedRect:b xRadius:r yRadius:r] fill];
    NSDictionary *a = @{NSFontAttributeName: self.font, NSForegroundColorAttributeName: self.textColor};
    NSSize ts = [self.text sizeWithAttributes:a];
    [self.text drawAtPoint:NSMakePoint(round(NSMidX(b) - ts.width / 2.0), round(NSMidY(b) - ts.height / 2.0)) withAttributes:a];
}

@end

#pragma mark - VKLangTile

@implementation VKLangTile

- (BOOL)isFlipped { return YES; }

- (void)setVietnamese:(BOOL)vietnamese { _vietnamese = vietnamese; self.needsDisplay = YES; }

- (void)drawRect:(NSRect)dirtyRect {
    NSRect b = self.bounds;
    CGFloat w = b.size.width;
    NSColor *fill;
    if (self.vietnamese) fill = self.forceDarkStyle ? VKHex(0x3346D8) : VKTheme.primaryFill;
    else fill = self.forceDarkStyle ? VKHex(0x4B515C) : VKDynamic(0x4B515C, 0x4B515C);
    [fill setFill];
    [[NSBezierPath bezierPathWithRoundedRect:b xRadius:w * 0.24 yRadius:w * 0.24] fill];

    NSDictionary *a = @{NSFontAttributeName: VKFont(round(w * 0.36), NSFontWeightBold),
                        NSForegroundColorAttributeName: [NSColor whiteColor]};
    NSString *t = self.vietnamese ? @"VI" : @"EN";
    NSSize ts = [t sizeWithAttributes:a];
    [t drawAtPoint:NSMakePoint(round(NSMidX(b) - ts.width / 2.0), round(NSMidY(b) - ts.height / 2.0)) withAttributes:a];

    if (self.vietnamese) {
        CGFloat d = round(w * 0.16);
        NSRect dot = NSMakeRect(w - d - w * 0.1, w * 0.1, d, d);
        [VKTheme.jadeDot setFill];
        [[NSBezierPath bezierPathWithOvalInRect:dot] fill];
    }
}

@end

#pragma mark - VKFieldBox

@interface VKFieldBox ()
@property (nonatomic, strong, readwrite) NSTextField *field;
@property (nonatomic, copy, nullable) NSString *symbol;
@end

@implementation VKFieldBox

- (instancetype)initWithFrame:(NSRect)frame placeholder:(NSString *)placeholder font:(NSFont *)font symbol:(NSString *)symbol {
    self = [super initWithFrame:frame fill:VKTheme.fieldBg radius:8.0];
    if (self) {
        self.borderColor = VKDynamic(0xDADDE4, 0x343841);
        _symbol = [symbol copy];
        _field = VKPlainField(placeholder, font);
        [self addSubview:_field];
        [self layoutField];
    }
    return self;
}

- (void)layoutField {
    if (!self.field) return;
    NSLayoutManager *lm = [[NSLayoutManager alloc] init];
    CGFloat lh = ceil([lm defaultLineHeightForFont:self.field.font]) + 2.0;
    CGFloat x = self.symbol ? 34.0 : 12.0;
    self.field.frame = NSMakeRect(x, round((self.bounds.size.height - lh) / 2.0),
                                  MAX(10.0, self.bounds.size.width - x - 10.0), lh);
}

- (void)setFrameSize:(NSSize)newSize {
    [super setFrameSize:newSize];
    [self layoutField];
}

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];
    if (self.symbol) {
        VKDrawSymbol(self.symbol, 13, NSFontWeightMedium, VKTheme.textSecondary,
                     NSMakeRect(10, 0, 18, self.bounds.size.height));
    }
}

- (void)mouseDown:(NSEvent *)event {
    [self.window makeFirstResponder:self.field];
}

@end

#pragma mark - VKSavedIndicator

@implementation VKSavedIndicator

+ (instancetype)indicator {
    CGFloat w = VKTextWidth(@"Đã lưu tự động", VKFont(13, NSFontWeightMedium)) + 16.0;
    return [[VKSavedIndicator alloc] initWithFrame:NSMakeRect(0, 0, w, 18)];
}

- (BOOL)isFlipped { return YES; }

- (void)drawRect:(NSRect)dirtyRect {
    NSRect b = self.bounds;
    [VKTheme.accentGraphic setFill];
    [[NSBezierPath bezierPathWithOvalInRect:NSMakeRect(0, NSMidY(b) - 3.5, 7, 7)] fill];
    NSDictionary *a = @{NSFontAttributeName: VKFont(13, NSFontWeightMedium), NSForegroundColorAttributeName: VKTheme.accentText};
    NSString *t = @"Đã lưu tự động";
    NSSize ts = [t sizeWithAttributes:a];
    [t drawAtPoint:NSMakePoint(14, round(NSMidY(b) - ts.height / 2.0)) withAttributes:a];
}

@end

#pragma mark - VKMarkView

@implementation VKMarkView

- (BOOL)isFlipped { return YES; }

- (void)drawRect:(NSRect)dirtyRect {
    if (self.appIcon) {
        VKDrawAppIcon(self.bounds);
    } else {
        VKDrawMark(self.bounds, VKTheme.accentGraphic, VKTheme.primary);
    }
}

@end

#pragma mark - VKFakeTrafficLights

@implementation VKFakeTrafficLights

- (BOOL)isFlipped { return YES; }

- (void)drawRect:(NSRect)dirtyRect {
    NSArray<NSColor *> *colors = self.dimmed
        ? @[VKHex(0xFF5F57), VKHexA(0xFFFFFF, 0.3), VKHexA(0xFFFFFF, 0.3)]
        : @[VKHex(0xFF5F57), VKHex(0xFEBC2E), VKHex(0x28C840)];
    for (NSUInteger i = 0; i < colors.count; i++) {
        [colors[i] setFill];
        [[NSBezierPath bezierPathWithOvalInRect:NSMakeRect(i * 20.0, 0, 12, 12)] fill];
    }
}

@end
