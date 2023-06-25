/*
 ** input.cpp
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

#include "input.h"
#include "config.h"
#include "sharedstate.h"
#include "eventthread.h"
#include "input/keybindings.h"
#include "util/exception.h"
#include "util/util.h"

#include <SDL_scancode.h>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>
#include <SDL_clipboard.h>

#include <vector>
#include <cmath>
#include <unordered_map>
#include <string.h>
#include <assert.h>

#define BUTTON_CODE_COUNT 26

#define m(vk,sc) { vk, SDL_SCANCODE_##sc }
std::unordered_map<int, int> vKeyToScancode{
    // 0x01 LEFT MOUSE
    // 0x02 RIGHT MOUSE
    m(0x03, CANCEL),
    // 0x04 MIDDLE MOUSE
    // 0x05 XBUTTON 1
    // 0x06 XBUTTON 2
    // 0x07 undef
    m(0x08, BACKSPACE),
    m(0x09, TAB),
    // 0x0a reserved
    // 0x0b reserved
    m(0x0c, CLEAR),
    m(0x0d, RETURN),
    // 0x0e undefined
    // 0x0f undefined
    // 0x10 SHIFT (both)
    // 0x11 CONTROL (both)
    // 0x12 ALT (both)
    m(0x13, PAUSE),
    m(0x14, CAPSLOCK),
    // 0x15 KANA, HANGUL
    // 0x16 undefined
    // 0x17 JUNJA
    // 0x18 FINAL
    // 0x19 HANJA, KANJI
    // 0x1a undefined
    m(0x1b, ESCAPE),
    // 0x1c CONVERT
    // 0x1d NONCONVERT
    // 0x1e ACCEPT
    // 0x1f MODECHANGE
    m(0x20, SPACE),
    m(0x21, PAGEUP),
    m(0x22, PAGEDOWN),
    m(0x23, END),
    m(0x24, HOME),
    m(0x25, LEFT),
    m(0x26, UP),
    m(0x27, RIGHT),
    m(0x28, DOWN),
    m(0x29, SELECT),
    // 0x2A print
    m(0x2b, EXECUTE),
    m(0x2c, PRINTSCREEN),
    m(0x2d, INSERT),
    m(0x2e, DELETE),
    m(0x2f, HELP),
    m(0x30, 0), m(0x31, 1),
    m(0x32, 2), m(0x33, 3),
    m(0x34, 4), m(0x35, 5),
    m(0x36, 6), m(0x37, 7),
    m(0x38, 8), m(0x39, 9),
    // 0x3a-0x40 undefined
    m(0x41, A), m(0x42, B),
    m(0x43, C), m(0x44, D),
    m(0x45, E), m(0x46, F),
    m(0x47, G), m(0x48, H),
    m(0x49, I), m(0x4a, J),
    m(0x4b, K), m(0x4c, L),
    m(0x4d, M), m(0x4e, N),
    m(0x4f, O), m(0x50, P),
    m(0x51, Q), m(0x52, R),
    m(0x53, S), m(0x54, T),
    m(0x55, U), m(0x56, V),
    m(0x57, W), m(0x58, X),
    m(0x59, Y), m(0x5a, Z),
    m(0x5b, LGUI), m(0x5c, RGUI),
    m(0x5d, APPLICATION),
    // 0x5e reserved
    m(0x5f, SLEEP),
    m(0x60, KP_0), m(0x61, KP_1),
    m(0x62, KP_2), m(0x63, KP_3),
    m(0x64, KP_4), m(0x65, KP_5),
    m(0x66, KP_6), m(0x67, KP_7),
    m(0x68, KP_8), m(0x69, KP_9),
    m(0x6a, KP_MULTIPLY),
    m(0x6b, KP_PLUS),
    m(0x6c, RETURN), // SEPARATOR
    m(0x6d, KP_MINUS),
    m(0x6e, KP_DECIMAL),
    m(0x6f, KP_DIVIDE),
    m(0x70, F1), m(0x71, F2),
    m(0x72, F3), m(0x73, F4),
    m(0x74, F5), m(0x75, F6),
    m(0x76, F7), m(0x77, F8),
    m(0x78, F9), m(0x79, F10),
    m(0x7a, F11), m(0x7b, F12),
    m(0x7c, F13), m(0x7d, F14),
    m(0x7e, F15), m(0x7f, F16),
    m(0x80, F17), m(0x81, F18),
    m(0x82, F19), m(0x83, F20),
    m(0x84, F21), m(0x85, F22),
    m(0x86, F23), m(0x87, F24),
    // 0x88-0x8f unassigned
    m(0x90, NUMLOCKCLEAR),
    m(0x91, SCROLLLOCK),
    // 0x92-0x96 oem specific
    // 0x97-0x9f unassigned
    m(0xa0, LSHIFT),
    m(0xa1, RSHIFT),
    m(0xa2, LCTRL),
    m(0xa3, RCTRL),
    m(0xa4, LALT),
    m(0xa5, RALT),
    m(0xa6, AC_BACK),
    m(0xa7, AC_FORWARD),
    m(0xa8, AC_REFRESH),
    m(0xa9, AC_STOP),
    m(0xaa, AC_SEARCH),
    // 0xab BROWSER_FAVORITES
    m(0xac, AC_HOME),
    m(0xad, AUDIOMUTE),
    m(0xae, VOLUMEDOWN),
    m(0xaf, VOLUMEUP),
    m(0xb0, AUDIONEXT),
    m(0xb1, AUDIOPREV),
    m(0xb2, AUDIOSTOP),
    m(0xb3, AUDIOPLAY),
    m(0xb4, MAIL),
    m(0xb5, MEDIASELECT),
    // 0xb6 LAUNCH_APP1
    // 0xb7 LAUNCH_APP2
    // 0xb8-0xb9 reserved
    
    // Everything below here is OEM
    // and can vary by country
    m(0xba, SEMICOLON),
    m(0xbb, EQUALS),
    m(0xbc, COMMA),
    m(0xbd, MINUS),
    m(0xbe, PERIOD),
    m(0xbf, SLASH),
    m(0xc0, GRAVE),
    // 0xc1-0xd7 reserved
    // 0xd8-0xda unassigned
    m(0xdb, LEFTBRACKET),
    m(0xdc, BACKSLASH),
    m(0xdd, RIGHTBRACKET),
    m(0xde, APOSTROPHE),
    // 0xdf OEM_8
    // 0xe0 reserved
    // 0xe1 oem-specific
    // 0xe2 OEM_102
    // 0xe3-0xe4 oem-specific
    // 0xe5 PROCESSKEY
    // 0xe6 oem-specific
    // 0xe7 PACKET
    // 0xe8 unassigned
    // 0xe9-0xf5 oem_specific
    // 0xf6 ATTN
    m(0xf7, CRSEL),
    m(0xf8, EXSEL),
    // 0xf9 EREOF
    m(0xfa, AUDIOPLAY), // PLAY, guessing
    // 0xfb ZOOM
    // 0xfc NONAME
    // 0xfd PA1
    m(0xfe, CLEAR)
};
#undef m
#define m(keycode) { #keycode, SDL_SCANCODE_##keycode }
#define m_number(keycode) { "NUMBER_" #keycode, SDL_SCANCODE_##keycode }
std::unordered_map<std::string, int> strToScancode{
    m_number(0), m_number(1),
    m_number(2), m_number(3),
    m_number(4), m_number(5),
    m_number(6), m_number(7),
    m_number(8), m_number(9),
    m(A),
    m(AC_BACK),
    m(AC_BOOKMARKS),
    m(AC_FORWARD),
    m(AC_HOME),
    m(AC_REFRESH),
    m(AC_SEARCH),
    m(AC_STOP),
    m(AGAIN),
    m(ALTERASE),
    m(APOSTROPHE),
    m(APPLICATION),
    m(AUDIOMUTE),
    m(AUDIONEXT),
    m(AUDIOPLAY),
    m(AUDIOPREV),
    m(AUDIOSTOP),
    m(B),
    m(BACKSLASH),
    m(BACKSPACE),
    m(BRIGHTNESSDOWN),
    m(BRIGHTNESSUP),
    m(C),
    m(CALCULATOR),
    m(CANCEL),
    m(CAPSLOCK),
    m(CLEAR),
    m(CLEARAGAIN),
    m(COMMA),
    m(COMPUTER),
    m(COPY),
    m(CRSEL),
    m(CURRENCYSUBUNIT),
    m(CURRENCYUNIT),
    m(CUT),
    m(D),
    m(DECIMALSEPARATOR),
    m(DELETE),
    m(DISPLAYSWITCH),
    m(DOWN),
    m(E),
    m(EJECT),
    m(END),
    m(EQUALS),
    m(ESCAPE),
    m(EXECUTE),
    m(EXSEL),
    m(F),
    m(F1),  m(F2),
    m(F3),  m(F4),
    m(F5),  m(F6),
    m(F7),  m(F8),
    m(F9),  m(F10),
    m(F11), m(F12),
    m(F13), m(F14),
    m(F15), m(F16),
    m(F17), m(F18),
    m(F19), m(F20),
    m(F21), m(F22),
    m(F23), m(F24),
    m(FIND),
    m(G),
    m(GRAVE),
    m(H),
    m(HELP),
    m(HOME),
    m(I),
    m(INSERT),
    m(J),
    m(K),
    m(KBDILLUMDOWN),
    m(KBDILLUMTOGGLE),
    m(KBDILLUMUP),
    m(KP_0),
    m(KP_00),
    m(KP_000),
    m(KP_1), m(KP_2),
    m(KP_3), m(KP_4),
    m(KP_5), m(KP_6),
    m(KP_7), m(KP_8),
    m(KP_9),
    m(KP_A),
    m(KP_AMPERSAND),
    m(KP_AT),
    m(KP_B),
    m(KP_BACKSPACE),
    m(KP_BINARY),
    m(KP_C),
    m(KP_CLEAR),
    m(KP_CLEARENTRY),
    m(KP_COLON),
    m(KP_COMMA),
    m(KP_D),
    m(KP_DBLAMPERSAND),
    m(KP_DBLVERTICALBAR),
    m(KP_DECIMAL),
    m(KP_DIVIDE),
    m(KP_E),
    m(KP_ENTER),
    m(KP_EQUALS),
    m(KP_EQUALSAS400),
    m(KP_EXCLAM),
    m(KP_F),
    m(KP_GREATER),
    m(KP_HASH),
    m(KP_HEXADECIMAL),
    m(KP_LEFTBRACE),
    m(KP_LEFTPAREN),
    m(KP_LESS),
    m(KP_MEMADD),
    m(KP_MEMCLEAR),
    m(KP_MEMDIVIDE),
    m(KP_MEMMULTIPLY),
    m(KP_MEMRECALL),
    m(KP_MEMSTORE),
    m(KP_MEMSUBTRACT),
    m(KP_MINUS),
    m(KP_MULTIPLY),
    m(KP_OCTAL),
    m(KP_PERCENT),
    m(KP_PERIOD),
    m(KP_PLUS),
    m(KP_PLUSMINUS),
    m(KP_POWER),
    m(KP_RIGHTBRACE),
    m(KP_RIGHTPAREN),
    m(KP_SPACE),
    m(KP_TAB),
    m(KP_VERTICALBAR),
    m(KP_XOR),
    m(L),
    m(LALT),
    m(LCTRL),
    m(LEFT),
    m(LEFTBRACKET),
    m(LGUI),
    m(LSHIFT),
    m(M),
    m(MAIL),
    m(MEDIASELECT),
    m(MENU),
    m(MINUS),
    m(MODE),
    m(MUTE),
    m(N),
    m(NUMLOCKCLEAR),
    m(O),
    m(OPER),
    m(OUT),
    m(P),
    m(PAGEDOWN),
    m(PAGEUP),
    m(PASTE),
    m(PAUSE),
    m(PERIOD),
    m(POWER),
    m(PRINTSCREEN),
    m(PRIOR),
    m(Q),
    m(R),
    m(RALT),
    m(RCTRL),
    m(RETURN),
    m(RETURN2),
    m(RGUI),
    m(RIGHT),
    m(RIGHTBRACKET),
    m(RSHIFT),
    m(S),
    m(SCROLLLOCK),
    m(SELECT),
    m(SEMICOLON),
    m(SEPARATOR),
    m(SLASH),
    m(SLEEP),
    m(SPACE),
    m(STOP),
    m(SYSREQ),
    m(T),
    m(TAB),
    m(THOUSANDSSEPARATOR),
    m(U),
    m(UNDO),
    m(UNKNOWN),
    m(UP),
    m(V),
    m(VOLUMEDOWN),
    m(VOLUMEUP),
    m(W),
    m(WWW),
    m(X),
    m(Y),
    m(Z),
    m(INTERNATIONAL1),
    m(INTERNATIONAL2),
    m(INTERNATIONAL3),
    m(INTERNATIONAL4),
    m(INTERNATIONAL5),
    m(INTERNATIONAL6),
    m(INTERNATIONAL7),
    m(INTERNATIONAL8),
    m(INTERNATIONAL9),
    m(LANG1), m(LANG2),
    m(LANG3), m(LANG4),
    m(LANG5), m(LANG6),
    m(LANG7), m(LANG8),
    m(LANG9),
    m(NONUSBACKSLASH),
    m(NONUSHASH)
};

#undef m
#define m(ctrl) {#ctrl, SDL_CONTROLLER_BUTTON_##ctrl}

std::unordered_map<std::string, SDL_GameControllerButton> strToGCButton {
    m(A), m(B), m(X), m(Y),
    m(BACK), m(GUIDE), m(START),
    m(LEFTSTICK), m(RIGHTSTICK),
    m(LEFTSHOULDER), m(RIGHTSHOULDER),
    m(DPAD_UP), m(DPAD_DOWN), m(DPAD_LEFT), m(DPAD_RIGHT)
    
};

#undef m

const char *axisNames[] {
    "LStick X",
    "LStick Y",
    "RStick X",
    "RStick Y",
    "LTrigger",
    "RTrigger"
};

const char *buttonNames[] {
    "Pad A",
    "Pad B",
    "Pad X",
    "Pad Y",
    "Pad Back",
    "Pad Guide",
    "Pad Start",
    "Pad LS",
    "Pad RS",
    "Pad LB",
    "Pad RB",
    "Pad Up",
    "Pad Down",
    "Pad Left",
    "Pad Right",
    "Pad Misc",
    "Pad Paddle1",
    "Pad Paddle2",
    "Pad Paddle3",
    "Pad Paddle4",
    "Pad Touchpad"
};

struct ButtonState
{
    bool pressed;
    bool triggered;
    bool repeated;
    bool released;
    
    ButtonState()
    : pressed(false),
    triggered(false),
    repeated(false),
    released(false)
    {}
};

struct KbBindingData
{
    SDL_Scancode source;
    Input::ButtonCode target;
};

struct Binding
{
    Binding(Input::ButtonCode target = Input::None)
    : target(target)
    {}
    
    virtual bool sourceActive() const = 0;
    virtual bool sourceRepeatable() const = 0;
    
    Input::ButtonCode target;
};

/* Keyboard binding */
struct KbBinding : public Binding
{
    KbBinding() {}
    
