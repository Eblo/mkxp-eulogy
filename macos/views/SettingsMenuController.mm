//
//  SettingsMenuController.m
//  mkxp-z
//
//  Created by ゾロアーク on 1/15/21.
//

// This is a pretty rudimentary keybinding menu, and it replaces the normal one
// for macOS. The normal one basically just doesn't seem to work with ANGLE,
// so I cooked this one up in a hurry despite knowing next to zero about Xcode's
// interface builder in general.

// Yes, it is still a mess, but it is working.

#import <GameController/GameController.h>
#include <Availability.h>

#import <SDL_scancode.h>
#import <SDL_keyboard.h>
#import <SDL_video.h>

#import "sdl_codes.h"

#import "SettingsMenuController.h"

#import "input/input.h"
#import "input/keybindings.h"

#import "eventthread.h"
#import "sharedstate.h"
#import "config.h"

#import "assert.h"

static const int inputMapRowToCode[] {
    Input::Down, Input::Left, Input::Right, Input::Up,
    Input::A, Input:: B, Input::C, Input::X, Input::Y, Input::Z,
    Input::L, Input::R
};

typedef NSMutableArray<NSNumber*> BindingIndexArray;

@implementation SettingsMenu {
    __weak IBOutlet NSWindow *_window;
    __weak IBOutlet NSTableView *_table;
    __weak IBOutlet NSBox *bindingBox;
    
    // Binding buttons
    __weak IBOutlet NSButton *bindingButton1;
    __weak IBOutlet NSButton *bindingButton2;
    __weak IBOutlet NSButton *bindingButton3;
    __weak IBOutlet NSButton *bindingButton4;
    
    // For keeping track of which buttons to check for
    int sysver;
    
    
    // MKXP Keybindings
    BDescVec *binds;
    int currentButtonCode;
    
    // Whether currently waiting for some kind of input
    bool isListening;
    
    // For the current binding selection when the table is
    // reloaded from deleting/adding keybinds
    bool keepCurrentButtonSelection;
    
    NSMutableDictionary<NSNumber*, BindingIndexArray*> *nsbinds;
    NSMutableDictionary<NSNumber*, NSString*> *bindingNames;
}

+(SettingsMenu*)openWindow {
    SettingsMenu *s = [[SettingsMenu alloc] initWithNibName:@"settingsmenu" bundle:NSBundle.mainBundle];
    // Show the window as a sheet, window events will be sucked up by SDL though
    //[NSApplication.sharedApplication.mainWindow beginSheet:s.view.window completionHandler:^(NSModalResponse _){}];
    
    // Show the view in a new window instead, so key and controller events
    // can be captured without SDL's interference
    NSWindow *win = [NSWindow windowWithContentViewController:s];
    win.styleMask &= ~NSWindowStyleMaskResizable;
    win.styleMask &= ~NSWindowStyleMaskFullScreen;
    win.styleMask &= NSWindowStyleMaskTitled;
    win.title = @"Keybindings";
    [s setWindow:win];
    [win makeKeyAndOrderFront:self];
    
    return s;
}

-(SettingsMenu*)raise {
    if (_window)
        [_window makeKeyAndOrderFront:self];
    return self;
}

-(void)closeWindow {
    [self setNotListening:true];
    [_window close];
}

-(SettingsMenu*)setWindow:(NSWindow*)window {
    _window = window;
    return self;
}

- (IBAction)acceptButton:(NSButton *)sender {
    shState->rtData().bindingUpdateMsg.post(*binds);
    storeBindings(*binds, shState->config());
    [self closeWindow];
}
- (IBAction)cancelButton:(NSButton *)sender {
    [self closeWindow];
}
- (IBAction)resetBindings:(NSButton *)sender {
    binds->clear();
    BDescVec tmp = genDefaultBindings(shState->config());
    binds->assign(tmp.begin(), tmp.end());
    
    [self setNotListening:false];
}

- (void)viewDidLoad {
    [super viewDidLoad];
    
    sysver = 12;
    if (@available(macOS 10.14, *)) sysver += 2;
    if (@available(macOS 10.15, *)) sysver++;
    if (@available(macOS 11.0, *)) sysver++;
    
    isListening = false;
    keepCurrentButtonSelection = false;
    
    [self initDelegateWithTable:_table];
    _table.delegate = self;
    _table.dataSource = self;
    [self setNotListening:true];
    _table.enabled = true;
    
    bindingBox.title = @"";
    [self setButtonNames:0];
}

- (void)keyDown:(NSEvent *)event {
    [super keyDown:event];
    if (!isListening) return;
    
    BindingDesc d;
    d.target = (Input::ButtonCode)currentButtonCode;
    SourceDesc s;
    s.type = Key;
    s.d.scan = darwin_scancode_table[event.keyCode];
    d.src = s;
    binds->push_back(d);
    [self setNotListening:true];
}

