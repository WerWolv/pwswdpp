#include <iostream>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cmath>
#include <map>
#include <thread>
#include <mutex>

#include "events.hpp"
#include "overlay_manager.hpp"

#include "devices/event_poller.hpp"
#include "devices/framebuffer.hpp"
#include "devices/uinput.hpp"
#include "devices/screen.hpp"
#include "devices/audio.hpp"
#include "devices/power.hpp"

static pwswd::dev::EventPoller buttonEvent("/dev/input/event0");
static pwswd::dev::EventPoller joystickEvent("/dev/input/event3");

static pwswd::dev::Framebuffer framebuffer("/dev/fb0");

static pwswd::dev::UInput mouse("/dev/uinput", "OpenDingux mouse daemon", { 0x03, 1, 1, 1 });
static pwswd::dev::Screen screen;
static pwswd::dev::Audio audio;
static pwswd::dev::Power power;

static pwswd::OverlayManager overlayManager;
static pwswd::MouseMode mouseModeState = pwswd::MouseMode::Deactivated;
static std::mutex mousePositionLock;
static std::int8_t mouseVelocityX = 0, mouseVelocityY = 0;

void handlePowerShortcut(pwswd::Button button) {
    switch (button) {
        case pwswd::Button::Start:
            mouse.inject(pwswd::createButtonInputEvent(pwswd::Button::Home, pwswd::ButtonState::Pressed));
            mouse.inject(pwswd::createButtonInputEvent(pwswd::Button::Home, pwswd::ButtonState::Released));
            mouse.inject(pwswd::createSyncEvent());
            break;
        case pwswd::Button::Select:
            power.killForegroundApplication();
            break;
        case pwswd::Button::DpadRight:
            screen.increaseSharpness();
            break;
        case pwswd::Button::DpadLeft:
            screen.decreaseSharpness();
            break;
        case pwswd::Button::DpadUp:
            screen.increaseBrightness();
            break;
        case pwswd::Button::DpadDown:
            screen.decreaseBrightness();
            break;
        case pwswd::Button::VolumeUp:
            screen.toggleDisplayStyle();
            break;
        case pwswd::Button::VolumeDown:
            audio.mute();
            break;
        case pwswd::Button::L3:
            if (mouseModeState == pwswd::MouseMode::LeftJoyStick) {
                joystickEvent.ungrab();
                mouseModeState = pwswd::MouseMode::Deactivated;
            }
            else {
                joystickEvent.grab();
                mouseModeState = pwswd::MouseMode::LeftJoyStick;
            }
            break;
        case pwswd::Button::R3:
            if (mouseModeState == pwswd::MouseMode::RightJoyStick) {
                joystickEvent.ungrab();
                mouseModeState = pwswd::MouseMode::Deactivated;
            }
            else {
                joystickEvent.grab();
                mouseModeState = pwswd::MouseMode::RightJoyStick;
            }
            break;
        default: break;
    }
}

void handleShortcuts(pwswd::Button button) {
    switch (button) {
        case pwswd::Button::VolumeUp:
            audio.increase();
            break;
        case pwswd::Button::VolumeDown:
            audio.decrease();
            break;
        default: break;
    }
}



void drawOverlay() {
    // map the framebuffer into the address space
    framebuffer.map();

    while (true) {
        {
            std::scoped_lock lock(framebuffer);

            // Refresh variable screen info to detect resolution / bpp changes
            framebuffer.refreshScreenInfo();
            // Draw overlays
            overlayManager.render();
        }

        usleep(1000);
    }
}

