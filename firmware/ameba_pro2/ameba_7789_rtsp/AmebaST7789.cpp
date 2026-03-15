#include "AmebaST7789.h"
#include "SPI.h"

#include "font5x7.h"

#include <inttypes.h>

// ==== 放在檔案前面（全域）或 class 內的常數 ====
static uint16_t XSTART = 0;   // 多數 320x240 模組為 0,0。若圖像偏移再調
static uint16_t YSTART = 0;

AmebaST7789::AmebaST7789(int csPin, int dcPin, int resetPin)
{
    _csPin = csPin;    // TODO: no effect now, use pin 10 as default
    _dcPin = dcPin;
    _resetPin = resetPin;

    _dcPort = _dcMask = 0;

    _width = ST7789_TFTWIDTH;
    _height = ST7789_TFTHEIGHT;

    cursor_x = 0;
    cursor_y = 0;
    foreground = ST7789_WHITE;
    background = ST7789_BLACK;
    fontsize = 1;
    rotation = 0;
}

void AmebaST7789::begin(void)
{
    pinMode(_resetPin, OUTPUT);
    pinMode(_dcPin, OUTPUT);
    SPI1.begin();

    reset();                 // 你的 reset() 原樣即可

    // Sleep Out
    digitalWrite(_dcPin, 0); SPI1.transfer(0x11);
    delay(120);

    // Interface Pixel Format: 16-bit
    digitalWrite(_dcPin, 0); SPI1.transfer(0x3A);
    digitalWrite(_dcPin, 1); SPI1.transfer(0x55);

    // Memory Access Control (方向 & RGB/BGR)
    digitalWrite(_dcPin, 0); SPI1.transfer(0x36);
    digitalWrite(_dcPin, 1); SPI1.transfer(ST7789_MADCTL_RGB); // 先用 0x00 = RGB。若紅藍互換改 0x08

    // 可選：反相（有些面板需要）
    digitalWrite(_dcPin, 0); SPI1.transfer(ST7789_INVON);

    // Display ON
    digitalWrite(_dcPin, 0); SPI1.transfer(0x29);
}

void AmebaST7789::setOffset(uint16_t xstart, uint16_t ystart) {
    XSTART = xstart;
    YSTART = ystart;
}

void AmebaST7789::setAddress(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    x0 += XSTART; x1 += XSTART;
    y0 += YSTART; y1 += YSTART;

    // Column address set (0x2A)
    digitalWrite(_dcPin, 0);
    SPI1.transfer(0x2A);
    digitalWrite(_dcPin, 1);
    SPI1.transfer(x0 >> 8); SPI1.transfer(x0 & 0xFF);
    SPI1.transfer(x1 >> 8); SPI1.transfer(x1 & 0xFF);

    // Row address set (0x2B)
    digitalWrite(_dcPin, 0);
    SPI1.transfer(0x2B);
    digitalWrite(_dcPin, 1);
    SPI1.transfer(y0 >> 8); SPI1.transfer(y0 & 0xFF);
    SPI1.transfer(y1 >> 8); SPI1.transfer(y1 & 0xFF);

    // RAM write (0x2C)
    digitalWrite(_dcPin, 0);
    SPI1.transfer(0x2C);
}

void AmebaST7789::writecommand(uint8_t command)
{
    //    *portOutputRegister(_dcPort) &= ~(_dcMask);
    digitalWrite(_dcPin, 0);
    SPI1.transfer(command);
}

void AmebaST7789::writedata(uint8_t data)
{
    //    *portOutputRegister(_dcPort) |=  (_dcMask);
    digitalWrite(_dcPin, 1);
    SPI1.transfer(data);
}

void AmebaST7789::setRotation(uint8_t m)
{
    rotation = (m & 3);

    digitalWrite(_dcPin, 0); SPI1.transfer(0x36); // MADCTL
    digitalWrite(_dcPin, 1);

    uint8_t madctl = ST7789_MADCTL_RGB;   // 先用 RGB；若顏色顛倒把 (madctl |= 0x08)

    switch (rotation) {
    case 0: // 直立
        madctl |= ST7789_MADCTL_RGB;             // RGB，若顏色顛倒再加 0x08
        _width  = 240; _height = 320;
        break;
    case 1: // 右旋90 (橫向)
        madctl |= 0x60;             // MV|MX
        _width  = 320; _height = 240;
        break;
    case 2: // 180 (直立倒轉)
        madctl |= 0xC0;             // MX|MY
        _width  = 240; _height = 320;
        break;
    case 3: // 左旋270 (橫向)
        madctl |= 0xA0;             // MV|MY
        _width  = 320; _height = 240;
        break;
    }

    SPI1.transfer(madctl);
}

