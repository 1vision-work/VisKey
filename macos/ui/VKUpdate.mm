// VKUpdate.mm — see VKUpdate.h. MIT License, 1VISION.

#import "VKUpdate.h"
#import "VKModel.h"
#include <string>
#include "VKUpdatePolicy.hpp"

NSString *const VKRepoURL = @"https://github.com/1vision-work/VisKey";
static NSString *const kVKReleasesAPI = @"https://api.github.com/repos/1vision-work/VisKey/releases?per_page=5";

static const NSTimeInterval kVKUpdateTimeout = 8.0;
static const NSTimeInterval kVKAutoCheckLaunchDelay = 20.0;      // let startup finish first
static const NSTimeInterval kVKAutoCheckTimerInterval = 3 * 3600.0;  // re-evaluate; the policy throttles to 24 h

#pragma mark - Version helpers

static NSArray<NSNumber *> *VKVersionParts(NSString *version) {
    NSString *v = [version stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
    if ([v hasPrefix:@"v"] || [v hasPrefix:@"V"]) v = [v substringFromIndex:1];
    NSRange cut = [v rangeOfCharacterFromSet:[NSCharacterSet characterSetWithCharactersInString:@"-+ "]];
    if (cut.location != NSNotFound) v = [v substringToIndex:cut.location];
    NSMutableArray<NSNumber *> *parts = [NSMutableArray array];
    for (NSString *p in [v componentsSeparatedByString:@"."]) [parts addObject:@(p.integerValue)];
    return parts;
}

static NSComparisonResult VKCompareVersions(NSString *a, NSString *b) {
    NSArray<NSNumber *> *pa = VKVersionParts(a), *pb = VKVersionParts(b);
    NSUInteger n = MAX(pa.count, pb.count);
    for (NSUInteger i = 0; i < n; i++) {
        NSInteger x = i < pa.count ? pa[i].integerValue : 0;
        NSInteger y = i < pb.count ? pb[i].integerValue : 0;
        if (x != y) return x < y ? NSOrderedAscending : NSOrderedDescending;
    }
    return NSOrderedSame;
}

#pragma mark - Result

@interface VKUpdateResult ()
@property (nonatomic, readwrite) VKUpdateStatus status;
@property (nonatomic, copy, readwrite) NSString *currentVersion;
@property (nonatomic, copy, readwrite, nullable) NSString *latestVersion;
@property (nonatomic, copy, readwrite, nullable) NSString *releaseURL;
@property (nonatomic, readwrite) NSInteger httpCode;
@end

@implementation VKUpdateResult

+ (instancetype)resultWithStatus:(VKUpdateStatus)status
                  currentVersion:(NSString *)currentVersion
                   latestVersion:(NSString *)latestVersion
                      releaseURL:(NSString *)releaseURL
                        httpCode:(NSInteger)httpCode {
    VKUpdateResult *r = [[VKUpdateResult alloc] init];
    r.status = status;
    r.currentVersion = currentVersion;
    r.latestVersion = latestVersion;
    r.releaseURL = releaseURL;
    r.httpCode = httpCode;
    return r;
}

- (BOOL)isError {
    return self.status == VKUpdateStatusNetworkError || self.status == VKUpdateStatusHTTPError ||
           self.status == VKUpdateStatusBadData;
}

- (NSString *)message {
    switch (self.status) {
        case VKUpdateStatusUpToDate:
            return [NSString stringWithFormat:@"Đã là phiên bản mới nhất (%@).", self.currentVersion];
        case VKUpdateStatusAvailable:
            return [NSString stringWithFormat:@"Có bản mới %@", self.latestVersion ?: @""];
        case VKUpdateStatusNoRelease:
            return @"Chưa có bản phát hành nào trên GitHub.";
        case VKUpdateStatusNetworkError:
            return @"Không kiểm tra được cập nhật. Hãy kiểm tra kết nối mạng rồi thử lại.";
        case VKUpdateStatusHTTPError:
            return [NSString stringWithFormat:@"Máy chủ GitHub trả về lỗi %ld. Thử lại sau.", (long)self.httpCode];
        case VKUpdateStatusBadData:
            return @"Dữ liệu cập nhật không hợp lệ.";
    }
    return @"";
}

@end

#pragma mark - Request

static VKUpdateResult *VKParseReleases(NSData *data, NSURLResponse *response, NSError *error, NSString *current) {
    if (error || !data) {
        return [VKUpdateResult resultWithStatus:VKUpdateStatusNetworkError currentVersion:current latestVersion:nil releaseURL:nil httpCode:0];
    }
    NSInteger code = [response isKindOfClass:[NSHTTPURLResponse class]] ? ((NSHTTPURLResponse *)response).statusCode : 0;
    if (code != 200) {
        return [VKUpdateResult resultWithStatus:VKUpdateStatusHTTPError currentVersion:current latestVersion:nil releaseURL:nil httpCode:code];
    }
    id obj = [NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
    if (![obj isKindOfClass:[NSArray class]]) {
        return [VKUpdateResult resultWithStatus:VKUpdateStatusBadData currentVersion:current latestVersion:nil releaseURL:nil httpCode:code];
    }
    // Newest non-draft release; pre-releases count (VisKey publishes its builds as pre-releases).
    NSDictionary *latest = nil;
    for (id rel in (NSArray *)obj) {
        if (![rel isKindOfClass:[NSDictionary class]]) continue;
        if ([rel[@"draft"] respondsToSelector:@selector(boolValue)] && [rel[@"draft"] boolValue]) continue;
        if (![rel[@"tag_name"] isKindOfClass:[NSString class]]) continue;
        latest = rel;
        break;
    }
    if (!latest) {
        return [VKUpdateResult resultWithStatus:VKUpdateStatusNoRelease currentVersion:current latestVersion:nil releaseURL:nil httpCode:code];
    }
    NSString *tag = latest[@"tag_name"];
    NSString *clean = [tag hasPrefix:@"v"] || [tag hasPrefix:@"V"] ? [tag substringFromIndex:1] : tag;
    if (VKCompareVersions(clean, current) == NSOrderedDescending) {
        id html = latest[@"html_url"];
        NSString *url = [html isKindOfClass:[NSString class]] ? html : [VKRepoURL stringByAppendingString:@"/releases"];
        return [VKUpdateResult resultWithStatus:VKUpdateStatusAvailable currentVersion:current latestVersion:clean releaseURL:url httpCode:code];
    }
    return [VKUpdateResult resultWithStatus:VKUpdateStatusUpToDate currentVersion:current latestVersion:nil releaseURL:nil httpCode:code];
}

void VKFetchLatestRelease(NSString *currentVersion, void (^completion)(VKUpdateResult *result)) {
    NSString *current = [currentVersion copy];
    NSURL *url = [NSURL URLWithString:kVKReleasesAPI];
    NSMutableURLRequest *req = [NSMutableURLRequest requestWithURL:url
                                                       cachePolicy:NSURLRequestReloadIgnoringLocalCacheData
                                                   timeoutInterval:kVKUpdateTimeout];
    req.HTTPShouldHandleCookies = NO;
    [req setValue:[NSString stringWithFormat:@"VisKey/%@ (macOS)", current] forHTTPHeaderField:@"User-Agent"];
    [req setValue:@"application/vnd.github+json" forHTTPHeaderField:@"Accept"];
    NSURLSessionConfiguration *cfg = [NSURLSessionConfiguration ephemeralSessionConfiguration];
    cfg.timeoutIntervalForRequest = kVKUpdateTimeout;
    cfg.timeoutIntervalForResource = kVKUpdateTimeout;
    cfg.HTTPShouldSetCookies = NO;
    cfg.HTTPCookieAcceptPolicy = NSHTTPCookieAcceptPolicyNever;
    cfg.HTTPCookieStorage = nil;
    cfg.URLCache = nil;
    // nil delegate queue: the completion handler runs on a background queue, parsing included.
    NSURLSession *session = [NSURLSession sessionWithConfiguration:cfg];
    NSURLSessionDataTask *task = [session dataTaskWithRequest:req completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
        VKUpdateResult *result = VKParseReleases(data, response, error, current);
        dispatch_async(dispatch_get_main_queue(), ^{
            completion(result);
        });
    }];
    [task resume];
    [session finishTasksAndInvalidate];
}

#pragma mark - Checker

@interface VKUpdateChecker ()
@property (nonatomic, weak) VKModel *model;
@property (nonatomic, strong, nullable) NSTimer *launchTimer;
@property (nonatomic, strong, nullable) NSTimer *timer;
@end

@implementation VKUpdateChecker

- (instancetype)initWithModel:(VKModel *)model {
    self = [super init];
    if (self) {
        _model = model;
    }
    return self;
}

- (void)dealloc {
    [self stop];
}

- (void)start {
    if (self.timer) return;
    __weak VKUpdateChecker *weakSelf = self;
    self.launchTimer = [NSTimer scheduledTimerWithTimeInterval:kVKAutoCheckLaunchDelay repeats:NO block:^(NSTimer *t) {
        [weakSelf autoCheckIfDue];
    }];
    self.launchTimer.tolerance = 10.0;
    self.timer = [NSTimer scheduledTimerWithTimeInterval:kVKAutoCheckTimerInterval repeats:YES block:^(NSTimer *t) {
        VKUpdateChecker *s = weakSelf;
        if (!s) { [t invalidate]; return; }
        [s autoCheckIfDue];
    }];
    self.timer.tolerance = 30 * 60.0;
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(modelChanged:)
                                                 name:VKModelDidChangeNotification object:self.model];
    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self selector:@selector(didWake:)
                                                               name:NSWorkspaceDidWakeNotification object:nil];
}