void calculateMouseMovement() {
    std::int32_t joystickDisplacementX = 0, joystickDisplacementY = 0;

    while (true) {
        joystickEvent.wait();

        auto eventData = joystickEvent.get<pwswd::InputEvent>();

        const auto &type = static_cast<pwswd::EventType>(eventData.type);

        // Don't handle inputs when the screen is off or mouse mode is disabled
        if (power.isScreenOff() || mouseModeState == pwswd::MouseMode::Deactivated)
            continue;

        // When mouse mode is active, pass through any button press events
        if (type == pwswd::EventType::Buttons) {
            const auto &button = static_cast<pwswd::Button>(eventData.code);

            // Remap L3 to left mouse button and R3 to right mouse button
            if (button == pwswd::Button::L3)
                eventData.code = static_cast<std::uint16_t>(pwswd::Button::MouseLeft);
            else if (button == pwswd::Button::R3)
                eventData.code = static_cast<std::uint16_t>(pwswd::Button::MouseRight);

            mouse.inject(eventData);
            continue;

        // Discard any other events except the relative axis one
        } else if (type == pwswd::EventType::Synchronization)
            continue;


        const auto &axis = static_cast<pwswd::RelativeAxis>(eventData.code);
        const auto &value  = eventData.value;

        // Only use selected joystick for mouse movement
        if (mouseModeState == pwswd::MouseMode::LeftJoyStick && (axis == pwswd::RelativeAxis::AxisRX || axis == pwswd::RelativeAxis::AxisRY))
            continue;
        if (mouseModeState == pwswd::MouseMode::RightJoyStick && (axis == pwswd::RelativeAxis::AxisX || axis == pwswd::RelativeAxis::AxisY))
            continue;        

        // Determine the mouse movement direction and speed based on the right joystick's deplacement 
        switch (axis) {
            case pwswd::RelativeAxis::AxisX:
                joystickDisplacementX = pwswd::JoyStickXAxisCenter - value;
                break;
            case pwswd::RelativeAxis::AxisRX:
                joystickDisplacementX = value - pwswd::JoyStickXAxisCenter;
                break;
            case pwswd::RelativeAxis::AxisY:
                joystickDisplacementY = pwswd::JoyStickYAxisCenter - value;
                break;
            case pwswd::RelativeAxis::AxisRY:
                joystickDisplacementY = value - pwswd::JoyStickYAxisCenter;
                break;
        }

        //  When joystick is outside the deadzone, linearly determine the cursor speed from it
        if (std::abs(joystickDisplacementX) > pwswd::JoyStickDeadZone)
            mouseVelocityX = (joystickDisplacementX > 0 ? 1 : -1) * ((std::abs(joystickDisplacementX) - pwswd::JoyStickDeadZone) / pwswd::JoyStickSpeedDownscaler);
        else 
            mouseVelocityX = 0;

        if (std::abs(joystickDisplacementY) > pwswd::JoyStickDeadZone)
            mouseVelocityY = (joystickDisplacementY > 0 ? 1 : -1) * ((std::abs(joystickDisplacementY) - pwswd::JoyStickDeadZone) / pwswd::JoyStickSpeedDownscaler);
        else 
            mouseVelocityY = 0;
        
    }
}

void moveMouse() {
    // Initialize mouse uinput device
    mouse.setEventFilterBit(pwswd::EventType::Buttons);
    mouse.setKeyFilterBit(pwswd::Button::MouseLeft);
    mouse.setKeyFilterBit(pwswd::Button::MouseRight);

    mouse.setKeyFilterBit(pwswd::Button::A);
    mouse.setKeyFilterBit(pwswd::Button::B);
    mouse.setKeyFilterBit(pwswd::Button::X);
    mouse.setKeyFilterBit(pwswd::Button::Y);
    mouse.setKeyFilterBit(pwswd::Button::DpadUp);
    mouse.setKeyFilterBit(pwswd::Button::DpadDown);
    mouse.setKeyFilterBit(pwswd::Button::DpadLeft);
    mouse.setKeyFilterBit(pwswd::Button::DpadRight);
    mouse.setKeyFilterBit(pwswd::Button::Start);
    mouse.setKeyFilterBit(pwswd::Button::Select);
    mouse.setKeyFilterBit(pwswd::Button::L1);
    mouse.setKeyFilterBit(pwswd::Button::L2);
    mouse.setKeyFilterBit(pwswd::Button::R1);
    mouse.setKeyFilterBit(pwswd::Button::R2);
    mouse.setKeyFilterBit(pwswd::Button::Home);

    mouse.setEventFilterBit(pwswd::EventType::RelativeAxes);
    mouse.setRelativeFilterBit(pwswd::RelativeAxis::AxisX);
    mouse.setRelativeFilterBit(pwswd::RelativeAxis::AxisY);
    mouse.createDevice();

    while (true) {
        mouse.inject(pwswd::createRelativeAxisInputEvent(pwswd::RelativeAxis::AxisX, mouseVelocityX));
        mouse.inject(pwswd::createRelativeAxisInputEvent(pwswd::RelativeAxis::AxisY, mouseVelocityY));
        mouse.inject(pwswd::createSyncEvent());

        usleep(10000);
    }
}

