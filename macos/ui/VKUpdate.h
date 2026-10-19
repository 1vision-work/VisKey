// VKUpdate.h — update check shared by the "Kiểm tra cập nhật" button and the automatic check.
// One anonymous GET of the public GitHub releases list: no cookies, no IDs, no query added.
// MIT License, 1VISION.

#import <Cocoa/Cocoa.h>

@class VKModel;

NS_ASSUME_NONNULL_BEGIN

extern NSString *const VKRepoURL;

typedef NS_ENUM(NSInteger, VKUpdateStatus) {
    VKUpdateStatusUpToDate = 0,
    VKUpdateStatusAvailable,
    VKUpdateStatusNoRelease,
    VKUpdateStatusNetworkError,
    VKUpdateStatusHTTPError,
    VKUpdateStatusBadData
};

@interface VKUpdateResult : NSObject
@property (nonatomic, readonly) VKUpdateStatus status;
@property (nonatomic, copy, readonly) NSString *currentVersion;
@property (nonatomic, copy, readonly, nullable) NSString *latestVersion;  // without "v"; set when Available
@property (nonatomic, copy, readonly, nullable) NSString *releaseURL;     // set when Available
@property (nonatomic, readonly) NSInteger httpCode;
+ (instancetype)resultWithStatus:(VKUpdateStatus)status
                  currentVersion:(NSString *)currentVersion
                   latestVersion:(nullable NSString *)latestVersion
                      releaseURL:(nullable NSString *)releaseURL
                        httpCode:(NSInteger)httpCode;
- (BOOL)isError;
/// Vietnamese status line for the "Giới thiệu" page.
- (NSString *)message;
@end

/// Fetches the latest non-draft release (8 s timeout, ephemeral session) off the main thread.
/// `completion` runs on the main queue.
void VKFetchLatestRelease(NSString *currentVersion, void (^completion)(VKUpdateResult *result));

/// Runs manual and automatic checks against the model's update state (see VKModel.h).
/// Automatic: at most once per 24 h since the last attempt; evaluated shortly after launch,
/// every few hours by a timer, after wake and when the setting is switched on.
@interface VKUpdateChecker : NSObject
- (instancetype)initWithModel:(VKModel *)model;
- (void)start;
- (void)stop;
/// "Kiểm tra cập nhật" button (ignored while a check is running).
- (void)checkNow;
@end

NS_ASSUME_NONNULL_END