#define checkButtonStart if (0) {}
#define checkButtonEnd else { return; }
#define checkButtonElement(e, b, n, min) \
else if (sysver >= min && element == gamepad.e && gamepad.b.isPressed) { \
s.type = CButton; \
s.d.cb = n; \
}

#define checkButtonAlt(b, n, min) checkButtonElement(b, b, n, min)

#define checkButton(b, n, min) checkButtonAlt(button##b, n, min)

#define setAxisData(a, n) \
GCControllerAxisInput *axis = gamepad.a; \
s.type = CAxis; \
s.d.ca.axis = n; \
s.d.ca.dir = (axis.value >= 0) ? AxisDir::Positive : AxisDir::Negative;

#define checkAxis(el, a, n) else if (element == gamepad.el && (gamepad.el.a.value >= 0.5 || gamepad.el.a.value <= -0.5)) { setAxisData(el.a, n); }

- (void)registerJoystickAction:(GCExtendedGamepad*)gamepad element:(GCControllerElement*)element {
    if (!isListening) return;
    
    BindingDesc d;
    d.target = (Input::ButtonCode)currentButtonCode;
    SourceDesc s;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunguarded-availability-new"
    checkButtonStart
    checkButton(A, SDL_CONTROLLER_BUTTON_A, 12)
    checkButton(B, SDL_CONTROLLER_BUTTON_B, 12)
    checkButton(X, SDL_CONTROLLER_BUTTON_X, 12)
    checkButton(Y, SDL_CONTROLLER_BUTTON_Y, 12)
    checkButtonElement(dpad, dpad.up, SDL_CONTROLLER_BUTTON_DPAD_UP, 12)
    checkButtonElement(dpad, dpad.down, SDL_CONTROLLER_BUTTON_DPAD_DOWN, 12)
    checkButtonElement(dpad, dpad.left, SDL_CONTROLLER_BUTTON_DPAD_LEFT, 12)
    checkButtonElement(dpad, dpad.right, SDL_CONTROLLER_BUTTON_DPAD_RIGHT, 12)
    checkButtonAlt(leftShoulder, SDL_CONTROLLER_BUTTON_LEFTSHOULDER, 12)
    checkButtonAlt(rightShoulder, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, 12)

#if __MAC_OS_X_VERSION_MAX_ALLOWED >= __MAC_10_14_1
    // Requires macOS 10.14.1+
    checkButtonAlt(leftThumbstickButton, SDL_CONTROLLER_BUTTON_LEFTSTICK, 14)
    checkButtonAlt(rightThumbstickButton, SDL_CONTROLLER_BUTTON_RIGHTSTICK, 14)
#else
#warning "This SDK doesn't support the detection of thumbstick buttons. You will not be able to rebind them from the menu on 10.14.1 or higher."
#endif

#if __MAC_OS_X_VERSION_MAX_ALLOWED >= __MAC_10_15
    // Requires macOS 10.15+
    checkButton(Menu, SDL_CONTROLLER_BUTTON_START, 15)
    checkButton(Options, SDL_CONTROLLER_BUTTON_BACK, 15)
#else
#warning "This SDK doesn't support the detection of Start and Back buttons. You will not be able to rebind them from the menu on 10.15 or higher."
#endif
    
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= __MAC_11_0
    // Requires macOS 11.0+
    checkButton(Home, SDL_CONTROLLER_BUTTON_GUIDE, 16)
#else
#warning "This SDK doesn't support the detection of the Guide button. You will not be able to rebind it from the menu on 11.0 or higher."
#endif
    
    checkAxis(leftThumbstick, xAxis, SDL_CONTROLLER_AXIS_LEFTX)
    checkAxis(leftThumbstick, yAxis, SDL_CONTROLLER_AXIS_LEFTY)
    checkAxis(rightThumbstick, xAxis, SDL_CONTROLLER_AXIS_RIGHTX)
    checkAxis(rightThumbstick, yAxis, SDL_CONTROLLER_AXIS_RIGHTY)
    
    else if (element == gamepad.leftTrigger && (gamepad.leftTrigger.value >= 0.5 || gamepad.leftTrigger.value <= -0.5)) {
        s.type = CAxis;
        s.d.ca.axis = SDL_CONTROLLER_AXIS_TRIGGERLEFT;
        s.d.ca.dir = AxisDir::Positive;
    }
    
    else if (element == gamepad.rightTrigger && (gamepad.rightTrigger.value >= 0.5 || gamepad.rightTrigger.value <= -0.5)) {
        s.type = CAxis;
        s.d.ca.axis = SDL_CONTROLLER_AXIS_TRIGGERRIGHT;
        s.d.ca.dir = AxisDir::Positive;
    }
    
    checkButtonEnd;
    
#pragma clang diagnostic pop
    d.src = s;
    binds->push_back(d);
    [self setNotListening:true];
}

