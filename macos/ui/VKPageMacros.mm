// VKPageMacros.mm — "Gõ tắt" page (design 03). MIT License, 1VISION.

#import "VKPages.h"

static NSString *const kVKMacroRowId = @"VKMacroRow";
static const CGFloat kVKMacroValueX = 150.0;

#pragma mark - Row view

@interface VKMacroRow : VKControlView
@property (nonatomic, copy) NSString *key;
@property (nonatomic, copy) NSString *value;
@property (nonatomic, strong) VKChip *chip;
@property (nonatomic, strong) NSTextField *valueLabel;
@property (nonatomic, strong) NSTextField *valueField;
@property (nonatomic, strong) NSTextField *countLabel;
@property (nonatomic, strong) VKButton *editButton;
@property (nonatomic, strong) VKButton *deleteButton;
@property (nonatomic) BOOL editing;
@property (nonatomic) BOOL cancelled;
- (void)configureWithKey:(NSString *)key value:(NSString *)value count:(NSInteger)count;
- (void)updateHoverState;
@end

@implementation VKMacroRow

- (instancetype)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        self.showsHandCursor = NO;
        self.bottomSeparator = YES;
        _chip = [VKChip chipWithText:@"" font:VKMono(13.5, NSFontWeightMedium) text:VKTheme.primary fill:VKTheme.primaryTint height:26];
        [self addSubview:_chip];
        _valueLabel = VKLabel(@"", VKFont(14.5, NSFontWeightRegular), VKTheme.text);
        [self addSubview:_valueLabel];
        _valueField = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 100, 24)];
        _valueField.font = VKFont(14, NSFontWeightRegular);
        _valueField.bezelStyle = NSTextFieldRoundedBezel;
        _valueField.bezeled = YES;
        _valueField.editable = YES;
        _valueField.hidden = YES;
        [self addSubview:_valueField];
        _countLabel = VKLabel(@"", VKFont(13.5, NSFontWeightRegular), VKTheme.textSecondary);
        [self addSubview:_countLabel];
        _editButton = [VKButton buttonWithTitle:@"" symbol:@"pencil" style:VKButtonStyleIcon];
        _editButton.frame = NSMakeRect(0, 0, 30, 30);
        _editButton.toolTip = @"Sửa nội dung";
        [self addSubview:_editButton];
        _deleteButton = [VKButton buttonWithTitle:@"" symbol:@"trash" style:VKButtonStyleDanger];
        _deleteButton.frame = NSMakeRect(0, 0, 30, 30);
        _deleteButton.toolTip = @"Xoá cụm từ";
        [self addSubview:_deleteButton];
        __weak VKMacroRow *weakSelf = self;
        self.onHoverChanged = ^(BOOL hovering) { [weakSelf updateHoverState]; };
        [self layoutRow];
        [self updateHoverState];
    }
    return self;
}

- (void)setFrameSize:(NSSize)newSize {
    [super setFrameSize:newSize];
    [self layoutRow];
}

- (void)layoutRow {
    CGFloat w = self.bounds.size.width, h = self.bounds.size.height;
    if (!self.chip) return;
    VKSetOrigin(self.chip, 20, round((h - self.chip.frame.size.height) / 2.0));
    CGFloat countX = MAX(kVKMacroValueX + 120, w - 246);
    NSRect vf = self.valueLabel.frame;
    vf.size.width = MIN(vf.size.width, countX - kVKMacroValueX - 16);
    vf.origin = NSMakePoint(kVKMacroValueX, round((h - vf.size.height) / 2.0));
    self.valueLabel.frame = vf;
    self.valueField.frame = NSMakeRect(kVKMacroValueX - 4, round((h - 24) / 2.0), countX - kVKMacroValueX - 12, 24);
    VKPlaceLabel(self.countLabel, countX, h / 2.0);
    VKSetOrigin(self.deleteButton, w - 20 - 30, round((h - 30) / 2.0));
    VKSetOrigin(self.editButton, w - 20 - 30 - 8 - 30, round((h - 30) / 2.0));
}

