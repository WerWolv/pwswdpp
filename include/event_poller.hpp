#pragma once

#include <string>

#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <stdexcept>
#include <sys/ioctl.h>

namespace pwswd {

    class EventPoller {
    public:
        EventPoller(const std::string &eventPath) {
            this->m_epollfd = epoll_create1(0);

            if (this->m_epollfd == 0)
                throw std::runtime_error("Failed to create epoll!");

            this->m_eventfd = open(eventPath.c_str(), O_RDONLY | O_NONBLOCK);

            if (this->m_eventfd == 0)
                throw std::runtime_error("Failed to open event!");

            this->m_eventData.events = EPOLLIN;
            this->m_eventData.data.fd = this->m_eventfd;

	        epoll_ctl(this->m_epollfd, EPOLL_CTL_ADD, this->m_eventfd, &this->m_eventData);
        }

        ~EventPoller() {
            close(this->m_eventfd);
            close(this->m_epollfd);
        }

        bool wait(std::int32_t timeout = -1) {
            return epoll_wait(this->m_epollfd, &this->m_eventData, 1, timeout) > 0;
        }

        template<typename T>
        [[nodiscard]] T get() {
            T data;

            std::memset(&data, 0x00, sizeof(T));
            read(this->m_eventfd, &data, sizeof(T));

            return data;
        }

        template<typename T>
        void inject(T data) {
            write(this->m_eventfd, &data, sizeof(T));
        }

        void grab() {
            if (this->m_grabbed)
                return;

            if (ioctl(this->m_eventfd, IoCtlCommandEventGrab, true) != -1)
                this->m_grabbed = true;
            else throw std::runtime_error("Failed to grab event!");
        }

        void ungrab() {
            if (!this->m_grabbed)
                return;

            if (ioctl(this->m_eventfd, IoCtlCommandEventGrab, false) != -1)
                this->m_grabbed = false;
            else throw std::runtime_error("Failed to ungrab event!");
        }

        bool isGrabbed() {
            return this->m_grabbed;
        }

    private:
        static constexpr std::uint32_t IoCtlCommandEventGrab = 0x8004'4590;

        int m_epollfd, m_eventfd;
        epoll_event m_eventData;
        bool m_grabbed = false;
    };

}