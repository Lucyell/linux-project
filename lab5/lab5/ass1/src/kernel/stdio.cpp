#include "stdio.h"
#include "os_type.h"
#include "asm_utils.h"
#include "os_modules.h"
#include "stdarg.h"
#include "stdlib.h"

STDIO::STDIO()
{
    initialize();
}

void STDIO::initialize()
{
    screen = (uint8 *)0xb8000;
}

void STDIO::print(uint x, uint y, uint8 c, uint8 color)
{
    if (x >= 25 || y >= 80)
        return;

    uint pos = x * 80 + y;
    screen[2 * pos] = c;
    screen[2 * pos + 1] = color;
}

void STDIO::print(uint8 c, uint8 color)
{
    uint cursor = getCursor();
    screen[2 * cursor] = c;
    screen[2 * cursor + 1] = color;
    cursor++;
    if (cursor == 25 * 80)
    {
        rollUp();
        cursor = 24 * 80;
    }
    moveCursor(cursor);
}

void STDIO::print(uint8 c)
{
    print(c, 0x07);
}

void STDIO::moveCursor(uint position)
{
    if (position >= 80 * 25)
        return;

    uint8 temp;

    temp = (position >> 8) & 0xff;
    asm_out_port(0x3d4, 0x0e);
    asm_out_port(0x3d5, temp);

    temp = position & 0xff;
    asm_out_port(0x3d4, 0x0f);
    asm_out_port(0x3d5, temp);
}

uint STDIO::getCursor()
{
    uint pos = 0;
    uint8 temp = 0;

    asm_out_port(0x3d4, 0x0e);
    asm_in_port(0x3d5, &temp);
    pos = ((uint)temp) << 8;

    asm_out_port(0x3d4, 0x0f);
    asm_in_port(0x3d5, &temp);
    pos |= ((uint)temp);

    return pos;
}

void STDIO::moveCursor(uint x, uint y)
{
    if (x >= 25 || y >= 80)
        return;

    moveCursor(x * 80 + y);
}

void STDIO::rollUp()
{
    for (uint i = 80; i < 25 * 80; ++i)
    {
        screen[2 * (i - 80)] = screen[2 * i];
        screen[2 * (i - 80) + 1] = screen[2 * i + 1];
    }

    for (uint i = 24 * 80; i < 25 * 80; ++i)
    {
        screen[2 * i] = ' ';
        screen[2 * i + 1] = 0x07;
    }
}

int STDIO::print(const char *const str)
{
    int i = 0;
    for (i = 0; str[i]; ++i)
    {
        if (str[i] == '\n')
        {
            uint row = getCursor() / 80;
            if (row == 24)
            {
                rollUp();
            }
            else
            {
                ++row;
            }
            moveCursor(row * 80);
        }
        else
        {
            print(str[i]);
        }
    }
    return i;
}

int printf_add_to_buffer(char *buffer, char c, int &idx, const int BUF_LEN)
{
    int counter = 0;
    buffer[idx++] = c;
    if (idx == BUF_LEN)
    {
        buffer[idx] = '\0';
        counter = stdio.print(buffer);
        idx = 0;
    }
    return counter;
}

void ftos(char *buffer, double value, int precision)
{
    if (value < 0)
    {
        *buffer++ = '-';
        value = -value;
    }

    int intPart = (int)value;
    double fracPart = value - intPart;

    char intBuf[20];
    itos(intBuf, intPart, 10);
    char *p = intBuf;
    while (*p) {
        *buffer++ = *p++;
    }


    *buffer++ = '.';

    for (int i = 0; i < precision; ++i)
    {
        fracPart *= 10;
        int digit = (int)fracPart;
        *buffer++ = '0' + digit;
        fracPart -= digit;
    }

    *buffer = '\0';
}

int printf(const char *const fmt, ...)
{
    const int BUF_LEN = 32;
    char buffer[BUF_LEN + 1];
    char number[64];
    int idx = 0, counter = 0;
    va_list ap;
    va_start(ap, fmt);

    for (int i = 0; fmt[i]; ++i)
    {
        if (fmt[i] != '%')
        {
            counter += printf_add_to_buffer(buffer, fmt[i], idx, BUF_LEN);
        }
        else
        {
            i++;
            if (fmt[i] == '\0') break;

            if (fmt[i] == '.')
            {
                int precision = fmt[i + 1] - '0';
                i += 2;

                char spec = fmt[i];
                if (spec == 'f' || spec == 'e')
                {
                    double f = va_arg(ap, double);
                    char fbuf[64];

                    if (spec == 'f')
                        ftos(fbuf, f, precision);
                    else
                    {
                        int exp = 0;
                        while (f >= 10.0) { f /= 10.0; exp++; }
                        while (f < 1.0 && f != 0.0) { f *= 10.0; exp--; }

                        ftos(fbuf, f, precision);
                        int len = 0;
                        while (fbuf[len]) ++len;
                        fbuf[len++] = 'e';
                        fbuf[len++] = (exp < 0) ? '-' : '+';
                        if (exp < 0) exp = -exp;
                        if (exp < 10) fbuf[len++] = '0';
                        itos(fbuf + len, exp, 10);
                    }

                    buffer[idx] = '\0';
                    counter += stdio.print(buffer);
                    counter += stdio.print(fbuf);
                    idx = 0;
                }
                continue;
            }

            switch (fmt[i])
            {
            case '%':
                counter += printf_add_to_buffer(buffer, '%', idx, BUF_LEN);
                break;

            case 'c':
                counter += printf_add_to_buffer(buffer, va_arg(ap, int), idx, BUF_LEN);
                break;

            case 's':
                buffer[idx] = '\0';
                counter += stdio.print(buffer);
                counter += stdio.print(va_arg(ap, const char *));
                idx = 0;
                break;

            case 'd':
            case 'x':
            {
                int val = va_arg(ap, int);
                if (fmt[i] == 'd' && val < 0)
                {
                    counter += printf_add_to_buffer(buffer, '-', idx, BUF_LEN);
                    val = -val;
                }
                itos(number, val, (fmt[i] == 'd') ? 10 : 16);
                for (int j = 0; number[j]; ++j)
                    counter += printf_add_to_buffer(buffer, number[j], idx, BUF_LEN);
                break;
            }

            case 'f':
            {
                double f = va_arg(ap, double);
                ftos(number, f, 6);
                buffer[idx] = '\0';
                counter += stdio.print(buffer);
                counter += stdio.print(number);
                idx = 0;
                break;
            }
            }
        }
    }

    buffer[idx] = '\0';
    counter += stdio.print(buffer);
    return counter;
}

