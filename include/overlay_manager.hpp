#pragma once

#include <cstdint>
#include <queue>

#include <sys/time.h>

#include "devices/framebuffer.hpp"
#include "events.hpp"

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
        std::uint32_t timeoutMs;
    };

    class OverlayManager {
    public:
        OverlayManager() {
            this->m_currOverlay = { OverlayType::None, 0, 0 };
            this->m_framebuffer = nullptr;
        }

        void initialize(pwswd::dev::Framebuffer *framebuffer) {
            this->m_framebuffer = framebuffer;
        }

        void enqueueOverlay(Overlay overlay) {
            this->m_overlayQueue.push(overlay);
        }

        void renewOverlay(std::uint32_t newTimeoutMs = 0) {
            if (this->m_currOverlay.type == OverlayType::None)
                return;

            if (gettimeofday(std::addressof(this->m_startTime), nullptr) != 0)
                return;

            if (newTimeoutMs > 0)
                this->m_currOverlay.timeoutMs = newTimeoutMs;
        }

        void render() {
            // Don't draw overlays if the framebuffer hasn't been set
            if (this->m_framebuffer == nullptr)
                return;

            // Nothing to render if no overlay is in queue or currently visible
            if (this->m_overlayQueue.empty() && this->m_currOverlay.type == OverlayType::None)
                return;

            // If there's currently no overlay visible but the queue isn't empty, dequeue the oldest one
            if (this->m_currOverlay.type == OverlayType::None) {
                this->dequeueOverlay();
            }

            timeval currTime;
            if (gettimeofday(std::addressof(currTime), nullptr) != 0)
                return;

            // Remove the current overlay once its time has ellapsed
            std::uint64_t ellapsedTime = pwswd::toMicroSeconds(currTime) - pwswd::toMicroSeconds(this->m_startTime);
            if (ellapsedTime >= (this->m_currOverlay.timeoutMs * 1E3))
                this->m_currOverlay = { OverlayType::None, 0, 0 };

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

        pwswd::dev::Framebuffer *m_framebuffer;

        void dequeueOverlay() {
            if (gettimeofday(std::addressof(this->m_startTime), nullptr) != 0)
                return;

            this->m_currOverlay = this->m_overlayQueue.front();
            this->m_overlayQueue.pop();
        }
    };

}