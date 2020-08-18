#pragma once

#include <sys/wait.h>

#include "event_poller.hpp"
#include "screen.hpp"

namespace pwswd::dev {

    class Power {
    public:
        Power() : m_buttonEvent(nullptr), m_screen(nullptr), m_isScreenOff(false) {}

        void initialize(pwswd::dev::EventPoller *buttonEvent, pwswd::dev::Screen *screen) {
            this->m_buttonEvent = buttonEvent;
            this->m_screen = screen;
        }

        void powerOff() {
            this->m_screen->enableBlanking();
            execlp("/sbin/poweroff", "/sbin/poweroff", nullptr);
        }

        void toggleSleepMode() {

            if (!this->m_isScreenOff) {
                this->m_buttonEvent->grab();
                this->m_screen->enableBlanking();
                this->m_screen->stopRendering();
            } else {
                this->m_screen->disableBlanking();
                this->m_screen->continueRendering();
                this->m_buttonEvent->ungrab();
            }

            this->m_isScreenOff = !this->m_isScreenOff;
        }

        void killForegroundApplication() {
            // Kill framebuffer application
            {
                pid_t child = fork();
                if (!child)
                    execlp("fuser", "fuser", "-k", "-HUP", "/dev/fb0", nullptr);

                int status = 0;
                waitpid(child, &status, 0);

                if (!status)
                    return;
            }

            // Kill console application
            {
                pid_t child = fork();
                if (!child)
                    execlp("fuser", "fuser", "-k", "-HUP", "/dev/tty1", nullptr);

                int status = 0;
                waitpid(child, &status, 0);
            }
        }

        bool isScreenOff() {
            return this->m_isScreenOff;
        }

    private:
        bool m_isScreenOff;

        pwswd::dev::EventPoller *m_buttonEvent;
        pwswd::dev::Screen *m_screen;
    };

}