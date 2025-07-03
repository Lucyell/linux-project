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
    // 初始化串口 COM1
    asm_out_port(0x3F8 + 1, 0x00);    // 禁止中断
    asm_out_port(0x3F8 + 3, 0x80);    // 打开 DLAB，设置波特率除数
    asm_out_port(0x3F8 + 0, 0x03);    // 除数低字节，波特率 38400
    asm_out_port(0x3F8 + 1, 0x00);    // 除数高字节
    asm_out_port(0x3F8 + 3, 0x03);    // 8位数据，无校验，1位停止位
    asm_out_port(0x3F8 + 2, 0xC7);    // 启用FIFO，清除，14字节阈值
    asm_out_port(0x3F8 + 4, 0x0B);    // 启用中断，RTS/DSR
}

void STDIO::putChar(char c)
{
    uint8 temp;
    // 等待发送缓冲区为空
    do {
        asm_in_port(0x3F8 + 5, &temp);
    } while (!(temp & 0x20));

    // 写数据
    asm_out_port(0x3F8, c);
}


void STDIO::print(uint8 c, uint8 color)
{
    (void)color; // 忽略颜色
    if (c == '\n') {
        putChar('\r'); // 回车
        putChar('\n'); // 换行
    } else {
        putChar(c);
    }
}

void STDIO::print(uint8 c)
{
    print(c, 0x07);
}

// 不需要moveCursor、getCursor、rollUp这些了，可以留空实现
void STDIO::moveCursor(uint position) {}
uint STDIO::getCursor() { return 0; }
void STDIO::moveCursor(uint x, uint y) {}
void STDIO::rollUp() {}

void STDIO::print(uint x, uint y, uint8 c, uint8 color)
{
    (void)x;
    (void)y;
    print(c, color);
}

int STDIO::print(const char *const str)
{
    int count = 0;
    for (int i = 0; str[i]; ++i)
    {
        print(str[i]);
        ++count;
    }
    return count;
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

int printf(const char *const fmt, ...)
{
    const int BUF_LEN = 32;
    char buffer[BUF_LEN + 1];
    char number[33];
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

            switch (fmt[i])
            {
            case '%':
                counter += printf_add_to_buffer(buffer, '%', idx, BUF_LEN);
                break;
            case 'c':
                counter += printf_add_to_buffer(buffer, (char)va_arg(ap, int), idx, BUF_LEN);
                break;
            case 's':
                buffer[idx] = '\0';
                idx = 0;
                counter += stdio.print(buffer);
                counter += stdio.print(va_arg(ap, const char *));
                break;
            case 'd':
            case 'x':
                int temp = va_arg(ap, int);
                if (temp < 0 && fmt[i] == 'd')
                {
                    counter += printf_add_to_buffer(buffer, '-', idx, BUF_LEN);
                    temp = -temp;
                }
                itos(number, temp, (fmt[i] == 'd' ? 10 : 16));
                for (int j = 0; number[j]; ++j)
                {
                    counter += printf_add_to_buffer(buffer, number[j], idx, BUF_LEN);
                }
                break;
            }
        }
    }
    buffer[idx] = '\0';
    counter += stdio.print(buffer);
    va_end(ap);
    return counter;
}