    KbBinding(const KbBindingData &data)
    : Binding(data.target),
    source(data.source)
    {}
    
    bool sourceActive() const
    {
        /* Special case aliases */
        if (source == SDL_SCANCODE_LSHIFT)
            return EventThread::keyStates[source]
            || EventThread::keyStates[SDL_SCANCODE_RSHIFT];
        
        if (source == SDL_SCANCODE_RETURN)
            return EventThread::keyStates[source]
            || EventThread::keyStates[SDL_SCANCODE_KP_ENTER];
        
        return EventThread::keyStates[source];
    }
    
    bool sourceRepeatable() const
    {
        return true;
        /*
         return (source >= SDL_SCANCODE_A     && source <= SDL_SCANCODE_0)    ||
         (source >= SDL_SCANCODE_RIGHT && source <= SDL_SCANCODE_UP)   ||
         (source >= SDL_SCANCODE_F1    && source <= SDL_SCANCODE_F12);
         */
    }
    
    SDL_Scancode source;
};

struct CtrlButtonBinding : public Binding
{
    CtrlButtonBinding() {}
    
    bool sourceActive() const
    {
        return EventThread::controllerState.buttons[source];
    }
    
    bool sourceRepeatable() const
    {
        return true;
    }
    
    SDL_GameControllerButton source;
};