- (void)configureWithKey:(NSString *)key value:(NSString *)value count:(NSInteger)count {
    self.key = key;
    self.value = value;
    self.editing = NO;
    self.cancelled = NO;
    self.valueField.hidden = YES;
    self.valueLabel.hidden = NO;
    self.chip.text = key;
    [self.chip sizeToFitText];
    self.valueLabel.stringValue = value;
    [self.valueLabel sizeToFit];
    self.countLabel.stringValue = [NSString stringWithFormat:@"%@ lần", VKFormatCount(count)];
    [self layoutRow];
    [self updateHoverState];
}

- (void)updateHoverState {
    BOOL active = self.hovering || self.editing;
    self.chip.fillColor = active ? VKTheme.primaryFill : VKTheme.primaryTint;
    self.chip.textColor = active ? [NSColor whiteColor] : VKTheme.primary;
    self.chip.needsDisplay = YES;
    self.editButton.hidden = !active;
    self.deleteButton.hidden = !active;
    self.fillColor = active ? VKTheme.hoverBg : nil;
}

@end

#pragma mark - Page

@interface VKMacrosPage ()
@property (nonatomic, strong) VKSwitch *masterSwitch;
@property (nonatomic, strong) VKFieldBox *keyBox;
@property (nonatomic, strong) VKFieldBox *valueBox;
@property (nonatomic, strong) VKFieldBox *filterBox;
@property (nonatomic, strong) VKSegmented *sortControl;
@property (nonatomic, strong) NSScrollView *scroll;
@property (nonatomic, strong) NSTableView *table;
@property (nonatomic, strong) NSTextField *footerLabel;
@property (nonatomic, strong) NSArray<NSString *> *keys;
@property (nonatomic) BOOL byUsage;
@property (nonatomic) NSInteger messageToken;
@property (nonatomic) BOOL showingMessage;
@end

@implementation VKMacrosPage

