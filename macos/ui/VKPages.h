// VKPages.h — content pages of the VisKey settings window ("Bảng điều khiển").
// MIT License, 1VISION.

#import <Cocoa/Cocoa.h>
#import "VKUIKit.h"
#import "VKModel.h"

NS_ASSUME_NONNULL_BEGIN

extern const CGFloat kVKPagePad;

/// Base class: flipped page view with layout helpers and search highlighting.
@interface VKPage : NSView
@property (nonatomic, strong, readonly) VKModel *model;
@property (nonatomic, readonly) CGFloat pageWidth;
@property (nonatomic, readonly) CGFloat viewportHeight;
@property (nonatomic, readonly) CGFloat contentWidth;  // pageWidth - 2 * kVKPagePad
@property (nonatomic, readonly) BOOL snapshot;
@property (nonatomic, strong, readonly) NSMutableArray<VKBox *> *searchRows;
@property (nonatomic, copy) NSString *pageTitle;
@property (nonatomic, copy) NSString *keywords;

- (instancetype)initWithModel:(VKModel *)model width:(CGFloat)width height:(CGFloat)viewportHeight snapshot:(BOOL)snapshot;
- (void)build;                 // subclasses create their views here (called once by init)
- (void)reload;                // refresh from the model
- (void)environmentChanged;    // AX permission / conflicts / current app changed
- (void)pageWillAppear;
- (void)pageDidDisappear;
/// Highlights rows matching `query`; returns YES if the page (title, keywords or a row) matches.
- (BOOL)applySearch:(NSString *)query;
- (void)setContentHeight:(CGFloat)height;

// Layout helpers (flipped coordinates, x/y relative to the page)
- (CGFloat)addTitle:(NSString *)title subtitle:(nullable NSAttributedString *)subtitle;
- (NSTextField *)addSectionLabel:(NSString *)text x:(CGFloat)x y:(CGFloat)y;
- (VKBox *)addCard:(NSRect)frame;
- (VKControlView *)addRowToCard:(VKBox *)card
                              y:(CGFloat)y
                         height:(CGFloat)height
                          title:(NSString *)title
                       subtitle:(nullable NSString *)subtitle
                   monoSubtitle:(BOOL)mono
                         symbol:(nullable NSString *)symbol
                      accessory:(nullable NSView *)accessory
                           last:(BOOL)last;
- (void)showAlert:(NSString *)message info:(nullable NSString *)info;
@end

@interface VKGeneralPage : VKPage
@end

@interface VKMacrosPage : VKPage <NSTableViewDataSource, NSTableViewDelegate, NSTextFieldDelegate>
@end

@interface VKAppsPage : VKPage <NSTableViewDataSource, NSTableViewDelegate, NSTextFieldDelegate>
@end

@interface VKShortcutsPage : VKPage
@end

@interface VKPrivacyPage : VKPage
@end

@interface VKAboutPage : VKPage
@end

/// Hotkey recorder control used on the "Phím tắt" page.
@interface VKHotkeyRecorder : VKControlView
@property (nonatomic, strong) VKHotkey *hotkey;
@property (nonatomic, readonly) BOOL recording;
@property (nonatomic) BOOL previewRecording;  // draw the recording state without capturing (snapshot)
@property (nonatomic, copy, nullable) void (^onChange)(VKHotkey *hotkey);
+ (void)stopActiveRecorder;
- (void)startRecording;
- (void)stopRecording;
@end

NS_ASSUME_NONNULL_END
