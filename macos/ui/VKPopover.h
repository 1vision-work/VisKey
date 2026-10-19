// VKPopover.h — menu-bar popover content (design 02, top). MIT License, 1VISION.

#import <Cocoa/Cocoa.h>
#import "VKModel.h"

NS_ASSUME_NONNULL_BEGIN

@interface VKPopoverController : NSViewController
- (instancetype)initWithModel:(VKModel *)model;
- (void)reload;
@end

NS_ASSUME_NONNULL_END
