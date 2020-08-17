#pragma once

#include <string>
#include <stdexcept>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <cstring>

#include "events.hpp"

namespace pwswd::dev {

    struct InputId {
        std::uint16_t bustype;
        std::uint16_t vendor;
        std::uint16_t product;
        std::uint16_t version;
    };

    struct UInputUserDev {
        char name[80];
        InputId id;
        std::uint32_t ffEffectsMax;
        std::uint32_t absMax[0x40];
        std::uint32_t absMin[0x40];
        std::uint32_t absFuzz[0x40];
        std::uint32_t absFlat[0x40];
    };

    class UInput {
    public:
        UInput(const std::string &devPath, std::string name, InputId id) {
            this->m_uinputfd = open(devPath.c_str(), O_RDWR);

            if (this->m_uinputfd == -1)
                throw std::runtime_error("Failed to open uinput device!");

            UInputUserDev device = { 0 };
            std::strncpy(device.name, name.c_str(), sizeof(UInputUserDev::name) - 1);
            device.id = id;
            
            write(this->m_uinputfd, std::addressof(device), sizeof(UInputUserDev));
        }

        template<typename T>
        [[nodiscard]] T get() {
            T data;

            std::memset(&data, 0x00, sizeof(T));
            read(this->m_uinputfd, &data, sizeof(T));

            return data;
        }

        template<typename T>
        void inject(T data) {
            write(this->m_uinputfd, &data, sizeof(T));
        }

        void setEventFilterBit(pwswd::EventType type) {
            if (ioctl(this->m_uinputfd, IoCtlCommandUInputSetEventBit, std::uint32_t(type)))
                throw std::runtime_error("Failed to set event bit!");
        }

        void setKeyFilterBit(pwswd::Button button) {
            if (ioctl(this->m_uinputfd, IoCtlCommandUInputSetKeyBit, std::uint32_t(button)))
                throw std::runtime_error("Failed to set key bit!");
        }

        void setRelativeFilterBit(pwswd::RelativeAxis axis) {
            if (ioctl(this->m_uinputfd, IoCtlCommandUInputSetRelativeBit, std::uint32_t(axis)))
                throw std::runtime_error("Failed to set rel bit!");
        }

        void createDevice() {
            if (ioctl(this->m_uinputfd, IoCtlCommandUInputDeviceCreate))
                throw std::runtime_error("Failed to create device!");
        }

    private:
        static constexpr std::uint32_t IoCtlCommandUInputSetEventBit = 0x8004'5564;
        static constexpr std::uint32_t IoCtlCommandUInputSetKeyBit = 0x8004'5565;
        static constexpr std::uint32_t IoCtlCommandUInputSetRelativeBit = 0x8004'5566;
        static constexpr std::uint32_t IoCtlCommandUInputDeviceCreate = 0x2000'5501;

        int m_uinputfd;
    };

}