- (void)build {
    __weak VKMacrosPage *weakSelf = self;
    CGFloat pad = kVKPagePad, W = self.contentWidth;
    self.keys = @[];
    self.keywords = @"gõ tắt macro viết tắt nhập xuất";

    NSMutableAttributedString *sub = [[NSMutableAttributedString alloc] init];
    [sub appendAttributedString:VKAttr(@"Tự giữ kiểu chữ: ", VKFont(13.5, NSFontWeightRegular), VKTheme.textSecondary)];
    [sub appendAttributedString:VKAttr(@"ko → không · Ko → Không · KO → KHÔNG", VKMono(13, NSFontWeightRegular), VKTheme.text)];
    CGFloat y = [self addTitle:@"Gõ tắt" subtitle:sub];

    self.masterSwitch = [VKSwitch switchWithOn:YES];
    VKSetOrigin(self.masterSwitch, pad + W - self.masterSwitch.frame.size.width, 30);
    self.masterSwitch.onToggle = ^(BOOL on) {
        weakSelf.model.enableMacros = on;
        [weakSelf.model commit];
    };
    [self addSubview:self.masterSwitch];

    VKButton *exportBtn = [VKButton buttonWithTitle:@"Xuất" symbol:@"square.and.arrow.down" style:VKButtonStyleSecondary];
    exportBtn.frame = NSMakeRect(self.masterSwitch.frame.origin.x - 18 - [exportBtn fittingWidth], 26, [exportBtn fittingWidth], 32);
    exportBtn.onClick = ^{ [weakSelf exportMacros]; };
    [self addSubview:exportBtn];
    VKButton *importBtn = [VKButton buttonWithTitle:@"Nhập" symbol:@"square.and.arrow.up" style:VKButtonStyleSecondary];
    importBtn.frame = NSMakeRect(exportBtn.frame.origin.x - 10 - [importBtn fittingWidth], 26, [importBtn fittingWidth], 32);
    importBtn.onClick = ^{ [weakSelf importMacros]; };
    [self addSubview:importBtn];

    // Add card
    VKBox *add = [self addCard:NSMakeRect(pad, y - 4, W, 86)];
    NSTextField *kl = VKAttrLabel(VKAttrKern(@"VIẾT TẮT", VKFont(11.5, NSFontWeightSemibold), VKTheme.textSecondary, 0.6));
    VKSetOrigin(kl, 20, 12);
    [add addSubview:kl];
    self.keyBox = [[VKFieldBox alloc] initWithFrame:NSMakeRect(20, 34, 150, 36) placeholder:@"vd: cty" font:VKMono(14, NSFontWeightRegular) symbol:nil];
    self.keyBox.field.delegate = self;
    self.keyBox.field.target = self;
    self.keyBox.field.action = @selector(keyFieldReturn:);
    [add addSubview:self.keyBox];
    VKIconTile *arrow = [[VKIconTile alloc] initWithFrame:NSMakeRect(176, 34, 24, 36)];
    arrow.fillColor = [NSColor clearColor];
    arrow.tintColor = VKTheme.textSecondary;
    arrow.symbolName = @"arrow.right";
    arrow.symbolSize = 14;
    [add addSubview:arrow];
    CGFloat addW = 100;
    NSTextField *vl = VKAttrLabel(VKAttrKern(@"THAY BẰNG", VKFont(11.5, NSFontWeightSemibold), VKTheme.textSecondary, 0.6));
    VKSetOrigin(vl, 206, 12);
    [add addSubview:vl];
    self.valueBox = [[VKFieldBox alloc] initWithFrame:NSMakeRect(206, 34, W - 206 - 20 - addW - 12, 36)
                                          placeholder:@"vd: công ty" font:VKFont(14, NSFontWeightRegular) symbol:nil];
    self.valueBox.field.target = self;
    self.valueBox.field.action = @selector(addMacro:);
    [add addSubview:self.valueBox];
    VKButton *addBtn = [VKButton buttonWithTitle:@"Thêm" symbol:@"plus" style:VKButtonStylePrimary];
    addBtn.font = VKFont(14, NSFontWeightSemibold);
    addBtn.frame = NSMakeRect(W - 20 - addW, 34, addW, 36);
    addBtn.onClick = ^{ [weakSelf addMacro:nil]; };
    [add addSubview:addBtn];
    if (self.snapshot) {
        self.keyBox.field.stringValue = @"cty";
        self.valueBox.field.stringValue = @"công ty";
    }

    // Filter + sort
    y = NSMaxY(add.frame) + 16;
    CGFloat segW = 176;
    self.filterBox = [[VKFieldBox alloc] initWithFrame:NSMakeRect(pad, y, W - segW - 12, 36)
                                           placeholder:@"Lọc cụm từ..." font:VKFont(14, NSFontWeightRegular) symbol:@"magnifyingglass"];
    self.filterBox.field.delegate = self;
    [self addSubview:self.filterBox];
    self.sortControl = [[VKSegmented alloc] initWithFrame:NSMakeRect(pad + W - segW, y, segW, 36) items:@[@"A → Z", @"Hay dùng"]];
    self.sortControl.font = VKFont(14, NSFontWeightRegular);
    self.sortControl.selectedFont = VKFont(14, NSFontWeightSemibold);
    self.sortControl.onSelect = ^(NSInteger idx) {
        weakSelf.byUsage = (idx == 1);
        [weakSelf reloadTable];
    };
    [self addSubview:self.sortControl];

    // Table card
    y = NSMaxY(self.filterBox.frame) + 14;
    CGFloat cardH = self.viewportHeight - 24 - y;
    VKBox *card = [self addCard:NSMakeRect(pad, y, W, cardH)];
    VKBox *header = [[VKBox alloc] initWithFrame:NSMakeRect(1, 1, W - 2, 42) fill:nil radius:0];
    header.bottomSeparator = YES;
    [card addSubview:header];
    CGFloat countX = MAX(kVKMacroValueX + 120, (W - 2) - 246);
    NSArray *cols = @[@[@"VIẾT TẮT", @20], @[@"NỘI DUNG", @(kVKMacroValueX)], @[@"ĐÃ DÙNG", @(countX)]];
    for (NSArray *c in cols) {
        NSTextField *l = VKAttrLabel(VKAttrKern(c[0], VKFont(12, NSFontWeightSemibold), VKTheme.textSecondary, 0.6));
        VKPlaceLabel(l, [c[1] doubleValue], 21);
        [header addSubview:l];
    }

    CGFloat footH = 44;
    self.scroll = [[NSScrollView alloc] initWithFrame:NSMakeRect(1, 43, W - 2, cardH - 43 - footH)];
    self.scroll.drawsBackground = NO;
    self.scroll.hasVerticalScroller = YES;
    self.scroll.autohidesScrollers = YES;
    self.scroll.borderType = NSNoBorder;
    self.scroll.contentView.drawsBackground = NO;

    self.table = [[NSTableView alloc] initWithFrame:self.scroll.bounds];
    NSTableColumn *col = [[NSTableColumn alloc] initWithIdentifier:@"macro"];
    col.width = W - 2;
    col.resizingMask = NSTableColumnAutoresizingMask;
    [self.table addTableColumn:col];
    self.table.headerView = nil;
    self.table.style = NSTableViewStyleFullWidth;
    self.table.rowHeight = 48;
    self.table.intercellSpacing = NSMakeSize(0, 0);
    self.table.backgroundColor = [NSColor clearColor];
    self.table.selectionHighlightStyle = NSTableViewSelectionHighlightStyleNone;
    self.table.focusRingType = NSFocusRingTypeNone;
    self.table.gridStyleMask = NSTableViewGridNone;
    self.table.columnAutoresizingStyle = NSTableViewUniformColumnAutoresizingStyle;
    self.table.dataSource = self;
    self.table.delegate = self;
    self.scroll.documentView = self.table;
    [card addSubview:self.scroll];

    VKBox *footer = [[VKBox alloc] initWithFrame:NSMakeRect(1, cardH - footH - 1, W - 2, footH) fill:nil radius:0];
    footer.topSeparator = YES;
    [card addSubview:footer];
    self.footerLabel = VKLabel(@"", VKFont(13, NSFontWeightRegular), VKTheme.textSecondary);
    [footer addSubview:self.footerLabel];
    VKSavedIndicator *saved = [VKSavedIndicator indicator];
    VKSetOrigin(saved, footer.frame.size.width - 20 - saved.frame.size.width, round((footH - 18) / 2.0));
    [footer addSubview:saved];
}

