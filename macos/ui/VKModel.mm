// VKModel.mm — see VKModel.h. MIT License, 1VISION.

#import "VKModel.h"
#import "VKUpdate.h"
#import <ApplicationServices/ApplicationServices.h>
#import <Carbon/Carbon.h>

NSNotificationName const VKModelDidChangeNotification = @"VKModelDidChangeNotification";
NSNotificationName const VKMacroUsageDidChangeNotification = @"VKMacroUsageDidChangeNotification";
NSNotificationName const VKEnvironmentDidChangeNotification = @"VKEnvironmentDidChangeNotification";
NSNotificationName const VKUpdateStateDidChangeNotification = @"VKUpdateStateDidChangeNotification";

static const NSInteger kVKKeySpace = 49;
static const NSUInteger kVKMaxMacroKeyLength = 16;
static const NSUInteger kVKMaxMacroValueLength = 300;

#pragma mark - VKHotkey

@interface VKHotkey ()
@property (nonatomic, readwrite) NSInteger keyCode;
@property (nonatomic, readwrite) NSUInteger modifiers;
@end

@implementation VKHotkey

+ (NSUInteger)relevantModifierMask {
    return NSEventModifierFlagControl | NSEventModifierFlagOption | NSEventModifierFlagShift | NSEventModifierFlagCommand;
}

+ (instancetype)none {
    return [self hotkeyWithKeyCode:-1 modifiers:0];
}

+ (instancetype)hotkeyWithKeyCode:(NSInteger)keyCode modifiers:(NSUInteger)modifiers {
    VKHotkey *h = [[VKHotkey alloc] init];
    h.keyCode = keyCode < 0 ? -1 : keyCode;
    h.modifiers = keyCode < 0 ? 0 : (modifiers & [self relevantModifierMask]);
    return h;
}

+ (instancetype)hotkeyFromPlist:(id)plist fallback:(VKHotkey *)fallback {
    if (![plist isKindOfClass:[NSDictionary class]]) return fallback;
    NSDictionary *d = plist;
    NSNumber *kc = d[@"keyCode"];
    NSNumber *mods = d[@"modifiers"];
    if (![kc isKindOfClass:[NSNumber class]]) return fallback;
    NSUInteger m = [mods isKindOfClass:[NSNumber class]] ? mods.unsignedIntegerValue : 0;
    return [self hotkeyWithKeyCode:kc.integerValue modifiers:m];
}

- (id)copyWithZone:(NSZone *)zone {
    return [VKHotkey hotkeyWithKeyCode:self.keyCode modifiers:self.modifiers];
}

- (NSDictionary *)plist {
    return @{@"keyCode": @(self.keyCode), @"modifiers": @(self.modifiers)};
}

- (BOOL)isEmpty {
    return self.keyCode < 0;
}

- (BOOL)isEqualToHotkey:(VKHotkey *)other {
    if (!other) return NO;
    if (self.isEmpty || other.isEmpty) return self.isEmpty && other.isEmpty;
    return self.keyCode == other.keyCode && self.modifiers == other.modifiers;
}

+ (NSString *)nameForKeyCode:(NSInteger)keyCode {
    static NSDictionary<NSNumber *, NSString *> *names = nil;
    if (!names) {
        // ANSI virtual key codes (HIToolbox kVK_*).
        names = @{
            @0: @"A", @1: @"S", @2: @"D", @3: @"F", @4: @"H", @5: @"G", @6: @"Z", @7: @"X", @8: @"C", @9: @"V",
            @11: @"B", @12: @"Q", @13: @"W", @14: @"E", @15: @"R", @16: @"Y", @17: @"T",
            @18: @"1", @19: @"2", @20: @"3", @21: @"4", @22: @"6", @23: @"5", @24: @"=", @25: @"9", @26: @"7",
            @27: @"-", @28: @"8", @29: @"0", @30: @"]", @31: @"O", @32: @"U", @33: @"[", @34: @"I", @35: @"P",
            @36: @"Return", @37: @"L", @38: @"J", @39: @"'", @40: @"K", @41: @";", @42: @"\\", @43: @",",
            @44: @"/", @45: @"N", @46: @"M", @47: @".", @48: @"Tab", @49: @"Space", @50: @"`", @51: @"⌫",
            @53: @"Esc", @117: @"⌦", @115: @"Home", @119: @"End", @116: @"PgUp", @121: @"PgDn",
            @123: @"←", @124: @"→", @125: @"↓", @126: @"↑",
            @122: @"F1", @120: @"F2", @99: @"F3", @118: @"F4", @96: @"F5", @97: @"F6", @98: @"F7",
            @100: @"F8", @101: @"F9", @109: @"F10", @103: @"F11", @111: @"F12"
        };
    }
    NSString *n = names[@(keyCode)];
    return n ?: [NSString stringWithFormat:@"#%ld", (long)keyCode];
}