+(NSString*)nameForBinding:(SourceDesc&)desc {
    switch (desc.type) {
        case Key:
            if (desc.d.scan == SDL_SCANCODE_LSHIFT) return @"Shift";
            return @(SDL_GetScancodeName(desc.d.scan));
        case CAxis:
            return [NSString stringWithFormat:@"%s%s",
                    shState->input().getAxisName(desc.d.ca.axis), desc.d.ca.dir == Negative ? "-" : "+"];
        case CButton:
            return @(shState->input().getButtonName(desc.d.cb));
        default:
            return @"Invalid";
    }
}

-(id)initDelegateWithTable:(NSTableView*)tbl {
    binds = new BDescVec;
    nsbinds = [NSMutableDictionary new];
    bindingNames = [NSMutableDictionary new];
    
    RGSSThreadData &data = shState->rtData();
    
#define SET_BINDING(code) bindingNames[@(Input::code)] = @(#code)
#define SET_BINDING_CUSTOM(code, value) bindingNames[@(Input::code)] = @(value)
    SET_BINDING(Down);
    SET_BINDING(Left);
    SET_BINDING(Right);
    SET_BINDING(Up);
    SET_BINDING(A);
    SET_BINDING(B);
    SET_BINDING(C);
    SET_BINDING(X);
    SET_BINDING(Y);
    SET_BINDING(Z);
    SET_BINDING(L);
    SET_BINDING(R);
    
#define SET_BINDING_CONF(code, value) \
if (!data.config.kbActionNames.value.empty()) bindingNames[@(Input::code)] = \
    @(data.config.kbActionNames.value.c_str())
    SET_BINDING_CONF(A,a);
    SET_BINDING_CONF(B,b);
    SET_BINDING_CONF(C,c);
    SET_BINDING_CONF(X,x);
    SET_BINDING_CONF(Y,y);
    SET_BINDING_CONF(Z,z);
    SET_BINDING_CONF(L,l);
    SET_BINDING_CONF(R,r);
    
    BDescVec oldBinds;
    data.bindingUpdateMsg.get(oldBinds);
    
    if (oldBinds.size() <= 0) {
        BDescVec defaults = genDefaultBindings(data.config);
        binds->assign(defaults.begin(), defaults.end());
    }
    else {
        binds->assign(oldBinds.begin(), oldBinds.end());
    }
    
    [self loadBinds];
    [self enableButtons:false];
    
    return self;
}

- (void) loadBinds {
    [nsbinds removeAllObjects];
    
    for (int i = 0; i < sizeof(inputMapRowToCode) / sizeof(Input::ButtonCode); i++)
        nsbinds[@(inputMapRowToCode[i])] = [NSMutableArray new];
    
    for (int i = 0; i < binds->size(); i++) {
        NSNumber *key = @(binds->at(i).target);
        if (nsbinds[key] == nil) {
            nsbinds[key] = [NSMutableArray new];
        }
        NSMutableArray *b = nsbinds[key];
        [b addObject:@(i)];
    }
}

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    return nsbinds.count;
}

- (NSView *)tableView:(NSTableView *)tableView viewForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {
    NSString *tableID = tableColumn.identifier;
    int buttonCode = inputMapRowToCode[row];
    
    NSTableCellView *cell = [tableView makeViewWithIdentifier:tableID owner:self];
    if ([tableID isEqualToString:@"action"]) {
        cell.textField.stringValue = bindingNames[@(inputMapRowToCode[row])];
        cell.textField.font = [NSFont boldSystemFontOfSize:cell.textField.font.pointSize];
        return cell;
    }
    
    long col = tableID.integerValue;
    
    if (nsbinds[@(buttonCode)].count < col) {
        cell.textField.stringValue = @"";
        return cell;
    }
    
    NSNumber *d = nsbinds[@(buttonCode)][col-1];
    BindingDesc &bind = binds->at(d.intValue);
    cell.textField.stringValue = [SettingsMenu nameForBinding:bind.src];
    return cell;
}

- (id)tableView:(NSTableView *)tableView objectValueForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {
    NSString *tableID = tableColumn.identifier;
    int buttonCode = inputMapRowToCode[row];
    
    if ([tableID isEqualToString:@"action"]) {
        return @(buttonCode);
    }
    
    long col = tableID.integerValue;
    
    if (nsbinds[@(buttonCode)].count < col) {
        return 0;
    }
    
    NSNumber *d = nsbinds[@(buttonCode)][col-1];
    BindingDesc &bind = binds->at(d.intValue);
    return @(bind.src.d.scan);
}

