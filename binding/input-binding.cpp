/*
 ** input-binding.cpp
 **
 ** This file is part of mkxp.
 **
 ** Copyright (C) 2013 Jonas Kulla <Nyocurio@gmail.com>
 **
 ** mkxp is free software: you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation, either version 2 of the License, or
 ** (at your option) any later version.
 **
 ** mkxp is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with mkxp.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <SDL_joystick.h>
#include <string>


#include "eventthread.h"

#include "binding-util.h"
#include "util/exception.h"
#include "input/input.h"
#include "sharedstate.h"
#include "src/util/util.h"

RB_METHOD(inputDelta) {
    RB_UNUSED_PARAM;
    
    return ULL2NUM(shState->input().getDelta());
}

RB_METHOD(inputUpdate) {
    RB_UNUSED_PARAM;
    
    shState->input().update();
    
    return Qnil;
}

static int getButtonArg(VALUE *argv) {
    int num;
    
    if (FIXNUM_P(*argv)) {
        num = FIX2INT(*argv);
    } else if (SYMBOL_P(*argv) && rgssVer >= 3) {
        VALUE symHash = getRbData()->buttoncodeHash;
#if RAPI_FULL > 187
        num = FIX2INT(rb_hash_lookup2(symHash, *argv, INT2FIX(Input::None)));
#else
        VALUE res = rb_hash_aref(symHash, *argv);
        if (!NIL_P(res))
            num = FIX2INT(res);
        else
            num = Input::None;
#endif
    } else {
        // FIXME: RMXP allows only few more types that
        // don't make sense (symbols in pre 3, floats)
        num = 0;
    }
    
    return num;
}

static int getScancodeArg(VALUE *argv) {
    const char *scancode = rb_id2name(SYM2ID(*argv));
    int code{};
    try {
        code = strToScancode[scancode];
    } catch (...) {
        rb_raise(rb_eRuntimeError, "%s is not a valid name of an SDL scancode.", scancode);
    }
    
    return code;
}

static int getControllerButtonArg(VALUE *argv) {
    const char *button = rb_id2name(SYM2ID(*argv));
    int btn{};
    try {
        btn = strToGCButton[button];
    } catch (...) {
        rb_raise(rb_eRuntimeError, "%s is not a valid name of an SDL Controller button.", button);
    }
    
    return btn;
}

RB_METHOD(inputPress) {
    RB_UNUSED_PARAM;
    
    rb_check_argc(argc, 1);
    
    VALUE button;
    rb_scan_args(argc, argv, "1", &button);
    
    int num = getButtonArg(&button);
    
    return rb_bool_new(shState->input().isPressed(num));
}

RB_METHOD(inputTrigger) {
    RB_UNUSED_PARAM;
    
    rb_check_argc(argc, 1);
    
    VALUE button;
    rb_scan_args(argc, argv, "1", &button);
    
    int num = getButtonArg(&button);
    
    return rb_bool_new(shState->input().isTriggered(num));
}

RB_METHOD(inputRepeat) {
    RB_UNUSED_PARAM;
    
    rb_check_argc(argc, 1);
    
    VALUE button;
    rb_scan_args(argc, argv, "1", &button);
    
    int num = getButtonArg(&button);
    
    return rb_bool_new(shState->input().isRepeated(num));
}

RB_METHOD(inputRelease) {
    RB_UNUSED_PARAM;
    
    rb_check_argc(argc, 1);
    
    VALUE button;
    rb_scan_args(argc, argv, "1", &button);
    
    int num = getButtonArg(&button);
    
    return rb_bool_new(shState->input().isReleased(num));
}

RB_METHOD(inputCount) {
    RB_UNUSED_PARAM;
    
    rb_check_argc(argc, 1);
    
    VALUE button;
    rb_scan_args(argc, argv, "1", &button);
    
    int num = getButtonArg(&button);
    
    return UINT2NUM(shState->input().count(num));
}

RB_METHOD(inputRepeatTime) {
    RB_UNUSED_PARAM;
    
    rb_check_argc(argc, 1);
    
    VALUE button;
    rb_scan_args(argc, argv, "1", &button);
    
    int num = getButtonArg(&button);
    
    return ULL2NUM(shState->input().repeatTime(num));
}

RB_METHOD(inputPressEx) {
    RB_UNUSED_PARAM;
    
    VALUE button;
    rb_scan_args(argc, argv, "1", &button);
    
    if (SYMBOL_P(button)) {
        int num = getScancodeArg(&button);
        return rb_bool_new(shState->input().isPressedEx(num, 0));
    }
    
    return rb_bool_new(shState->input().isPressedEx(NUM2INT(button), 1));
}

RB_METHOD(inputTriggerEx) {
    RB_UNUSED_PARAM;
    
    VALUE button;
    rb_scan_args(argc, argv, "1", &button);
    
    if (SYMBOL_P(button)) {
        int num = getScancodeArg(&button);
        return rb_bool_new(shState->input().isTriggeredEx(num, 0));
    }
    
    return rb_bool_new(shState->input().isTriggeredEx(NUM2INT(button), 1));
}

RB_METHOD(inputRepeatEx) {
    RB_UNUSED_PARAM;
    
    VALUE button;
    rb_scan_args(argc, argv, "1", &button);
    
    if (SYMBOL_P(button)) {
        int num = getScancodeArg(&button);
        return rb_bool_new(shState->input().isRepeatedEx(num, 0));
    }
    
    return rb_bool_new(shState->input().isRepeatedEx(NUM2INT(button), 1));
}

RB_METHOD(inputReleaseEx) {
    RB_UNUSED_PARAM;
    
    VALUE button;
    rb_scan_args(argc, argv, "1", &button);
    
    if (SYMBOL_P(button)) {
        int num = getScancodeArg(&button);
        return rb_bool_new(shState->input().isReleasedEx(num, 0));
    }
    
    return rb_bool_new(shState->input().isReleasedEx(NUM2INT(button), 1));
}

RB_METHOD(inputCountEx) {
    RB_UNUSED_PARAM;
    
    VALUE button;
    rb_scan_args(argc, argv, "1", &button);
    
    if (SYMBOL_P(button)) {
        int num = getScancodeArg(&button);
        return UINT2NUM(shState->input().repeatcount(num, 0));
    }
    
    return UINT2NUM(shState->input().repeatcount(NUM2INT(button), 1));
}

RB_METHOD(inputRepeatTimeEx) {
    RB_UNUSED_PARAM;
    
    VALUE button;
    rb_scan_args(argc, argv, "1", &button);
    
    if (SYMBOL_P(button)) {
        int num = getScancodeArg(&button);
        return ULL2NUM(shState->input().repeatTimeEx(num, 0));
    }
    
    return ULL2NUM(shState->input().repeatTimeEx(NUM2INT(button), 1));
}

RB_METHOD(inputDir4) {
    RB_UNUSED_PARAM;
    
    return rb_fix_new(shState->input().dir4Value());
}

RB_METHOD(inputDir8) {
    RB_UNUSED_PARAM;
    
    return rb_fix_new(shState->input().dir8Value());
}

/* Non-standard extensions */
RB_METHOD(inputMouseX) {
    RB_UNUSED_PARAM;
    
    return rb_fix_new(shState->input().mouseX());
}

