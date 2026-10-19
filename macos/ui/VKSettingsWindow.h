// VKSettingsWindow.h — "Bảng điều khiển" window (sidebar + pages). MIT License, 1VISION.

#import <Cocoa/Cocoa.h>
#import "VKModel.h"

NS_ASSUME_NONNULL_BEGIN

@interface VKSettingsWindowController : NSWindowController <NSWindowDelegate, NSTextFieldDelegate>
@property (nonatomic, readonly) VKSettingsPage currentPage;
- (instancetype)initWithModel:(VKModel *)model snapshot:(BOOL)snapshot;
- (void)showPage:(VKSettingsPage)page;
@end

NS_ASSUME_NONNULL_END
