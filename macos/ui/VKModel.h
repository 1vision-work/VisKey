// VKModel.h — VisKey macOS settings/state store shared by the keyboard adapter and the UI.
// Persistent mode reads/writes NSUserDefaults; snapshot mode keeps everything in memory.
// MIT License, 1VISION.

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

extern NSNotificationName const VKModelDidChangeNotification;        // settings changed (saved)
extern NSNotificationName const VKMacroUsageDidChangeNotification;   // usage counters changed (coalesced)
extern NSNotificationName const VKEnvironmentDidChangeNotification;  // AX permission / current app / conflicts
extern NSNotificationName const VKUpdateStateDidChangeNotification;  // update check started / finished / notice seen

@class VKUpdateResult;

typedef NS_ENUM(NSInteger, VKAppMode) {
    VKAppModeVI = 0,
    VKAppModeEN = 1,
    VKAppModeRemember = 2
};

typedef NS_ENUM(NSInteger, VKRuleSource) {
    VKRuleSourceDefault = 0,  // "Mặc định"
    VKRuleSourceUser = 1,     // "Bạn đặt"
    VKRuleSourceAuto = 2      // "Tự ghi nhớ"
};

typedef NS_ENUM(NSInteger, VKNewAppPolicy) {
    VKNewAppPolicyRemember = 0,  // "Nhớ lần cuối"
    VKNewAppPolicyVI = 1,        // "Luôn VI"
    VKNewAppPolicyEN = 2         // "Luôn EN"
};

typedef NS_ENUM(NSInteger, VKSettingsPage) {
    VKSettingsPageGeneral = 0,
    VKSettingsPageMacros = 1,
    VKSettingsPageApps = 2,
    VKSettingsPageShortcuts = 3,
    VKSettingsPagePrivacy = 4,
    VKSettingsPageAbout = 5
};

#pragma mark - Hotkey

@interface VKHotkey : NSObject <NSCopying>
@property (nonatomic, readonly) NSInteger keyCode;     // virtual key code, -1 = none
@property (nonatomic, readonly) NSUInteger modifiers;  // subset of NSEventModifierFlagControl|Option|Shift|Command
+ (instancetype)none;
+ (instancetype)hotkeyWithKeyCode:(NSInteger)keyCode modifiers:(NSUInteger)modifiers;
+ (instancetype)hotkeyFromPlist:(nullable id)plist fallback:(VKHotkey *)fallback;
+ (NSUInteger)relevantModifierMask;
+ (NSString *)nameForKeyCode:(NSInteger)keyCode;
- (NSDictionary *)plist;
- (BOOL)isEmpty;
- (BOOL)isEqualToHotkey:(nullable VKHotkey *)other;
- (NSArray<NSString *> *)keycapLabels;  // @[@"⌃", @"Space"]
- (NSString *)shortString;              // "⌃ Space"
- (NSString *)longString;               // "⌃ Control + Space"
- (NSString *)keyName;                  // "Space"
@end

#pragma mark - Per-app rule

@interface VKAppRule : NSObject
@property (nonatomic, copy) NSString *bundleId;
@property (nonatomic, copy, nullable) NSString *name;
@property (nonatomic) VKAppMode mode;
@property (nonatomic) VKRuleSource source;
@property (nonatomic, strong, nullable) NSNumber *lastVietnamese;  // remembered value (BOOL)
@property (nonatomic) NSTimeInterval addedAt;
+ (instancetype)ruleWithBundleId:(NSString *)bundleId name:(nullable NSString *)name mode:(VKAppMode)mode source:(VKRuleSource)source;
+ (nullable instancetype)ruleFromPlist:(id)plist bundleId:(NSString *)bundleId;
- (NSDictionary *)plist;
/// Value this rule forces when the app becomes active, or nil (no opinion).
- (nullable NSNumber *)effectiveVietnamese;
@end

#pragma mark - Actions implemented by the app controller

@protocol VKAppActions <NSObject>
- (void)vk_setVietnamese:(BOOL)vietnamese;      // explicit user switch from UI (HUD + per-app memory)
- (void)vk_openSettingsPage:(VKSettingsPage)page;
- (void)vk_setLaunchAtLogin:(BOOL)on;
- (void)vk_setRuleForCurrentApp:(VKAppMode)mode;
- (void)vk_requestAccessibility;               // prompt + open System Settings
- (void)vk_hotkeyRecordingChanged:(BOOL)recording;
- (void)vk_closePopover;
- (void)vk_quit;
- (void)vk_checkForUpdates;                    // "Kiểm tra cập nhật" button
- (void)vk_openUpdateDownload;                 // open the release page of the available update

@end

#pragma mark - Model

@interface VKModel : NSObject

+ (VKModel *)shared;
+ (void)setShared:(VKModel *)model;

- (instancetype)initPersistent:(BOOL)persistent defaultMacros:(NSDictionary<NSString *, NSString *> *)defaultMacros;

@property (nonatomic, readonly) BOOL persistent;
@property (nonatomic, weak, nullable) id<VKAppActions> actions;