struct CtrlAxisBinding : public Binding
{
    CtrlAxisBinding() {}
    
    CtrlAxisBinding(uint8_t source, AxisDir dir, Input::ButtonCode target)
    : Binding(target), source(source), dir(dir) {}
    
    bool sourceActive() const
    {
        float val = EventThread::controllerState.axes[source];
        
        if (dir == Negative)
            return val < -JAXIS_THRESHOLD;
        else
            return val > JAXIS_THRESHOLD;
    }
    
    bool sourceRepeatable() const {
        return true;
    }
    
    uint8_t source;
    AxisDir dir;
};

/* Mouse button binding */
struct MsBinding : public Binding
{
    MsBinding() {}
    
    MsBinding(int buttonIndex,
              Input::ButtonCode target)
    : Binding(target),
    index(buttonIndex)
    {}
    
    bool sourceActive() const
    {
        return EventThread::mouseState.buttons[index];
    }
    
    bool sourceRepeatable() const
    {
        return true;
    }
    
    int index;
};

/* Not rebindable */
static const KbBindingData staticKbBindings[] =
{
    { SDL_SCANCODE_LSHIFT, Input::Shift },
    { SDL_SCANCODE_RSHIFT, Input::Shift },
    { SDL_SCANCODE_LCTRL,  Input::Ctrl  },
    { SDL_SCANCODE_RCTRL,  Input::Ctrl  },
    { SDL_SCANCODE_LALT,   Input::Alt   },
    { SDL_SCANCODE_RALT,   Input::Alt   },
    { SDL_SCANCODE_F5,     Input::F5    },
    { SDL_SCANCODE_F6,     Input::F6    },
    { SDL_SCANCODE_F7,     Input::F7    },
    { SDL_SCANCODE_F8,     Input::F8    },
    { SDL_SCANCODE_F9,     Input::F9    }
};