void AmebaST7789::fillScreen(uint16_t color)
{
    fillRectangle(0, 0, _width, _height, color);
}

void AmebaST7789::clr(void)
{
    fillScreen(background);
}

void AmebaST7789::drawBitmap(int16_t x, int16_t y, int16_t w, int16_t h, const unsigned short *color)
{
    uint8_t color_hi, color_lo;

    if ((x >= _width) || (y >= _height)) {
        return;
    }

    if ((x + w - 1) >= _width) {
        w = _width - x;
    }

    if ((y + h - 1) >= _height) {
        h = _height - y;
    }

    setAddress(x, y, (x + w - 1), (y + h - 1));

    uint32_t pixelCount = h * w;
    uint32_t i;

    //*portOutputRegister(_dcPort) |=  (_dcMask);
    digitalWrite(_dcPin, 1);
    for (i = 0; i < pixelCount; i++) {
        color_hi = color[i] >> 8;
        color_lo = color[i] & 0xFF;
        SPI1.transfer(color_hi);
        SPI1.transfer(color_lo);
    }
}

void AmebaST7789::fillRectangle(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    uint8_t color_hi, color_lo;

    if ((x >= _width) || (y >= _height)) {
        return;
    }

    if ((x + w - 1) >= _width) {
        w = _width - x;
    }

    if ((y + h - 1) >= _height) {
        h = _height - y;
    }

    setAddress(x, y, (x + w - 1), (y + h - 1));

    uint32_t pixelCount = h * w;
    uint32_t i;
    color_hi = color >> 8;
    color_lo = color & 0xFF;

    //    *portOutputRegister(_dcPort) |=  (_dcMask);
    digitalWrite(_dcPin, 1);
    for (i = 0; i < pixelCount; i++) {
        SPI1.transfer(color_hi);
        SPI1.transfer(color_lo);
    }
}

void AmebaST7789::drawPixel(int16_t x, int16_t y, uint16_t color)
{

    if ((x < 0) || (x >= _width) || (y < 0) || (y >= _height)) return;

    setAddress(x, y, x, y);     // 單點 = 1x1 視窗（關鍵！）
    digitalWrite(_dcPin, 1);
    SPI1.transfer(color >> 8);
    SPI1.transfer(color & 0xFF);
}

void AmebaST7789::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
    int16_t temp;
    uint8_t color_hi;
    uint8_t color_lo;
    bool exchange_xy;
    int16_t idx, dx, dy, linelen, err, ystep;

    if (x0 > x1) {
        temp = x0;
        x0 = x1;
        x1 = temp;
        temp = y0;
        y0 = y1;
        y1 = temp;
    }

    color_hi = color >> 8;
    color_lo = color & 0xFF;

    if (x0 == x1) {
        // draw vertical line
        if (y0 < 0) {
            y0 = 0;
        }
        if (y1 < 0) {
            y1 = 0;
        }
        if (y0 >= _height) {
            y0 = _height;
        }
        if (y1 >= _height) {
            y1 = _height;
        }

        setAddress(x0, y0, x1, y1);
        //        *portOutputRegister(_dcPort) |=  (_dcMask);
        digitalWrite(_dcPin, 1);
        linelen = abs(y1 - y0);
        for (idx = 0; idx < linelen; idx++) {
            SPI1.transfer(color_hi);
            SPI1.transfer(color_lo);
        }
    } else if (y0 == y1) {
        // draw horizontal line
        if (x0 < 0) {
            x0 = 0;
        }
        if (x1 < 0) {
            x1 = 0;
        }
        if (x0 >= _width) {
            x0 = _width - 1;
        }
        if (x1 >= _width) {
            x1 = _width - 1;
        }

        setAddress(x0, y0, x1, y1);
        //        *portOutputRegister(_dcPort) |=  (_dcMask);
        digitalWrite(_dcPin, 1);
        linelen = abs(x1 - x0);
        for (idx = 0; idx < linelen; idx++) {
            SPI1.transfer(color_hi);
            SPI1.transfer(color_lo);
        }
    } else {
        // Bresenham's line algorithm
        exchange_xy = (abs(y1 - y0) > (x1 - x0)) ? true : false;
        if (exchange_xy) {
            temp = x0;
            x0 = y0;
            y0 = temp;
            temp = x1;
            x1 = y1;
            y1 = temp;
        }

        if (x0 > x1) {
            temp = x0;
            x0 = x1;
            x1 = temp;
            temp = y0;
            y0 = y1;
            y1 = temp;
        }

        dx = x1 - x0;
        dy = abs(y1 - y0);
        err = dx / 2;
        ystep = (y0 < y1) ? 1 : -1;

        for (; x0 <= x1; x0++) {
            if (exchange_xy) {
                drawPixel(y0, x0, color);
            } else {
                drawPixel(x0, y0, color);
            }
            err -= dy;
            if (err < 0) {
                y0 += ystep;
                err += dx;
            }
        }
    }
}

