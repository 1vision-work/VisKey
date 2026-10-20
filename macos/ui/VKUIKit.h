// VKUIKit.h — VisKey macOS UI kit: brand colors (design/tokens.json), fonts,
// the VisKey mark / app icon drawn with NSBezierPath, and small custom-drawn
// controls (switch, segmented, buttons, cards, keycaps...).
// All views are flipped (top-left origin) and draw with dynamic NSColors so
// light / dark appearance is resolved at draw time.
// MIT License, 1VISION.

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

#pragma mark - Colors & fonts

NSColor *VKHex(uint32_t rgb);
NSColor *VKHexA(uint32_t rgb, CGFloat alpha);
NSColor *VKDynamic(uint32_t light, uint32_t dark);
NSColor *VKDynamicA(uint32_t light, CGFloat lightAlpha, uint32_t dark, CGFloat darkAlpha);
BOOL VKAppearanceIsDark(NSAppearance *_Nullable appearance);

@interface VKTheme : NSObject
+ (NSColor *)windowBg;
+ (NSColor *)sidebarBg;
+ (NSColor *)groupBg;
+ (NSColor *)surface;
+ (NSColor *)separator;
+ (NSColor *)text;
+ (NSColor *)textSecondary;
+ (NSColor *)textTertiary;
+ (NSColor *)primary;
+ (NSColor *)primaryFill;
+ (NSColor *)primaryTint;
+ (NSColor *)sidebarSelected;
+ (NSColor *)accentText;
+ (NSColor *)accentGraphic;
+ (NSColor *)accentTint;
+ (NSColor *)jadeDot;
+ (NSColor *)warning;
+ (NSColor *)warningTint;
+ (NSColor *)error;
+ (NSColor *)controlBg;
+ (NSColor *)controlBorder;
+ (NSColor *)toggleOff;
+ (NSColor *)trackBg;
+ (NSColor *)fieldBg;
+ (NSColor *)keycapBg;
+ (NSColor *)hoverBg;
+ (NSColor *)darkSegmentFill;
+ (NSColor *)iconTileBg;
@end

NSFont *VKFont(CGFloat size, NSFontWeight weight);
NSFont *VKMono(CGFloat size, NSFontWeight weight);

#pragma mark - Brand drawing

/// Draws the VisKey symbol (jade chevron + "V") from design/icons/symbol.svg (64x64 viewBox)
/// scaled into `rect`. Works in flipped and non-flipped contexts.
void VKDrawMark(NSRect rect, NSColor *chevronColor, NSColor *vColor);
/// Draws the rounded app icon (design/icons/appicon-1024.svg) filling `rect`.
void VKDrawAppIcon(NSRect rect);
/// Template image of the mark for the status bar.
NSImage *VKStatusMarkImage(CGFloat pointSize);

/// SF Symbol (macOS 11+) or nil if the name is unknown.
NSImage *_Nullable VKSymbol(NSString *name, CGFloat pointSize, NSFontWeight weight);
/// Draws an SF Symbol tinted with `color`, centered in `rect` (aspect preserved).
void VKDrawSymbol(NSString *name, CGFloat pointSize, NSFontWeight weight, NSColor *color, NSRect rect);
/// Draws a keycap with `label` in rect.
void VKDrawKeycap(NSRect rect, NSString *label, NSFont *font, BOOL highlighted);
/// Draws a checkmark badge (filled circle + white check).
void VKDrawCheckBadge(NSRect rect, NSColor *fill);

#pragma mark - Labels & layout helpers

