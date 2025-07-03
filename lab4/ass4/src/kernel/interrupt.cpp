#include "interrupt.h"
#include "os_type.h"
#include "os_constant.h"
#include "asm_utils.h"
#include "stdio.h"

extern STDIO stdio;

int times = 0;

InterruptManager::InterruptManager()
{
    initialize();
}

void InterruptManager::initialize()
{
    // 初始化中断计数变量
    times = 0;
    
    // 初始化IDT
    IDT = (uint32 *)IDT_START_ADDRESS;
    asm_lidt(IDT_START_ADDRESS, 256 * 8 - 1);

    for (uint i = 0; i < 256; ++i)
    {
        setInterruptDescriptor(i, (uint32)asm_unhandled_interrupt, 0);
    }

    // 初始化8259A芯片
    initialize8259A();
}

void InterruptManager::setInterruptDescriptor(uint32 index, uint32 address, byte DPL)
{
    IDT[index * 2] = (CODE_SELECTOR << 16) | (address & 0xffff);
    IDT[index * 2 + 1] = (address & 0xffff0000) | (0x1 << 15) | (DPL << 13) | (0xe << 8);
}

void InterruptManager::initialize8259A()
{
    // ICW 1
    asm_out_port(0x20, 0x11);
    asm_out_port(0xa0, 0x11);
    // ICW 2
    IRQ0_8259A_MASTER = 0x20;
    IRQ0_8259A_SLAVE = 0x28;
    asm_out_port(0x21, IRQ0_8259A_MASTER);
    asm_out_port(0xa1, IRQ0_8259A_SLAVE);
    // ICW 3
    asm_out_port(0x21, 4);
    asm_out_port(0xa1, 2);
    // ICW 4
    asm_out_port(0x21, 1);
    asm_out_port(0xa1, 1);

    // OCW 1 屏蔽主片所有中断，但主片的IRQ2需要开启
    asm_out_port(0x21, 0xfb);
    // OCW 1 屏蔽从片所有中断
    asm_out_port(0xa1, 0xff);
}

void InterruptManager::enableTimeInterrupt()
{
    uint8 value;
    // 读入主片OCW
    asm_in_port(0x21, &value);
    // 开启主片时钟中断，置0开启
    value = value & 0xfe;
    asm_out_port(0x21, value);
}

void InterruptManager::disableTimeInterrupt()
{
    uint8 value;
    asm_in_port(0x21, &value);
    // 关闭时钟中断，置1关闭
    value = value | 0x01;
    asm_out_port(0x21, value);
}

void InterruptManager::setTimeInterrupt(void *handler)
{
    setInterruptDescriptor(IRQ0_8259A_MASTER, (uint32)handler, 0);
}

// 中断处理函数
namespace {
    int x = 0;
    int y = 0;
    int direction = 0;
    char current_char = 'A';
    int steps = 0;
    uint8 bg_color = 0x1;
    uint8 fg_color = 0xF;
    uint8 current_color = (bg_color << 4) | fg_color;
}



extern "C" void c_time_interrupt_handler() {
    
    

    // 根据方向移动
    switch (direction) {
        case 0: // 右
            if (x < 24) {
                x++;
            } else {
                direction = 1; // 下
                y++;
            }
            break;
        case 1: // 下
            if (y < 79) {
                y++;
            } else {
                direction = 2; // 左
                x--;
            }
            break;
        case 2: // 左
            if (x > 0) {
                x--;
            } else {
                direction = 3; // 上
                y--;
            }
            break;
        case 3: // 上
            if (y > 0) {
                y--;
            } else {
                direction = 0; // 右
                x++;
            }
            break;
    }

    // 更新颜色（每6次移动改变）
    if (++steps >= 6) {
        bg_color = (bg_color + 1) % 16;
        fg_color = (fg_color + 1) % 16;
        current_color = (bg_color << 4) | fg_color;
        steps = 0;
    }

    // 更新字符（A-Z循环）
    if (++current_char > 'Z') current_char = 'A';

    // 在新位置输出字符及颜色
    stdio.print(x, y, current_char, current_color);
}
