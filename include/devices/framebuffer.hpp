#pragma once

#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <stdexcept>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <poll.h>
#include <mutex>

namespace pwswd::dev {

    struct fb_bitfield {
        std::uint32_t offset;   
        std::uint32_t length;   
        std::uint32_t msb_right;
   };

    struct fb_fix_screeninfo {
        char id[16];
        unsigned long smem_start;
        std::uint32_t smem_len;
        std::uint32_t type;
        std::uint32_t type_aux;
        std::uint32_t visual;
        std::uint16_t xpanstep;
        std::uint16_t ypanstep;
        std::uint16_t ywrapstep;
        std::uint32_t line_length;
        unsigned long mmio_start;
        std::uint32_t mmio_len;
        std::uint32_t accel;
        std::uint16_t capabilities;
        std::uint16_t reserved[2];
    };

    struct fb_var_screeninfo {
       std::uint32_t xres;          
       std::uint32_t yres;
       std::uint32_t xres_virtual;  
       std::uint32_t yres_virtual;
       std::uint32_t xoffset;       
       std::uint32_t yoffset;       
       std::uint32_t bits_per_pixel;
       std::uint32_t grayscale;     
       struct fb_bitfield red;      
       struct fb_bitfield green;    
       struct fb_bitfield blue;
       struct fb_bitfield transp;   
       std::uint32_t nonstd;        
       std::uint32_t activate;      
       std::uint32_t height;        
       std::uint32_t width;         
       std::uint32_t accel_flags;   
       std::uint32_t pixclock;      
       std::uint32_t left_margin;   
       std::uint32_t right_margin;  
       std::uint32_t upper_margin;  
       std::uint32_t lower_margin;
       std::uint32_t hsync_len;     
       std::uint32_t vsync_len;     
       std::uint32_t sync;          
       std::uint32_t vmode;         
       std::uint32_t rotate;        
       std::uint32_t colorspace;    
       std::uint32_t reserved[4];   
   };


    class Framebuffer {
    public:
        Framebuffer(const std::string& fbPath) : m_fbPath(fbPath) {
            this->m_framebufferfd = -1;

            this->open();

            if (this->m_framebufferfd == -1)
                throw std::runtime_error("Failed to open framebuffer device!");

            
            if (ioctl(this->m_framebufferfd, IoCtlCommandFramebufferGetFScreenInfo, &this->m_fixScreenInfo) < 0)
                throw std::runtime_error("Failed to get fixed screen info!");

            this->refreshScreenInfo();
        }

        bool open() {
            if (this->m_framebufferfd == -1)
                this->m_framebufferfd = ::open(this->m_fbPath.c_str(), O_RDWR);

            return this->m_framebufferfd != -1;
        }

        void close() {
            if (this->m_framebufferfd != -1)
                ::close(this->m_framebufferfd);
            this->m_framebufferfd = -1;
        }

        [[nodiscard]] std::pair<std::uint32_t, std::uint32_t> getResolution() {
            return { this->m_varScreenInfo.xres, this->m_varScreenInfo.yres };
        }

        [[nodiscard]] std::uint32_t getBitsPerPixel() {
            return this->m_varScreenInfo.bits_per_pixel;
        }

        [[nodiscard]] std::uint32_t getStride() {
            switch(getBitsPerPixel()) {
                case 15: return 2;
                case 16: return 2;
                case 24: return 3;
                case 32: return 4;
                default: return getBitsPerPixel() >> 3;
            }
        }

        [[nodiscard]] std::size_t getSize() {
            return this->m_fixScreenInfo.line_length * m_varScreenInfo.yres;
        }

        void refreshScreenInfo() {
            if (ioctl(this->m_framebufferfd, IoCtlCommandFramebufferGetVScreenInfo, &this->m_varScreenInfo) < 0)
                throw std::runtime_error("Failed to get variable screen info!");
        }

        void map() {
            if (this->m_framebuffer == nullptr)
                this->m_framebuffer = static_cast<std::uint8_t*>(mmap(nullptr, this->getSize() * NumFramebuffers, PROT_READ | PROT_WRITE, MAP_SHARED, this->m_framebufferfd, 0));
        }

        void unmap() {
            if (this->m_framebuffer != nullptr)
                munmap(this->m_framebuffer, this->getSize() * NumFramebuffers);
            this->m_framebuffer = nullptr;
        }

        void lock() {
            this->m_lock.lock();
        }

        void unlock() {
            this->m_lock.unlock();
        }

        [[nodiscard]] std::uint8_t* getAddress() {
            return this->m_framebuffer;
        }

        inline std::uint32_t encodeColor(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) {
            switch (this->getBitsPerPixel()) {
                case 15: return ((b & 0x1F) << 10) | ((g & 0x1F) << 5) | (r & 0x1F);
                case 16: return ((b & 0x1F) << 11) | ((g & 0x3F) << 5) | (r & 0x1F);
                case 24: return (b << 16) | (g << 8) | (r);
                case 32: return (a << 24) | (b << 16) | (g << 8) | (r);
                default: return 0x00;
            }
        }

        inline void drawRect(std::uint32_t x, std::uint32_t y, std::uint32_t w, std::uint32_t h, std::uint32_t color) {
            auto [xres, yres] = this->getResolution();

            auto bpp = this->getStride();
            auto encodedColor = this->encodeColor((color & 0xFF000000) >> 24, (color & 0x00FF0000) >> 16, (color & 0x0000FF00) >> 8, (color & 0x000000FF));

            std::uint32_t realX = (x * xres) / ScreenWidth,
                          realY = (y * yres) / ScreenHeight, 
                          realW = (w * xres) / ScreenWidth, 
                          realH = (h * yres) / ScreenHeight;

            for (std::uint8_t fb = 0; fb < NumFramebuffers; fb++)
                for (std::uint32_t drawX = realX; drawX < (realX + realW); drawX++)
                    for (std::uint32_t drawY = realY; drawY < (realY + realH); drawY++)
                        std::memcpy(&this->getAddress()[(fb * xres * yres * bpp) + (drawY * xres * bpp) + (drawX * bpp)], std::addressof(encodedColor), bpp);
        }


        static constexpr std::uint32_t ScreenWidth = 640;
        static constexpr std::uint32_t ScreenHeight = 480;

        static constexpr std::uint32_t OverlayWidth = 350;
        static constexpr std::uint32_t OverlayHeight = 50;

        static constexpr std::uint8_t  NumFramebuffers = 3;
    private:
        static constexpr std::uint32_t IoCtlCommandFramebufferGetVScreenInfo = ('F' << 8) | 0x00;
        static constexpr std::uint32_t IoCtlCommandFramebufferGetFScreenInfo = ('F' << 8) | 0x02;

        std::string m_fbPath;
        std::mutex m_lock;

        int m_framebufferfd = -1;

        fb_fix_screeninfo m_fixScreenInfo;
        fb_var_screeninfo m_varScreenInfo;

        std::uint8_t *m_framebuffer = nullptr;
    };

}