// Typing state & options
@property (nonatomic) BOOL vietnameseEnabled;
@property (nonatomic, copy) NSString *inputMethod;  // "telex" | "vni"
@property (nonatomic) BOOL modernOrthography;
@property (nonatomic) BOOL autoRestoreEnglish;
@property (nonatomic) BOOL fixAutocomplete;
@property (nonatomic) BOOL enableMacros;
@property (nonatomic) BOOL quickTelex;
@property (nonatomic) BOOL smartPerAppMode;
@property (nonatomic) BOOL launchAtLogin;
@property (nonatomic) BOOL showHUD;
@property (nonatomic) BOOL onboardingDone;

// Update check. Persisted: autoUpdateCheck (default YES), lastUpdateCheck, lastNotifiedVersion.
@property (nonatomic) BOOL autoUpdateCheck;
@property (nonatomic) NSTimeInterval lastUpdateCheck;  // seconds since 1970 of the last attempt (any outcome), 0 = never
@property (nonatomic, copy, nullable) NSString *lastNotifiedVersion;  // newest version already announced
// Runtime only (never saved)
@property (nonatomic) BOOL updateChecking;
@property (nonatomic, strong, nullable) VKUpdateResult *lastUpdateResult;
@property (nonatomic, copy, nullable) NSString *availableUpdateVersion;  // newer release found this session
@property (nonatomic, copy, nullable) NSString *availableUpdateURL;
@property (nonatomic) BOOL updateNoticePending;  // sidebar dot + popover line until seen

// Macros
@property (nonatomic, strong) NSMutableDictionary<NSString *, NSString *> *macros;
@property (nonatomic, strong) NSMutableDictionary<NSString *, NSNumber *> *macroUsage;

// Per-app
@property (nonatomic, strong) NSMutableDictionary<NSString *, VKAppRule *> *appRules;
@property (nonatomic) VKNewAppPolicy unknownAppPolicy;

// Hotkeys
@property (nonatomic, strong) VKHotkey *primaryHotkey;
@property (nonatomic, strong) VKHotkey *secondaryHotkey;
@property (nonatomic, strong) VKHotkey *methodHotkey;
@property (nonatomic) BOOL hotkeyRecording;

// Environment (live values in normal mode, demo values in snapshot mode)
@property (nonatomic, copy, nullable) NSString *currentAppBundleId;  // last frontmost app other than VisKey
@property (nonatomic, copy, nullable) NSString *currentAppName;
@property (nonatomic, strong, nullable) NSNumber *demoAccessibilityTrusted;
@property (nonatomic, strong, nullable) NSNumber *demoSystemCtrlSpace;
@property (nonatomic, strong, nullable) NSNumber *demoSecureInput;
@property (nonatomic, strong, nullable) NSNumber *demoRunningFrontmost;

- (void)load;
- (void)save;
/// Save (if persistent) and post VKModelDidChangeNotification.
- (void)commit;
- (void)notifyEnvironmentChanged;
/// Saves only the update-check keys (if persistent).
- (void)saveUpdateState;
- (void)notifyUpdateStateChanged;
- (void)loadDemoData;

// Macros
- (NSArray<NSString *> *)macroKeysSortedByUsage:(BOOL)byUsage filter:(nullable NSString *)filter;
- (NSInteger)usageForMacro:(NSString *)key;
- (void)recordMacroUse:(NSString *)key;
+ (nullable NSString *)validationErrorForMacroKey:(NSString *)key value:(NSString *)value;
/// Adds / replaces a macro. Returns a Vietnamese error message or nil on success (does not commit).
- (nullable NSString *)setMacroKey:(NSString *)key value:(NSString *)value;
- (void)removeMacro:(NSString *)key;
/// Imports JSON ({"version":1,"macros":{...}}) or "key<TAB>value" lines. Returns the number imported.
- (NSInteger)importMacrosFromData:(NSData *)data errors:(NSMutableArray<NSString *> *)errors;
- (nullable NSData *)exportMacrosData;

// Per-app rules
+ (NSDictionary<NSString *, NSString *> *)defaultEnglishApps;
- (NSArray<VKAppRule *> *)sortedRules;
- (nullable VKAppRule *)ruleForBundle:(nullable NSString *)bundleId;
- (void)setRule:(VKAppRule *)rule;
- (void)removeRuleForBundle:(NSString *)bundleId;
- (void)restoreDefaultRules;
+ (NSString *)displayNameForBundle:(NSString *)bundleId fallback:(nullable NSString *)fallback;
+ (nullable NSImage *)iconForBundle:(NSString *)bundleId;

// Environment queries
- (BOOL)accessibilityTrusted;
- (BOOL)systemUsesCtrlSpace;     // macOS "Select the previous input source" = ⌃Space and enabled
- (BOOL)hasHotkeyConflict;
- (BOOL)secureInputActive;
- (BOOL)isRunningFrontmost:(NSString *)bundleId;

// Text helpers
- (BOOL)isTelex;
- (NSString *)methodDisplayName;
- (NSString *)statusSubtitle;
- (NSString *)perAppSummary;
- (NSString *)versionString;
- (NSString *)hudHint;
/// "Lần kiểm tra cuối: 08:30 19/10/2026", or nil if never checked.
- (nullable NSString *)lastUpdateCheckText;

@end

NS_ASSUME_NONNULL_END
