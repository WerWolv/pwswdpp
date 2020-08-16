#pragma once

#include <string>

#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <stdexcept>

namespace pwswd::dev {

    class Screen {
    public:
        Screen() {
            this->m_blankingfd = open("/sys/class/graphics/fb0/blank", O_RDWR);
            this->m_sharpnessUpscalingfd = open("/sys/devices/platform/jz-lcd.0/sharpness_upscaling", O_RDWR);
            this->m_sharpnessDownscalingfd = open("/sys/devices/platform/jz-lcd.0/sharpness_downscaling", O_RDWR);
            this->m_keepAspectRatiofd = open("/sys/devices/platform/jz-lcd.0/keep_aspect_ratio", O_RDWR);
            this->m_integerScalingfd = open("/sys/devices/platform/jz-lcd.0/integer_scaling", O_RDWR);
            this->m_brightnessfd = open("/sys/devices/platform/pwm-backlight/backlight/pwm-backlight/brightness", O_RDWR);

            if (this->m_blankingfd == 0 || this->m_sharpnessUpscalingfd == 0 || this->m_sharpnessDownscalingfd == 0 || this->m_keepAspectRatiofd == 0 || this->m_brightnessfd == 0)
                throw std::runtime_error("Failed to access framebuffer settings");

            char buffer[10] = { 0 };
            read(this->m_sharpnessUpscalingfd, buffer, 9);
            this->m_sharpness = strtol(buffer, nullptr, 10);

            read(this->m_keepAspectRatiofd, buffer, 1);
            this->m_displayStyle = buffer[0] == 'Y';

            read(this->m_integerScalingfd, buffer, 1);
            this->m_displayStyle |= (buffer[0] == 'Y') << 1;

            this->m_brightnessIndex = 19;
        }

        ~Screen() {
            close(this->m_blankingfd);
            close(this->m_sharpnessUpscalingfd);
            close(this->m_sharpnessDownscalingfd);
        }

        void enableBlanking() {
            write(this->m_blankingfd, "1", 1);
        }

        void disableBlanking() {
            write(this->m_blankingfd, "0", 1);
        }

        void stopRendering() {
            pid_t child = fork();

            if (!child)
                execlp("fuser", "fuser", "-k", "-STOP", "/dev/fb0", nullptr);


            int status = 0;
            waitpid(child, &status, 0);
        }

        void continueRendering() {
            pid_t child = fork();

            if (!child)
                execlp("fuser", "fuser", "-k", "-CONT", "/dev/fb0", nullptr);

            int status = 0;
            waitpid(child, &status, 0);        
        }

        void increaseSharpness() {
            if (this->m_sharpness == 0)
                return;

            this->m_sharpness--;

            auto sharpnessString = std::to_string(this->m_sharpness);

            write(this->m_sharpnessUpscalingfd, sharpnessString.c_str(), sharpnessString.length());
            write(this->m_sharpnessDownscalingfd, sharpnessString.c_str(), sharpnessString.length());
        }

        void decreaseSharpness() {
            if (this->m_sharpness >= 32)
                return;

            this->m_sharpness++;

            auto sharpnessString = std::to_string(this->m_sharpness);

            write(this->m_sharpnessUpscalingfd, sharpnessString.c_str(), sharpnessString.length());
            write(this->m_sharpnessDownscalingfd, sharpnessString.c_str(), sharpnessString.length());
        }

        void increaseBrightness() {
            if (this->m_brightnessIndex >= sizeof(BrightnessValues))
                return;

            this->m_brightnessIndex++;

            auto brightnessString = std::to_string(BrightnessValues[this->m_brightnessIndex]);

            write(this->m_brightnessfd, brightnessString.c_str(), brightnessString.length());
        }

        void decreaseBrightness() {
            if (this->m_brightnessIndex == 0)
                return;

            this->m_brightnessIndex--;

            auto brightnessString = std::to_string(BrightnessValues[this->m_brightnessIndex]);

            write(this->m_brightnessfd, brightnessString.c_str(), brightnessString.length());
        }

        void toggleDisplayStyle() {
            if (this->m_displayStyle == 3)
                this->m_displayStyle = 0;
            else
                this->m_displayStyle++;

            write(this->m_keepAspectRatiofd, (this->m_displayStyle & 0b01) ? "Y" : "N", 1);
            write(this->m_integerScalingfd, (this->m_displayStyle & 0b10) ? "Y" : "N", 1);
        }

    private:
        static constexpr std::uint8_t BrightnessValues[] = { 8, 9, 10, 11, 12, 13, 14, 15, 16, 18, 19, 20, 25, 30, 35, 40, 45, 50, 80, 100, 150, 255 };

        int m_blankingfd;
        int m_sharpnessUpscalingfd;
        int m_sharpnessDownscalingfd;
        int m_keepAspectRatiofd;
        int m_integerScalingfd;
        int m_brightnessfd;

        std::uint8_t m_sharpness;
        std::uint8_t m_displayStyle;
        std::uint8_t m_brightnessIndex;
    };

}