int main() {
    bool activatedShortcut = false;
    bool powerButtonDown = false;
    timeval timeSincePowerButtonPress = { 0 };

    // Initialize services and devices
    overlayManager.initialize(std::addressof(framebuffer));
    power.initialize(std::addressof(buttonEvent), std::addressof(screen));

    // Prevent Hangup signals from terminating us
    signal(SIGHUP, [](int){});

    // Start overlay drawing thread
    std::thread overlayThread(drawOverlay);
    std::thread joystickToMouseCalculatorThread(calculateMouseMovement);
    std::thread mouseMoveThread(moveMouse);

    while (true) {
        // Wait for any button to be pressed
        buttonEvent.wait();

        auto eventData = buttonEvent.get<pwswd::InputEvent>();

        const auto &type   = static_cast<pwswd::EventType>(eventData.type);
        const auto &button = static_cast<pwswd::Button>(eventData.code);
        const auto &state  = static_cast<pwswd::ButtonState>(eventData.value);

        // Discard any synchronization events
        if (type == pwswd::EventType::Synchronization)
            continue;

        // Handle power button press
        if (button == pwswd::Button::Power) {
            std::uint64_t timeSincePowerButtonDown = pwswd::toMicroSeconds(eventData.time) - pwswd::toMicroSeconds(timeSincePowerButtonPress);

            switch (state) {
                case pwswd::ButtonState::Pressed:
                    timeSincePowerButtonPress = eventData.time;
                    powerButtonDown = true;
                    buttonEvent.grab();     // Prevent applications from getting any button inputs
                    break;
                case pwswd::ButtonState::Released:  
                    
                    // Unblock button inputs for other apps
                    buttonEvent.ungrab();

                    // Don't enter sleep mode if the user pressed any button after holding down the power button
                    if (!activatedShortcut) {
                        if (timeSincePowerButtonDown < pwswd::PowerButtonShortPressDuration) {
                            // Lock drawing to the framebuffer
                            std::scoped_lock lock(framebuffer);
                            // Close the framebuffer device to prevent pwswd++ from being paused
                            framebuffer.close();

                            // Toggle sleep mode
                            power.toggleSleepMode();

                            // Reopen the framebuffer device after pausing is done
                            framebuffer.open();
                        }
                    }

                    activatedShortcut = false;
                    powerButtonDown = false;

                    break;
                case pwswd::ButtonState::Held:

                    // If no button was pressed after the power button was held down for a certain duration, power off the device
                    if (!activatedShortcut)
                        if (timeSincePowerButtonDown > pwswd::PowerButtonLongPressDuration)
                            power.powerOff();
                    break;
            }
        
        // Handle all other button presses
        } else {
            if (state == pwswd::ButtonState::Pressed) {
                // Don't handle shortcuts if the screen is off
                if (power.isScreenOff())
                    continue;
                
                // Handle shortcuts with the power button held down
                if (powerButtonDown) {
                    handlePowerShortcut(static_cast<pwswd::Button>(eventData.code));
                    activatedShortcut = true;

                // Handle shortcuts without the power button held down
                } else {
                    handleShortcuts(static_cast<pwswd::Button>(eventData.code));
                }
            }
        }
    }
}