- (NSString *)keyName {
    return self.isEmpty ? @"" : [VKHotkey nameForKeyCode:self.keyCode];
}

- (NSArray<NSString *> *)modifierGlyphs {
    NSMutableArray *a = [NSMutableArray array];
    if (self.modifiers & NSEventModifierFlagControl) [a addObject:@"⌃"];
    if (self.modifiers & NSEventModifierFlagOption) [a addObject:@"⌥"];
    if (self.modifiers & NSEventModifierFlagShift) [a addObject:@"⇧"];
    if (self.modifiers & NSEventModifierFlagCommand) [a addObject:@"⌘"];
    return a;
}

- (NSArray<NSString *> *)keycapLabels {
    if (self.isEmpty) return @[];
    NSMutableArray *a = [[self modifierGlyphs] mutableCopy];
    [a addObject:[self keyName]];
    return a;
}

- (NSString *)shortString {
    return [[self keycapLabels] componentsJoinedByString:@" "];
}

- (NSString *)longString {
    if (self.isEmpty) return @"";
    NSMutableArray *parts = [NSMutableArray array];
    if (self.modifiers & NSEventModifierFlagControl) [parts addObject:@"⌃ Control"];
    if (self.modifiers & NSEventModifierFlagOption) [parts addObject:@"⌥ Option"];
    if (self.modifiers & NSEventModifierFlagShift) [parts addObject:@"⇧ Shift"];
    if (self.modifiers & NSEventModifierFlagCommand) [parts addObject:@"⌘ Command"];
    [parts addObject:[self keyName]];
    return [parts componentsJoinedByString:@" + "];
}

@end

#pragma mark - VKAppRule

static NSString *VKModeString(VKAppMode m) {
    switch (m) {
        case VKAppModeVI: return @"vi";
        case VKAppModeEN: return @"en";
        case VKAppModeRemember: return @"remember";
    }
    return @"remember";
}

static NSString *VKSourceString(VKRuleSource s) {
    switch (s) {
        case VKRuleSourceDefault: return @"default";
        case VKRuleSourceUser: return @"user";
        case VKRuleSourceAuto: return @"auto";
    }
    return @"user";
}

@implementation VKAppRule

+ (instancetype)ruleWithBundleId:(NSString *)bundleId name:(NSString *)name mode:(VKAppMode)mode source:(VKRuleSource)source {
    VKAppRule *r = [[VKAppRule alloc] init];
    r.bundleId = bundleId;
    r.name = name;
    r.mode = mode;
    r.source = source;
    r.addedAt = [NSDate date].timeIntervalSince1970;
    return r;
}

+ (instancetype)ruleFromPlist:(id)plist bundleId:(NSString *)bundleId {
    if (![plist isKindOfClass:[NSDictionary class]] || bundleId.length == 0) return nil;
    NSDictionary *d = plist;
    VKAppRule *r = [[VKAppRule alloc] init];
    r.bundleId = bundleId;
    NSString *mode = [d[@"mode"] isKindOfClass:[NSString class]] ? d[@"mode"] : @"remember";
    r.mode = [mode isEqualToString:@"vi"] ? VKAppModeVI : ([mode isEqualToString:@"en"] ? VKAppModeEN : VKAppModeRemember);
    NSString *src = [d[@"source"] isKindOfClass:[NSString class]] ? d[@"source"] : @"user";
    r.source = [src isEqualToString:@"default"] ? VKRuleSourceDefault : ([src isEqualToString:@"auto"] ? VKRuleSourceAuto : VKRuleSourceUser);
    NSString *last = [d[@"last"] isKindOfClass:[NSString class]] ? d[@"last"] : nil;
    if (last) r.lastVietnamese = @([last isEqualToString:@"vi"]);
    if ([d[@"name"] isKindOfClass:[NSString class]]) r.name = d[@"name"];
    if ([d[@"addedAt"] isKindOfClass:[NSNumber class]]) r.addedAt = [d[@"addedAt"] doubleValue];
    return r;
}

- (NSDictionary *)plist {
    NSMutableDictionary *d = [NSMutableDictionary dictionary];
    d[@"mode"] = VKModeString(self.mode);
    d[@"source"] = VKSourceString(self.source);
    if (self.lastVietnamese) d[@"last"] = self.lastVietnamese.boolValue ? @"vi" : @"en";
    if (self.name.length) d[@"name"] = self.name;
    d[@"addedAt"] = @(self.addedAt);
    return d;
}

- (NSNumber *)effectiveVietnamese {
    switch (self.mode) {
        case VKAppModeVI: return @YES;
        case VKAppModeEN: return @NO;
        case VKAppModeRemember: return self.lastVietnamese;
    }
    return nil;
}

