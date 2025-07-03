; If you meet compile error, try 'sudo apt install gcc-multilib g++-multilib' first
%include "head.include"

your_if:
    mov eax, [a1]
    cmp eax, 40
    jl .else_if          ; a1 < 40, check next condition
    ; a1 >= 40
    add eax, 3
    cdq                  ; Sign extend EAX into EDX:EAX
    mov ebx, 5
    idiv ebx             ; EAX = (a1 + 3) / 5
    mov [if_flag], eax
    jmp .if_end
.else_if:
    cmp eax, 18
    jl .else            ; a1 < 18, use else branch
    ; a1 >= 18 and <40
    imul eax, 2         ; EAX = a1 * 2
    mov ebx, 80
    sub ebx, eax        ; EBX = 80 - a1*2
    mov [if_flag], ebx
    jmp .if_end
.else:
    ; a1 < 18
    mov eax, [a1]
    shl eax, 5          ; EAX = a1 << 5
    mov [if_flag], eax
.if_end:

your_while:
    mov eax, [a2]
    cmp eax, 25
    jge .end_while      ; Exit if a2 >= 25
.loop:
    call my_random      ; Result in EAX (AL is the char)
    mov ebx, [a2]
    shl ebx, 1          ; EBX = a2 * 2 (byte offset)
    mov edi, [while_flag]
    add edi, ebx        ; EDI = &while_flag[a2*2]
    mov [edi], al       ; Store AL to the array
    inc dword [a2]      ; Increment a2
    ; Check condition again
    mov eax, [a2]
    cmp eax, 25
    jl .loop
.end_while:

%include "end.include"

your_function:
    push ebp
    mov ebp, esp
    sub esp, 4           ; Local variable i at [ebp-4]
    mov dword [ebp-4], 0 ; i = 0
.loop_start:
    mov eax, [your_string] ; EAX points to string start
    mov edx, [ebp-4]     ; EDX = i
    mov cl, [eax + edx]  ; CL = your_string[i]
    test cl, cl
    jz .loop_end         ; Exit if null terminator
    pushad               ; Save all registers
    movzx ecx, cl        ; Zero-extend char to ECX
    add ecx, 9           ; Add 9 to the character
    push ecx             ; Push argument for print_a_char
    call print_a_char
    add esp, 4           ; Clean up stack
    popad                ; Restore all registers
    inc dword [ebp-4]    ; i++
    jmp .loop_start
.loop_end:
    leave
    ret