- (void)reload {
    VKModel *m = self.model;
    self.masterSwitch.on = m.enableMacros;
    self.filterBox.field.placeholderString = [NSString stringWithFormat:@"Lọc %@ cụm từ...", VKFormatCount((NSInteger)m.macros.count)];
    self.sortControl.selectedIndex = self.byUsage ? 1 : 0;
    [self reloadTable];
}

- (BOOL)isEditingRow {
    NSInteger n = self.table.numberOfRows;
    for (NSInteger i = 0; i < n; i++) {
        VKMacroRow *row = [self.table viewAtColumn:0 row:i makeIfNecessary:NO];
        if ([row isKindOfClass:[VKMacroRow class]] && row.editing) return YES;
    }
    return NO;
}

- (void)reloadTable {
    if ([self isEditingRow]) return;
    self.keys = [self.model macroKeysSortedByUsage:self.byUsage filter:self.filterBox.field.stringValue];
    [self.table reloadData];
    [self updateFooter];
}

- (void)updateFooter {
    if (self.showingMessage) return;
    self.footerLabel.textColor = VKTheme.textSecondary;
    self.footerLabel.stringValue = [NSString stringWithFormat:@"Hiển thị %@ / %@ cụm từ",
                                    VKFormatCount((NSInteger)self.keys.count), VKFormatCount((NSInteger)self.model.macros.count)];
    VKPlaceLabel(self.footerLabel, 20, 22);
}