@end

#pragma mark - VKModel

static VKModel *gSharedModel = nil;

@interface VKModel ()
@property (nonatomic, readwrite) BOOL persistent;
@property (nonatomic, copy) NSDictionary<NSString *, NSString *> *defaultMacros;
@end

@implementation VKModel

+ (VKModel *)shared {
    return gSharedModel;
}

+ (void)setShared:(VKModel *)model {
    gSharedModel = model;
}

- (instancetype)initPersistent:(BOOL)persistent defaultMacros:(NSDictionary<NSString *, NSString *> *)defaultMacros {
    self = [super init];
    if (self) {
        _persistent = persistent;
        _defaultMacros = [defaultMacros copy];
        [self applyDefaults];
    }
    return self;
}

- (void)applyDefaults {
    _vietnameseEnabled = YES;
    _inputMethod = @"telex";
    _modernOrthography = NO;
    _autoRestoreEnglish = YES;
    _fixAutocomplete = YES;
    _enableMacros = YES;
    _quickTelex = NO;
    _smartPerAppMode = YES;
    _launchAtLogin = NO;
    _showHUD = YES;
    _onboardingDone = NO;
    _autoUpdateCheck = YES;
    _lastUpdateCheck = 0;
    _lastNotifiedVersion = nil;
    _macros = [self.defaultMacros mutableCopy] ?: [NSMutableDictionary dictionary];
    _macroUsage = [NSMutableDictionary dictionary];
    _appRules = [NSMutableDictionary dictionary];
    _unknownAppPolicy = VKNewAppPolicyRemember;
    _primaryHotkey = [VKHotkey hotkeyWithKeyCode:kVKKeySpace modifiers:NSEventModifierFlagControl];
    _secondaryHotkey = [VKHotkey none];
    _methodHotkey = [VKHotkey none];
    [self seedDefaultRules];
}

- (void)seedDefaultRules {
    NSDictionary<NSString *, NSString *> *defs = [VKModel defaultEnglishApps];
    NSTimeInterval t = 1.0;
    for (NSString *bundle in @[@"com.apple.Terminal", @"com.googlecode.iterm2", @"dev.warp.Warp-Stable", @"io.alacritty", @"net.kovidgoyal.kitty"]) {
        if (self.appRules[bundle]) continue;
        VKAppRule *r = [VKAppRule ruleWithBundleId:bundle name:defs[bundle] mode:VKAppModeEN source:VKRuleSourceDefault];
        r.addedAt = t;
        t += 1.0;
        self.appRules[bundle] = r;
    }
}

static BOOL VKBool(NSUserDefaults *defs, NSString *key, BOOL fallback) {
    id v = [defs objectForKey:key];
    return v ? [defs boolForKey:key] : fallback;
}

- (void)load {
    if (!self.persistent) return;
    NSUserDefaults *defs = [NSUserDefaults standardUserDefaults];
    [self applyDefaults];
    // Same first-run rule as VisKey 1.0: until "vietnameseEnabled" exists, keep the defaults.
    if ([defs objectForKey:@"vietnameseEnabled"] != nil) {
        _vietnameseEnabled = [defs boolForKey:@"vietnameseEnabled"];
        _smartPerAppMode = [defs boolForKey:@"smartPerAppMode"];
        _launchAtLogin = [defs boolForKey:@"launchAtLogin"];
        NSString *im = [defs stringForKey:@"inputMethod"];
        _inputMethod = [im isEqualToString:@"vni"] ? @"vni" : @"telex";
        _modernOrthography = [defs boolForKey:@"modernOrthography"];
        _autoRestoreEnglish = [defs boolForKey:@"autoRestoreEnglish"];
        _fixAutocomplete = [defs boolForKey:@"fixAutocomplete"];
        _enableMacros = [defs boolForKey:@"enableMacros"];
        _quickTelex = [defs boolForKey:@"quickTelex"];
        NSDictionary *saved = [defs dictionaryForKey:@"customMacros"];
        if (saved) {
            NSMutableDictionary *m = [NSMutableDictionary dictionary];
            for (id k in saved) {
                id v = saved[k];
                if ([k isKindOfClass:[NSString class]] && [v isKindOfClass:[NSString class]]) m[k] = v;
            }
            _macros = m;
        }
    }
    _showHUD = VKBool(defs, @"showHUD", YES);
    _onboardingDone = VKBool(defs, @"onboardingDone", NO);
    _autoUpdateCheck = VKBool(defs, @"autoUpdateCheck", YES);
    _lastUpdateCheck = [defs doubleForKey:@"lastUpdateCheck"];
    _lastNotifiedVersion = [[defs stringForKey:@"lastNotifiedVersion"] copy];

    NSDictionary *usage = [defs dictionaryForKey:@"macroUsage"];
    for (id k in usage) {
        id v = usage[k];
        if ([k isKindOfClass:[NSString class]] && [v isKindOfClass:[NSNumber class]]) _macroUsage[k] = v;
    }

    NSDictionary *rules = [defs dictionaryForKey:@"appRules"];
    if (rules) {
        [_appRules removeAllObjects];
        for (id k in rules) {
            if (![k isKindOfClass:[NSString class]]) continue;
            VKAppRule *r = [VKAppRule ruleFromPlist:rules[k] bundleId:k];
            if (r) _appRules[k] = r;
        }
    }
    NSString *policy = [defs stringForKey:@"newAppPolicy"];
    if ([policy isEqualToString:@"vi"]) _unknownAppPolicy = VKNewAppPolicyVI;
    else if ([policy isEqualToString:@"en"]) _unknownAppPolicy = VKNewAppPolicyEN;
    else _unknownAppPolicy = VKNewAppPolicyRemember;

    _primaryHotkey = [VKHotkey hotkeyFromPlist:[defs objectForKey:@"hotkeyPrimary"] fallback:_primaryHotkey];
    _secondaryHotkey = [VKHotkey hotkeyFromPlist:[defs objectForKey:@"hotkeySecondary"] fallback:_secondaryHotkey];
    _methodHotkey = [VKHotkey hotkeyFromPlist:[defs objectForKey:@"hotkeyMethod"] fallback:_methodHotkey];
}

