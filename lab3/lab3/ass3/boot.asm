bits 16
org 0x7C00

start:
    cli
    xor ax, ax
    mov ds, ax
    lgdt [gdt_descriptor]      ; 加载GDT

    ; 进入保护模式
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; 跳转到32位代码段
    jmp CODE_SEG:pm_start

; GDT定义
gdt_start:
    dq 0x0000000000000000      ; 空描述符

gdt_code:                       ; 代码段描述符
    dw 0xFFFF                   ; 段界限 0-15
    dw 0x0000                   ; 基址 0-15
    db 0x00                     ; 基址 16-23
    db 0x9A                     ; P=1, DPL=0, 代码段, 可读/执行
    db 0xCF                     ; G=1, D/B=1, 界限 16-19
    db 0x00                     ; 基址 24-31

gdt_data:                       ; 数据段描述符
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x92                     ; P=1, DPL=0, 数据段, 可读写
    db 0xCF
    db 0x00
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1  ; GDT大小
    dd gdt_start                ; GDT地址

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

bits 32
pm_start:
    mov dword [0xB8000], 0x07450741  ; 显示"EA"（白色背景蓝色字母）
    ; 初始化段寄存器
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, 0x7C00             ; 栈指针初始化
    
    mov edi, 0xB8000           ; 显存基地址
    mov esi, (12 * 80 + 40) * 2 ; 屏幕中央坐标计算
                                ; (12行, 40列) * 2字节/字符

    ; 写入'P'，证明已经进入保护模式
    mov byte [edi + esi], 'P'
    mov byte [edi + esi + 1], 0x7F

    ; 初始化显示相关寄存器
    mov edi, 0xB8000            ; 显存基地址
    xor esi, esi                ; 位置偏移量

    ; 初始化变量
    mov byte [direction], 0
    mov byte [char], 'A'
    mov byte [counter], 0
    mov byte [bg_color], 0x10
    mov byte [fg_color], 0x0F
    call update_color

main_loop:
    ; 方向判断
    cmp byte [direction], 0
    je move_right
    cmp byte [direction], 1
    je move_down
    cmp byte [direction], 2
    je move_left
    jmp move_up

move_right:
    add esi, 2
    cmp esi, 158
    jl move_done
    mov byte [direction], 1
    jmp move_done

move_down:
    add esi, 160
    cmp esi, 3840
    jl move_done
    mov byte [direction], 2
    jmp move_done

move_left:
    sub esi, 2
    cmp esi, 3840
    jg move_done
    mov byte [direction], 3
    jmp move_done

move_up:
    sub esi, 160
    cmp esi, 158
    jg move_done
    mov byte [direction], 0

move_done:
    ; 更新计数器
    inc byte [counter]
    cmp byte [counter], 6
    jb skip_color_update

    ; 更新颜色
    call update_color
    mov byte [counter], 0

skip_color_update:
    ; 更新字符
    inc byte [char]
    cmp byte [char], 'Z'
    jle .char_ok
    mov byte [char], 'A'

.char_ok:
    ; 写入显存
    mov eax, edi
    add eax, esi
    mov al, [char]
    mov ah, [color]
    mov [edi + esi], ax

    ; 延迟
    mov ecx, 0x02FFFFFF
delay:
    dec ecx
    jnz delay

    jmp main_loop

update_color:
    inc byte [bg_color]
    inc byte [fg_color]
    and byte [bg_color], 0x0F   ; 保持颜色范围
    and byte [fg_color], 0x0F
    mov al, [bg_color]
    shl al, 4
    or al, [fg_color]
    mov [color], al
    ret

; 数据段
direction db 0
char db 0
counter db 0
bg_color db 0
fg_color db 0
color db 0

times 510 - ($ - $$) db 0
dw 0xAA55
