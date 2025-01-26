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

#include <vector>
#include <string>
#include <iostream>

const unsigned char digits[] = {
0b00000110,
0b00001001,
0b00001001,
0b00001001,
0b00000110,

0b00000010,
0b00000110,
0b00001010,
0b00000010,
0b00000010,

0b00000110,
0b00001001,
0b00000010,
0b00000100,
0b00001111,

0b00000110,
0b00001001,
0b00000011,
0b00001001,
0b00000110,

0b00000010,
0b00000100,
0b00001000,
0b00001111,
0b00000010,

0b00001111,
0b00001000,
0b00001110,
0b00000001,
0b00001110,

0b00000110,
0b00001000,
0b00001110,
0b00001001,
0b00000110,

0b00001111,
0b00000001,
0b00000010,
0b00000100,
0b00001000,

0b00000110,
0b00001001,
0b00000110,
0b00001001,
0b00000110,

0b00000110,
0b00001001,
0b00000111,
0b00000001,
0b00000110,

0b00000000,
0b00000000,
0b00000000,
0b00000110,
0b00000110,

0b00000000,
0b00000110,
0b00000000,
0b00000110,
0b00000000,

0b00000000,
0b00000000,
0b00000000,
0b00000000,
0b00000000,
};



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
        for (int y = 0; y < 5; y++) {
            for (int x = 0; x < 5; x++) {
                int val = ((*(p+y))>>(4-x)) & 1;
                put_bit(x+x0,y+y0,val);
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
        char i = 0;
        if ((digit <= '9')&&(digit >='0')) {
            i = digit - '0';
        } else if (digit == '.') {
            i = 10;
        } else if (digit == ':') {
            i = 11;            
        } else {
            i = 12;
        }
        p = digits+4*i;
        print_bitmap(x,y,p);
    }

    void print_digit_string(int x, int y, const char *digits) {
        while (*digits) {
            print_digit(x,y,*digits);
            digits++;
            x+=6;
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



int main() {
    display_ssd1306_t ssddisp;
    ssddisp.print_digit_string(0,0,"127.0.0.1");
    ssddisp.blit();
    return 0;
}