- (void)save {
    if (!self.persistent) return;
    NSUserDefaults *defs = [NSUserDefaults standardUserDefaults];
    [defs setBool:self.vietnameseEnabled forKey:@"vietnameseEnabled"];
    [defs setBool:self.smartPerAppMode forKey:@"smartPerAppMode"];
    [defs setBool:self.launchAtLogin forKey:@"launchAtLogin"];
    [defs setObject:self.inputMethod forKey:@"inputMethod"];
    [defs setBool:self.modernOrthography forKey:@"modernOrthography"];
    [defs setBool:self.autoRestoreEnglish forKey:@"autoRestoreEnglish"];
    [defs setBool:self.fixAutocomplete forKey:@"fixAutocomplete"];
    [defs setBool:self.enableMacros forKey:@"enableMacros"];
    [defs setBool:self.quickTelex forKey:@"quickTelex"];
    [defs setObject:[self.macros copy] forKey:@"customMacros"];
    [defs setBool:self.showHUD forKey:@"showHUD"];
    [defs setBool:self.onboardingDone forKey:@"onboardingDone"];
    [defs setObject:[self.macroUsage copy] forKey:@"macroUsage"];

    NSMutableDictionary *rules = [NSMutableDictionary dictionary];
    for (NSString *k in self.appRules) rules[k] = [self.appRules[k] plist];
    [defs setObject:rules forKey:@"appRules"];
    NSString *policy = self.unknownAppPolicy == VKNewAppPolicyVI ? @"vi" : (self.unknownAppPolicy == VKNewAppPolicyEN ? @"en" : @"remember");
    [defs setObject:policy forKey:@"newAppPolicy"];
    [defs setObject:[self.primaryHotkey plist] forKey:@"hotkeyPrimary"];
    [defs setObject:[self.secondaryHotkey plist] forKey:@"hotkeySecondary"];
    [defs setObject:[self.methodHotkey plist] forKey:@"hotkeyMethod"];
    [self saveUpdateState];
}

- (void)saveUpdateState {
    if (!self.persistent) return;
    NSUserDefaults *defs = [NSUserDefaults standardUserDefaults];
    [defs setBool:self.autoUpdateCheck forKey:@"autoUpdateCheck"];
    [defs setDouble:self.lastUpdateCheck forKey:@"lastUpdateCheck"];
    if (self.lastNotifiedVersion.length) [defs setObject:self.lastNotifiedVersion forKey:@"lastNotifiedVersion"];
    else [defs removeObjectForKey:@"lastNotifiedVersion"];
}

- (void)commit {
    [self save];
    [[NSNotificationCenter defaultCenter] postNotificationName:VKModelDidChangeNotification object:self];
}

- (void)notifyEnvironmentChanged {
    [[NSNotificationCenter defaultCenter] postNotificationName:VKEnvironmentDidChangeNotification object:self];
}

- (void)notifyUpdateStateChanged {
    [[NSNotificationCenter defaultCenter] postNotificationName:VKUpdateStateDidChangeNotification object:self];
}