NSTextField *VKLabel(NSString *text, NSFont *font, NSColor *color);
NSTextField *VKAttrLabel(NSAttributedString *text);
/// Wrapping label laid out at `width`; frame height is set to fit.
NSTextField *VKWrapLabel(NSString *text, NSFont *font, NSColor *color, CGFloat width);
NSTextField *VKWrapAttrLabel(NSAttributedString *text, CGFloat width);
NSAttributedString *VKAttr(NSString *text, NSFont *font, NSColor *color);
NSAttributedString *VKAttrKern(NSString *text, NSFont *font, NSColor *color, CGFloat kern);
CGFloat VKTextWidth(NSString *text, NSFont *font);
void VKSetOrigin(NSView *view, CGFloat x, CGFloat y);
/// Resizes a (non-wrapping) label to fit and places it so that its vertical center is `centerY`.
void VKPlaceLabel(NSTextField *label, CGFloat x, CGFloat centerY);
/// Borderless, background-less editable text field.
NSTextField *VKPlainField(NSString *placeholder, NSFont *font);
/// Formats an integer with Vietnamese grouping (2.417).
NSString *VKFormatCount(NSInteger value);

#pragma mark - Base views

/// Flipped view with fill / border / corner radius, optional hairline separators,
/// optional search highlight and an optional custom draw block.
@interface VKBox : NSView
@property (nonatomic, strong, nullable) NSColor *fillColor;
@property (nonatomic, strong, nullable) NSColor *borderColor;
@property (nonatomic) CGFloat borderWidth;
@property (nonatomic) CGFloat cornerRadius;
@property (nonatomic) BOOL dashedBorder;
@property (nonatomic) BOOL topSeparator;
@property (nonatomic) BOOL bottomSeparator;
@property (nonatomic) CGFloat separatorInset;
@property (nonatomic) BOOL rightSeparator;
@property (nonatomic, copy, nullable) NSString *searchText;
@property (nonatomic) BOOL searchHighlighted;
@property (nonatomic, copy, nullable) void (^drawHandler)(NSRect bounds);
- (instancetype)initWithFrame:(NSRect)frame fill:(nullable NSColor *)fill radius:(CGFloat)radius;
@end

/// Base clickable view: hover tracking, click block, first-mouse, hand cursor.
@interface VKControlView : VKBox
@property (nonatomic, readonly) BOOL hovering;
@property (nonatomic, readonly) BOOL pressed;
@property (nonatomic) BOOL enabled;
@property (nonatomic) BOOL showsHandCursor;
@property (nonatomic, copy, nullable) void (^onClick)(void);
@property (nonatomic, copy, nullable) void (^onHoverChanged)(BOOL hovering);
/// Called on a completed click (mouse up inside). Subclasses override; the base calls onClick.
- (void)handleClickAtPoint:(NSPoint)point;
@end

#pragma mark - Controls

@interface VKSwitch : VKControlView
@property (nonatomic) BOOL on;
@property (nonatomic, copy, nullable) void (^onToggle)(BOOL on);
+ (instancetype)switchWithOn:(BOOL)on;   // 40x24
+ (instancetype)largeSwitchWithOn:(BOOL)on; // 48x28
@end

typedef NS_ENUM(NSInteger, VKSegmentStyle) {
    VKSegmentStylePill = 0,   // grey track, selected item = raised white pill
    VKSegmentStyleFilled = 1  // grey track, selected item filled (color per index), white text
};

@interface VKSegmented : VKControlView
@property (nonatomic, copy) NSArray<NSString *> *items;
@property (nonatomic) NSInteger selectedIndex;
@property (nonatomic) VKSegmentStyle style;
@property (nonatomic, strong) NSFont *font;
@property (nonatomic, strong) NSFont *selectedFont;
@property (nonatomic, copy, nullable) NSColor *_Nonnull (^fillForIndex)(NSInteger index);
@property (nonatomic, copy, nullable) void (^onSelect)(NSInteger index);
- (instancetype)initWithFrame:(NSRect)frame items:(NSArray<NSString *> *)items;
@end

typedef NS_ENUM(NSInteger, VKButtonStyle) {
    VKButtonStylePrimary = 0,
    VKButtonStyleSecondary,
    VKButtonStyleLink,
    VKButtonStylePlain,
    VKButtonStyleDashed,
    VKButtonStyleIcon,      // square icon button with hover background
    VKButtonStyleDanger
};

