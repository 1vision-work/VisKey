// VKOnboarding.h — first-run window: choose input method, grant Accessibility, try it (design 04).
// MIT License, 1VISION.

#import <Cocoa/Cocoa.h>
#import "VKModel.h"

NS_ASSUME_NONNULL_BEGIN

@interface VKOnboardingWindowController : NSWindowController <NSWindowDelegate>
@property (nonatomic, readonly) NSInteger step;  // 1...3
/// Called (on the main thread) as soon as AXIsProcessTrusted() turns true while the window is open.
@property (nonatomic, copy, nullable) void (^onPermissionGranted)(void);
- (instancetype)initWithModel:(VKModel *)model snapshot:(BOOL)snapshot;
- (void)showStep:(NSInteger)step;
@end

NS_ASSUME_NONNULL_END
