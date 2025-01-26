/******************************************************************************

MIT License

Copyright (c) 2018 Tadeusz Puźniakowski

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

******************************************************************************/

// constants from tutorial https://kapie.com/2016/ssd1306-128x64-bit-oled-display-interfacing-with-raspberry-pi/ by Ken Hughes

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <ifaddrs.h>
#include <arpa/inet.h>

#include <string>
#include <vector>
#include <iostream>

#include "font8x8_basic.h"


class display_ssd1306_t {
public:
    std::vector<std::vector<unsigned char>> _display;
    int _addr; ///< device address
    int _handle; ///< file handle to i2c device

    void write_buff(const std::vector < unsigned char > data) {
        if (ioctl(_handle, I2C_SLAVE, _addr) < 0) throw "error in ioctl(_handle, I2C_SLAVE, _addr) < 0";
        else write(_handle, data.data(), data.size());
    }

    void blit() {
        write_buff({0x00,0x21,0x00,0x7F,0x22,0x00,0x07}); // write buffer sequence
        for(auto &line:_display) write_buff(line);
    }

    void put_bit(int x, int y, int c) {
        if ((x < 0) || (x >= 128) || (y < 0) || (y >= 64)) return;
        int prev = _display[y>>3][x+1];
        prev = c*(prev | (1 << (y&7))) | (1-c)*(prev & (~(1 << (y&7))));
        _display[y>>3][x+1] = prev;
    }

    void print_bitmap(int x0, int y0, const unsigned char *p) {
        for (int y = 0; y < 8; y++) {
            int row = p[y];
            for (int x = 0; x < 9; x++) {
                int val = row & 1;
                put_bit(x+x0,y+y0,val);
                row = row >> 1;
            }
        }
    }

    /**
     * @brief Print single digit on a buffer
     * 
     * @param x position x
     * @param y position y
     * @param digit digit. It only supports '0'-'9', '.', ' '
     */
    void print_digit(int x, int y, char digit) {
        const unsigned char *p;
        char i = digit; //0;
        if (i > 128) i = 0;
        p = (unsigned char *)(font8x8_basic[i]);
        print_bitmap(x,y,p);
    }

    void print_digit_string(int x, int y, const std::string s) {
        const char *digits = s.c_str();
        while (*digits) {
            print_digit(x,y,*digits);
            digits++;
            x+=8;
        }
    }


    display_ssd1306_t(const int address_ = 0x3C, const std::string &devfile_ = "/dev/i2c-1") {
        _addr = address_;
        _handle = open(devfile_.c_str(), O_RDWR);
        if (_handle < 0) throw "error opening i2c device";
        write_buff({0x00,0xAE,0xA8,0x3F,0xD3,0x00,0x40,0xA1,0xC8,0xDA,0x12,0x81,0x7F,0xA4,0xA6,0xD5,0x80,0x8D,0x14,0xD9,0x22,0xD8,0x30,0x20,0x00,0xAF}); ///< initialize device
        _display = std::vector<std::vector<unsigned char>>(8,std::vector<unsigned char>(129));
        for(auto &line: _display) line[0] = 0x40; // first byte in each line must be set to 0x040
    }

    virtual ~display_ssd1306_t() {
        close(_handle);
    }
};



std::vector<std::string> get_local_addresses() {
    std::vector<std::string> ipv4_addresses;
    struct ifaddrs *ifaddr, *ifa;
    int family;
    if (getifaddrs(&ifaddr) == -1) {
        throw std::invalid_argument("Error: getifaddrs failed");
        return ipv4_addresses;
    }
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;
        family = ifa->ifa_addr->sa_family;
        if (family == AF_INET) {
            struct sockaddr_in *ipv4_addr = (struct sockaddr_in *)ifa->ifa_addr;
            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(ipv4_addr->sin_addr), ip_str, INET_ADDRSTRLEN);
            if (std::string(ip_str) != std::string("127.0.0.1"))
                ipv4_addresses.push_back(ip_str);
        }
    }
    freeifaddrs(ifaddr);
    return ipv4_addresses;
}


int main() {
    display_ssd1306_t ssddisp;
    ssddisp.print_digit_string(0,0,"Witaj w swiecie     ");
    int y = 9;
    for (auto s : get_local_addresses()) {
        ssddisp.print_digit_string(0,y,s);
        y+= 9;
    }
    ssddisp.blit();
    return 0;
}