static elementsN(staticKbBindings);

/* Maps ButtonCode enum values to indices
 * in the button state array */
static const int mapToIndex[] =
{
    0, 0,
    1, 0, 2, 0, 3, 0, 4, 0,
    0,
    5, 6, 7, 8, 9, 10, 11, 12,
    0, 0,
    13, 14, 15,
    0,
    16, 17, 18, 19, 20,
    0, 0, 0, 0, 0, 0, 0, 0,
    21, 22, 23, 24, 25
};

static elementsN(mapToIndex);

static const Input::ButtonCode dirs[] =
{ Input::Down, Input::Left, Input::Right, Input::Up };

static const int dirFlags[] =
{
    1 << Input::Down,
    1 << Input::Left,
    1 << Input::Right,
    1 << Input::Up
};

/* Dir4 is always zero on these combinations */
static const int deadDirFlags[] =
{
    dirFlags[0] | dirFlags[3],
    dirFlags[1] | dirFlags[2]
};

static const Input::ButtonCode otherDirs[4][3] =
{
    { Input::Left, Input::Right, Input::Up    }, /* Down  */
    { Input::Down, Input::Up,    Input::Right }, /* Left  */
    { Input::Down, Input::Up,    Input::Left  }, /* Right */
    { Input::Left, Input::Right, Input::Up    }  /* Up    */
};

struct InputPrivate
{
    std::vector<KbBinding> kbStatBindings;
    std::vector<KbBinding> kbBindings;
    std::vector<CtrlAxisBinding> ctrlABindings;
    std::vector<CtrlButtonBinding> ctrlBBindings;
    std::vector<MsBinding> msBindings;
    
    /* Collective binding array */
    std::vector<Binding*> bindings;
    
    ButtonState stateArray[BUTTON_CODE_COUNT*2];
    
    ButtonState *states;
    ButtonState *statesOld;
    
    // Raw keystates
    uint8_t rawStateArray[SDL_NUM_SCANCODES*2];
    
    uint8_t *rawStates;
    uint8_t *rawStatesOld;
    
    // Gamepad button states
    uint8_t rawButtonStateArray[SDL_CONTROLLER_BUTTON_MAX*2];
    
    uint8_t *rawButtonStates;
    uint8_t *rawButtonStatesOld;
    
    // Gamepad axes & mouse coordinates
    int16_t axisStateArray[SDL_CONTROLLER_AXIS_MAX];
    int mousePos[2];
    bool mouseInWindow;
    