- (void)showMessage:(NSString *)message error:(BOOL)error {
    self.messageToken += 1;
    NSInteger token = self.messageToken;
    self.showingMessage = YES;
    self.footerLabel.textColor = error ? VKTheme.error : VKTheme.accentText;
    self.footerLabel.stringValue = message;
    VKPlaceLabel(self.footerLabel, 20, 22);
    __weak VKMacrosPage *weakSelf = self;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(4 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        VKMacrosPage *s = weakSelf;
        if (!s || s.messageToken != token) return;
        s.showingMessage = NO;
        [s updateFooter];
    });
}

#pragma mark Actions

- (void)keyFieldReturn:(id)sender {
    [self.window makeFirstResponder:self.valueBox.field];
}

- (void)addMacro:(id)sender {
    NSString *key = self.keyBox.field.stringValue;
    NSString *value = self.valueBox.field.stringValue;
    if (key.length == 0 && value.length == 0) return;
    NSString *err = [self.model setMacroKey:key value:value];
    if (err) {
        [self showMessage:err error:YES];
        return;
    }
    NSString *k = [[key stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]] lowercaseString];
    self.keyBox.field.stringValue = @"";
    self.valueBox.field.stringValue = @"";
    [self.model commit];
    [self showMessage:[NSString stringWithFormat:@"Đã thêm “%@”.", k] error:NO];
    [self.window makeFirstResponder:self.keyBox.field];
}

- (void)beginEditRow:(VKMacroRow *)row {
    if (!row.key) return;
    row.editing = YES;
    row.cancelled = NO;
    row.valueField.stringValue = row.value ?: @"";
    row.valueField.delegate = self;
    row.valueField.hidden = NO;
    row.valueLabel.hidden = YES;
    [row updateHoverState];
    [self.window makeFirstResponder:row.valueField];
}

- (void)deleteRow:(VKMacroRow *)row {
    if (!row.key) return;
    NSString *k = row.key;
    [self.model removeMacro:k];
    [self.model commit];
    [self showMessage:[NSString stringWithFormat:@"Đã xoá “%@”.", k] error:NO];
}

- (void)importMacros {
    if (self.snapshot || !self.window) return;
    NSOpenPanel *panel = [NSOpenPanel openPanel];
    panel.canChooseFiles = YES;
    panel.canChooseDirectories = NO;
    panel.allowsMultipleSelection = NO;
    panel.message = @"Chọn tệp JSON của VisKey hoặc tệp văn bản mỗi dòng “viết tắt<TAB>nội dung”.";
    panel.prompt = @"Nhập";
    __weak VKMacrosPage *weakSelf = self;
    [panel beginSheetModalForWindow:self.window completionHandler:^(NSModalResponse result) {
        if (result != NSModalResponseOK || !panel.URL) return;
        VKMacrosPage *s = weakSelf;
        if (!s) return;
        NSData *data = [NSData dataWithContentsOfURL:panel.URL];
        if (!data) {
            [s showAlert:@"Không đọc được tệp." info:panel.URL.path];
            return;
        }
        NSMutableArray<NSString *> *errors = [NSMutableArray array];
        NSInteger n = [s.model importMacrosFromData:data errors:errors];
        if (n > 0) [s.model commit];
        NSString *info = @"";
        if (errors.count) {
            NSArray *shown = errors.count > 8 ? [errors subarrayWithRange:NSMakeRange(0, 8)] : errors;
            info = [shown componentsJoinedByString:@"\n"];
            if (errors.count > 8) info = [info stringByAppendingFormat:@"\n… và %lu lỗi khác.", (unsigned long)(errors.count - 8)];
        }
        NSString *title = n > 0 ? [NSString stringWithFormat:@"Đã nhập %@ cụm từ.", VKFormatCount(n)] : @"Không nhập được cụm từ nào.";
        dispatch_async(dispatch_get_main_queue(), ^{ [s showAlert:title info:info]; });
    }];
}

