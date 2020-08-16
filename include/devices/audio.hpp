#pragma once 

#include <cstdint>
#include <unistd.h>
#include <fcntl.h>
#include <cstdio>
#include <sys/wait.h>

namespace pwswd::dev {

    class Audio {
    public:
        Audio() { }

        void toggle() {
            if (this->m_muted)
                this->unmute();
            else
                this->mute();

            this->m_muted = !this->m_muted;
        }
        
        void mute() {
            pid_t child = fork();

            if (!child)
                execlp("amixer", "-M", "-q", "set", "PCM", "mute", nullptr);

            int status = 0;
            waitpid(child, &status, 0);
        }

        void unmute() {
            pid_t child = fork();

            if (!child)
                execlp("amixer", "-M", "-q", "set", "PCM", "unmute", nullptr);

            int status = 0;
            waitpid(child, &status, 0);
        }

        void increase(std::uint8_t step = 5) {
            char percentage[7];

            snprintf(percentage, 6, "%u%%+", step);

            pid_t child = fork();

            if (!child)
                execlp("amixer", "-M", "-q", "set", "PCM", percentage, nullptr);

            int status = 0;
            waitpid(child, &status, 0);
        }

        void decrease(std::uint8_t step = 5) {
            char percentage[7];

            snprintf(percentage, 6, "%u%%-", step);

            pid_t child = fork();

            if (!child)
                execlp("amixer", "-M", "-q", "set", "PCM", percentage, nullptr);

            int status = 0;
            waitpid(child, &status, 0);
        }

    private:
        bool m_muted = false;
    };

}