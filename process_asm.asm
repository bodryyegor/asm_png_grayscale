section .text
global grayscale_averaging_asm

grayscale_averaging_asm:
    push rbp
    mov rbp, rsp
    push rbx
    push r12

    mov r12, rdi
    mov ebx, esi
    mov eax, edx
    imul eax, ebx
    test eax, eax
    jz .end

    mov ecx, eax
    mov rdi, r12

.loop:
    movzx eax, byte [rdi]
    movzx edx, byte [rdi+1]
    add eax, edx
    movzx edx, byte [rdi+2]
    add eax, edx

    imul eax, 0xAAAB
    shr eax, 17

    mov byte [rdi], al
    mov byte [rdi+1], al
    mov byte [rdi+2], al

    add rdi, 3
    dec ecx
    jnz .loop

.end:
    pop r12
    pop rbx
    pop rbp
    ret
