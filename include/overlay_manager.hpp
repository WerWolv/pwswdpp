#pragma once

#include <cstdint>
#include <queue>

#include <sys/time.h>

#include "framebuffer.hpp"

namespace pwswd {

    enum class OverlayType {
        None,

        VolumeSlider,
        BrightnessSlider,
        SharpnessSlider,

        AspectRatioPopup,
        MutePopup,
        HeadphonesPopup,
        MouseModePopup,
        JoystickModePopup,
        ScreenshotPopup
    };

    struct Overlay {
        OverlayType type;
        std::uint32_t value;
        std::uint32_t visibleMs;
    };

    class OverlayManager {
    public:
        OverlayManager() {
            this->m_currOverlay = { OverlayType::None, 0, 0 };
            this->m_framebuffer = nullptr;
        }

        void initialize(pwswd::Framebuffer *framebuffer) {
            this->m_framebuffer = framebuffer;
        }

        void enqueueOverlay(Overlay overlay) {
            this->m_overlayQueue.push(overlay);
            printf("Added overlay\n");
        }

        void render() {
            printf("1\n");
            // Don't draw overlays if the framebuffer hasn't been set
            if (this->m_framebuffer == nullptr)
                return;
            printf("2\n");
            // Nothing to render if no overlay is in queue or currently visible
            if (this->m_overlayQueue.empty() && this->m_currOverlay.type == OverlayType::None)
                return;
            printf("3\n");
            // If there's currently no overlay visible but the queue isn't empty, dequeue the oldest one
            if (this->m_currOverlay.type == OverlayType::None) {
                this->dequeueOverlay();
                printf("4\n");
            }
            printf("5\n");
            timeval currTime;
            if (gettimeofday(std::addressof(currTime), nullptr) != 0)
                return;
            printf("6\n");
            // Remove the current overlay once its time has ellapsed
            std::uint64_t ellapsedTime = (currTime.tv_sec - this->m_startTime.tv_sec) * 1E6 + (currTime.tv_usec - this->m_startTime.tv_usec);
            if (ellapsedTime >= (this->m_currOverlay.visibleMs * 1E3))
                this->m_currOverlay = { OverlayType::None, 0, 0 };
            printf("7\n");
            // Render overlays
            switch (this->m_currOverlay.type) {
                case OverlayType::VolumeSlider:
                    this->m_framebuffer->drawRect(0, 0, 50, 50, 0xFF0000FF);
                    break;
                case OverlayType::BrightnessSlider:
                    this->m_framebuffer->drawRect(0, 0, 50, 50, 0x00FF00FF);
                    break;
                default: break;
            }
        }

    private:
        timeval m_startTime;

        Overlay m_currOverlay;
        std::queue<Overlay> m_overlayQueue;

        pwswd::Framebuffer *m_framebuffer;

        void dequeueOverlay() {
            if (gettimeofday(std::addressof(this->m_startTime), nullptr) != 0)
                return;

            this->m_currOverlay = this->m_overlayQueue.front();
            this->m_overlayQueue.pop();
        }
    };

}