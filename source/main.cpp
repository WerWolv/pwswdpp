#include <iostream>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <map>
#include <thread>
#include <mutex>

#include "events.hpp"
#include "event_poller.hpp"
#include "framebuffer.hpp"
#include "overlay_manager.hpp"
#include "devices/screen.hpp"
#include "devices/audio.hpp"
#include "devices/power.hpp"

static constexpr std::uint32_t PowerButtonShortPressDuration = 300E3;
static constexpr std::uint32_t PowerButtonLongPressDuration = 2E6;

static pwswd::EventPoller buttonEvent("/dev/input/event0");
static pwswd::Framebuffer framebuffer("/dev/fb0");
static pwswd::OverlayManager overlayManager;
static pwswd::dev::Screen screen;
static pwswd::dev::Audio audio;
static pwswd::dev::Power power;

void handlePowerShortcut(pwswd::Button button) {
    switch (button) {
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
            audio.toggle();
            break;
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
    }
}

void drawOverlay() {
    // map the framebuffer into the address space
    framebuffer.map();

    while (true) {
        {
            std::scoped_lock lock(framebuffer);

            // Refresh variable and fixed screen info to detect resolution / bpp changes
            framebuffer.refreshScreenInfo();
            // Draw overlays
            overlayManager.render();
        }

        usleep(1000);
    }
}

int main() {
    bool activatedShortcut = false;
    bool powerButtonDown = false;
    timeval timeSincePowerButtonPress;

    // Initialize services and devices
    overlayManager.initialize(std::addressof(framebuffer));
    power.initialize(std::addressof(buttonEvent), std::addressof(screen));

    // Prevent Hangup signals from terminating us
    signal(SIGHUP, [](int){});

    // Start overlay drawing thread
    std::thread overlayThread(drawOverlay);

    while (true) {
        // Wait for any button to be pressed
        buttonEvent.wait();

        auto eventData = buttonEvent.get<pwswd::input_event>();

        if (eventData.code == 0)
            continue;

        // Handle power button press
        if (eventData.code == static_cast<std::uint16_t>(pwswd::Button::Power)) {
            std::uint64_t timeSincePowerButtonDown = (eventData.time.tv_sec - timeSincePowerButtonPress.tv_sec) * 1E6 + (eventData.time.tv_usec - timeSincePowerButtonPress.tv_usec);

            switch (static_cast<pwswd::ButtonState>(eventData.value)) {
                case pwswd::ButtonState::Pressed:
                    timeSincePowerButtonPress = eventData.time;
                    powerButtonDown = true;
                    buttonEvent.grab();     // Prevent applications from getting any button inputs
                    break;
                case pwswd::ButtonState::Released:  
                    
                    // Don't enter sleep mode if the user pressed any button after holding down the power button
                    if (!activatedShortcut) {
                        if (timeSincePowerButtonDown < PowerButtonShortPressDuration) {
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

                    // Unblock button inputs for other apps
                    buttonEvent.ungrab();
                    break;
                case pwswd::ButtonState::Held:

                    // If no button was pressed after the power button was held down for a certain duration, power off the device
                    if (!activatedShortcut)
                        if (timeSincePowerButtonDown > PowerButtonLongPressDuration)
                            power.powerOff();
                    break;
            }
        
        // Handle all other button presses
        } else {
            if (static_cast<pwswd::ButtonState>(eventData.value) == pwswd::ButtonState::Pressed) {
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


    /*pwswd::EventPoller joystickEvent("/dev/input/event3");

    while (true) {
        joystickEvent.wait();

        auto eventData = joystickEvent.get<input_event>();

        if (eventData.code != 0)
            continue;

        std::cout << eventData.code << " " << eventData.value << std::endl;
    }*/
}