- (void)exportMacros {
    if (self.snapshot || !self.window) return;
    NSSavePanel *panel = [NSSavePanel savePanel];
    panel.nameFieldStringValue = @"VisKey-go-tat.json";
    panel.prompt = @"Xuất";
    panel.message = @"Lưu bảng gõ tắt ra tệp JSON.";
    __weak VKMacrosPage *weakSelf = self;
    [panel beginSheetModalForWindow:self.window completionHandler:^(NSModalResponse result) {
        if (result != NSModalResponseOK || !panel.URL) return;
        VKMacrosPage *s = weakSelf;
        if (!s) return;
        NSData *data = [s.model exportMacrosData];
        NSError *err = nil;
        BOOL ok = data && [data writeToURL:panel.URL options:NSDataWritingAtomic error:&err];
        if (ok) {
            [s showMessage:[NSString stringWithFormat:@"Đã xuất %@ cụm từ.", VKFormatCount((NSInteger)s.model.macros.count)] error:NO];
        } else {
            NSString *info = err.localizedDescription ?: panel.URL.path;
            dispatch_async(dispatch_get_main_queue(), ^{ [s showAlert:@"Không lưu được tệp." info:info]; });
        }
    }];
}

#pragma mark Table

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    return (NSInteger)self.keys.count;
}

- (NSView *)tableView:(NSTableView *)tableView viewForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {
    VKMacroRow *view = [tableView makeViewWithIdentifier:kVKMacroRowId owner:self];
    if (!view) {
        view = [[VKMacroRow alloc] initWithFrame:NSMakeRect(0, 0, tableView.bounds.size.width, 48)];
        view.identifier = kVKMacroRowId;
    }
    if (row < 0 || row >= (NSInteger)self.keys.count) return view;
    NSString *key = self.keys[(NSUInteger)row];
    [view configureWithKey:key value:self.model.macros[key] ?: @"" count:[self.model usageForMacro:key]];
    __weak VKMacrosPage *weakSelf = self;
    __weak VKMacroRow *weakRow = view;
    view.editButton.onClick = ^{ [weakSelf beginEditRow:weakRow]; };
    view.deleteButton.onClick = ^{ [weakSelf deleteRow:weakRow]; };
    return view;
}

- (BOOL)tableView:(NSTableView *)tableView shouldSelectRow:(NSInteger)row {
    return NO;
}

#pragma mark NSTextFieldDelegate

- (void)controlTextDidChange:(NSNotification *)notification {
    if (notification.object == self.filterBox.field) {
        [self reloadTable];
    }
}

- (void)controlTextDidEndEditing:(NSNotification *)notification {
    NSTextField *field = notification.object;
    VKMacroRow *row = (VKMacroRow *)field.superview;
    if (![row isKindOfClass:[VKMacroRow class]] || !row.editing) return;
    row.editing = NO;
    if (!row.cancelled) {
        NSString *err = [self.model setMacroKey:row.key value:field.stringValue];
        if (err) {
            [self showMessage:err error:YES];
        } else {
            [self.model commit];
            [self showMessage:[NSString stringWithFormat:@"Đã cập nhật “%@”.", row.key] error:NO];
        }
    }
    row.cancelled = NO;
    [self reloadTable];
}

- (BOOL)control:(NSControl *)control textView:(NSTextView *)textView doCommandBySelector:(SEL)commandSelector {
    VKMacroRow *row = (VKMacroRow *)control.superview;
    if ([row isKindOfClass:[VKMacroRow class]] && row.editing && commandSelector == @selector(cancelOperation:)) {
        row.cancelled = YES;
        [self.window makeFirstResponder:self.table];
        return YES;
    }
    return NO;
}

@end