    Input::ButtonCode repeating;
    int rawRepeating;
    int buttonRepeating;
    
    unsigned int repeatCount;
    unsigned int rawRepeatCount;
    unsigned int buttonRepeatCount;
    
    unsigned long long repeatTime;
    unsigned long long rawRepeatTime;
    unsigned long long buttonRepeatTime;
    
    unsigned int repeatStart;
    unsigned int repeatDelay;
    
    unsigned long long last_update;

    int vScrollDistance;
    
    struct
    {
        int active;
        Input::ButtonCode previous;
    } dir4Data;
    
    struct
    {
        int active;
    } dir8Data;
    
    void recalcRepeatTime(unsigned int fps) {
        double framems = 1.f / fps;
        
        // Approximate time in milliseconds
        double start = (rgssVer >= 2) ? 0.375 : 0.400;
        double delay = 0.100;
        
        repeatStart = ceil(start / framems);
        repeatDelay = ceil(delay / framems);
    }
    
    InputPrivate(const RGSSThreadData &rtData)
    {
        last_update = 0;
        
        initStaticKbBindings();
        initMsBindings();
        
        /* Main thread should have these posted by now */
        checkBindingChange(rtData);
        
        int fps = rtData.config.fixedFramerate;
        if (!fps) fps = (rgssVer >= 2) ? 60 : 40;
        recalcRepeatTime(fps);
        
        states    = stateArray;
        statesOld = stateArray + BUTTON_CODE_COUNT;
        
        rawStates = rawStateArray;
        rawStatesOld = rawStateArray + SDL_NUM_SCANCODES;
        
        rawButtonStates = rawButtonStateArray;
        rawButtonStatesOld = rawButtonStateArray + SDL_CONTROLLER_BUTTON_MAX;
        
        /* Clear buffers */
        clearBuffer();
        swapBuffers();
        clearBuffer();
        
        repeating = Input::None;
        repeatCount = 0;
        
        dir4Data.active = 0;
        dir4Data.previous = Input::None;
        
        dir8Data.active = 0;
        
        vScrollDistance = 0;
    }
    
    inline ButtonState &getStateCheck(int code)
    {
        int index;
        
        if (code < 0 || (size_t) code > mapToIndexN-1)
            index = 0;
        else
            index = mapToIndex[code];
        
        return states[index];
    }
    
    inline ButtonState &getState(Input::ButtonCode code)
    {
        return states[mapToIndex[code]];
    }
    
    inline ButtonState &getOldState(Input::ButtonCode code)
    {
        return statesOld[mapToIndex[code]];
    }
    
    ButtonState getStateRaw(int code, bool useVKey)
    {
        ButtonState b;
        int scancode = (useVKey) ? -1 : code;
        if (scancode < 0)
        {
            switch (code)
            {
                case 0x10:
                    return getState(Input::Shift);
                    break;
                    
                case 0x11:
                    return getState(Input::Ctrl);
                    break;
                    
                case 0x12:
                    return getState(Input::Alt);
                    break;
                    
                case 0x1:
                    return getState(Input::MouseLeft);
                    break;
                    
                case 0x2:
                    return getState(Input::MouseRight);
                    break;
                    
                case 0x4:
                    return getState(Input::MouseMiddle);
                    break;
                    
                default:
                    ButtonState b;
                    try
                {
                    scancode = vKeyToScancode[code];
                }
                    catch (...) {}
                    if (scancode == -1) return b;
                    break;
            }
        }
        
        b.pressed = rawStates[scancode];
        b.triggered = (rawStates[scancode] && !rawStatesOld[scancode]);
        b.released = (!rawStates[scancode] && rawStatesOld[scancode]);
        
        b.repeated = (rawRepeating == scancode) && (rawRepeatCount >= repeatStart && ((rawRepeatCount+1) % repeatDelay) == 0);
        
        return b;
    }
    
    ButtonState getControllerButtonState(int button) {
        ButtonState b;
        
        b.pressed = rawButtonStates[button];
        b.triggered = (rawButtonStates[button] && !rawButtonStatesOld[button]);
        b.released = (!rawButtonStates[button] && rawButtonStatesOld[button]);
        
        b.repeated = (buttonRepeating == button) && (buttonRepeatCount >= repeatStart && ((buttonRepeatCount+1) % repeatDelay) == 0);
        
        return b;
    }
    
    void swapBuffers()
    {
        ButtonState *tmp = states;
        states = statesOld;
        statesOld = tmp;
        
        uint8_t *tmpr = rawStates;
        rawStates = rawStatesOld;
        rawStatesOld = tmpr;
        
        uint8_t *tmpb = rawButtonStates;
        rawButtonStates = rawButtonStatesOld;
        rawButtonStatesOld = tmpb;
    }
    
    void clearBuffer()
    {
        const size_t size = sizeof(ButtonState) * BUTTON_CODE_COUNT;
        memset(states, 0, size);
        
        memset(rawStates, 0, SDL_NUM_SCANCODES);
        
        memset(rawButtonStates, 0, SDL_CONTROLLER_BUTTON_MAX);
    }
    
    void checkBindingChange(const RGSSThreadData &rtData)
    {
        BDescVec d;
        
        if (!rtData.bindingUpdateMsg.poll(d))
            return;
        
        applyBindingDesc(d);
    }
    