RB_METHOD(inputMouseY) {
    RB_UNUSED_PARAM;
    
    return rb_fix_new(shState->input().mouseY());
}

RB_METHOD(inputScrollV) {
    RB_UNUSED_PARAM;
    
    return rb_fix_new(shState->input().scrollV());
}

RB_METHOD(inputMouseInWindow) {
    RB_UNUSED_PARAM;
    
    return rb_bool_new(shState->input().mouseInWindow());
}

RB_METHOD(inputRawKeyStates) {
    RB_UNUSED_PARAM;
    
    VALUE ret = rb_ary_new();

    uint8_t *states = shState->input().rawKeyStates();
    
    for (unsigned int i = 0; i < shState->input().rawKeyStatesLength(); i++)
        rb_ary_push(ret, rb_bool_new(states[i]));
    
    return ret;
}

#define M_SYMBOL(x) ID2SYM(rb_intern(x))
#define POWERCASE(v, c)                                                        \
case SDL_JOYSTICK_POWER_##c:                                                 \
v = M_SYMBOL(#c);                                                          \
break;

RB_METHOD(inputControllerConnected) {
    RB_UNUSED_PARAM;
    
    return rb_bool_new(shState->input().getControllerConnected());
}

RB_METHOD(inputControllerName) {
    RB_UNUSED_PARAM;
    
    if (!shState->input().getControllerConnected())
        return rb_utf8_str_new_cstr("");
    
    return rb_utf8_str_new_cstr(shState->input().getControllerName());
}

RB_METHOD(inputControllerPowerLevel) {
    RB_UNUSED_PARAM;
    
    VALUE ret;
    
    if (!shState->input().getControllerConnected())
        ret = M_SYMBOL("UNKNOWN");
    
    switch (shState->input().getControllerPowerLevel()) {
            POWERCASE(ret, MAX);
            POWERCASE(ret, WIRED);
            POWERCASE(ret, FULL);
            POWERCASE(ret, MEDIUM);
            POWERCASE(ret, LOW);
            POWERCASE(ret, EMPTY);
            
        default:
            ret = M_SYMBOL("UNKNOWN");
            break;
    }
    
    return ret;
}

RB_METHOD(inputKeyMapping) {
    RB_UNUSED_PARAM;
    
    rb_check_argc(argc, 1);
    
    VALUE button;
    rb_scan_args(argc, argv, "1", &button);

    VALUE keyHash = rb_hash_new();
    VALUE kbmKeys = rb_ary_new();
    VALUE gamepadKeys = rb_ary_new();
    rb_hash_aset(keyHash, M_SYMBOL("KBM"), kbmKeys);
    rb_hash_aset(keyHash, M_SYMBOL("GAMEPAD"), gamepadKeys);

    BDescVec binds;
	shState->rtData().bindingUpdateMsg.get(binds);
    int num = getButtonArg(&button);
    for (size_t i = 0; i < binds.size(); ++i)
    {
        if(binds[i].target == num) {
            switch(binds[i].src.type) {
                case Invalid:
                    break;
                case Key:
                    rb_ary_push(kbmKeys, rb_str_new_cstr(SDL_GetScancodeName(binds[i].src.d.scan)));
                    break;
                case CButton:
                    rb_ary_push(gamepadKeys, rb_str_new_cstr(shState->input().getButtonName(binds[i].src.d.cb)));
                    break;
                case CAxis:
                    rb_ary_push(gamepadKeys, rb_str_new_cstr(shState->input().getAxisName(binds[i].src.d.ca.axis)));
                    break;
                default:
                    break;
            }
        }
    } 
    
    return keyHash;
}

RB_METHOD(inputLastDevice) {
    RB_UNUSED_PARAM;    
    
    return M_SYMBOL(shState->eThread().getLastInputDevice().c_str());
}

#define AXISFUNC(n, ax1, ax2) \
RB_METHOD(inputControllerGet##n##Axis) {\
RB_UNUSED_PARAM;\
VALUE ret = rb_ary_new(); \
if (!shState->eThread().getControllerConnected()) {\
rb_ary_push(ret, rb_float_new(0)); rb_ary_push(ret, rb_float_new(0)); \
}\
rb_ary_push(ret, rb_float_new(shState->input().getControllerAxisValue(SDL_CONTROLLER_AXIS_##ax1) / 32767.0)); \
rb_ary_push(ret, rb_float_new(shState->input().getControllerAxisValue(SDL_CONTROLLER_AXIS_##ax2) / 32767.0)); \
return ret; \
}

AXISFUNC(Left, LEFTX, LEFTY);
AXISFUNC(Right, RIGHTX, RIGHTY);
AXISFUNC(Trigger, TRIGGERLEFT, TRIGGERRIGHT);

#undef POWERCASE
#undef M_SYMBOL

RB_METHOD(inputControllerPressEx) {
    RB_UNUSED_PARAM;
    
    VALUE button;
    rb_scan_args(argc, argv, "1", &button);
    
    if (SYMBOL_P(button)) {
        int num = getControllerButtonArg(&button);
        return rb_bool_new(shState->input().controllerIsPressedEx(num));
    }
    
    return rb_bool_new(shState->input().controllerIsPressedEx(NUM2INT(button)));
}

RB_METHOD(inputControllerTriggerEx) {
    RB_UNUSED_PARAM;
    
    VALUE button;
    rb_scan_args(argc, argv, "1", &button);
    
    if (SYMBOL_P(button)) {
        int num = getControllerButtonArg(&button);
        return rb_bool_new(shState->input().controllerIsTriggeredEx(num));
    }
    
    return rb_bool_new(shState->input().controllerIsTriggeredEx(NUM2INT(button)));
}

RB_METHOD(inputControllerRepeatEx) {
    RB_UNUSED_PARAM;
    
    VALUE button;
    rb_scan_args(argc, argv, "1", &button);
    
    if (SYMBOL_P(button)) {
        int num = getControllerButtonArg(&button);
        return rb_bool_new(shState->input().controllerIsRepeatedEx(num));
    }
    
    return rb_bool_new(shState->input().controllerIsRepeatedEx(NUM2INT(button)));
}

RB_METHOD(inputControllerReleaseEx) {
    RB_UNUSED_PARAM;
    
    VALUE button;
    rb_scan_args(argc, argv, "1", &button);
    
    if (SYMBOL_P(button)) {
        int num = getControllerButtonArg(&button);
        return rb_bool_new(shState->input().controllerIsReleasedEx(num));
    }
    
    return rb_bool_new(shState->input().controllerIsReleasedEx(NUM2INT(button)));
}

RB_METHOD(inputControllerCountEx) {
    RB_UNUSED_PARAM;
    
    VALUE button;
    rb_scan_args(argc, argv, "1", &button);
    
    if (SYMBOL_P(button)) {
        int num = getControllerButtonArg(&button);
        return rb_bool_new(shState->input().controllerRepeatcount(num));
    }
    
    return rb_bool_new(shState->input().controllerRepeatcount(NUM2INT(button)));
}

RB_METHOD(inputControllerRepeatTimeEx) {
    RB_UNUSED_PARAM;
    
    VALUE button;
    rb_scan_args(argc, argv, "1", &button);
    
    if (SYMBOL_P(button)) {
        int num = getControllerButtonArg(&button);
        return rb_bool_new(shState->input().controllerRepeatTimeEx(num));
    }
    
    return rb_bool_new(shState->input().controllerRepeatTimeEx(NUM2INT(button)));
}

RB_METHOD(inputControllerRawButtonStates) {
    RB_UNUSED_PARAM;
    
    VALUE ret = rb_ary_new();
    uint8_t *states = shState->input().rawButtonStates();
    
    for (unsigned int i = 0; i < shState->input().rawButtonStatesLength(); i++)
        rb_ary_push(ret, rb_bool_new(states[i]));
    
    return ret;
}

RB_METHOD(inputControllerRawAxes) {
    RB_UNUSED_PARAM;
    
    VALUE ret = rb_ary_new();
    int16_t *states = shState->input().rawAxes();
    
    for (unsigned int i = 0; i < shState->input().rawAxesLength(); i++)
        rb_ary_push(ret, rb_float_new(states[i] / 32767.0));
    
    return ret;
}

RB_METHOD(inputGetMode) {
    RB_UNUSED_PARAM;
    
    return rb_bool_new(shState->input().getTextInputMode());
}

RB_METHOD(inputSetMode) {
    RB_UNUSED_PARAM;
    
    bool mode;
    rb_get_args(argc, argv, "b", &mode RB_ARG_END);
    
    shState->input().setTextInputMode(mode);
    
    return mode;
}

RB_METHOD(inputGets) {
    RB_UNUSED_PARAM;
    shState->eThread().lockText(true);
    VALUE ret = rb_utf8_str_new_cstr(shState->input().getText());
    shState->input().clearText();
    shState->eThread().lockText(false);
    return ret;
}

RB_METHOD(inputGetClipboard) {
    RB_UNUSED_PARAM;
    VALUE ret;
    try {
        ret = rb_utf8_str_new_cstr(shState->input().getClipboardText());
    } catch (const Exception &e) {
        raiseRbExc(e);
    }
    return ret;
}

RB_METHOD(inputSetClipboard) {
    RB_UNUSED_PARAM;
    
    VALUE str;
    rb_scan_args(argc, argv, "1", &str);
    
    SafeStringValue(str);
    
    try {
        shState->input().setClipboardText(RSTRING_PTR(str));
    } catch (const Exception &e) {
        raiseRbExc(e);
    }
    return str;
}

struct {
    const char *str;
    Input::ButtonCode val;
} static buttonCodes[] = {{"DOWN", Input::Down},
    {"LEFT", Input::Left},
    {"RIGHT", Input::Right},
    {"UP", Input::Up},
    {"C", Input::C},
    {"Z", Input::Z},
    {"A", Input::A},
    {"B", Input::B},
    {"X", Input::X},
    {"Y", Input::Y},
    {"L", Input::L},
    {"R", Input::R},
    {"SHIFT", Input::Shift},
    {"CTRL", Input::Ctrl},
    {"ALT", Input::Alt},
    {"F5", Input::F5},
    {"F6", Input::F6},
    {"F7", Input::F7},
    {"F8", Input::F8},
    {"F9", Input::F9},
    
    {"MOUSELEFT", Input::MouseLeft},
    {"MOUSEMIDDLE", Input::MouseMiddle},
    {"MOUSERIGHT", Input::MouseRight},
    {"MOUSEX1", Input::MouseX1},
    {"MOUSEX2", Input::MouseX2}
};

static elementsN(buttonCodes);

void inputBindingInit() {
    VALUE module = rb_define_module("Input");
    
    _rb_define_module_function(module, "delta", inputDelta);
    _rb_define_module_function(module, "update", inputUpdate);
    _rb_define_module_function(module, "press?", inputPress);
    _rb_define_module_function(module, "trigger?", inputTrigger);
    _rb_define_module_function(module, "repeat?", inputRepeat);
    _rb_define_module_function(module, "release?", inputRelease);
    _rb_define_module_function(module, "count", inputCount);
    _rb_define_module_function(module, "time?", inputRepeatTime);
    _rb_define_module_function(module, "pressex?", inputPressEx);
    _rb_define_module_function(module, "triggerex?", inputTriggerEx);
    _rb_define_module_function(module, "repeatex?", inputRepeatEx);
    _rb_define_module_function(module, "releaseex?", inputReleaseEx);
    _rb_define_module_function(module, "repeatcount", inputCountEx);
    _rb_define_module_function(module, "timeex?", inputRepeatTimeEx);
    _rb_define_module_function(module, "dir4", inputDir4);
    _rb_define_module_function(module, "dir8", inputDir8);
    
    _rb_define_module_function(module, "mouse_x", inputMouseX);
    _rb_define_module_function(module, "mouse_y", inputMouseY);
    _rb_define_module_function(module, "scroll_v", inputScrollV);
    _rb_define_module_function(module, "mouse_in_window", inputMouseInWindow);
    _rb_define_module_function(module, "mouse_in_window?", inputMouseInWindow);
    
    _rb_define_module_function(module, "raw_key_states", inputRawKeyStates);
    
    VALUE submod = rb_define_module_under(module, "Controller");
    _rb_define_module_function(submod, "connected?", inputControllerConnected);
    _rb_define_module_function(submod, "name", inputControllerName);
    _rb_define_module_function(submod, "power_level", inputControllerPowerLevel);
    _rb_define_module_function(submod, "axes_left", inputControllerGetLeftAxis);
    _rb_define_module_function(submod, "axes_right", inputControllerGetRightAxis);
    _rb_define_module_function(submod, "axes_trigger", inputControllerGetTriggerAxis);
    _rb_define_module_function(submod, "raw_button_states", inputControllerRawButtonStates);
    _rb_define_module_function(submod, "raw_axes", inputControllerRawAxes);
    _rb_define_module_function(submod, "pressex?", inputControllerPressEx);
    _rb_define_module_function(submod, "triggerex?", inputControllerTriggerEx);
    _rb_define_module_function(submod, "repeatex?", inputControllerRepeatEx);
    _rb_define_module_function(submod, "releaseex?", inputControllerReleaseEx);
    _rb_define_module_function(submod, "repeatcount", inputControllerCountEx);
    _rb_define_module_function(submod, "timeex?", inputControllerRepeatTimeEx);
    
    _rb_define_module_function(module, "text_input", inputGetMode);
    _rb_define_module_function(module, "text_input=", inputSetMode);
    _rb_define_module_function(module, "gets", inputGets);
    
    _rb_define_module_function(module, "clipboard", inputGetClipboard);
    _rb_define_module_function(module, "clipboard=", inputSetClipboard);

    _rb_define_module_function(module, "key_mapping", inputKeyMapping);
    _rb_define_module_function(module, "last_device", inputLastDevice);
    
    if (rgssVer >= 3) {
        VALUE symHash = rb_hash_new();
        
        for (size_t i = 0; i < buttonCodesN; ++i) {
            ID sym = rb_intern(buttonCodes[i].str);
            VALUE val = INT2FIX(buttonCodes[i].val);
            
            /* In RGSS3 all Input::XYZ constants are equal to :XYZ symbols,
             * to be compatible with the previous convention */
            rb_const_set(module, sym, ID2SYM(sym));
            rb_hash_aset(symHash, ID2SYM(sym), val);
        }
        
        rb_iv_set(module, "buttoncodes", symHash);
        getRbData()->buttoncodeHash = symHash;
    } else {
        for (size_t i = 0; i < buttonCodesN; ++i) {
            ID sym = rb_intern(buttonCodes[i].str);
            VALUE val = INT2FIX(buttonCodes[i].val);
            
            rb_const_set(module, sym, val);
        }
    }
}
