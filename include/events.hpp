#pragma once

#include <cstdint>
#include <sys/time.h>

namespace pwswd {

    #define DEAD_ZONE		450
#define SLOW_MOUSE_ZONE		600
#define AXIS_ZERO_0		1730
#define AXIS_ZERO_1		1730
#define AXIS_ZERO_3		1620
#define AXIS_ZERO_4		1620

    constexpr std::uint16_t JoyStickDeadZone = 450;

    enum class ButtonState {
        Released = 0,
        Pressed = 1,
        Held = 2
    };

    enum class Button {
        Select      = 1,
        Start       = 28,
        A           = 29,
        B           = 56,
        X           = 57,
        Y           = 42,
        L1          = 15,
        L2          = 104,
        L3          = 98,
        R1          = 14,
        R2          = 109,
        R3          = 83,
        DpadUp      = 103,
        DpadDown    = 108,
        DpadLeft    = 105,
        DpadRight   = 106,
        Power       = 116,
        VolumeUp    = 115,
        VolumeDown  = 114
    };

    struct input_event {
        struct timeval time;
        std::uint16_t type;
        std::uint16_t code;
        std::uint32_t value;
    };

}