    template<class B>
    void appendBindings(std::vector<B> &bind)
    {
        for (size_t i = 0; i < bind.size(); ++i)
            bindings.push_back(&bind[i]);
    }
    
    void applyBindingDesc(const BDescVec &d)
    {
        kbBindings.clear();
        ctrlABindings.clear();
        ctrlBBindings.clear();
        
        for (size_t i = 0; i < d.size(); ++i)
        {
            const BindingDesc &desc = d[i];
            const SourceDesc &src = desc.src;
            
            if (desc.target == Input::None)
                continue;
            
            switch (desc.src.type)
            {
                case Invalid :
                    break;
                case Key :
                {
                    KbBinding bind;
                    bind.source = src.d.scan;
                    bind.target = desc.target;
                    kbBindings.push_back(bind);
                    
                    break;
                }
                case CAxis :
                {
                    CtrlAxisBinding bind;
                    bind.source = src.d.ca.axis;
                    bind.dir = src.d.ca.dir;
                    bind.target = desc.target;
                    ctrlABindings.push_back(bind);
                    
                    break;
                }
                case CButton :
                {
                    CtrlButtonBinding bind;
                    bind.source = src.d.cb;
                    bind.target = desc.target;
                    ctrlBBindings.push_back(bind);
                    
                    break;
                }
                default :
                    assert(!"unreachable");
            }
        }
        
        bindings.clear();
        
        appendBindings(kbStatBindings);
        appendBindings(msBindings);
        
        appendBindings(kbBindings);
        appendBindings(ctrlABindings);
        appendBindings(ctrlBBindings);
    }
    
    void initStaticKbBindings()
    {
        kbStatBindings.clear();
        
        for (size_t i = 0; i < staticKbBindingsN; ++i)
            kbStatBindings.push_back(KbBinding(staticKbBindings[i]));
    }
    
    void initMsBindings()
    {
        msBindings.resize(5);
        
        size_t i = 0;
        msBindings[i++] = MsBinding(SDL_BUTTON_LEFT,   Input::MouseLeft);
        msBindings[i++] = MsBinding(SDL_BUTTON_MIDDLE, Input::MouseMiddle);
        msBindings[i++] = MsBinding(SDL_BUTTON_RIGHT,  Input::MouseRight);
        msBindings[i++] = MsBinding(SDL_BUTTON_X1, Input::MouseX1);
        msBindings[i++] = MsBinding(SDL_BUTTON_X2, Input::MouseX2);
    }
    
    void pollBindings(Input::ButtonCode &repeatCand)
    {
        for (Binding *bind : bindings) {
            // Get all binding states
            pollBindingPriv(*bind, repeatCand);
        }
        
        for (Binding *bind : bindings) {
            // Check for released buttons, now that all the bindings
            // have been checked
            ButtonState &state = getState(bind->target);
            ButtonState &oldState = getOldState(bind->target);
            if (!state.pressed && oldState.pressed)
                state.released = true;
        }
        
        updateDir4();
        updateDir8();
    }
    
    void pollBindingPriv(const Binding &b,
                         Input::ButtonCode &repeatCand)
    {
        if (!b.sourceActive())
            return;
        
        if (b.target == Input::None)
            return;
        
        ButtonState &state = getState(b.target);
        ButtonState &oldState = getOldState(b.target);
        
        state.pressed = true;
        
        /* Must have been released before to trigger */
        if (!oldState.pressed)
            state.triggered = true;
        
        /* Unbound keys don't create/break repeat */
        if (repeatCand != Input::None)
            return;
        
        if (repeating != b.target &&
            !oldState.pressed)
        {
            if (b.sourceRepeatable())
                repeatCand = b.target;
            else
            /* Unrepeatable keys still break current repeat */
                repeating = Input::None;
        }
    }
    
    void updateRaw()
    {
        
        memcpy(rawStates, shState->eThread().keyStates, SDL_NUM_SCANCODES);
        
        for (int i = 0; i < SDL_NUM_SCANCODES; i++)
        {
            if (rawStates[i] && rawStatesOld[i])
            {
                if (rawRepeating == i)
                {
                    rawRepeatCount++;
                }
                else
                {
                    rawRepeatCount = 0;
                    rawRepeatTime = shState->runTime();
                    rawRepeating = i;
                }
                
                return;
            }
        }
        
        rawRepeating = -1;
        
    }
    
    void updateControllerRaw()
    {
        for (int i = 0; i < SDL_CONTROLLER_AXIS_MAX; i++)
            axisStateArray[i] = shState->eThread().controllerState.axes[i];
        
        memcpy(rawButtonStates, shState->eThread().controllerState.buttons, SDL_CONTROLLER_BUTTON_MAX);
        
        for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; i++)
        {
            if (rawButtonStates[i] && rawButtonStatesOld[i])
            {
                if (buttonRepeating == i)
                    rawRepeatCount++;
                else
                {
                    buttonRepeatCount = 0;
                    buttonRepeatTime = shState->runTime();
                    buttonRepeating = i;
                }
                
                return;
            }
        }
        