void AmebaST7789::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
    drawLine(x0, y0, x1, y1, foreground);
}

void AmebaST7789::drawRectangle(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    drawLine(x, y, x, (y + h), color);
    drawLine(x, y, (x + w), y, color);
    drawLine((x + w), y, (x + w), (y + h), color);
    drawLine(x, (y + h), (x + w), (y + h), color);
}

void AmebaST7789::drawRectangle(int16_t x, int16_t y, int16_t w, int16_t h)
{
    drawRectangle(x, y, w, h, foreground);
}

void AmebaST7789::drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    drawPixel(x0, (y0 + r), color);
    drawPixel(x0, (y0 - r), color);
    drawPixel((x0 + r), y0, color);
    drawPixel((x0 - r), y0, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        drawPixel((x0 + x), (y0 + y), color);
        drawPixel((x0 - x), (y0 + y), color);
        drawPixel((x0 + x), (y0 - y), color);
        drawPixel((x0 - x), (y0 - y), color);
        drawPixel((x0 + y), (y0 + x), color);
        drawPixel((x0 - y), (y0 + x), color);
        drawPixel((x0 + y), (y0 - x), color);
        drawPixel((x0 - y), (y0 - x), color);
    }
}

void AmebaST7789::drawCircle(int16_t x0, int16_t y0, int16_t r)
{
    drawCircle(x0, y0, r, foreground);
}

size_t AmebaST7789::write(uint8_t c)
{
    if (c == '\n') {
        cursor_y += fontsize * 8;
        cursor_x = 0;
    } else if (c == '\r') {

    } else {
        if ((cursor_x + fontsize * 6) >= _width) {
            // new line
            cursor_x = 0;
            cursor_y += fontsize * 8;
        }
        drawChar(c);
    }

    return 1;
}

int16_t AmebaST7789::getWidth()
{
    return _width;
}

int16_t AmebaST7789::getHeight()
{
    return _height;
}

void AmebaST7789::setCursor(int16_t x, int16_t y)
{
    cursor_x = x;
    cursor_y = y;
}

void AmebaST7789::setForeground(uint16_t color)
{
    foreground = color;
}

void AmebaST7789::setBackground(uint16_t _background)
{
    background = _background;
}

void AmebaST7789::setFontSize(uint8_t size)
{
    fontsize = size;
}

void AmebaST7789::drawChar(unsigned char c)
{
    drawChar(cursor_x, cursor_y, c, foreground, background, fontsize);
}

void AmebaST7789::drawChar(int16_t x, int16_t y, unsigned char c, uint16_t _fontcolor, uint16_t _background, uint8_t _fontsize)
{
    int i, j;
    uint8_t line;

    foreground = _fontcolor;
    background = _background;
    fontsize = _fontsize;

    if ((x >= _width) || (y >= _height) || (x + 6 * fontsize - 1) < 0 || (y + 8 * fontsize - 1) < 0) {
        return;
    }

    for (i = 0; i < 6; i++) {
        if (i < 5) {
            line = font5x7[(c * 5 + i)];
        } else {
            line = 0x00;
        }
        for (j = 0; j < 8; j++, line >>= 1) {
            if (line & 0x01) {
                if (fontsize == 1) {
                    drawPixel((x + i), (y + j), foreground);
                } else {
                    fillRectangle((x + i * fontsize), (y + j * fontsize), fontsize, fontsize, foreground);
                }
            } else if (background != foreground) {
                if (fontsize == 1) {
                    drawPixel((x + i), (y + j), background);
                } else {
                    fillRectangle((x + i * fontsize), (y + j * fontsize), fontsize, fontsize, background);
                }
            }
        }
    }

    // update cursor
    cursor_x += fontsize * 6;
    cursor_y = y;
}

void AmebaST7789::reset(void)
{
    if (_resetPin > 0) {
        digitalWrite(_resetPin, HIGH);
        delay(5);
        digitalWrite(_resetPin, LOW);
        delay(20);
        digitalWrite(_resetPin, HIGH);
        delay(150);
    }
}