- (void)loadDemoData {
    [self applyDefaults];
    _launchAtLogin = YES;
    NSArray *demo = @[
        @[@"bt", @"bình thường", @212], @[@"dc", @"được", @1038], @[@"ko", @"không", @2417],
        @[@"mn", @"mọi người", @386], @[@"ntn", @"như thế nào", @154], @[@"uh", @"ừ", @97],
        @[@"vn", @"Việt Nam", @640], @[@"bn", @"bao nhiêu", @58], @[@"cg", @"cũng", @311],
        @[@"dt", @"điện thoại", @42], @[@"gd", @"gia đình", @27], @[@"hcm", @"Hồ Chí Minh", @19],
        @[@"hn", @"Hà Nội", @33], @[@"ht", @"hệ thống", @61], @[@"kq", @"kết quả", @24],
        @[@"lm", @"làm", @205], @[@"nt", @"nhắn tin", @16], @[@"qt", @"quan trọng", @48],
        @[@"sn", @"sinh nhật", @9], @[@"tg", @"thời gian", @73], @[@"tks", @"cảm ơn", @129],
        @[@"tl", @"trả lời", @37], @[@"vd", @"ví dụ", @88], @[@"vs", @"với", @264]
    ];
    [_macros removeAllObjects];
    [_macroUsage removeAllObjects];
    for (NSArray *row in demo) {
        _macros[row[0]] = row[1];
        _macroUsage[row[0]] = row[2];
    }

    [_appRules removeAllObjects];
    VKAppRule *(^add)(NSString *, NSString *, VKAppMode, VKRuleSource, NSNumber *) =
        ^VKAppRule *(NSString *bundle, NSString *name, VKAppMode mode, VKRuleSource src, NSNumber *last) {
            VKAppRule *r = [VKAppRule ruleWithBundleId:bundle name:name mode:mode source:src];
            r.lastVietnamese = last;
            r.addedAt = (NSTimeInterval)(self.appRules.count + 1);
            self.appRules[bundle] = r;
            return r;
        };
    add(@"com.apple.Terminal", @"Terminal", VKAppModeEN, VKRuleSourceDefault, nil);
    add(@"com.googlecode.iterm2", @"iTerm2", VKAppModeEN, VKRuleSourceDefault, nil);
    add(@"com.microsoft.VSCode", @"Trình soạn code", VKAppModeEN, VKRuleSourceUser, nil);
    add(@"com.apple.Safari", @"Safari", VKAppModeRemember, VKRuleSourceAuto, @YES);
    add(@"com.apple.Notes", @"Ghi chú", VKAppModeVI, VKRuleSourceUser, nil);
    add(@"com.tinyspeck.slackmacgap", @"Ứng dụng chat", VKAppModeRemember, VKRuleSourceAuto, @YES);
    add(@"dev.warp.Warp-Stable", @"Warp", VKAppModeEN, VKRuleSourceDefault, nil);
    add(@"io.alacritty", @"Alacritty", VKAppModeEN, VKRuleSourceDefault, nil);
    add(@"net.kovidgoyal.kitty", @"Kitty", VKAppModeEN, VKRuleSourceDefault, nil);

    self.currentAppBundleId = @"com.apple.Safari";
    self.currentAppName = @"Safari";
    self.demoAccessibilityTrusted = @NO;
    self.demoSystemCtrlSpace = @YES;
    self.demoSecureInput = @NO;
    self.demoRunningFrontmost = @YES;

    // Fixed demo stamp (19/10/2026 08:30 local time) and result; nothing is fetched in demo mode.
    NSDateComponents *dc = [[NSDateComponents alloc] init];
    dc.year = 2026;
    dc.month = 10;
    dc.day = 19;
    dc.hour = 8;
    dc.minute = 30;
    NSDate *demoDate = [[NSCalendar calendarWithIdentifier:NSCalendarIdentifierGregorian] dateFromComponents:dc];
    _lastUpdateCheck = demoDate ? demoDate.timeIntervalSince1970 : 0;
    self.updateChecking = NO;
    self.lastUpdateResult = [VKUpdateResult resultWithStatus:VKUpdateStatusUpToDate currentVersion:[self versionString]
                                               latestVersion:nil releaseURL:nil httpCode:200];
    self.availableUpdateVersion = nil;
    self.availableUpdateURL = nil;
    self.updateNoticePending = NO;
}

#pragma mark Macros