        buttonRepeating = -1;
    }
    
    void updateDir4()
    {
        int dirFlag = 0;
        
        for (size_t i = 0; i < 4; ++i)
            dirFlag |= (getState(dirs[i]).pressed ? dirFlags[i] : 0);
        
        if (dirFlag == deadDirFlags[0] || dirFlag == deadDirFlags[1])
        {
            dir4Data.active = Input::None;
            return;
        }
        
        if (dir4Data.previous != Input::None)
        {
            /* Check if prev still pressed */
            if (getState(dir4Data.previous).pressed)
            {
                for (size_t i = 0; i < 3; ++i)
                {
                    Input::ButtonCode other =
                    otherDirs[(dir4Data.previous/2)-1][i];
                    
                    if (!getState(other).pressed)
                        continue;
                    
                    dir4Data.active = other;
                    return;
                }
            }
        }
        
        for (size_t i = 0; i < 4; ++i)
        {
            if (!getState(dirs[i]).pressed)
                continue;
            
            dir4Data.active = dirs[i];
            dir4Data.previous = dirs[i];
            return;
        }
        
        dir4Data.active   = Input::None;
        dir4Data.previous = Input::None;
    }
    
    void updateDir8()
    {
        static const int combos[4][4] =
        {
            { 2, 1, 3, 0 },
            { 1, 4, 0, 7 },
            { 3, 0, 6, 9 },
            { 0, 7, 9, 8 }
        };
        
        dir8Data.active = 0;
        
        for (size_t i = 0; i < 4; ++i)
        {
            Input::ButtonCode one = dirs[i];
            
            if (!getState(one).pressed)
                continue;
            
            for (int j = 0; j < 3; ++j)
            {
                Input::ButtonCode other = otherDirs[i][j];
                
                if (!getState(other).pressed)
                    continue;
                
                dir8Data.active = combos[(one/2)-1][(other/2)-1];
                return;
            }
            
            dir8Data.active = one;
            return;
        }
    }
};


Input::Input(const RGSSThreadData &rtData)
{
    p = new InputPrivate(rtData);
}

unsigned long long Input::getDelta() {
    return shState->runTime() - p->last_update;
}

void Input::recalcRepeat(unsigned int fps) {
    p->recalcRepeatTime(fps);
}

void Input::update()
{
    shState->checkShutdown();
    p->checkBindingChange(shState->rtData());
    
    p->swapBuffers();
    p->clearBuffer();
    
    ButtonCode repeatCand = None;
    
    /* Poll all bindings */
    p->pollBindings(repeatCand);
    
    // Update raw keys, controller buttons and axes
    p->updateRaw();
    p->updateControllerRaw();
    
    // Record mouse positions
    p->mousePos[0] = shState->eThread().mouseState.x;
    p->mousePos[1] = shState->eThread().mouseState.y;
    p->mouseInWindow = shState->eThread().mouseState.inWindow;
    
    
    /* Check for new repeating key */
    if (repeatCand != None && repeatCand != p->repeating)
    {
        p->repeating = repeatCand;
        p->repeatCount = 0;
        p->repeatTime = shState->runTime();
        p->getState(repeatCand).repeated = true;
        
        p->last_update = p->repeatTime;
        return;
    }
    
    /* Check if repeating key is still pressed */
    if (p->getState(p->repeating).pressed)
    {
        p->repeatCount++;
        
        /*
         bool repeated;
         if (rgssVer >= 2)
         repeated = p->repeatCount >= 23 && ((p->repeatCount+1) % 6) == 0;
         else
         repeated = p->repeatCount >= 15 && ((p->repeatCount+1) % 4) == 0;
         */
        bool repeated = p->repeatCount >= p->repeatStart && ((p->repeatCount+1) % p->repeatDelay) == 0;
        p->getState(p->repeating).repeated |= repeated;
        
        p->last_update = shState->runTime();
        return;
    }
    
    p->repeating = None;
    
    /* Fetch new cumulative scroll distance and reset counter */
    p->vScrollDistance = SDL_AtomicSet(&EventThread::verticalScrollDistance, 0);
    
    p->last_update = shState->runTime();
}

std::vector<std::string> Input::getBindings(ButtonCode code) {
    std::vector<std::string> ret;
    for (const auto &b : p->kbBindings) {
        if (b.target != code) continue;
        ret.push_back(SDL_GetScancodeName(b.source));
    }
    
    for (const auto &b : p->ctrlBBindings) {
        if (b.target != code) continue;
        ret.push_back(std::string("CBUTTON") + std::to_string(b.source));
    }
    
    for (const auto &b : p->ctrlABindings) {
        if (b.target != code) continue;
        ret.push_back(std::string("CAXIS") + std::to_string(b.source));
    }
    
    return ret;
}

bool Input::isPressed(int button)
{
    return p->getStateCheck(button).pressed;
}

bool Input::isTriggered(int button)
{
    return p->getStateCheck(button).triggered;
}

bool Input::isRepeated(int button)
{
    return p->getStateCheck(button).repeated;
}

bool Input::isReleased(int button) {
    return p->getStateCheck(button).released;
}

unsigned int Input::count(int button) {
    if (button != p->repeating)
        return 0;
    
    return p->repeatCount;
}

unsigned long long Input::repeatTime(int button) {
    if (button != p->repeating)
        return 0;
    
    return shState->runTime() - p->repeatTime;
}

