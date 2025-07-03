#include "asm_utils.h"
#include "interrupt.h"
#include "stdio.h"

// 屏幕IO处理器
STDIO stdio;
// 中断管理器
InterruptManager interruptManager;


extern "C" void setup_kernel()
{
    // 中断处理部件
    interruptManager.initialize();
    // 屏幕IO处理部件
    stdio.initialize();
    interruptManager.enableTimeInterrupt();
    interruptManager.setTimeInterrupt((void *)asm_time_interrupt_handler);
    //asm_enable_interrupt();
    printf("print percentage: %%\n"
           "print char \"N\": %c\n"
           "print string \"Hello World!\": %s\n"
           "print decimal: \"-1234\": %d\n"
           "print hexadecimal \"0x7abcdef0\": %x\n",
           'N', "Hello World!", -1234, 0x7abcdef0);
    printf("print float (default 6 digits): %f\n"
       "print float rounded to 1 digit: %.1f\n"
       "print float rounded to 3 digits: %.3f\n"
       "print float with trailing zeros: %.6f\n"
       "print scientific notation: %.2e\n"
       "print scientific notation (3 digits): %.3e\n"
       "print negative float: %f\n"
       "print small number in %%e: %.1e\n",
       3.1415926,       // %f
       3.1415926,       // %.1f
       3.1415926,       // %.3f
       3.1,             // %.6f
       0.00123,         // %.2e
       123456.0,        // %.3e
       -2.71828,        // %f
       0.0000456        // %.1e
);

           
    //uint a = 1 / 0;
    asm_halt();
}