- (NSArray<NSString *> *)macroKeysSortedByUsage:(BOOL)byUsage filter:(NSString *)filter {
    NSString *q = [filter stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
    NSMutableArray<NSString *> *keys = [NSMutableArray array];
    NSStringCompareOptions opts = NSCaseInsensitiveSearch | NSDiacriticInsensitiveSearch;
    for (NSString *k in self.macros) {
        if (q.length) {
            NSString *v = self.macros[k];
            if ([k rangeOfString:q options:opts].location == NSNotFound &&
                [v rangeOfString:q options:opts].location == NSNotFound) continue;
        }
        [keys addObject:k];
    }
    [keys sortUsingComparator:^NSComparisonResult(NSString *a, NSString *b) {
        if (byUsage) {
            NSInteger ua = [self usageForMacro:a], ub = [self usageForMacro:b];
            if (ua != ub) return ua > ub ? NSOrderedAscending : NSOrderedDescending;
        }
        return [a localizedStandardCompare:b];
    }];
    return keys;
}

- (NSInteger)usageForMacro:(NSString *)key {
    return self.macroUsage[key].integerValue;
}

- (void)recordMacroUse:(NSString *)key {
    if (key.length == 0 || !self.macros[key]) return;
    self.macroUsage[key] = @([self usageForMacro:key] + 1);
    if (self.persistent) {
        [[NSUserDefaults standardUserDefaults] setObject:[self.macroUsage copy] forKey:@"macroUsage"];
    }
    NSNotification *n = [NSNotification notificationWithName:VKMacroUsageDidChangeNotification object:self];
    [[NSNotificationQueue defaultQueue] enqueueNotification:n
                                               postingStyle:NSPostWhenIdle
                                               coalesceMask:NSNotificationCoalescingOnName
                                                   forModes:nil];
}

+ (NSString *)validationErrorForMacroKey:(NSString *)key value:(NSString *)value {
    if (key.length == 0) return @"Hãy nhập chữ viết tắt.";
    if (key.length > kVKMaxMacroKeyLength) return @"Viết tắt tối đa 16 ký tự.";
    NSCharacterSet *letters = [NSCharacterSet characterSetWithCharactersInString:
                               @"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"];
    if ([key rangeOfCharacterFromSet:[letters invertedSet]].location != NSNotFound) {
        return @"Viết tắt chỉ gồm chữ cái a–z không dấu, không khoảng trắng.";
    }
    if (value.length == 0) return @"Nội dung thay thế không được để trống.";
    if (value.length > kVKMaxMacroValueLength) return @"Nội dung quá dài (tối đa 300 ký tự).";
    if ([value rangeOfCharacterFromSet:[NSCharacterSet characterSetWithCharactersInString:@"\t\r\n"]].location != NSNotFound) {
        return @"Nội dung không được chứa TAB hoặc xuống dòng.";
    }
    return nil;
}

- (NSString *)setMacroKey:(NSString *)key value:(NSString *)value {
    NSString *k = [[key stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]] lowercaseString];
    NSString *v = [value stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
    NSString *err = [VKModel validationErrorForMacroKey:k value:v];
    if (err) return err;
    self.macros[k] = v;
    return nil;
}

- (void)removeMacro:(NSString *)key {
    [self.macros removeObjectForKey:key];
    [self.macroUsage removeObjectForKey:key];
}

- (NSInteger)importMacrosFromData:(NSData *)data errors:(NSMutableArray<NSString *> *)errors {
    NSString *text = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
    if (!text) {
        [errors addObject:@"Tệp không phải văn bản UTF-8."];
        return 0;
    }
    if ([text hasPrefix:@"﻿"]) text = [text substringFromIndex:1];
    NSString *trimmed = [text stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
    NSInteger imported = 0;

    if ([trimmed hasPrefix:@"{"]) {
        NSError *jsonErr = nil;
        id obj = [NSJSONSerialization JSONObjectWithData:[trimmed dataUsingEncoding:NSUTF8StringEncoding] options:0 error:&jsonErr];
        if (![obj isKindOfClass:[NSDictionary class]]) {
            [errors addObject:@"Tệp JSON không hợp lệ."];
            return 0;
        }
        NSDictionary *root = obj;
        id version = root[@"version"];
        if ([version isKindOfClass:[NSNumber class]] && [version integerValue] > 1) {
            [errors addObject:[NSString stringWithFormat:@"Tệp dùng định dạng phiên bản %ld, VisKey này chỉ đọc được phiên bản 1.", (long)[version integerValue]]];
            return 0;
        }
        id macros = root[@"macros"];
        if (![macros isKindOfClass:[NSDictionary class]]) {
            [errors addObject:@"Tệp JSON thiếu mục \"macros\"."];
            return 0;
        }
        NSDictionary *m = macros;
        NSArray *keys = [[m allKeys] sortedArrayUsingSelector:@selector(compare:)];
        for (id k in keys) {
            id v = m[k];
            if (![k isKindOfClass:[NSString class]] || ![v isKindOfClass:[NSString class]]) {
                [errors addObject:[NSString stringWithFormat:@"“%@”: nội dung phải là chuỗi.", k]];
                continue;
            }
            NSString *err = [self setMacroKey:k value:v];
            if (err) [errors addObject:[NSString stringWithFormat:@"“%@”: %@", k, err]];
            else imported++;
        }
        return imported;
    }

    NSArray<NSString *> *lines = [text componentsSeparatedByCharactersInSet:[NSCharacterSet newlineCharacterSet]];
    NSInteger lineNo = 0;
    for (NSString *raw in lines) {
        lineNo++;
        NSString *line = [raw stringByTrimmingCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@"  "]];
        if (line.length == 0 || [line hasPrefix:@"#"]) continue;
        NSRange tab = [line rangeOfString:@"\t"];
        if (tab.location == NSNotFound) {
            [errors addObject:[NSString stringWithFormat:@"Dòng %ld: thiếu ký tự TAB giữa viết tắt và nội dung.", (long)lineNo]];
            continue;
        }
        NSString *k = [line substringToIndex:tab.location];
        NSString *v = [line substringFromIndex:NSMaxRange(tab)];
        NSString *err = [self setMacroKey:k value:v];
        if (err) [errors addObject:[NSString stringWithFormat:@"Dòng %ld: %@", (long)lineNo, err]];
        else imported++;
    }
    if (imported == 0 && errors.count == 0) [errors addObject:@"Không tìm thấy cụm từ nào trong tệp."];
    return imported;
}

- (NSData *)exportMacrosData {
    NSDictionary *root = @{@"version": @1, @"macros": [self.macros copy]};
    return [NSJSONSerialization dataWithJSONObject:root
                                           options:(NSJSONWritingPrettyPrinted | NSJSONWritingSortedKeys)
                                             error:nil];
}

#pragma mark Rules

+ (NSDictionary<NSString *, NSString *> *)defaultEnglishApps {
    return @{
        @"com.apple.Terminal": @"Terminal",
        @"com.googlecode.iterm2": @"iTerm2",
        @"dev.warp.Warp-Stable": @"Warp",
        @"io.alacritty": @"Alacritty",
        @"net.kovidgoyal.kitty": @"Kitty"
    };
}

- (NSArray<VKAppRule *> *)sortedRules {
    return [self.appRules.allValues sortedArrayUsingComparator:^NSComparisonResult(VKAppRule *a, VKAppRule *b) {
        if (a.addedAt != b.addedAt) return a.addedAt < b.addedAt ? NSOrderedAscending : NSOrderedDescending;
        return [a.bundleId compare:b.bundleId];
    }];
}

- (VKAppRule *)ruleForBundle:(NSString *)bundleId {
    return bundleId.length ? self.appRules[bundleId] : nil;
}

- (void)setRule:(VKAppRule *)rule {
    if (rule.bundleId.length == 0) return;
    self.appRules[rule.bundleId] = rule;
}

- (void)removeRuleForBundle:(NSString *)bundleId {
    [self.appRules removeObjectForKey:bundleId];
}

- (void)restoreDefaultRules {
    [self.appRules removeAllObjects];
    [self seedDefaultRules];
}

+ (NSString *)displayNameForBundle:(NSString *)bundleId fallback:(NSString *)fallback {
    if (fallback.length) return fallback;
    static NSMutableDictionary<NSString *, NSString *> *cache = nil;
    if (!cache) cache = [NSMutableDictionary dictionary];
    NSString *cached = cache[bundleId];
    if (cached) return cached;
    NSString *name = bundleId;
    NSURL *url = [[NSWorkspace sharedWorkspace] URLForApplicationWithBundleIdentifier:bundleId];
    if (url) {
        NSString *display = [[NSFileManager defaultManager] displayNameAtPath:url.path];
        if ([display hasSuffix:@".app"]) display = [display substringToIndex:display.length - 4];
        if (display.length) name = display;
    }
    cache[bundleId] = name;
    return name;
}

+ (NSImage *)iconForBundle:(NSString *)bundleId {
    static NSMutableDictionary<NSString *, id> *cache = nil;
    if (!cache) cache = [NSMutableDictionary dictionary];
    id cached = cache[bundleId];
    if (cached) return [cached isKindOfClass:[NSImage class]] ? cached : nil;
    NSImage *icon = nil;
    NSURL *url = [[NSWorkspace sharedWorkspace] URLForApplicationWithBundleIdentifier:bundleId];
    if (url) icon = [[NSWorkspace sharedWorkspace] iconForFile:url.path];
    cache[bundleId] = icon ?: (id)[NSNull null];
    return icon;
}

#pragma mark Environment

- (BOOL)accessibilityTrusted {
    if (self.demoAccessibilityTrusted) return self.demoAccessibilityTrusted.boolValue;
    return AXIsProcessTrusted() ? YES : NO;
}

- (BOOL)systemUsesCtrlSpace {
    if (self.demoSystemCtrlSpace) return self.demoSystemCtrlSpace.boolValue;
    // Read-only look at macOS "Select the previous input source" (symbolic hot key 60).
    CFPropertyListRef value = CFPreferencesCopyAppValue(CFSTR("AppleSymbolicHotKeys"), CFSTR("com.apple.symbolichotkeys"));
    id obj = CFBridgingRelease(value);
    if (![obj isKindOfClass:[NSDictionary class]]) return YES;  // never customised: macOS default is ⌃Space, enabled
    NSDictionary *entry = ((NSDictionary *)obj)[@"60"];
    if (![entry isKindOfClass:[NSDictionary class]]) return YES;
    id enabled = entry[@"enabled"];
    if ([enabled respondsToSelector:@selector(boolValue)] && ![enabled boolValue]) return NO;
    id val = entry[@"value"];
    if ([val isKindOfClass:[NSDictionary class]]) {
        id params = ((NSDictionary *)val)[@"parameters"];
        if ([params isKindOfClass:[NSArray class]] && [params count] >= 3) {
            NSInteger keyCode = [params[1] integerValue];
            NSUInteger mods = [params[2] unsignedIntegerValue] & [VKHotkey relevantModifierMask];
            return keyCode == kVKKeySpace && mods == NSEventModifierFlagControl;
        }
    }
    return YES;
}

- (BOOL)hasHotkeyConflict {
    VKHotkey *ctrlSpace = [VKHotkey hotkeyWithKeyCode:kVKKeySpace modifiers:NSEventModifierFlagControl];
    return [self.primaryHotkey isEqualToHotkey:ctrlSpace] && [self systemUsesCtrlSpace];
}

- (BOOL)secureInputActive {
    if (self.demoSecureInput) return self.demoSecureInput.boolValue;
    return IsSecureEventInputEnabled() ? YES : NO;
}

- (BOOL)isRunningFrontmost:(NSString *)bundleId {
    if (bundleId.length == 0) return NO;
    if (self.demoRunningFrontmost) return self.demoRunningFrontmost.boolValue && [bundleId isEqualToString:self.currentAppBundleId];
    return [bundleId isEqualToString:self.currentAppBundleId];
}

#pragma mark Text helpers

- (BOOL)isTelex {
    return ![self.inputMethod isEqualToString:@"vni"];
}

- (NSString *)methodDisplayName {
    return [self isTelex] ? @"Telex" : @"VNI";
}

- (NSString *)statusSubtitle {
    NSMutableArray *parts = [NSMutableArray arrayWithObject:[self methodDisplayName]];
    [parts addObject:self.modernOrthography ? @"Dấu kiểu mới" : @"Dấu kiểu cũ"];
    if (self.autoRestoreEnglish) [parts addObject:@"Tự khôi phục từ tiếng Anh"];
    return [parts componentsJoinedByString:@" · "];
}

- (NSString *)perAppSummary {
    if (!self.smartPerAppMode) return @"Tắt";
    VKAppRule *cur = [self ruleForBundle:self.currentAppBundleId];
    VKAppRule *pick = (cur && cur.mode != VKAppModeRemember) ? cur : nil;
    if (!pick) {
        for (VKAppRule *r in [self sortedRules]) {
            if (r.mode != VKAppModeRemember) { pick = r; break; }
        }
    }
    if (!pick) return [NSString stringWithFormat:@"%lu ứng dụng", (unsigned long)self.appRules.count];
    NSString *name = [VKModel displayNameForBundle:pick.bundleId fallback:pick.name];
    return [NSString stringWithFormat:@"%@ → %@", name, pick.mode == VKAppModeVI ? @"VI" : @"EN"];
}

- (NSString *)versionString {
    NSString *v = [NSBundle mainBundle].infoDictionary[@"CFBundleShortVersionString"];
    return v.length ? v : @"1.0.0";
}

- (NSString *)lastUpdateCheckText {
    if (self.lastUpdateCheck <= 0) return nil;
    static NSDateFormatter *fmt = nil;
    if (!fmt) {
        fmt = [[NSDateFormatter alloc] init];
        fmt.locale = [NSLocale localeWithLocaleIdentifier:@"en_US_POSIX"];
        fmt.dateFormat = @"HH:mm dd/MM/yyyy";
    }
    NSDate *d = [NSDate dateWithTimeIntervalSince1970:self.lastUpdateCheck];
    return [NSString stringWithFormat:@"Lần kiểm tra cuối: %@", [fmt stringFromDate:d]];
}

- (NSString *)hudHint {
    if (self.primaryHotkey.isEmpty) return [self methodDisplayName];
    return [NSString stringWithFormat:@"%@ · %@ để đổi", [self methodDisplayName], [self.primaryHotkey shortString]];
}

@end