- (void)tableViewSelectionDidChange:(NSNotification *)notification {
    if (isListening)
        isListening = false;
    
    if (keepCurrentButtonSelection) {
        if (keepCurrentButtonSelection) keepCurrentButtonSelection = false;
        [_table deselectRow:_table.selectedRow];
        [self setButtonNames:currentButtonCode];
        bindingBox.title = bindingNames[@(currentButtonCode)];
        return;
    }
    
    if (_table.selectedRow > -1) {
        int buttonCode = inputMapRowToCode[_table.selectedRow];
        currentButtonCode = buttonCode;
        [self setButtonNames:buttonCode];
        bindingBox.title = bindingNames[@(currentButtonCode)];
    }
    else {
        [self setButtonNames:0];
        bindingBox.title = @"";
    }
}

- (int)setButtonNames:(int)input {
    if (!input) {
        bindingButton1.title = @"";
        bindingButton2.title = bindingButton1.title;
        bindingButton3.title = bindingButton1.title;
        bindingButton4.title = bindingButton1.title;
        [self enableButtons:false];
        return 0;
    }
    BindingIndexArray *nsbind = nsbinds[@(input)];
    NSMutableArray<NSString*> *pnames = [NSMutableArray new];
    for (int i = 0; i < 4; i++) {
        if (!nsbind.count || i > nsbind.count - 1) {
            [pnames addObject:@"(Empty)"];
        }
        else {
            BindingDesc &b = binds->at(nsbind[i].intValue);
            NSString *bindingName = [SettingsMenu nameForBinding:b.src];
            [pnames addObject:bindingName];
        }
    }
    
    bindingButton1.title = pnames[0];
    bindingButton2.title = pnames[1];
    bindingButton3.title = pnames[2];
    bindingButton4.title = pnames[3];
    [self enableButtons:true];
    
    return (int)pnames.count;
}

- (void)enableButtons:(bool)defaultSetting {
    BindingIndexArray *currentBind = nsbinds[@(currentButtonCode)];
    bindingButton1.enabled = defaultSetting;
    bindingButton2.enabled = defaultSetting && currentBind.count > 0;
    
    bindingButton3.enabled = defaultSetting && currentBind.count > 1;
    bindingButton4.enabled = defaultSetting && currentBind.count > 2;
}

- (IBAction)binding1Clicked:(NSButton *)sender {
    if (nsbinds[@(currentButtonCode)].count > 0) {
        [self removeBinding:0 forInput:currentButtonCode];
        return;
    }
    [self setListening:sender];
}
- (IBAction)binding2Clicked:(NSButton *)sender {
    if (nsbinds[@(currentButtonCode)].count > 1) {
        [self removeBinding:1 forInput:currentButtonCode];
        return;
    }
    [self setListening:sender];
}
- (IBAction)binding3Clicked:(NSButton *)sender {
    if (nsbinds[@(currentButtonCode)].count > 2) {
        [self removeBinding:2 forInput:currentButtonCode];
        return;
    }
    [self setListening:sender];
}
- (IBAction)binding4Clicked:(NSButton *)sender {
    if (nsbinds[@(currentButtonCode)].count > 3) {
        [self removeBinding:3 forInput:currentButtonCode];
        return;
    }
    [self setListening:sender];
}

- (void)removeBinding:(int)bindIndex forInput:(int)input {
    NSMutableArray<NSNumber*> *bind = nsbinds[@(input)];
    int bi = bind[bindIndex].intValue;
    binds->erase(binds->begin() + bi);
    
    [self setNotListening:true];
}

- (void)setListening:(NSButton*)src {
    if (isListening) {
        [self setNotListening:true];
        return;
    }
    
    // Stops receiving keyDown events if it's disabled, apparently
    //_table.enabled = false;
    
    [self enableButtons:false];
    
    if (src == nil) return;
    
    src.title = @"Click to Cancel...";
    src.enabled = true;
    isListening = true;
    
    NSArray<GCController*>* controllers = [GCController controllers];
    if (controllers.count <= 0) return;
    GCController *gamepad = controllers[0];
    if (gamepad.extendedGamepad == nil || gamepad.extendedGamepad.valueChangedHandler != nil) return;
    gamepad.extendedGamepad.valueChangedHandler = ^(GCExtendedGamepad *gamepad, GCControllerElement *element)
    {[self registerJoystickAction:gamepad element:element];};
}

- (void)setNotListening:(bool)keepCurrentSelection {
    [self loadBinds];
    
    keepCurrentButtonSelection = keepCurrentSelection;
    isListening = false;
    [_table reloadData];
    [self setButtonNames:currentButtonCode];
    
}

-(void)dealloc {
    delete binds;
}

@end