- (void)stop {
    [self.launchTimer invalidate];
    self.launchTimer = nil;
    [self.timer invalidate];
    self.timer = nil;
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [[[NSWorkspace sharedWorkspace] notificationCenter] removeObserver:self];
}

- (void)modelChanged:(NSNotification *)n {
    [self autoCheckIfDue];  // e.g. "Tự động kiểm tra cập nhật" just switched on
}

- (void)didWake:(NSNotification *)n {
    [self autoCheckIfDue];
}

- (void)autoCheckIfDue {
    VKModel *m = self.model;
    if (!m || !m.persistent || m.updateChecking) return;
    NSTimeInterval now = [NSDate date].timeIntervalSince1970;
    if (!vkupdate::ShouldAutoCheck(m.autoUpdateCheck, now, m.lastUpdateCheck)) return;
    [self runCheckAutomatic:YES];
}

- (void)checkNow {
    VKModel *m = self.model;
    if (!m || !m.persistent || m.updateChecking) return;
    [self runCheckAutomatic:NO];
}

- (void)runCheckAutomatic:(BOOL)automatic {
    VKModel *m = self.model;
    m.updateChecking = YES;
    // Record the attempt up front: it throttles the next automatic check whatever the outcome.
    m.lastUpdateCheck = [NSDate date].timeIntervalSince1970;
    [m saveUpdateState];
    [m notifyUpdateStateChanged];
    __weak VKUpdateChecker *weakSelf = self;
    VKFetchLatestRelease([m versionString], ^(VKUpdateResult *result) {
        [weakSelf finishCheck:result automatic:automatic];
    });
}

- (void)finishCheck:(VKUpdateResult *)result automatic:(BOOL)automatic {
    VKModel *m = self.model;
    if (!m) return;
    m.updateChecking = NO;
    m.lastUpdateResult = result;
    if (result.status == VKUpdateStatusAvailable) {
        m.availableUpdateVersion = result.latestVersion;
        m.availableUpdateURL = result.releaseURL;
        std::string latest = result.latestVersion.UTF8String ?: "";
        std::string notified = m.lastNotifiedVersion.UTF8String ?: "";
        if (vkupdate::ShouldNotify(true, latest, notified)) {
            // A manual check already shows the result on the page: remember the version, no badge.
            if (automatic) m.updateNoticePending = YES;
            m.lastNotifiedVersion = result.latestVersion;
        }
    } else if (result.status == VKUpdateStatusUpToDate || result.status == VKUpdateStatusNoRelease) {
        m.availableUpdateVersion = nil;
        m.availableUpdateURL = nil;
        m.updateNoticePending = NO;
    } else if (automatic) {
        NSLog(@"[VisKey] Automatic update check failed: %@", [result message]);
    }
    [m saveUpdateState];
    [m notifyUpdateStateChanged];
}

@end