@interface VKButton : VKControlView
@property (nonatomic, copy) NSString *title;
@property (nonatomic, copy, nullable) NSString *symbolName;
@property (nonatomic, copy, nullable) NSString *trailingSymbolName;
@property (nonatomic) VKButtonStyle style;
@property (nonatomic, strong) NSFont *font;
@property (nonatomic, strong, nullable) NSColor *tintOverride;
+ (instancetype)buttonWithTitle:(NSString *)title symbol:(nullable NSString *)symbol style:(VKButtonStyle)style;
- (CGFloat)fittingWidth;
@end

/// Selectable card ("Telex", "Kiểu cũ"...) with a check badge when selected.
@interface VKSelectCard : VKControlView
@property (nonatomic, copy) NSString *title;
@property (nonatomic, copy) NSString *subtitle;
@property (nonatomic) BOOL monoSubtitle;
@property (nonatomic) BOOL selected;
@end

/// A row of keycaps (e.g. ⌃ Space).
@interface VKKeycapsView : NSView
@property (nonatomic, copy) NSArray<NSString *> *keys;
@property (nonatomic, strong) NSFont *font;
@property (nonatomic) CGFloat capHeight;
@property (nonatomic) BOOL highlighted;
@property (nonatomic) BOOL tinted;  // primary tint caps (recorder style)
+ (CGFloat)widthForKeys:(NSArray<NSString *> *)keys font:(NSFont *)font;
- (instancetype)initWithKeys:(NSArray<NSString *> *)keys font:(NSFont *)font height:(CGFloat)height;
- (void)sizeToFitKeys;
@end

/// Rounded tile with an SF Symbol or short text in the middle.
@interface VKIconTile : NSView
@property (nonatomic, copy, nullable) NSString *symbolName;
@property (nonatomic, copy, nullable) NSString *text;
@property (nonatomic, strong, nullable) NSImage *image;
@property (nonatomic, strong) NSColor *fillColor;
@property (nonatomic, strong) NSColor *tintColor;
@property (nonatomic) CGFloat cornerRadius;
@property (nonatomic) CGFloat symbolSize;
@property (nonatomic) BOOL circle;
@end

/// Small rounded chip with text.
@interface VKChip : NSView
@property (nonatomic, copy) NSString *text;
@property (nonatomic, strong) NSFont *font;
@property (nonatomic, strong) NSColor *textColor;
@property (nonatomic, strong) NSColor *fillColor;
@property (nonatomic) CGFloat hPadding;
+ (instancetype)chipWithText:(NSString *)text font:(NSFont *)font text:(NSColor *)fg fill:(NSColor *)bg height:(CGFloat)height;
- (void)sizeToFitText;
@end

/// "VI"/"EN" language tile with jade dot (status card, HUD).
@interface VKLangTile : NSView
@property (nonatomic) BOOL vietnamese;
@property (nonatomic) BOOL forceDarkStyle;
@end

/// Rounded field container with a borderless NSTextField inside (and optional leading symbol).
@interface VKFieldBox : VKBox
@property (nonatomic, strong, readonly) NSTextField *field;
- (instancetype)initWithFrame:(NSRect)frame placeholder:(NSString *)placeholder font:(NSFont *)font symbol:(nullable NSString *)symbol;
@end

/// "● Đã lưu tự động" indicator.
@interface VKSavedIndicator : NSView
+ (instancetype)indicator;
@end

/// Small view that draws the VisKey mark.
@interface VKMarkView : NSView
@property (nonatomic) BOOL appIcon;  // draw the full app icon instead of the bare mark
@end

/// Traffic-light dots, drawn only in snapshot mode (real windows have real ones).
@interface VKFakeTrafficLights : NSView
@property (nonatomic) BOOL dimmed;
@end

NS_ASSUME_NONNULL_END