bool Input::isPressedEx(int code, bool isVKey)
{
    return p->getStateRaw(code, isVKey).pressed;
}

bool Input::isTriggeredEx(int code, bool isVKey)
{
    return p->getStateRaw(code, isVKey).triggered;
}

bool Input::isRepeatedEx(int code, bool isVKey)
{
    return p->getStateRaw(code, isVKey).repeated;
}

bool Input::isReleasedEx(int code, bool isVKey)
{
    return p->getStateRaw(code, isVKey).released;
}

bool Input::controllerIsPressedEx(int button) {
    return p->getControllerButtonState(button).pressed;
}

bool Input::controllerIsTriggeredEx(int button) {
    return p->getControllerButtonState(button).triggered;
}

bool Input::controllerIsRepeatedEx(int button) {
    return p->getControllerButtonState(button).repeated;
}

bool Input::controllerIsReleasedEx(int button) {
    return p->getControllerButtonState(button).released;
}

unsigned int Input::repeatcount(int code, bool isVKey) {
    int c = code;
    if (isVKey) {
        try {
            c = vKeyToScancode[code];
        }
        catch (...) {
            return 0;
        }
    }
    if (c != p->rawRepeating)
        return 0;
    
    return p->rawRepeatCount;
}

unsigned int Input::controllerRepeatcount(int button) {
    if (button != p->buttonRepeating)
        return 0;
    
    return p->buttonRepeatCount;
}

unsigned long long Input::repeatTimeEx(int code, bool isVKey) {
    int c = code;
    if (isVKey) {
        try { c = vKeyToScancode[code]; }
        catch (...) { return 0; }
    }
    
    if (c != p->rawRepeating)
        return 0;
    
    return shState->runTime() - p->rawRepeatTime;
}

unsigned long long Input::controllerRepeatTimeEx(int button) {
    if (button != p->buttonRepeating)
        return 0;
    
    return shState->runTime() - p->buttonRepeatTime;
}

uint8_t *Input::rawKeyStates(){
    return p->rawStates;
}

unsigned int Input::rawKeyStatesLength() {
    return sizeof(p->rawStateArray) / 2;
}

uint8_t *Input::rawButtonStates() {
    return p->rawButtonStates;
}

unsigned int Input::rawButtonStatesLength() {
    return sizeof(p->rawButtonStateArray) / 2;
}

int16_t *Input::rawAxes() {
    return p->axisStateArray;
}

unsigned int Input::rawAxesLength() {
    return sizeof(p->axisStateArray) / sizeof(int16_t);
}

short Input::getControllerAxisValue(SDL_GameControllerAxis axis) {
    if (axis < 0 || (uint32_t)axis >= rawAxesLength())
        return 0;
    
    return rawAxes()[axis];
}

bool Input::mouseInWindow() {
    return p->mouseInWindow;
}

int Input::dir4Value()
{
    return p->dir4Data.active;
}

int Input::dir8Value()
{
    return p->dir8Data.active;
}

int Input::mouseX()
{
    RGSSThreadData &rtData = shState->rtData();
    
    return (p->mousePos[0] - rtData.screenOffset.x) * rtData.sizeResoRatio.x;
}

int Input::mouseY()
{
    RGSSThreadData &rtData = shState->rtData();
    
    return (p->mousePos[1] - rtData.screenOffset.y) * rtData.sizeResoRatio.y;
}

int Input::scrollV()
 {
     return p->vScrollDistance;
 }

bool Input::getControllerConnected()
{
    return shState->eThread().getControllerConnected();
}

const char *Input::getControllerName()
{
    return (getControllerConnected()) ?
    SDL_GameControllerName(shState->eThread().controller()) :
    0;
}

int Input::getControllerPowerLevel()
{
    if (!getControllerConnected())
        return SDL_JOYSTICK_POWER_UNKNOWN;
    
    SDL_Joystick *js = SDL_GameControllerGetJoystick(shState->eThread().controller());
    return SDL_JoystickCurrentPowerLevel(js);
}

bool Input::getTextInputMode()
{
    return (SDL_IsTextInputActive() == SDL_TRUE);
}

void Input::setTextInputMode(bool mode)
{
    shState->eThread().requestTextInputMode(mode);
}

const char *Input::getText()
{
    return shState->eThread().textInputBuffer.c_str();
}

void Input::clearText()
{
    shState->eThread().textInputBuffer.clear();
}

char *Input::getClipboardText()
{
    char *tx = SDL_GetClipboardText();
    if (!tx)
        throw Exception(Exception::SDLError, "Failed to get clipboard text: %s", SDL_GetError());
    return tx;
}

void Input::setClipboardText(char *text)
{
    if (SDL_SetClipboardText(text) < 0)
        throw Exception(Exception::SDLError, "Failed to set clipboard text: %s", SDL_GetError());
}

const char *Input::getAxisName(SDL_GameControllerAxis axis) {
    if (axis < 0 || axis >= SDL_CONTROLLER_AXIS_MAX)
        return "Invalid";
    
    return axisNames[axis];
}

const char *Input::getButtonName(SDL_GameControllerButton button) {
    if (button < 0 || button >= SDL_CONTROLLER_BUTTON_MAX)
        return "Invalid";
    
    return buttonNames[button];
}

Input::~Input()
{
    delete p;
}
