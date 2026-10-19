// VKHUD.h — VI/EN toggle HUD (design 02, bottom). MIT License, 1VISION.

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@interface VKHUDController : NSObject
/// Shows a non-activating, click-through panel near the bottom of the screen with the mouse; fades out after ~1.2 s.
- (void)showVietnamese:(BOOL)vietnamese hint:(NSString *)hint;
/// The HUD content view (used by the panel and by --ui-snapshot).
+ (NSView *)hudViewVietnamese:(BOOL)vietnamese hint:(NSString *)hint;
@end

NS_ASSUME_NONNULL_END
